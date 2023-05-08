//
//  ECTextViewImp.cpp
//  
//
//  Created by Yufeng Wu on 2/27/2020.
//  Last modified: 4/9/2021
//

#include "ECTextViewImp.h"
#include <termios.h>
#include <cstdlib>
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
#include "ECObserver.h"

using namespace  std;

// global variable
static bool fRawMode = false;
static struct termios orig_termios; /* In order to restore at exit.*/

static void SetRawMode(bool f)
{
    fRawMode = f;
}
static bool IsRawMode()
{
    return fRawMode;
}

static void WndAtExit(void) {
    //disableRawMode(STDIN_FILENO);
    if (IsRawMode()) {
        tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios);
        SetRawMode(false);
    }
}

/* Raw mode: 1960 magic shit. */
static void  EnableRawMode(int fd) {
    struct termios raw;
    
    if (IsRawMode() ) return; /* Already enabled. */
    if (!isatty(STDIN_FILENO)) throw string("Error in atty standard input");
    atexit(WndAtExit);
    if (tcgetattr(fd,&orig_termios) == -1) throw string("Fatal error: tcgetattr (0)");
    
    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer. */
    raw.c_cc[VMIN] = 0; /* Return each byte, or zero for timeout. */
    raw.c_cc[VTIME] = 1; /* 100 ms timeout (unit is tens of second). */
    
    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) throw string("Fatal error: tcsetattr");
    SetRawMode(true);
}



//***********************************************************
// Text view configuration


ECTextViewConfig :: ECTextViewConfig()
{
    InitParams();
    InitWndSize();
}

void ECTextViewConfig :: Dump() const
{
    cout << "Window size: [" << screenrows << "," << screencols << "]\n";
}

void ECTextViewConfig :: InitParams()
{
    this->cx = 0;
    this->cy = 0;
    //this->rawmode = false;
}
    
