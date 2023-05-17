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

class ECCommand;

class ECController
{
public:
    ECController(ECTextViewImp *TextViewImp, const std::string &filename);
    
    //Handles Misc keypresses
    void HandleKey(int key);

    //Add text using command class
    void AddText(char ch);
    
    //Remove text using command class
    void RemoveText();

    //Handle enter using command class
    void HandleEnter();


    void OpenFile();

    void SaveFile();

    void Redo();

    
    void Undo();

    string getCurStatus()
    {
        return curStatus;
    }

    vector<string>& GetRows() {return Rows;}

    //TextViewImp refreshed after any modification
    void UpdateTextViewImpRows();

    //Get rows out of view
    std::stack<string>& Get_Top_Rows(){return Top_Rows;}
    std::stack<string>& Get_Bottom_Rows(){return Bottom_Rows;}

private:
    ECTextViewImp *_TextViewImp;
    vector<string> Rows;

    std::stack<string> Top_Rows;
    std::stack<string> Bottom_Rows;

    void HandleWrapDown();
    void HandleWrapUp();
    void HandleWrapRight();
    void HandleWrapLeft();    


    string _filename;
    std::string curStatus = "command";
    
    std::vector<char> brackets = {'(', ')', '{', '}', '[', ']', '<', '>'};

    set<string> keywords;
    void LoadKeywords();
    void HighlightKeywords();
    
    std::stack<ECCommand*> CommandStack;
    std::stack<ECCommand*> RedoStack;

};

#endif
//first teasdkadsk
