// Controller.cpp
#include "Controller.h"
#include "TextView.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include "Command.h"
#include <cctype>
#include <set>
#include <iomanip>
#include <filesystem>


namespace
{
bool OpenProjectFile(std::ifstream &file, const std::filesystem::path &relativePath)
{
    const std::filesystem::path cwdCandidate = relativePath;
    file.open(cwdCandidate);
    if (file.is_open())
        return true;

    std::error_code ec;
    const auto exePath = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (ec)
        return false;

    const auto exeDir = exePath.parent_path();
    const std::filesystem::path exeCandidate = exeDir / relativePath;
    file.open(exeCandidate);
    if (file.is_open())
        return true;

    const std::filesystem::path repoCandidate = exeDir.parent_path() / relativePath;
    file.open(repoCandidate);
    return file.is_open();
}
} // namespace

Controller::Controller(TextView *TextViewImp, const std::string &filename) : _TextViewImp(TextViewImp), _filename(filename)
{
    UpdateStatusRow("ready");

    OpenFile();
    LoadKeywords();

    SetDocCursor(0, 0);
    UpdateStatusRow("");
    UpdateTextViewImpRows();
}

Controller::~Controller()
{
    while (!CommandStack.empty())
    {
        delete CommandStack.top();
        CommandStack.pop();
    }
    while (!RedoStack.empty())
    {
        delete RedoStack.top();
        RedoStack.pop();
    }
}

int Controller::GetRowStart() const
{
    if (!ShowLines)
        return 0;

    const int totalLines = std::max(1, (int)model.GetLines().size());
    int digits = 1;
    for (int n = std::max(1, totalLines); n >= 10; n /= 10)
        digits++;
    digits = std::max(3, digits);
    const int rowStart = digits + 2; // number width + "  "

    // If the window is too narrow, automatically hide line numbers.
    if (rowStart >= _TextViewImp->GetColNumInView())
        return 0;
    return rowStart;
}

void Controller::EnsureCursorVisible(int &docY, int &docX)
{
    _TextViewImp->SyncWindowSize();

    model.EnsureNonEmpty();
    const auto &lines = model.GetLines();

    docY = std::clamp(docY, 0, (int)lines.size() - 1);
    docX = std::clamp(docX, 0, (int)lines[docY].size());

    const int viewRows = std::max(1, _TextViewImp->GetRowNumInView());
    if (docY < rowOffset)
        rowOffset = docY;
    if (docY >= rowOffset + viewRows)
        rowOffset = std::max(0, docY - (viewRows - 1));

    const int maxOffset = std::max(0, (int)lines.size() - viewRows);
    rowOffset = std::clamp(rowOffset, 0, maxOffset);

    const int textWidth = GetTextWidth();
    if (docX < colOffset)
        colOffset = docX;
    if (docX > colOffset + (textWidth - 1))
        colOffset = std::max(0, docX - (textWidth - 1));
}

void Controller::SetDocCursor(int docY, int docX)
{
    model.SetCursor(docY, docX);
    docY = model.GetCursorY();
    docX = model.GetCursorX();
    EnsureCursorVisible(docY, docX);

    const int viewY = std::clamp(docY - rowOffset, 0, std::max(0, _TextViewImp->GetRowNumInView() - 1));
    const int viewX = GetRowStart() + (docX - colOffset);
    _TextViewImp->SetCursorY(viewY);
    _TextViewImp->SetCursorX(std::max(0, viewX));
}

void Controller::ClearRedoStack()
{
    while (!RedoStack.empty())
    {
        delete RedoStack.top();
        RedoStack.pop();
    }
}