// Init default window size
void ECTextViewConfig :: InitWndSize()
{
    //
    const int ifd = STDIN_FILENO;
    const int ofd = STDIN_FILENO;
    
    struct winsize ws;
    
    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* ioctl() failed. Try to query the terminal itself. */
        int orig_row, orig_col, retval;
        
        /* Get the initial position so we can restore it later. */
        retval = GetCursorPosition(ifd,ofd,&orig_row,&orig_col);
        if (retval == -1)
        {
            throw string("Cannot get cursor position");
        }
        
        /* Go to right/bottom margin and get position. */
        if (write(ofd,"\x1b[999C\x1b[999B",12) != 12)
        {
            throw string("Cannot write to standard output");
        }
        retval = GetCursorPosition(ifd,ofd,&this->screenrows,&this->screencols);
        if (retval == -1)
        {
            throw string("Cannot get cursor position2");
        }
        
        /* Restore position. */
        char seq[32];
        snprintf(seq,32,"\x1b[%d;%dH",orig_row,orig_col);
        if (write(ofd,seq,strlen(seq)) == -1) {
            throw string("Cannot write to standard output");
        }
    } else {
        this->screencols = ws.ws_col;
        this->screenrows = ws.ws_row;
    }
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned. */
int ECTextViewConfig :: GetCursorPosition(int ifd, int ofd, int *rows, int *cols) {
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

void ECTextViewConfig :: AddStatusRow( const std::string &statusMsgLeft, const std::string &statusMsgRight, bool fBlackBackground )
{
    listStatusMsgsLeft.push_back(statusMsgLeft);
    listStatusMsgsRight.push_back(statusMsgRight);
    listStatusBlackBackground.push_back(fBlackBackground);
}

//***********************************************************
// A textview implementation
// This is based on the simple Kilo code from GitHub


ECTextViewImp :: ECTextViewImp() : fQuit(false), keyLastPressed(KEY_NULL)
{
    status.Dump();
    Init();
}

ECTextViewImp :: ~ECTextViewImp()
{
}

void ECTextViewImp :: Init()
{
    //
    EnableRawMode(STDIN_FILENO);
}

void ECTextViewImp :: Show()
{
    // enter loop
    while( ShouldQuit() == false )
    {
        Refresh();
//cout << "Waiting for key: \n";
        keyLastPressed = ReadKey(STDIN_FILENO);
        
        // just priint for now
        if( keyLastPressed == CTRL_Q)
        {
            Quit();
            break;
        }
        
        // notify
        Notify();
    }
}

void ECTextViewImp :: Refresh()
{
    // build buffer from scratch
    ClearBuffer();
    for(unsigned int i=0; i<listRows.size(); ++i)
    {
        AppendRowBuffer(i, listRows[i]);
    }
    FinishRowsBuffer();
    
//cout << "Number of rows: " << GetNumRows() << endl;
//exit(1);
    
    // render all current rows
    write(STDOUT_FILENO, bufferWnd.c_str(), bufferWnd.length());
}


void ECTextViewImp :: AddRow(const std::string &strRow)
{
//cout << "Addiing row: " << strRow << endl;
    this->listRows.push_back(strRow);
//cout << "Size: " << this->listRows.size() << ",  getsize: " << GetNumRows() << endl;
}

//*********************************************************************
void ECTextViewImp :: FinishRowsBuffer()
{
    // fill out the window
    for(int r=GetNumRows(); r<status.GetWndRowNum(); ++r)
    {
        bufferWnd += "~\x1b[0K\r\n";
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
        for (int j = 0; j < status.GetCursorX(); j++) {
            if (j < strlen(row)  && row[j] == TAB) cx += 7-((cx)%8);
            cx++;
        }
    }
    snprintf(buf,sizeof(buf),"\x1b[%d;%dH",status.GetCursorY()+1,cx);
    bufferWnd += buf;
    
    // show cursor
    bufferWnd += "\x1b[?25h";
}

void ECTextViewImp :: ClearBuffer()
{
    //
    this->bufferWnd.clear();
    
    // append pre-chosen fields
    // hide cursor
    bufferWnd += "\x1b[?25l";
    // go home
    bufferWnd += "\x1b[H";
}

void ECTextViewImp :: AppendRowBuffer(int row, const string &strRow)
{
    // row: the index of row. strRow: content of the row
    if( this->clrTextInfo.find(row) == this->clrTextInfo.end() )
    {
        bufferWnd += strRow;
    }
    else
    {
        // there is color info. So do this one by one
        int clrCurr = -1;
        int clrRangeRight = -1;
        for(int i=0; i<(int)strRow.size(); ++i)
        {
            std::map<int, std::pair<int, TEXT_COLOR> > :: iterator it = this->clrTextInfo[row].find(i);
            int clrNew = clrCurr;
            if( it != this->clrTextInfo[row].end() )
            {
                clrNew = it->second.second;
                clrRangeRight = it->second.first;
            }
            else if( i > clrRangeRight && clrRangeRight >= 0 )
            {
                clrNew = -1;
                clrRangeRight = -1;
            }
            if( clrNew != clrCurr )
            {
                if( clrNew != -1 )
                {
                    // change color now
                    char buf[16];
                    snprintf(buf,sizeof(buf),"\x1b[%dm",clrNew);
                    string strbuf = buf;
                    bufferWnd += strbuf;
                }
                else
                {
                    // change to default color
                    bufferWnd += "\x1b[39m";
                }
                clrCurr = clrNew;
            }
            // add the current symbol
            bufferWnd += strRow[i];
        }
    }
    bufferWnd += "\x1b[39m";
    bufferWnd += "\x1b[0K";
    bufferWnd += "\r\n";
}
            

void ECTextViewImp :: Quit()
{
    //
    fQuit = true;
    
    // cleaup the screen
    ClearBuffer();
    std::cout << "\033[2J\033[H";
}

int ECTextViewImp :: GetPressedKey()
{
    //
    return keyLastPressed;
}

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
int ECTextViewImp :: ReadKey(int fd) {
    int nread;
    char c, seq[3];
    while ((nread = read(fd,&c,1)) == 0);
    if (nread == -1) exit(1);
    
    while(1) {
        switch(c) {
            case ESC:    /* escape sequence */
                /* If this is just an ESC, we'll timeout here. */
                if (read(fd,seq,1) == 0) return ESC;
                if (read(fd,seq+1,1) == 0) return ESC;
                
                /* ESC [ sequences. */
                if (seq[0] == '[') {
                    if (seq[1] >= '0' && seq[1] <= '9') {
                        /* Extended escape, read additional byte. */
                        if (read(fd,seq+2,1) == 0) return ESC;
                        if (seq[2] == '~') {
                            switch(seq[1]) {
                                case '3': return DEL_KEY;
                                case '5': return PAGE_UP;
                                case '6': return PAGE_DOWN;
                            }
                        }
                    } else {
                        switch(seq[1]) {
                            case 'A': return ARROW_UP;
                            case 'B': return ARROW_DOWN;
                            case 'C': return ARROW_RIGHT;
                            case 'D': return ARROW_LEFT;
                            case 'H': return HOME_KEY;
                            case 'F': return END_KEY;
                        }
                    }
                }
                
                /* ESC O sequences. */
                else if (seq[0] == 'O') {
                    switch(seq[1]) {
                        case 'H': return HOME_KEY;
                        case 'F': return END_KEY;
                    }
                }
                break;
            default:
                return c;
        }
    }
}

void ECTextViewImp :: AppendStatusMsg(int rs)
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
    if(status.IsStatusRowBlkBackground(rs))
    {
        bufferWnd += "\x1b[0m";
    }

    // There shouldn't be an extra carriage return at the end of the last status message
    if(rs != status.GetNumStatusRows()-1) {
        bufferWnd += "\r\n";
    }
}

// call to change the text color
void ECTextViewImp :: SetColor(int row, int colBegin, int colEnd, TEXT_COLOR clr)
{
    // save the text color setting
    if( clr != TEXT_COLOR_DEF )
    {
        std::pair<int, TEXT_COLOR> pp(colEnd, clr);
        clrTextInfo[row][colBegin] = pp;
    }
    else
    {
        // erase this record
        if( clrTextInfo.find(row) != clrTextInfo.end() )
        {
            if( clrTextInfo[row].find(colBegin) != clrTextInfo[row].end() )
            {
                clrTextInfo[row].erase(colBegin);
            }
        }
    }
}

