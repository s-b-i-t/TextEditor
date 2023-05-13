// ECCommand.cpp

#include "ECCommand.h"
#include "ECController.h"
#include <string>
using namespace std;

InsertTextCommand::InsertTextCommand(ECTextViewImp *TextViewImp, ECController *Controller, char ch) : _TextViewImp(TextViewImp), _ch(ch), _Controller(Controller) {}

void InsertTextCommand::execute()
{
    int current_y = _TextViewImp->GetCursorY();
    int current_x = _TextViewImp->GetCursorX();
    if (current_y > _Controller->GetRows().size())
    {
        _Controller->GetRows().resize(current_y + 1);
    }

    _Controller->GetRows()[current_y].insert(current_x, 1, _ch);
    _TextViewImp->SetCursorX(current_x + 1);
    _Controller->UpdateTextViewImpRows();
}

void InsertTextCommand::unexecute()
{
    int current_y = _TextViewImp->GetCursorY();
    int current_x = _TextViewImp->GetCursorX();

    _Controller->GetRows()[current_y].erase(current_x - 1, 1);
    _TextViewImp->SetCursorX(current_x - 1);
}

RemoveTextCommand::RemoveTextCommand(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller) {}

void RemoveTextCommand::execute()
{
    int current_y = _TextViewImp->GetCursorY();
    int current_x = _TextViewImp->GetCursorX();
    _removedChar = _Controller->GetRows()[current_y].at(current_x - 1); // Save the character that is about to be removed
    _Controller->GetRows()[current_y].erase(current_x - 1, 1);
    _TextViewImp->SetCursorX(current_x - 1);
    _Controller->UpdateTextViewImpRows();
}

void RemoveTextCommand::unexecute()
{
    int current_y = _TextViewImp->GetCursorY();
    int current_x = _TextViewImp->GetCursorX();
    _Controller->GetRows()[current_y].insert(current_x, 1, _removedChar);
    _TextViewImp->SetCursorX(current_x + 1);
    _Controller->UpdateTextViewImpRows();
}

EnterCommand::EnterCommand(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller) {}

void EnterCommand::execute()
{
    int current_y = _TextViewImp->GetCursorY();
    _split_pos = _TextViewImp->GetCursorX();

    _remaining_text = _Controller->GetRows()[current_y].substr(_split_pos);
    _Controller->GetRows()[current_y].erase(_split_pos, string::npos);
    _Controller->GetRows().insert(_Controller->GetRows().begin() + current_y + 1, _remaining_text);

    _TextViewImp->SetCursorY(current_y + 1);
    _TextViewImp->SetCursorX(0);
    _Controller->UpdateTextViewImpRows();
}

void EnterCommand::unexecute()
{
    int current_y = _TextViewImp->GetCursorY();
    _Controller->GetRows()[current_y - 1].append(_Controller->GetRows()[current_y]);
    _Controller->GetRows().erase(_Controller->GetRows().begin() + current_y);
    _TextViewImp->SetCursorY(current_y - 1);
    _TextViewImp->SetCursorX(_split_pos);
    _Controller->UpdateTextViewImpRows();
}

MergeLinesCommand::MergeLinesCommand(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller) {}

void MergeLinesCommand::execute()
{
    int current_y = _TextViewImp->GetCursorY();
    string row_contents = _Controller->GetRows()[current_y];
    _Controller->GetRows().erase(_Controller->GetRows().begin() + current_y);
    _TextViewImp->SetCursorX(_Controller->GetRows()[current_y - 1].size());
    _Controller->GetRows()[current_y - 1].append(row_contents);
    _TextViewImp->SetCursorY(current_y - 1);
    _Controller->UpdateTextViewImpRows();
}

void MergeLinesCommand::unexecute()
{
    int current_y = _TextViewImp->GetCursorY();
    int current_x = _TextViewImp->GetCursorX();
    int split_pos = _Controller->GetRows()[current_y].size() - _TextViewImp->GetCursorX();
    string new_row = _Controller->GetRows()[current_y].substr(split_pos);
    _Controller->GetRows()[current_y] = _Controller->GetRows()[current_y].substr(0, split_pos);
    _Controller->GetRows().insert(_Controller->GetRows().begin() + current_y + 1, new_row);
    _TextViewImp->SetCursorY(current_y + 1);
    _TextViewImp->SetCursorX(0);
    _Controller->UpdateTextViewImpRows();
}