void Controller::UpdateStatusRow(const std::string &key)
{
    const int lineNum = model.GetCursorY() + 1;
    const int colNum = model.GetCursorX() + 1;

    std::ostringstream left;
    std::filesystem::path p(_filename);
    left << p.filename().string() << "  " << (IsInsertMode() ? "INSERT" : "COMMAND");
    if (!key.empty())
        left << "  (" << key << ")";

    std::ostringstream right;
    right << this->isModified << " "<< "Ln " << lineNum << ", Col " << colNum;

    _TextViewImp->ClearStatusRows();
    _TextViewImp->AddStatusRow(left.str(), right.str(), true);
}

void Controller::OpenFile()
{
    std::string err;
    model.LoadFromFile(_filename, &err);
    rowOffset = 0;
    colOffset = 0;
}

bool Controller::SaveFile(std::string *errMsg)
{
    std::string err;
    const bool ok = model.SaveToFile(_filename, &err);
    if (!ok && errMsg)
        *errMsg = err;
    return ok;
}

void Controller::HandleKey(int key)
{
    switch (key)
    {
    case ARROW_LEFT:
    {
        HandleLeft();
        break;
    }

    case ARROW_RIGHT:
    {
        HandleRight();
        break;
    }

    case ARROW_UP:
    {
        HandleUp();
        break;
    }

    case ARROW_DOWN:
    {
        HandleDown();
        break;
    }
    case ESC:
    {
        curStatus = "command";
        UpdateStatusRow("esc");

        break;
    }

    case CTRL_A:
    {
        curStatus = "command";
        UpdateStatusRow("cmd");
        break;
    }
    case 105: // i ascii value
    {
        curStatus = "insert";
        UpdateStatusRow("ins");
        break;
    }
    case 104: // h
    {
        HandleLeft();
        break;
    }
    case 106: // j
    {
        HandleDown();
        break;
    }
    case 107: // k
    {
        HandleUp();
        break;
    }
    case 108: // l
    {
        HandleRight();
        break;
    }
    case DEL_KEY:
    case 120: // x
    {
        // delete character under cursor
        model.EnsureNonEmpty();
        const auto &lines = model.GetLines();
        const int y = model.GetCursorY();
        if (!lines.empty())
        {
            const int docX = model.GetCursorX();
            if (docX < (int)lines[y].size())
            {
                Command *command = new DeleteTextCommand(this);
                command->execute();
                CommandStack.push(command);
                ClearRedoStack();
            }
        }
        break;
    }

    case CTRL_Q:
    {
        // Quit is handled in main loop to ensure raw mode cleanup.
        break;
    }
    case CTRL_S:
    {
        std::string err;
        if (SaveFile(&err))
            UpdateStatusRow("saved");
        else
            UpdateStatusRow("save failed");
        break;
    }

    case CTRL_Z:
    {
        Undo();
        UpdateStatusRow("undo");
        break;
    }
    case CTRL_Y:
    {
        Redo();
        UpdateStatusRow("redo");
        break;
    }
    }
    UpdateTextViewImpRows();
}

void Controller::HandleInput(int key)
{
    // Modal behavior:
    // - In command mode: route everything to HandleKey (so 'i' can switch to insert).
    // - In insert mode: printable keys/editing keys modify the buffer; others route to HandleKey.
    if (!IsInsertMode())
    {
        HandleKey(key);
        return;
    }

    bool didEdit = false;
    if (key == ENTER)
    {
        HandleEnter();
        didEdit = true;
    }
    else if (key == BACKSPACE)
    {
        RemoveText();
        didEdit = true;
    }
    else if (key == TAB)
    {
        AddText(' ');
        AddText(' ');
        AddText(' ');
        AddText(' ');
        didEdit = true;
    }
    else if (key >= 32 && key <= 126)
    {
        AddText(static_cast<char>(key));
        didEdit = true;
    }
    else
    {
        HandleKey(key);
        return;
    }

    if (didEdit)
        UpdateTextViewImpRows();
}

void Controller::HandleResize()
{
    SetDocCursor(model.GetCursorY(), model.GetCursorX());
    UpdateStatusRow("resize");
    UpdateTextViewImpRows();
}

