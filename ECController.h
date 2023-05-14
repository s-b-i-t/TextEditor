#ifndef ECCONTROLLER_H
#define ECCONTROLLER_H

#include "ECTextViewImp.h"
#include <string>
#include <vector>
#include <iostream>
#include <stack>
#include <deque>
#include <set>
using namespace std;


struct Operation {
    enum Type { ADD, REMOVE, ENTER } type;
    char ch;
    int x, y;
};

class ECCommand;

class ECController
{
public:
    ECController(ECTextViewImp *TextViewImp, const std::string &filename);
    void HandleKey(int key);

    void AddText(char ch);

    void RemoveText();

    void OpenFile();

    void SaveFile();

    void Redo();

    void HandleEnter();
    
    void Undo();

    string getCurStatus()
    {
        return curStatus;
    }

    vector<string>& GetRows() {return Rows;}

    void UpdateTextViewImpRows();




private:
    ECTextViewImp *_TextViewImp;
    vector<string> Rows;
    
    
    string _filename;
    std::string curStatus = "command";
    
    
    stack<Operation> undoStack;
    stack<Operation> redoStack;
    
    std::vector<char> brackets = {'(', ')', '{', '}', '[', ']', '<', '>'};

    set<string> keywords;
    void LoadKeywords();
    void HighlightKeywords();
    
    std::deque<string> UpRowDeque;
    std::deque<string> DownRowDeque;

    
    std::stack<ECCommand*> CommandStack;

    std::stack<ECCommand*> RedoStack;

    std::vector<int> horizontalOffsets;



};

#endif
