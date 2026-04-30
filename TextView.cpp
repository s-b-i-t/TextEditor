//  TextView.cpp

#include "TextView.h"
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdarg>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <signal.h>

using namespace  std;

// SIGWINCH flag (set by signal handler; polled by ReadKeyRaw()).
static volatile sig_atomic_t g_winch = 0;

static void HandleWinch(int)
{
    g_winch = 1;
}

static void InstallWinchHandler()
{
    static bool installed = false;
    if (installed)
        return;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = HandleWinch;
    sigemptyset(&sa.sa_mask);
    // Do not set SA_RESTART: we want reads to time out and let us surface KEY_RESIZE.
    sigaction(SIGWINCH, &sa, nullptr);
    installed = true;
}



//***********************************************************
// Text view configuration


TextViewConfig :: TextViewConfig()
{
    InitParams();
    InitWndSize();
}

bool TextViewConfig :: UpdateWndSize()
{
    const int oldRows = screenrows;
    const int oldCols = screencols;
    InitWndSize();
    return oldRows != screenrows || oldCols != screencols;
}

void TextViewConfig :: Dump() const
{
    cout << "Window size: [" << screenrows << "," << screencols << "]\n";
}

void TextViewConfig :: InitParams()
{
    this->cx = 0;
    this->cy = 0;
    //this->rawmode = false;
}
    
