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
#include <set>
#include <sstream>
#include <iomanip>

using namespace std;

ECController::ECController(ECTextViewImp *TextViewImp, const std::string &filename) : _TextViewImp(TextViewImp), _filename(filename)
{
    OpenFile();
    LoadKeywords();
    HighlightKeywords();
    if (Rows.size() == 0)
        Rows.push_back("");

    UpdateStatusRow(0, 0, 0, 0, 0, 0, "");
}

// debugging purposes
void ECController::UpdateStatusRow(int cx, int cy, int nx, int ny, int mx, int my, string key)
{
    _TextViewImp->ClearStatusRows();

    std::ostringstream text1;
    text1 << "current line: " << to_string(Get_Row_Num(ny)) 
          << " Out of viewline 5? " << (OutofView(5) ? "true" : "false");

    std::ostringstream text2;
    text2 << "key: " << key
          << " old pos:(" << to_string(cx) << "," << to_string(cy) << ")"
          << " new pos:(" << to_string(nx) << "," << to_string(ny) << ") max pos: (" << to_string(mx) << "," << to_string(my) << ")";

    _TextViewImp->AddStatusRow(text1.str(), text2.str(), true);
}


void ECController::OpenFile()
{
    ifstream file(_filename);
    int max_y = _TextViewImp->GetRowNumInView() - 1;
    if (file.is_open())
    {
        string line;
        int current_row = 0;
        while (getline(file, line))
        {
            if (current_row < max_y)
            {
                Rows.push_back(line);
            }
            else
            {
                // Use a stack to store overflow lines in reverse order
                // so that the line immediately after the last line in Rows
                // is at the top of Bottom_Rows.
                Bottom_Rows.push(line);
            }
            current_row++;
        }
        file.close();
    }
    // Reverse the order of lines in Bottom_Rows to match the original file order.
    stack<string> temp;
    while (!Bottom_Rows.empty())
    {
        temp.push(Bottom_Rows.top());
        Bottom_Rows.pop();
    }
    Bottom_Rows = temp;
    if (ShowLines)
        _TextViewImp->SetCursorX(5);
    UpdateTextViewImpRows();
}

void ECController::SaveFile()
{
    ofstream file(_filename);

    // write Top_Rows to the file in the correct order
    stack<string> temp = Top_Rows;
    vector<string> tempVec;
    while (!temp.empty())
    {
        tempVec.push_back(temp.top());
        temp.pop();
    }
    for (auto rit = tempVec.rbegin(); rit != tempVec.rend(); ++rit)
    {
        file << *rit << endl;
    }

    // write Rows to the file
    for (const auto &row : Rows)
    {
        file << row << endl;
    }

    // write Bottom_Rows to the file in the correct order
    temp = Bottom_Rows;
    tempVec.clear();
    while (!temp.empty())
    {
        tempVec.push_back(temp.top());
        temp.pop();
    }
    for (auto rit = tempVec.rbegin(); rit != tempVec.rend(); ++rit)
    {
        file << *rit << endl;
    }

    file.close();
}

