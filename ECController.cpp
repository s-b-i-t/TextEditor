// ECController.cpp
#include "ECController.h"
#include "ECTextViewImp.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
using namespace std;

ECController::ECController(ECTextViewImp *TextViewImp, const std::string &filename) : _TextViewImp(TextViewImp), _filename(filename)
{
    OpenFile();
    LoadKeywords();
    HighlightKeywords();
}

void ECController::OpenFile()
{
    ifstream file(_filename);

    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            Rows.push_back(line);
            _TextViewImp->Refresh();
        }
        file.close();
    }

    UpdateTextViewImpRows();
}

void ECController::SaveFile()
{
    ofstream file(_filename);

    for (const auto &row : Rows)
    {
        file << row << endl;
    }
    file.close();
}

void ECController::HandleKey(int key)
{
    switch (key)
    {
    case ARROW_LEFT:
    {
        int current_y = _TextViewImp->GetCursorY();
        int current_x = _TextViewImp->GetCursorX();

        _TextViewImp->SetCursorX(max(current_x - 1, 0));
        break;
    }

    case ARROW_RIGHT:
    {
        int row_length = Rows[_TextViewImp->GetCursorY()].length();
        if (_TextViewImp->GetCursorX() < row_length)
        {
            _TextViewImp->SetCursorX(min(_TextViewImp->GetCursorX() + 1, row_length));
        }
    }
    break;

    case ESC:
    {
        curStatus = "command";
        _TextViewImp->ClearStatusRows();
        _TextViewImp->AddStatusRow("ctrl-h for help", "mode: " + curStatus, true);

        break;
    }

    case CTRL_A:
    {
        curStatus = "command";
        _TextViewImp->ClearStatusRows();
        _TextViewImp->AddStatusRow("ctrl-h for help", "mode: " + curStatus, true);

        break;
    }
    case 105: // i ascii value
    {
        curStatus = "insert";
        _TextViewImp->ClearStatusRows();
        _TextViewImp->AddStatusRow("ctrl-h for help", "mode: " + curStatus, true);

        break;
    }
    case ARROW_UP:
    {
        int current_x = _TextViewImp->GetCursorX();
        int current_y = _TextViewImp->GetCursorY();
        if (current_y > 0)
        {
            int prev_row_length = Rows[current_y - 1].length();
            int new_x = min(current_x, prev_row_length);
            _TextViewImp->SetCursorY(current_y - 1);
            _TextViewImp->SetCursorX(new_x);
        }
        break;
    }
    case ARROW_DOWN:
    {

        int current_x = _TextViewImp->GetCursorX();
        int current_y = _TextViewImp->GetCursorY();
        int max_y = _TextViewImp->GetRowNumInView() - 1;

        if (current_y == Rows.size() - 1)
            break;

        if (current_y < max_y)
        {
            int next_row_length = Rows[current_y + 1].length();
            int new_x = min(current_x, next_row_length);
            if (current_y != max_y)
            {
                _TextViewImp->SetCursorY(current_y + 1);
            }
            _TextViewImp->SetCursorX(new_x);
        }

        break;
    }

    case CTRL_Q:
    {
        exit(0);
        break;
    }
    case CTRL_S:
    {
        SaveFile();
    }

    case CTRL_Z:
    {
        if (curStatus == "command")
            Undo();
        break;
    }
    case CTRL_Y:
    {
        if (curStatus == "command")
            Redo();
        break;
    }
    }

    _TextViewImp->Refresh();
}

void ECController::AddText(char ch)
{
    int y = _TextViewImp->GetCursorY();
    int x = _TextViewImp->GetCursorX();

    if (y >= Rows.size())
    {
        Rows.resize(y + 1);
    }

    if (x >= _TextViewImp->GetColNumInView() - 1) // subtract 1 here to account for the new character being added
    {
        Rows[y].erase(0, 1);
        Rows[y].push_back(ch);
    }
    else
    {
        Rows[y].insert(x, 1, ch);
        _TextViewImp->SetCursorX(x + 1);
    }

    Operation op;
    op.type = Operation::Type::ADD;
    op.ch = ch;
    op.x = x;
    op.y = y;
    undoStack.push(op);

    _TextViewImp->Refresh();
    UpdateTextViewImpRows();
    HighlightKeywords();

    if (ch == '[')
    {
        AddText(']');
        _TextViewImp->SetCursorX(_TextViewImp->GetCursorX() - 1);
    }
    if (ch == '{')
    {
        AddText('}');
        _TextViewImp->SetCursorX(_TextViewImp->GetCursorX() - 1);
    }
    if (ch == '(')
    {
        AddText(')');
        _TextViewImp->SetCursorX(_TextViewImp->GetCursorX() - 1);
    }

}

