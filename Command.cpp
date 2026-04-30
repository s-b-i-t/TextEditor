#include "Command.h"
#include "Controller.h"
#include <string>
#include <algorithm>
//
InsertTextCommand::InsertTextCommand(Controller *Controller, char ch) : _Controller(Controller), _ch(ch)
{
    current_x = _Controller->GetCursorDocX();
    current_y = _Controller->GetCursorDocY();
}

void InsertTextCommand::execute()
{
    auto &model = _Controller->GetModel();
    model.SetCursor(current_y, current_x);
    model.InsertChar(_ch);
}

void InsertTextCommand::unexecute()
{
    auto &model = _Controller->GetModel();
    model.SetCursor(current_y, current_x);
    model.Delete();
}

// Merging two rows when removing text is done by MergeLinesCommand
RemoveTextCommand::RemoveTextCommand(Controller *Controller) : _Controller(Controller)
{
    current_x = _Controller->GetCursorDocX();
    current_y = _Controller->GetCursorDocY();
}

void RemoveTextCommand::execute()
{
    auto &model = _Controller->GetModel();
    const auto &lines = model.GetLines();
    if (current_y < 0 || current_y >= (int)lines.size())
        return;
    const auto &row = lines[current_y];
    if (current_x <= 0 || current_x > (int)row.size())
        return;

    _removedPos = current_x - 1;
    _removedChar = row.at(_removedPos);

    model.SetCursor(current_y, current_x);
    model.Backspace();
}

void RemoveTextCommand::unexecute()
{
    auto &model = _Controller->GetModel();
    model.SetCursor(current_y, _removedPos);
    model.InsertChar(_removedChar);
}

//
MergeLinesCommand::MergeLinesCommand(Controller *Controller) : _Controller(Controller)
{
    current_x = _Controller->GetCursorDocX();  // expected to be 0 for merge-lines
    current_y = _Controller->GetCursorDocY();
}

void MergeLinesCommand::execute()
{
    auto &model = _Controller->GetModel();
    const auto &lines = model.GetLines();
    if (current_y <= 0 || current_y >= (int)lines.size())
        return;

    current_line = (int)lines[current_y - 1].size();

    model.SetCursor(current_y, 0);
    model.Backspace(); // merges current_y into current_y-1
}


void MergeLinesCommand::unexecute()
{
    // Undo merge: split the previous row back into two at current_line.
    auto &model = _Controller->GetModel();
    const auto &lines = model.GetLines();
    if (current_y <= 0 || current_y - 1 >= (int)lines.size())
        return;

    model.SetCursor(current_y - 1, current_line);
    model.InsertNewline();
}

//
EnterCommand::EnterCommand(Controller *Controller) : _Controller(Controller)
{
    current_x = _Controller->GetCursorDocX();
    current_y = _Controller->GetCursorDocY();
}

void EnterCommand::execute()
{
    auto &model = _Controller->GetModel();
    const auto &lines = model.GetLines();
    if (current_y < 0 || current_y >= (int)lines.size())
        return;

    const int split = std::clamp(current_x, 0, (int)lines[current_y].size());
    _split_pos = split;

    model.SetCursor(current_y, split);
    model.InsertNewline();
}

void EnterCommand::unexecute()
{
    auto &model = _Controller->GetModel();
    const auto &lines = model.GetLines();
    if (current_y < 0 || current_y >= (int)lines.size())
        return;

    const int nextY = current_y + 1;
    if (nextY < 0 || nextY >= (int)lines.size())
        return;

    model.SetCursor(nextY, 0);
    model.Backspace();
}

// Delete character under cursor.
DeleteTextCommand::DeleteTextCommand(Controller *Controller) : _Controller(Controller)
{
    current_x = _Controller->GetCursorDocX();
    current_y = _Controller->GetCursorDocY();
}

void DeleteTextCommand::execute()
{
    auto &model = _Controller->GetModel();
    const auto &lines = model.GetLines();
    if (current_y < 0 || current_y >= (int)lines.size())
        return;

    const auto &row = lines[current_y];
    const int pos = std::clamp(current_x, 0, (int)row.size());
    if (pos >= (int)row.size())
        return;

    _pos = pos;
    _removedChar = row.at(_pos);

    model.SetCursor(current_y, _pos);
    model.Delete();
}

void DeleteTextCommand::unexecute()
{
    auto &model = _Controller->GetModel();
    model.SetCursor(current_y, _pos);
    model.InsertChar(_removedChar);
    model.SetCursor(current_y, _pos);
}