void Controller::HandleUp()
{
    const auto &lines = model.GetLines();
    const int y = model.GetCursorY();
    const int x = model.GetCursorX();
    if (y <= 0 || lines.empty())
        return;
    const int newY = y - 1;
    const int newX = std::min(x, (int)lines[newY].size());
    SetDocCursor(newY, newX);
    UpdateStatusRow("up");
}

void Controller::HandleDown()
{
    const auto &lines = model.GetLines();
    const int y = model.GetCursorY();
    const int x = model.GetCursorX();
    if (lines.empty() || y >= (int)lines.size() - 1)
        return;
    const int newY = y + 1;
    const int newX = std::min(x, (int)lines[newY].size());
    SetDocCursor(newY, newX);
    UpdateStatusRow("down");
}

void Controller::HandleRight()
{
    const auto &lines = model.GetLines();
    if (lines.empty())
        return;

    int newY = model.GetCursorY();
    int newX = model.GetCursorX();
    const int lineLen = (int)lines[newY].size();

    if (newX < lineLen)
    {
        newX++;
    }
    else if (newY < (int)lines.size() - 1)
    {
        newY++;
        newX = 0;
    }

    SetDocCursor(newY, newX);
    UpdateStatusRow("right");
}

void Controller::HandleLeft()
{
    const auto &lines = model.GetLines();
    if (lines.empty())
        return;

    int newY = model.GetCursorY();
    int newX = model.GetCursorX();

    if (newX > 0)
    {
        newX--;
    }
    else if (newY > 0)
    {
        newY--;
        newX = (int)lines[newY].size();
    }

    SetDocCursor(newY, newX);
    UpdateStatusRow("left");
}

void Controller::AddText(char ch)
{
    if (!IsInsertMode())
        return;

    Command *command = new InsertTextCommand(this, ch);
    command->execute();
    CommandStack.push(command);
    ClearRedoStack();
    this->isModified = "(modified)";

    UpdateStatusRow(std::string(1, ch));

}

void Controller::RemoveText()
{
    if (!IsInsertMode())
        return;

    model.EnsureNonEmpty();

    const int docY = model.GetCursorY();
    const int docX = model.GetCursorX();
    if (docX == 0 && docY == 0)
        return;

    if (docX == 0)
    {
        Command *command = new MergeLinesCommand(this);
        command->execute();
        CommandStack.push(command);
        ClearRedoStack();
    }
    else
    {

        Command *command = new RemoveTextCommand(this);
        command->execute();
        CommandStack.push(command);
        ClearRedoStack();
    }
    this->isModified = "(modified)";
    UpdateStatusRow("backspace");

}

void Controller::HandleEnter()
{
    if (!IsInsertMode())
        return;

    Command *command = new EnterCommand(this);
    command->execute();
    CommandStack.push(command);
    ClearRedoStack();
    this->isModified = "(modified)";

    UpdateStatusRow("enter");
}

void Controller::Undo()
{
    if (CommandStack.empty())
        return;

    Command *command = CommandStack.top();
    CommandStack.pop();
    command->unexecute();

    RedoStack.push(command);
}

void Controller::Redo()
{
    if (RedoStack.empty())
        return;

    Command *command = RedoStack.top();
    RedoStack.pop();
    command->execute();

    CommandStack.push(command);
}

void Controller::LoadKeywords()
{
    keywords.clear();

    std::filesystem::path p(_filename);
    const std::string ext = p.extension().string();

    std::filesystem::path kwFile;
    if (ext == ".py")
        kwFile = "dependencies/pykeywords.txt";
    else if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".hpp" || ext == ".hh" || ext == ".hxx")
        kwFile = "dependencies/cppkeywords.txt";
    else if (ext == ".c" || ext == ".h")
        kwFile = "dependencies/ckeywords.txt";

    if (kwFile.empty())
        return;

    // Support running from the repo root and from packaged bin/ layouts.
    std::ifstream file;
    OpenProjectFile(file, kwFile);
    if (file.is_open())
    {
        std::string line;
        while (getline(file, line))
        {
            std::istringstream iss(line);
            std::string keyword;
            while (iss >> keyword)
            {
                keywords.insert(keyword);
            }
        }
        file.close();
    }
}

