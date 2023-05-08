#ifndef ECCONTROLLER_H
#define ECCONTROLLER_H

#include "ECTextViewImp.h"
#include <string>
#include <vector>
#include <iostream>
#include <stack>
#include <set>
using namespace std;

struct Operation {
    enum Type { ADD, REMOVE } type;
    char ch;
    int x, y;
};


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

    void Undo();

    string getCurStatus()
    {
        return curStatus;
    }

private:
    ECTextViewImp *_TextViewImp;
    vector<string> Rows;
    vector<string> tmpVector;
    string _filename;
    std::string curStatus = "command";
    stack<Operation> undoStack;
    stack<Operation> redoStack;

    void UpdateTextViewImpRows();
    set<string> keywords;
    void LoadKeywords();
    void HighlightKeywords();
};

#endif
