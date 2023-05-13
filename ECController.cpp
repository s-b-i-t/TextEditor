// ECController.cpp
#include "ECController.h"
#include "ECTextViewImp.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include "ECCommand.h"
using namespace std;

ECController::ECController(ECTextViewImp *TextViewImp, const std::string &filename) : _TextViewImp(TextViewImp), _filename(filename)
{
    OpenFile();
    LoadKeywords();
    HighlightKeywords();
    if (Rows.size() == 0)
        Rows.push_back("");
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

        if (current_x > 0)
        {
            _TextViewImp->SetCursorX(current_x - 1);
        }
        else if (current_y > 0)
        {
            int prev_row_length = Rows[current_y - 1].length();
            _TextViewImp->SetCursorY(current_y - 1);
            _TextViewImp->SetCursorX(prev_row_length);
        }
        break;
    }

    case ARROW_RIGHT:
    {
        int current_y = _TextViewImp->GetCursorY();
        int current_x = _TextViewImp->GetCursorX();
        int max_x = Rows[current_y].length();

        if (current_x < max_x)
        {
            _TextViewImp->SetCursorX(current_x + 1);
        }
        else if (current_y < Rows.size() - 1)
        {
            _TextViewImp->SetCursorY(current_y + 1);
            _TextViewImp->SetCursorX(0);
        }
        break;
    }

    case ARROW_UP:
    {
        int current_y = _TextViewImp->GetCursorY();
        int current_x = _TextViewImp->GetCursorX();
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

        int current_y = _TextViewImp->GetCursorY();
        int current_x = _TextViewImp->GetCursorX();
        int max_y = _TextViewImp->GetRowNumInView() - 1;

        if (current_y == Rows.size() - 1)
            break;

        if (current_y < max_y)
        {
            int next_row_length = Rows[current_y + 1].length();
            int new_x = min(current_x, next_row_length);

            _TextViewImp->SetCursorY(current_y + 1);
            _TextViewImp->SetCursorX(new_x);
        }

        break;
    }

    case ESC:
    {
        string tt = to_string(_TextViewImp->GetRowNumInView());
        curStatus = "command";
        _TextViewImp->ClearStatusRows();
        _TextViewImp->AddStatusRow("ctrl-h for help", "mode: " + tt, true);
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
        Undo();
        break;
    }
    case CTRL_Y:
    {
        Redo();
        break;
    }
    }

    _TextViewImp->Refresh();
}

void ECController::AddText(char ch)
{   
    if (curStatus == "command")
        return;
    ECCommand* command = new InsertTextCommand(_TextViewImp, this, ch);
    command->execute();
    CommandStack.push(command);
    HighlightKeywords();
}

void ECController::RemoveText()
{
    if (curStatus == "command")
        return;
    int y = _TextViewImp->GetCursorY();
    int x = _TextViewImp->GetCursorX();


    if (y >= Rows.size())
    {
        return;
    }

    if (x > 0)
    {
        ECCommand* command = new RemoveTextCommand(_TextViewImp, this);
        command->execute();
        CommandStack.push(command);
    }
    else
    {
        if (y > 0)
        {
            ECCommand* command = new MergeLinesCommand(_TextViewImp, this);
            command->execute();
            CommandStack.push(command);
        }
    }

    _TextViewImp->Refresh();
    HighlightKeywords();
}




void ECController::HandleEnter()
{
    if (curStatus == "command")
        return;
    ECCommand* command = new EnterCommand(_TextViewImp, this);
    command->execute();
    CommandStack.push(command);

}



void ECController::Undo()
{
    if (CommandStack.empty())
        return;

    ECCommand* command = CommandStack.top();
    CommandStack.pop();
    command->unexecute();

    RedoStack.push(command);
}

void ECController::Redo()
{
    if (RedoStack.empty())
        return;

    ECCommand* command = RedoStack.top();
    RedoStack.pop();
    command->execute();

    CommandStack.push(command);
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

void ECController::LoadKeywords()
{
    ifstream file;

    if (_filename.substr(_filename.length() - 2) == "py")
    {
        file.open("dependencies/pykeywords.txt");
    }
    else if (_filename.substr(_filename.length() - 3) == "cpp" || _filename.substr(_filename.length() - 1) == "c")
    {
        file.open("dependencies/cppkeywords.txt");
    }
    else if (_filename.substr(_filename.length() - 1) == "c" || _filename.substr(_filename.length() - 1) == "h")
    {
        file.open("dependencies/ckeywords.txt");
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
