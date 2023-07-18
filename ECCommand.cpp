#include "ECCommand.h"
#include "ECController.h"
#include <string>
#include <stack>
using namespace std;
//
InsertTextCommand::InsertTextCommand(ECTextViewImp *TextViewImp, ECController *Controller, char ch) : _TextViewImp(TextViewImp), _ch(ch), _Controller(Controller)
{
    _cursorX = _TextViewImp->GetCursorX();
    _cursorY = _TextViewImp->GetCursorY();
}

void InsertTextCommand::execute()
{
    if (_cursorY > _Controller->GetRows().size())
    {
        _Controller->GetRows().resize(_cursorY + 1);
    }

    _Controller->GetRows()[_cursorY].insert(_cursorX, 1, _ch);
    _TextViewImp->SetCursorX(_cursorX + 1);
    _Controller->UpdateTextViewImpRows();
}

void InsertTextCommand::unexecute()
{
    _TextViewImp->SetCursorX(_cursorX);
    _TextViewImp->SetCursorY(_cursorY);
    _Controller->GetRows()[_cursorY].erase(_cursorX, 1);
    _Controller->UpdateTextViewImpRows();
}



// Merging two rows when removing text is done by MergeLinesCommand
RemoveTextCommand::RemoveTextCommand(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller)
{
    _cursorX = _TextViewImp->GetCursorX();
    _cursorY = _TextViewImp->GetCursorY();
}

void RemoveTextCommand::execute()
{
    _removedChar = _Controller->GetRows()[_cursorY].at(_cursorX - 1); // Save the character that is about to be removed
    _Controller->GetRows()[_cursorY].erase(_cursorX - 1, 1);
    _TextViewImp->SetCursorX(_cursorX - 1);
    _Controller->UpdateTextViewImpRows();
}

void RemoveTextCommand::unexecute()
{
    _TextViewImp->SetCursorX(_cursorX - 1);
    _TextViewImp->SetCursorY(_cursorY);
    _Controller->GetRows()[_cursorY].insert(_cursorX - 1, 1, _removedChar);
    _Controller->UpdateTextViewImpRows();
}



//
EnterCommand::EnterCommand(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller)
{
    _cursorX = _TextViewImp->GetCursorX();
    _cursorY = _TextViewImp->GetCursorY();
}

void EnterCommand::execute()
{
        _split_pos = _cursorX;
        _removedFromTop = false;
        
}

void EnterCommand::unexecute()
{
}



//
MergeLinesCommand::MergeLinesCommand(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller)
{
    _cursorX = _TextViewImp->GetCursorX();
    _cursorY = _TextViewImp->GetCursorY();
}

void MergeLinesCommand::execute()
{
    string row_contents = _Controller->GetRows()[_cursorY];
    _Controller->GetRows().erase(_Controller->GetRows().begin() + _cursorY);
    _TextViewImp->SetCursorX(_Controller->GetRows()[_cursorY - 1].size());
    _Controller->GetRows()[_cursorY - 1].append(row_contents);
    _TextViewImp->SetCursorY(_cursorY - 1);
    _Controller->UpdateTextViewImpRows();
}

void MergeLinesCommand::unexecute()
{
    int split_pos = _Controller->GetRows()[_cursorY].size() - _cursorX;
    _TextViewImp->SetCursorY(_cursorY);
    _TextViewImp->SetCursorX(split_pos);
    string new_row = _Controller->GetRows()[_cursorY].substr(split_pos);
    _Controller->GetRows()[_cursorY] = _Controller->GetRows()[_cursorY].substr(0, split_pos);
    _Controller->GetRows().insert(_Controller->GetRows().begin() + _cursorY + 1, new_row);
    _Controller->UpdateTextViewImpRows();
}



// void EnterCommand::execute()
// {
    // _split_pos = _cursorX;
    // _removedFromTop = false;
//     int max_y = _TextViewImp->GetRowNumInView() - 1;

//     //break row from cursor position onwards and make it
//     _remaining_text = _Controller->GetRows()[_cursorY].substr(_split_pos);
//     _Controller->GetRows()[_cursorY].erase(_split_pos, string::npos);
//     _Controller->GetRows().insert(_Controller->GetRows().begin() + _cursorY + 1, _remaining_text);
    
    
//     if (_Controller->GetRows().size() > _TextViewImp->GetRowNumInView())
//     {
//         _removedRow = _Controller->GetRows()[0];
//         _Controller->Get_Top_Rows().push(_Controller->GetRows()[0]);
//         _Controller->GetRows().erase(_Controller->GetRows().begin());
//         _removedFromTop = true;
//     }

//     if (_cursorY < max_y)
//         _TextViewImp->SetCursorY(_cursorY + 1);
//     _TextViewImp->SetCursorX(0);
//     _Controller->UpdateTextViewImpRows();
// }

// void EnterCommand::unexecute()
// {
//     if (_removedFromTop)
//     {
//         _Controller->GetRows().insert(_Controller->GetRows().begin(), _removedRow);
//         _Controller->Get_Top_Rows().pop();
//     }
//     _TextViewImp->SetCursorY(_cursorY);
//     _TextViewImp->SetCursorX(_split_pos);
//     _Controller->GetRows()[_cursorY].append(_Controller->GetRows()[_cursorY + 1]);
//     _Controller->GetRows().erase(_Controller->GetRows().begin() + _cursorY + 1);
//     _Controller->UpdateTextViewImpRows();
// }