void ECController::HandleKey(int key)
{
    switch (key)
    {
    case ARROW_LEFT:
    {
        int current_y = Get_Cur_Y();
        int current_x = Get_Cur_X();
        int max_x = Get_Max_X();
        int max_y = Get_Max_Y();

        int new_x = current_x;
        int new_y = current_y;

        if (current_x == GetRowStart())
            HandleWrapUp();

        if (current_x > GetRowStart())
        {
            new_x = current_x - 1;
        }

        else if (current_y > 0)
        {
            int prev_row_length = Rows[current_y - 1].length();
            new_y = current_y - 1;
            new_x = prev_row_length + GetRowStart(); // potential bug
        }

        _TextViewImp->SetCursorX(new_x);
        _TextViewImp->SetCursorY(new_y);

        UpdateStatusRow(current_x, current_y, new_x, new_y, max_x, max_y, "left");
        break;
    }

    case ARROW_RIGHT:
    {
        int current_y = Get_Cur_Y();
        int current_x = Get_Cur_X();
        int max_x = Get_Max_X();
        int max_y = Get_Max_Y();

        int new_y = Get_Cur_Y();
        int new_x = Get_Cur_X();

        if (Bottom_Rows.empty() && current_y == max_y && current_x == max_x)
            return;

        if (current_x < max_x)
        {
            new_x = current_x + 1;
        }
        else if (current_y < max_y)
        {
            new_y = current_y + 1;
            new_x = GetRowStart();
        }
        else if (current_y == max_y && current_x == max_x && !Bottom_Rows.empty())
        {
            HandleWrapDown();
            new_x = GetRowStart();
        }

        _TextViewImp->SetCursorX(new_x);
        _TextViewImp->SetCursorY(new_y);

        UpdateStatusRow(current_x, current_y, new_x, new_y, max_x, max_y, "right");

        break;
    }

    case ARROW_UP:
    {
        int current_y = Get_Cur_Y();
        int current_x = Get_Cur_X();
        int max_x = Get_Max_X();
        int max_y = Get_Max_Y();

        int new_y = current_y;
        int new_x = current_x;

        if (current_y > 0)
        {
            new_y = current_y - 1;
            int prev_row_length = Rows[new_y].length();
            new_x = min(current_x, prev_row_length + GetRowStart());
        }
        else if (current_y == 0 && !Top_Rows.empty())
        {
            HandleWrapUp();
            new_y = 0; // since we're wrapping stay at the top row
            int prev_row_length = Rows[new_y].length();
            new_x = min(current_x, prev_row_length + GetRowStart());
        }

        _TextViewImp->SetCursorY(new_y);
        _TextViewImp->SetCursorX(new_x);

        UpdateStatusRow(current_x, current_y, new_x, new_y, max_x, max_y, "up");

        break;
    }

    case ARROW_DOWN:
    {
        int current_y = Get_Cur_Y();
        int current_x = Get_Cur_X();
        int max_x = Get_Max_X();
        int max_y = Get_Max_Y();

        int new_y = current_y;
        int new_x = current_x;

        if (current_y < max_y)
        {
            new_y = current_y + 1;
            int next_row_length = Rows[new_y].length();
            new_x = min(current_x, next_row_length + GetRowStart());
        }
        else if (current_y == max_y && !Bottom_Rows.empty())
        {
            HandleWrapDown();
            new_y = max_y; // since we're wrapping stay at the bottom row
            int next_row_length = Rows[new_y].length();
            new_x = min(current_x, next_row_length + GetRowStart());
        }

        _TextViewImp->SetCursorY(new_y);
        _TextViewImp->SetCursorX(new_x);

        UpdateStatusRow(current_x, current_y, new_x, new_y, max_x, max_y, "down");

        break;
    }
    case ESC:
    {

        int current_y = Get_Cur_Y();
        int current_x = Get_Cur_X();
        int max_x = Get_Max_X();
        int max_y = Get_Max_Y();

        int new_x = Get_Cur_X();
        int new_y = Get_Cur_Y();

        UpdateStatusRow(current_x, current_y, new_x, new_y, max_x, max_y, "esc");

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
        break;
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
    int current_y = Get_Cur_Y();
    int current_x = Get_Cur_X();
    int max_x = Get_Max_X();
    int max_y = Get_Max_Y();

    ECCommand *command = new InsertTextCommand(_TextViewImp, this, ch);
    command->execute();
    CommandStack.push(command);
    HighlightKeywords();

    int new_x = Get_Cur_X();
    int new_y = Get_Cur_Y();    

    UpdateStatusRow(current_x, current_y, new_x, new_y, max_x, max_y, std::string(1, ch));
}

void ECController::RemoveText()
{

    int current_x = Get_Cur_X();
    int current_y = Get_Cur_Y();

    if (current_x == GetRowStart() && current_y == 0)
        HandleWrapUp(); // cursor should go to last char

    if (current_y >= Rows.size())
    {
        return;
    }

    if (current_x > GetRowStart())
    {
        ECCommand *command = new RemoveTextCommand(_TextViewImp, this);
        command->execute();
        CommandStack.push(command);
    }
    else
    {
        if (current_y > 0)
        {
            ECCommand *command = new MergeLinesCommand(_TextViewImp, this);
            command->execute();
            CommandStack.push(command);
        }
    }

    HighlightKeywords();

    int new_x = Get_Cur_X();
    int new_y = Get_Cur_Y();
    int max_x = Get_Max_X();
    int max_y = Get_Max_Y();
    
    UpdateStatusRow(current_x, current_y, new_x, new_y, max_x, max_y, "backspace");    
}

void ECController::HandleEnter()
{
    if (curStatus == "command")
        return;

    int current_y = Get_Cur_Y();
    int current_x = Get_Cur_X();
    int max_x = Get_Max_X();
    int max_y = Get_Max_Y();

    int rows_size = Rows[current_y].length();

    ECCommand *command = new EnterCommand(_TextViewImp, this);
    command->execute();
    CommandStack.push(command);

    if (current_y == max_y)
        HandleWrapUp();
    int new_x = Get_Cur_X();
    int new_y = Get_Cur_Y();

    UpdateStatusRow(current_x, current_y, new_x, new_y, max_x, max_y, "enter");
}

void ECController::Undo()
{
    if (CommandStack.empty())
        return;

    ECCommand *command = CommandStack.top();
    CommandStack.pop();
    command->unexecute();

    RedoStack.push(command);
}

void ECController::Redo()
{
    if (RedoStack.empty())
        return;

    ECCommand *command = RedoStack.top();
    RedoStack.pop();
    command->execute();

    CommandStack.push(command);
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
        for (const auto &keyword : keywords)
        {
            size_t start_pos = 0;
            while ((start_pos = row.find(keyword, start_pos)) != std::string::npos)
            {
                size_t end_pos = start_pos + keyword.length();
                if ((start_pos == 0 || !isalnum(row[start_pos - 1])) &&
                    (end_pos == row.length() || !isalnum(row[end_pos])))
                {
                    _TextViewImp->SetColor(rowNum, start_pos, end_pos, TEXT_COLOR_BLUE);
                }
                start_pos += keyword.length();
            }
        }

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

void ECController::HandleWrapDown()
{
    int current_y = Get_Cur_Y();
    int max_y = Get_Max_Y();

    if (current_y == max_y && !Bottom_Rows.empty())
    {
        Top_Rows.push(Rows[0]);
        Rows.erase(Rows.begin());
        Rows.push_back(Bottom_Rows.top());
        Bottom_Rows.pop();
        UpdateTextViewImpRows();
    }
}

void ECController::HandleWrapUp()
{
    int current_y = Get_Cur_Y();
    if (current_y == 0 && !Top_Rows.empty())
    {
        Bottom_Rows.push(Rows[Rows.size() - 1]);
        Rows.pop_back();
        Rows.insert(Rows.begin(), Top_Rows.top());
        Top_Rows.pop();
        UpdateTextViewImpRows();
    }
}

void ECController ::HandleWrapRight()
{
    int current_y = Get_Cur_Y();
    int current_x = Get_Cur_X();
    int max_x = Get_Max_X();
    int max_y = Get_Max_Y();
}

void ECController ::HandleWrapLeft()
{
    ;
}

void ECController::UpdateTextViewImpRows()
{
    _TextViewImp->InitRows();
    int lineNumber = Top_Rows.size();
    if (ShowLines)
    {
        for (const auto &row : Rows)
        {
            lineNumber++;
            LinesInView.insert(lineNumber);
            std::ostringstream oss;
            oss << std::setw(3) << std::setfill(' ') << lineNumber << "  " << row;
            _TextViewImp->AddRow(oss.str());
        }
    }
    else
    {
        for (const auto &row : Rows)
        {
            lineNumber++;
            _TextViewImp->AddRow(row);
        }
    }
    HighlightKeywords();
    _TextViewImp->Refresh();
}