void Controller::HighlightKeywords()
{
    _TextViewImp->ClearColor();

    struct Span
    {
        int start = 0; // inclusive (doc column)
        int end = 0;   // exclusive (doc column)
        TextStyle style;
    };

    auto isIdentStart = [](char c) {
        const unsigned char uc = (unsigned char)c;
        return std::isalpha(uc) != 0 || c == '_';
    };
    auto isIdentChar = [](char c) {
        const unsigned char uc = (unsigned char)c;
        return std::isalnum(uc) != 0 || c == '_';
    };
    auto isDigit = [](char c) {
        const unsigned char uc = (unsigned char)c;
        return std::isdigit(uc) != 0;
    };
    auto isHexDigit = [](char c) {
        const unsigned char uc = (unsigned char)c;
        return std::isxdigit(uc) != 0;
    };
    auto isBracket = [](char c) {
        switch (c)
        {
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
            return true;
        default:
            return false;
        }
    };

    const auto &lines = model.GetLines();
    const int rowStart = GetRowStart();
    const int textWidth = GetTextWidth();
    const int viewRows = std::max(1, _TextViewImp->GetRowNumInView());
    const bool showLineNums = ShowLines && rowStart > 0;
    const int prefixLen = showLineNums ? rowStart : 0;
    const int visL = colOffset;
    const int visR = colOffset + textWidth;

    TextStyle lineNumStyle;
    lineNumStyle.dim = true;

    TextStyle commentStyle;
    commentStyle.fg = TEXT_COLOR_GREEN;
    commentStyle.dim = true;

    TextStyle stringStyle;
    stringStyle.fg = TEXT_COLOR_YELLOW;

    TextStyle numberStyle;
    numberStyle.fg = TEXT_COLOR_CYAN;

    TextStyle keywordStyle;
    keywordStyle.fg = TEXT_COLOR_BLUE;
    keywordStyle.bold = true;

    TextStyle bracketStyle;
    bracketStyle.fg = TEXT_COLOR_MAGENTA;

    auto parseQuoted = [&](const std::string &s, int i, char quote) {
        int j = i + 1;
        while (j < (int)s.size())
        {
            if (s[j] == '\\' && j + 1 < (int)s.size())
            {
                j += 2;
                continue;
            }
            if (s[j] == quote)
                return j + 1;
            j++;
        }
        return (int)s.size();
    };

    auto parseNumber = [&](const std::string &s, int i) {
        int j = i;
        const int n = (int)s.size();

        if (s[j] == '0' && j + 1 < n && (s[j + 1] == 'x' || s[j + 1] == 'X'))
        {
            j += 2;
            while (j < n && (isHexDigit(s[j]) || s[j] == '_'))
                j++;
        }
        else
        {
            bool hasDot = false;
            if (s[j] == '.')
            {
                hasDot = true;
                j++;
            }
            while (j < n && (isDigit(s[j]) || s[j] == '_'))
                j++;
            if (j < n && s[j] == '.' && !hasDot)
            {
                hasDot = true;
                j++;
                while (j < n && (isDigit(s[j]) || s[j] == '_'))
                    j++;
            }

            if (j < n && (s[j] == 'e' || s[j] == 'E'))
            {
                int k = j + 1;
                if (k < n && (s[k] == '+' || s[k] == '-'))
                    k++;
                if (k < n && isDigit(s[k]))
                {
                    j = k + 1;
                    while (j < n && (isDigit(s[j]) || s[j] == '_'))
                        j++;
                }
            }
        }

        // Suffixes (simple).
        while (j < n)
        {
            const char c = s[j];
            if (c == 'u' || c == 'U' || c == 'l' || c == 'L' || c == 'f' || c == 'F')
                j++;
            else
                break;
        }

        return j;
    };

    auto buildSpans = [&](const std::string &line) {
        std::vector<Span> spans;
        const int n = (int)line.size();
        int i = 0;
        while (i < n)
        {
            if (line[i] == '/' && i + 1 < n && line[i + 1] == '/')
            {
                spans.push_back({i, n, commentStyle});
                break;
            }

            if (line[i] == '"')
            {
                const int j = parseQuoted(line, i, '"');
                spans.push_back({i, j, stringStyle});
                i = j;
                continue;
            }

            if (line[i] == '\'')
            {
                const int j = parseQuoted(line, i, '\'');
                spans.push_back({i, j, stringStyle});
                i = j;
                continue;
            }

            const bool prevIdent = (i > 0 && isIdentChar(line[i - 1]));
            const bool startsNumber = (!prevIdent) &&
                                      (isDigit(line[i]) || (line[i] == '.' && i + 1 < n && isDigit(line[i + 1])));
            if (startsNumber)
            {
                const int j = parseNumber(line, i);
                spans.push_back({i, j, numberStyle});
                i = j;
                continue;
            }

            if (isIdentStart(line[i]))
            {
                int j = i + 1;
                while (j < n && isIdentChar(line[j]))
                    j++;
                const std::string ident = line.substr(i, (size_t)(j - i));
                if (keywords.find(ident) != keywords.end())
                {
                    spans.push_back({i, j, keywordStyle});
                }
                i = j;
                continue;
            }

            if (isBracket(line[i]))
            {
                spans.push_back({i, i + 1, bracketStyle});
                i++;
                continue;
            }

            i++;
        }

        return spans;
    };

    for (int viewRow = 0; viewRow < viewRows; ++viewRow)
    {
        const int docRow = rowOffset + viewRow;
        if (docRow < 0 || docRow >= (int)lines.size())
            continue;

        if (showLineNums)
            _TextViewImp->SetStyle(viewRow, 0, rowStart - 1, lineNumStyle);

        const auto &line = lines[docRow];
        const auto spans = buildSpans(line);
        for (const auto &sp : spans)
        {
            const int clipL = std::max(sp.start, visL);
            const int clipR = std::min(sp.end, visR);
            if (clipL >= clipR)
                continue;

            const int colBegin = prefixLen + (clipL - visL);
            const int colEnd = prefixLen + (clipR - visL) - 1;
            _TextViewImp->SetStyle(viewRow, colBegin, colEnd, sp.style);
        }
    }
}

