#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "TextView.h"
#include "Model.h"
#include <string>
#include <vector>
#include <iostream>
#include <stack>
#include <unordered_set>
#include <algorithm>
// forward declaration in order to operate stacks of Command objects for undo/redo
class Command;
class Controller
{
public:
    Controller(TextView *TextViewImp, const std::string &filename);
    ~Controller();

    // Top-level key routing (insert/command).
    void HandleInput(int key);

    // Handle Misc keypresses
    void HandleKey(int key);

    // Handle event by creating ECommand object
    void AddText(char ch);
    void RemoveText();
    void HandleEnter();
    void Redo();
    void Undo();

    void OpenFile();
    bool SaveFile(std::string *errMsg = nullptr);

    Model &GetModel() { return model; }
    const Model &GetModel() const { return model; }

    // Current Status Mode (insert/command)
    std::string &getCurStatus()
    {
        return curStatus;
    }

    bool IsInsertMode() const { return curStatus == "insert"; }

    // Cursor position in document coordinates (0-based).
    int GetCursorDocX() const { return model.GetCursorX(); }
    int GetCursorDocY() const { return model.GetCursorY(); }

    // Set cursor in document coordinates.
    void SetDocCursor(int docY, int docX);

    int GetColOffset() const { return colOffset; }

    // TextViewImp refreshed after any modification
    void UpdateTextViewImpRows();

    // Reconcile internal view/window state after a terminal resize.
    void HandleResize();

    int GetRowStart() const;
    int GetTextWidth() const { return std::max(1, _TextViewImp->GetColNumInView() - GetRowStart()); }

private:
    // Lines modified using GetRowStart() & UpdateTextViewImpRows()
    bool ShowLines = true;
    // debugging
    void UpdateStatusRow(const std::string &key);
    void EnsureCursorVisible(int &docY, int &docX);
    void ClearRedoStack();

    TextView *_TextViewImp;

    // Full document lines.
    Model model;

    // Topmost visible line index (0-based).
    int rowOffset = 0;

    //Handle user arrow movement
    void HandleUp();
    void HandleDown();
    void HandleRight();
    void HandleLeft();

    std::string _filename;

    std::string curStatus = "command";

    std::vector<char> brackets = {'(', ')', '{', '}', '[', ']', '<', '>'};

    std::unordered_set<std::string> keywords;
    void LoadKeywords();
    void HighlightKeywords();

    std::stack<Command *> CommandStack;
    std::stack<Command *> RedoStack;

    std::string lastkey = "";

    // Horizontal scroll offset into the document (0-based column).
    int colOffset = 0;

    std::string isModified = "(not modified)";
};

#endif
