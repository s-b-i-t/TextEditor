#ifndef ECCONTROLLER_H
#define ECCONTROLLER_H
#include "ECTextViewImp.h"
#include <string>
#include <vector>
#include <iostream>
#include <stack>
#include <deque>
#include <unordered_set>
using namespace std;
// forward declaration in order to operate stacks of ECCommand objects for undo/redo
class ECCommand;
class ECController
{
public:
    ECController(ECTextViewImp *TextViewImp, const std::string &filename);

    // Handle Misc keypresses
    void HandleKey(int key);

    // Handle event by creating ECommand object
    void AddText(char ch);
    void RemoveText();
    void HandleEnter();
    void Redo();
    void Undo();

    void OpenFile();
    void SaveFile();

    // Current Status Mode (insert/command)
    std::string &getCurStatus()
    {
        return curStatus;
    }

    // text stored in Rows vector up to number of rows in view
    std::vector<string> &GetRows() { return Rows; }
    // Get rows out of view
    std::stack<string> &Get_Top_Rows() { return Top_Rows; }
    std::stack<string> &Get_Bottom_Rows() { return Bottom_Rows; }

    int Get_Row_Num(int index) { return Top_Rows.size() + index + 1; }

    bool OutofView(int lineNumber)
    {
        return LinesInView.find(lineNumber) == LinesInView.end();
    }

    // TextViewImp refreshed after any modification
    void UpdateTextViewImpRows();

    int Get_Cur_Y()
    {
        return _TextViewImp->GetCursorY();
    }

    int Get_Cur_X()
    {
        return _TextViewImp->GetCursorX();
    }

    int Get_Max_Y()
    {
        if (_TextViewImp->GetRowNumInView() < Rows.size())
        {
            return _TextViewImp->GetRowNumInView();
        }
        return Rows.size() - 1;
    }

    int Get_Max_X()
    {
        int max_x;
        if (Rows[Get_Cur_Y()].size() < _TextViewImp->GetColNumInView())
        {
            max_x = Rows[Get_Cur_Y()].size() + GetRowStart();
        }
        else
        {
            max_x = _TextViewImp->GetColNumInView();
        }
        return max_x;
    }

    int GetRowStart()
    {
        if (ShowLines)
            return 5;
        return 0;
    }

private:
    // Lines modified using GetRowStart() & UpdateTextViewImpRows()
    bool ShowLines = true;
    // debugging
    void UpdateStatusRow(int cx, int cy, int nx, int ny, int mx, int my, std::string key);

    ECTextViewImp *_TextViewImp;

    // Stores all rows in deisplay
    std::vector<string> Rows;

    // Row not currently in display
    std::stack<string> Top_Rows;
    std::stack<string> Bottom_Rows;

    std::vector<string> Left_Chars;
    std::vector<string> Right_Chars;

    //Handle user arrow movement
    void HandleUp();
    void HandleDown();
    void HandleRight();
    void HandleLeft();

    // Text wrapping to shift view
    // Does not modify cursor
    void HandleWrapDown();
    void HandleWrapUp();
    void HandleWrapRight();
    void HandleWrapLeft();

    std::string _filename;

    std::string curStatus = "command";

    std::vector<char> brackets = {'(', ')', '{', '}', '[', ']', '<', '>'};

    std::unordered_set<string> keywords;
    void LoadKeywords();
    void HighlightKeywords();

    std::stack<ECCommand *> CommandStack;
    std::stack<ECCommand *> RedoStack;

    std::unordered_set<int> LinesInView;
    std::string lastkey = "";
};

#endif
