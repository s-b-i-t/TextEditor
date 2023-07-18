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

// forward declaration in order to operate stacks of ECCommand objects for undo/redo
class ECCommand;

class ECController
{
public:
    ECController(ECTextViewImp *TextViewImp, const std::string &filename);
    
    //Handle Misc keypresses
    void HandleKey(int key);

    //Handle event by creating ECommand object then add to command stack for undo/redo
    void AddText(char ch);
    void RemoveText();
    void HandleEnter();
    void Redo();
    void Undo();


    void OpenFile();
    void SaveFile();
    
    // Current Status Mode (insert/command)
    string& getCurStatus()
    {
        return curStatus;
    }

    //text stored in Rows vector up to number of rows in view
    vector<string>& GetRows() {return Rows;}

    //Get rows out of view
    std::stack<string>& Get_Top_Rows(){return Top_Rows;}
    std::stack<string>& Get_Bottom_Rows(){return Bottom_Rows;}

    //TextViewImp refreshed after any modification
    void UpdateTextViewImpRows();

    

private:
    //debugging
    void UpdateStatusRow(std::string text);

    ECTextViewImp *_TextViewImp;
    
    //Stores all rows in deisplay
    vector<string> Rows;

    //Row not currently in display
    std::stack<string> Top_Rows;
    std::stack<string> Bottom_Rows;

    //Text wrapping to shift view
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
