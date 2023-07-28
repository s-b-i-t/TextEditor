#include "ECCommand.h"
#include "ECController.h"
#include <string>
#include <stack>
using namespace std;
//
InsertTextCommand::InsertTextCommand(ECTextViewImp *TextViewImp, ECController *Controller, char ch) : _TextViewImp(TextViewImp), _ch(ch), _Controller(Controller)
{
    current_x = _Controller->Get_Cur_X();
    current_y = _Controller->Get_Cur_Y();
    max_x = _Controller->Get_Max_X();
    max_y = _Controller->Get_Max_Y();
    current_line = _Controller->Get_Row_Num(current_y);
}

void InsertTextCommand::execute() 
{
    if (current_y > _Controller->GetRows().size())
    {
        _Controller->GetRows().resize(current_y + 1);
    }

    _Controller->GetRows()[current_y].insert(current_x - _Controller->GetRowStart(), 1, _ch);
    _TextViewImp->SetCursorX(current_x + 1);
    _Controller->UpdateTextViewImpRows();
}

void InsertTextCommand::unexecute() 
{
    _TextViewImp->SetCursorX(current_x);
    _TextViewImp->SetCursorY(current_y);
    _Controller->GetRows()[current_y].erase(current_x - _Controller->GetRowStart(), 1);
    _Controller->UpdateTextViewImpRows();
}

// Merging two rows when removing text is done by MergeLinesCommand
RemoveTextCommand::RemoveTextCommand(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller)
{
    current_x = _Controller->Get_Cur_X();
    current_y = _Controller->Get_Cur_Y();
    max_x = _Controller->Get_Max_X();
    max_y = _Controller->Get_Max_Y();
        current_line = _Controller->Get_Row_Num(current_y);

}

void RemoveTextCommand::execute() 
{
    _removedChar = _Controller->GetRows()[current_y].at(current_x - _Controller->GetRowStart() - 1); // Save the character that is about to be removed
    _Controller->GetRows()[current_y].erase(current_x - _Controller->GetRowStart() - 1, 1);
    _TextViewImp->SetCursorX(current_x - 1);
    _Controller->UpdateTextViewImpRows();
}

void RemoveTextCommand::unexecute() 
{
    _TextViewImp->SetCursorX(current_x - 1);
    _TextViewImp->SetCursorY(current_y);
    _Controller->GetRows()[current_y].insert(current_x - _Controller->GetRowStart() - 1, 1, _removedChar);
    _Controller->UpdateTextViewImpRows();
}

//
MergeLinesCommand::MergeLinesCommand(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller)
{
    current_x = _Controller->Get_Cur_X();
    current_y = _Controller->Get_Cur_Y();
    max_x = _Controller->Get_Max_X();
    max_y = _Controller->Get_Max_Y();
    current_line = _Controller->Get_Row_Num(current_y);

}

void MergeLinesCommand::execute() 
{
    string row_contents = _Controller->GetRows()[current_y];
    _Controller->GetRows()[current_y - 1].append(row_contents);  // Merge the current line with the previous one

    // Instead of erasing the current line, shift all the lines below it up by one
    for (int i = current_y; i < _Controller->GetRows().size() - 1; ++i)
    {
        _Controller->GetRows()[i] = _Controller->GetRows()[i + 1];
    }

    // If there are lines in Bottom_Rows, bring the top one into view
    if (!_Controller->Get_Bottom_Rows().empty()) 
    {
        _Controller->GetRows().back() = _Controller->Get_Bottom_Rows().top();
        _Controller->Get_Bottom_Rows().pop();
    }
    else  // If there are no lines in Bottom_Rows, simply erase the last line in view
    {
        _Controller->GetRows().pop_back();
    }

    _TextViewImp->SetCursorX(_Controller->GetRows()[current_y - 1].size() + _Controller->GetRowStart());
    _TextViewImp->SetCursorY(current_y - 1);
    _Controller->UpdateTextViewImpRows();
}


void MergeLinesCommand::unexecute() 
{
    int split_pos = _Controller->GetRows()[current_y].size() - current_x;
    _TextViewImp->SetCursorY(current_y);
    _TextViewImp->SetCursorX(split_pos);
    string new_row = _Controller->GetRows()[current_y].substr(split_pos);
    _Controller->GetRows()[current_y] = _Controller->GetRows()[current_y].substr(0, split_pos);
    _Controller->GetRows().insert(_Controller->GetRows().begin() + current_y + 1, new_row);
    _Controller->UpdateTextViewImpRows();
}



//
EnterCommand::EnterCommand(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller)
{
    current_x = _Controller->Get_Cur_X();
    current_y = _Controller->Get_Cur_Y();
    max_x = _Controller->Get_Max_X();
    max_y = _Controller->Get_Max_Y();
    current_line = _Controller->Get_Row_Num(current_y);

}

void EnterCommand::execute() 
{
    _split_pos = current_x - _Controller->GetRowStart();  // consider the offset of line numbers
    _removedFromTop = false;
    _removedFromBottom = false;

    // break row from cursor position onwards and make it
    _remaining_text = _Controller->GetRows()[current_y].substr(_split_pos);
    _Controller->GetRows()[current_y].erase(_split_pos, string::npos);
    _Controller->GetRows().insert(_Controller->GetRows().begin() + min(current_y + 1, (int)_Controller->GetRows().size()), _remaining_text);

    if (_Controller->GetRows().size() > _TextViewImp->GetRowNumInView())
    {
        if (current_y < max_y)
        {
            _removedRow = _Controller->GetRows().back();
            _Controller->Get_Bottom_Rows().push(_removedRow);
            _Controller->GetRows().pop_back();
            _removedFromBottom = true;
        }
        else
        {
            _removedRow = _Controller->GetRows().front();
            _Controller->Get_Top_Rows().push(_Controller->GetRows().front());
            _Controller->GetRows().erase(_Controller->GetRows().begin());
            _removedFromTop = true;
        }
    }

    // Update condition to move cursor to new row
    if (current_y < _TextViewImp->GetRowNumInView() - 1 || _Controller->GetRows().size() < _TextViewImp->GetRowNumInView())
        _TextViewImp->SetCursorY(current_y + 1);

    _TextViewImp->SetCursorX(_Controller->GetRowStart());
    _Controller->UpdateTextViewImpRows();
}




void EnterCommand::unexecute() 
{
    if (_removedFromTop)
    {
        _Controller->GetRows().insert(_Controller->GetRows().begin(), _removedRow);
        _Controller->Get_Top_Rows().pop();
    }
    else if (_removedFromBottom)
    {
        _Controller->GetRows().push_back(_removedRow);
        _Controller->Get_Bottom_Rows().pop();
    }

    _TextViewImp->SetCursorY(current_y);
    _TextViewImp->SetCursorX(_split_pos);
    _Controller->GetRows()[current_y].append(_Controller->GetRows()[current_y + 1]);
    _Controller->GetRows().erase(_Controller->GetRows().begin() + current_y + 1);
    _Controller->UpdateTextViewImpRows();
}