void Controller::UpdateTextViewImpRows()
{
    _TextViewImp->SyncWindowSize();
    // Ensure offsets + view cursor are consistent with the model cursor before rendering.
    SetDocCursor(model.GetCursorY(), model.GetCursorX());
    _TextViewImp->InitRows();
    const auto &lines = model.GetLines();
    const int viewRows = std::max(1, _TextViewImp->GetRowNumInView());
    const int rowStart = GetRowStart();
    const int textWidth = GetTextWidth();
    const bool showLineNums = ShowLines && rowStart > 0;
    const int digits = showLineNums ? (rowStart - 2) : 0;

    for (int viewRow = 0; viewRow < viewRows; ++viewRow)
    {
        const int docRow = rowOffset + viewRow;
        if (docRow < 0 || docRow >= (int)lines.size())
        {
            _TextViewImp->AddRow("");
            continue;
        }

        const auto &line = lines[docRow];
        std::ostringstream oss;
        if (showLineNums)
        {
            oss << std::setw(digits) << std::setfill(' ') << (docRow + 1) << "  ";
        }

        if (colOffset < (int)line.size())
            oss << line.substr(colOffset, textWidth);

        _TextViewImp->AddRow(oss.str());
    }

    HighlightKeywords();
    _TextViewImp->Refresh();
}