// Init default window size
void TextViewConfig :: InitWndSize()
{
    //
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 || ws.ws_row == 0)
    {
        // Fall back to a sane default rather than querying the terminal via escape codes,
        // which can block in environments without a full terminal emulator.
        this->screencols = 80;
        this->screenrows = 24;
    }
    else
    {
        this->screencols = ws.ws_col;
        this->screenrows = ws.ws_row;
    }
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned. */
int TextViewConfig :: GetCursorPosition(int ifd, int ofd, int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;
    
    /* Report cursor location */
    if (write(ofd, "\x1b[6n", 4) != 4) return -1;
    
    /* Read the response: ESC [ rows ; cols R */
    while (i < sizeof(buf)-1) {
        if (read(ifd,buf+i,1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    
    /* Parse it. */
    if (buf[0] != ESC || buf[1] != '[') return -1;
    if (sscanf(buf+2,"%d;%d",rows,cols) != 2) return -1;
    return 0;
}

void TextViewConfig :: AddStatusRow( const std::string &statusMsgLeft, const std::string &statusMsgRight, bool fBlackBackground )
{
    listStatusMsgsLeft.push_back(statusMsgLeft);
    listStatusMsgsRight.push_back(statusMsgRight);
    listStatusBlackBackground.push_back(fBlackBackground);
}

//***********************************************************
// A textview implementation
// This is based on the simple Kilo code from GitHub


TextView :: TextView() : fQuit(false), keyLastPressed(KEY_NULL)
{
    Init();
}

TextView :: ~TextView()
{
    if (rawEnabled)
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
        rawEnabled = false;
    }

    // Best-effort cleanup: reset attributes and show cursor.
    if (isatty(STDOUT_FILENO))
    {
        const char *seq = "\x1b[0m\x1b[?25h";
        (void)write(STDOUT_FILENO, seq, strlen(seq));
    }
}

void TextView :: Init()
{
    initOk = true;
    initError.clear();
    rawEnabled = false;

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
    {
        initOk = false;
        initError = "Error: editor requires a TTY for stdin/stdout.";
        return;
    }

    if (tcgetattr(STDIN_FILENO, &origTermios) == -1)
    {
        initOk = false;
        initError = std::string("Error: tcgetattr failed: ") + std::strerror(errno);
        return;
    }

    struct termios raw = origTermios; /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - echoing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer. */
    raw.c_cc[VMIN] = 0;  /* Return each byte, or zero for timeout. */
    raw.c_cc[VTIME] = 1; /* 100 ms timeout (unit is tens of second). */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    {
        initOk = false;
        initError = std::string("Error: tcsetattr failed: ") + std::strerror(errno);
        return;
    }

    rawEnabled = true;
    InstallWinchHandler();
}

bool TextView :: SyncWindowSize()
{
    return status.UpdateWndSize();
}

int TextView :: ReadKey()
{
    if (!initOk)
        return CTRL_Q;
    keyLastPressed = ReadKeyRaw(STDIN_FILENO);
    return keyLastPressed;
}

void TextView :: Refresh()
{
    if (!initOk)
        return;

    // Keep window size in sync (useful even if SIGWINCH isn't delivered in some environments).
    SyncWindowSize();

    // build buffer from scratch
    ClearBuffer();
    for(unsigned int i=0; i<listRows.size(); ++i)
    {
        AppendRowBuffer(i, listRows[i]);
    }
    FinishRowsBuffer();
    
//cout << "Number of rows: " << GetNumRows() << endl;
//exit(1);
    
    // render all current rows (best-effort)
    const char *buf = bufferWnd.c_str();
    size_t remaining = bufferWnd.size();
    while (remaining > 0)
    {
        const ssize_t n = write(STDOUT_FILENO, buf, remaining);
        if (n == -1)
        {
            if (errno == EINTR)
                continue;
            initOk = false;
            initError = std::string("Error: write failed: ") + std::strerror(errno);
            break;
        }
        buf += n;
        remaining -= (size_t)n;
    }
}


void TextView :: AddRow(const std::string &strRow)
{
//cout << "Addiing row: " << strRow << endl;
    this->listRows.push_back(strRow);
//cout << "Size: " << this->listRows.size() << ",  getsize: " << GetNumRows() << endl;
}

//*********************************************************************
void TextView :: FinishRowsBuffer()
{
    // fill out the window
    for(int r=GetNumRows(); r<status.GetWndRowNum(); ++r)
    {
        bufferWnd += "\x1b[0m\x1b[0K\r\n";
    }
    
    // show status
    for(int i=0; i<status.GetNumStatusRows(); ++i)
    {
        AppendStatusMsg(i);
    }
    
    /* Put cursor at its current position. Note that the horizontal position
     * at which the cursor is displayed may be different compared to 'E.cx'
     * because of TABs. */
    char buf[32];
    int cx = 1;
    int filerow = status.GetCursorY();
    const char *row = (filerow >= GetNumRows() ) ? NULL :  GetRow(filerow);
    if (row) {
        const int rowLen = (int)strlen(row);
        for (int j = 0; j < status.GetCursorX(); j++) {
            if (j < rowLen && row[j] == TAB) cx += 7-((cx)%8);
            cx++;
        }
    }
    snprintf(buf,sizeof(buf),"\x1b[%d;%dH",status.GetCursorY()+1,cx);
    bufferWnd += buf;
    
    // show cursor
    bufferWnd += "\x1b[?25h";
}

void TextView :: ClearBuffer()
{
    //
    this->bufferWnd.clear();
    
    // append pre-chosen fields
    // hide cursor
    bufferWnd += "\x1b[?25l";
    // go home
    bufferWnd += "\x1b[H";
    // reset any previous attributes
    bufferWnd += "\x1b[0m";
}

void TextView :: AppendRowBuffer(int row, const string &strRow)
{
    const auto rowIt = styleInfo.find(row);
    if (rowIt == styleInfo.end() || rowIt->second.empty())
    {
        bufferWnd += strRow;
    }
    else
    {
        const auto &spans = rowIt->second; // start -> (endInclusive, style)
        bool active = false;
        int activeEnd = -1;
        TextStyle activeStyle;

        for (int i = 0; i < (int)strRow.size(); ++i)
        {
            const auto it = spans.find(i);
            if (it != spans.end())
            {
                // End any previous span and start the new one.
                if (active)
                {
                    bufferWnd += "\x1b[0m";
                    active = false;
                    activeEnd = -1;
                }

                activeEnd = it->second.first;
                activeStyle = it->second.second;
                if (!activeStyle.IsDefault() && activeEnd >= i)
                {
                    bufferWnd += "\x1b[0m";
                    if (activeStyle.bold)
                        bufferWnd += "\x1b[1m";
                    if (activeStyle.dim)
                        bufferWnd += "\x1b[2m";
                    if (activeStyle.fg != TEXT_COLOR_DEF)
                    {
                        char buf[16];
                        snprintf(buf, sizeof(buf), "\x1b[%dm", (int)activeStyle.fg);
                        bufferWnd += buf;
                    }
                    active = true;
                }
                else
                {
                    activeEnd = -1;
                }
            }

            bufferWnd += strRow[i];

            // Always reset immediately after the last character in a styled span.
            if (active && i == activeEnd)
            {
                bufferWnd += "\x1b[0m";
                active = false;
                activeEnd = -1;
            }
        }

        if (active)
            bufferWnd += "\x1b[0m";
    }

    // Reset at end-of-line to prevent bleed into later rows/status.
    bufferWnd += "\x1b[0m";
    bufferWnd += "\x1b[0K";
    bufferWnd += "\r\n";
}
            

void TextView :: Quit()
{
    //
    fQuit = true;
    
    // cleaup the screen
    ClearBuffer();
    std::cout << "\033[2J\033[H";
}

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
int TextView :: ReadKeyRaw(int fd) {
    if (!pendingInput.empty())
    {
        const unsigned char ch = (unsigned char)pendingInput.front();
        pendingInput.pop_front();
        return ch;
    }

    while (true)
    {
        if (g_winch)
        {
            g_winch = 0;
            return KEY_RESIZE;
        }

        char c;
        const int nread = (int)read(fd, &c, 1);
        if (nread == 0)
            continue;
        if (nread == -1)
        {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN)
                continue;
            initOk = false;
            initError = std::string("Error: read failed: ") + std::strerror(errno);
            return CTRL_Q;
        }

        if (c != ESC)
            return (unsigned char)c;

        // Escape sequence or plain ESC. If the following byte isn't part of a known
        // sequence, treat it as a plain ESC and "push back" the byte for the next read.
        char b1;
        const int n1 = (int)read(fd, &b1, 1);
        if (n1 != 1)
            return ESC;

        if (b1 != '[' && b1 != 'O')
        {
            pendingInput.push_back(b1);
            return ESC;
        }

        char b2;
        const int n2 = (int)read(fd, &b2, 1);
        if (n2 != 1)
        {
            pendingInput.push_back(b1);
            return ESC;
        }

        if (b1 == '[')
        {
            if (b2 >= '0' && b2 <= '9')
            {
                char b3;
                const int n3 = (int)read(fd, &b3, 1);
                if (n3 != 1)
                {
                    pendingInput.push_back(b1);
                    pendingInput.push_back(b2);
                    return ESC;
                }
                if (b3 == '~')
                {
                    switch (b2)
                    {
                    case '3':
                        return DEL_KEY;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    default:
                        return ESC;
                    }
                }
                return ESC;
            }

            switch (b2)
            {
            case 'A':
                return ARROW_UP;
            case 'B':
                return ARROW_DOWN;
            case 'C':
                return ARROW_RIGHT;
            case 'D':
                return ARROW_LEFT;
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            default:
                return ESC;
            }
        }

        // ESC O sequences.
        switch (b2)
        {
        case 'H':
            return HOME_KEY;
        case 'F':
            return END_KEY;
        default:
            return ESC;
        }
    }
}

void TextView :: AppendStatusMsg(int rs)
{
    bufferWnd += "\x1b[0K";
    if( status.IsStatusRowBlkBackground(rs) )
    {
        bufferWnd += "\x1b[7m";
    }
    string statusmsgLeft = status.GetStatusRowLeft(rs);
    string statusmsgRight = status.GetStatusRowRight(rs);
    int len = (int)statusmsgLeft.size();
    int rlen = (int)statusmsgRight.size();
    
    if (len > GetColNumInView())
    {
        statusmsgLeft = statusmsgLeft.substr( 0, GetColNumInView() );
        len = GetColNumInView();
    }
    bufferWnd += statusmsgLeft;
    
    while(len < GetColNumInView()) {
        if (GetColNumInView() - len == rlen) {
            bufferWnd += statusmsgRight;
            break;
        } else {
            bufferWnd += " ";
            len++;
        }
    }
    bufferWnd += "\x1b[0m";

    // There shouldn't be an extra carriage return at the end of the last status message
    if(rs != status.GetNumStatusRows()-1) {
        bufferWnd += "\r\n";
    }
}

void TextView :: SetStyle(int row, int colBegin, int colEnd, const TextStyle &style)
{
    if (!style.IsDefault())
    {
        std::pair<int, TextStyle> pp(colEnd, style);
        styleInfo[row][colBegin] = pp;
    }
    else
    {
        // erase this record
        if (styleInfo.find(row) != styleInfo.end())
        {
            if (styleInfo[row].find(colBegin) != styleInfo[row].end())
            {
                styleInfo[row].erase(colBegin);
            }
        }
    }
}

// call to change the text color (convenience wrapper)
void TextView :: SetColor(int row, int colBegin, int colEnd, TEXT_COLOR clr)
{
    TextStyle style;
    style.fg = clr;
    SetStyle(row, colBegin, colEnd, style);
}