void ECController::RemoveText()
{
    int y = _TextViewImp->GetCursorY();
    int x = _TextViewImp->GetCursorX();

    if (y >= Rows.size())
    {
        return;
    }

    char removedChar = '\0'; // Initialize removedChar to null character

    if (x == 0)
    {
        if (y == 0)
        {
            return;
        }
        string &current_row = Rows[y];
        int previous_row_length = Rows[y - 1].length();
        removedChar = current_row[0];
        Rows[y - 1] += current_row;
        Rows.erase(Rows.begin() + y);
        _TextViewImp->SetCursorY(y - 1);
        _TextViewImp->SetCursorX(previous_row_length);
    }

    else
    {
        if (x > 0)
        {
            removedChar = Rows[y][x - 1];
            Rows[y].erase(x - 1, 1);
            _TextViewImp->SetCursorX(x - 1);
        }
    }

    // Push the operation onto the undo stack
    if (removedChar != '\0')
    {
        Operation op;
        op.type = Operation::Type::REMOVE;
        op.ch = removedChar;
        op.x = x;
        op.y = y;
        undoStack.push(op);
    }

    _TextViewImp->Refresh();
    UpdateTextViewImpRows();
    HighlightKeywords();
}

void ECController::Undo()
{
    if (undoStack.empty())
        return;

    Operation op = undoStack.top();
    undoStack.pop();

    if (op.type == Operation::Type::ADD)
    {
        Rows[op.y].erase(op.x, 1);
    }
    else if (op.type == Operation::Type::REMOVE)
    {
        if (op.x == 0 && op.y > 0)
        {
            Rows[op.y - 1].erase(Rows[op.y - 1].size() - 1);
            Rows.insert(Rows.begin() + op.y, string(1, op.ch));
        }
        else
        {
            Rows[op.y].insert(op.x - 1, 1, op.ch);
        }
    }

    redoStack.push(op);

    _TextViewImp->SetCursorY(op.y);
    _TextViewImp->SetCursorX(op.x);
    _TextViewImp->Refresh();
    UpdateTextViewImpRows();
}

void ECController::HandleEnter()
{
    int current_x = _TextViewImp->GetCursorX();
    int current_y = _TextViewImp->GetCursorY();

    int max_y = _TextViewImp->GetRowNumInView() - 1;

    if (current_y == Rows.size() - 1 || Rows.size() == 0)
    {
        Rows.resize(current_y + 1);
    }

    string remaining_text = Rows[current_y].substr(current_x);
    Rows[current_y].erase(current_x, string::npos);

    Rows.insert(Rows.begin() + current_y + 1, remaining_text);

    if (Rows.size() > max_y + 1)
    {                                    // If the total number of rows in view would exceed max_y after inserting the new row
        DownRowDeque.push_back(Rows[0]); // Push the first row in the view to the deque
        Rows.erase(Rows.begin());        // Remove the first row from the view
    }

    _TextViewImp->SetCursorY(current_y == max_y ? max_y : current_y + 1); // Move the cursor to the next line if possible, otherwise keep it at the same line
    _TextViewImp->SetCursorX(0);
    _TextViewImp->Refresh();
    UpdateTextViewImpRows();
}

void ECController::UpdateTextViewImpRows()
{
    _TextViewImp->InitRows();
    for (const auto &row : Rows)
    {
        _TextViewImp->AddRow(row);
    }
    HighlightKeywords();
    _TextViewImp->Refresh();
}

void ECController::Redo()
{
    if (redoStack.empty())
        return;

    Operation op = redoStack.top();
    redoStack.pop();

    if (op.type == Operation::Type::ADD)
    {
        Rows[op.y].insert(op.x, 1, op.ch);
    }
    else if (op.type == Operation::Type::REMOVE)
    {
        if (op.x == 0 && op.y > 0)
        {
            Rows[op.y - 1].push_back(Rows[op.y][0]);
            Rows.erase(Rows.begin() + op.y);
        }
        else
        {
            Rows[op.y].erase(op.x - 1, 1);
        }
    }

    undoStack.push(op);

    _TextViewImp->SetCursorY(op.y);
    _TextViewImp->SetCursorX(op.x);
    _TextViewImp->Refresh();
    UpdateTextViewImpRows();
}

void ECController::LoadKeywords()
{
    ifstream file;

    if (_filename.substr(_filename.length() - 2) == "py")
    {
        file.open("pykeywords.txt");
    }
    else if (_filename.substr(_filename.length() - 3) == "cpp")
    {
        file.open("cppkeywords.txt");
    }
    else if (_filename.substr(_filename.length() - 1) == "c")
    {
        file.open("ckeywords.txt");
    }

    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            istringstream iss(line);
            string keyword;
            while (iss >> keyword)
            {
                keywords.insert(keyword);
            }
        }
        file.close();
    }
}

void ECController::HighlightKeywords()
{
    _TextViewImp->ClearColor();

    int rowNum = 0;
    for (const auto &row : Rows)
    {
        // Highlight keywords
        for (const auto &keyword : keywords)
        {
            std::regex keyword_regex("\\b" + keyword + "\\b");
            std::sregex_iterator it(row.begin(), row.end(), keyword_regex);
            std::sregex_iterator reg_end;

            for (; it != reg_end; ++it)
            {
                std::smatch match = *it;
                size_t pos = match.position();
                _TextViewImp->SetColor(rowNum, pos, pos + keyword.length(), TEXT_COLOR_BLUE);
            }
        }

        // Highlight brackets
        for (int i = 0; i < row.size(); i++)
        {
            for (int j = 0; j < brackets.size(); ++j)
            {
                if (row[i] == brackets[j])
                {
                    _TextViewImp->SetColor(rowNum, i, i, TEXT_COLOR_RED);
                }
            }
        }

        rowNum++;
    }
    _TextViewImp->Refresh();
}
