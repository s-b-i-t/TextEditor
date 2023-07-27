// ECCommand.h

#ifndef ECCOMMAND_H
#define ECCOMMAND_H
#include <string>
#include "ECTextViewImp.h"

class ECController;

class ECCommand {
public:
    virtual ~ECCommand() {}
    virtual void execute() = 0;
    virtual void unexecute() = 0;
protected:

    int current_x;
    int current_y;
    int max_x;
    int max_y;
    int current_line;
};



class InsertTextCommand : public ECCommand {
public:
    InsertTextCommand(ECTextViewImp* TextViewImp, ECController* Controller, char ch);
    virtual void execute();
    virtual void unexecute();

private:
    ECTextViewImp* _TextViewImp;
    ECController* _Controller;
    char _ch;
};



class RemoveTextCommand : public ECCommand {
public:
    RemoveTextCommand(ECTextViewImp* TextViewImp, ECController* Controller);

    virtual void execute();
    virtual void unexecute();

private:
    ECTextViewImp* _TextViewImp;
    ECController* _Controller;
    char _removedChar;
};



class EnterCommand : public ECCommand {
public:
    EnterCommand(ECTextViewImp* TextViewImp, ECController* Controller);
    virtual void execute();
    virtual void unexecute();

private:
    ECTextViewImp* _TextViewImp;
    ECController* _Controller;
    std::string _remaining_text;
    int _split_pos; 
    bool _removedFromTop = false;
    bool _removedFromBottom = false;

    std::string _removedRow; 

};




class MergeLinesCommand : public ECCommand {
public:
    MergeLinesCommand(ECTextViewImp* TextViewImp, ECController* Controller);
    virtual void execute();
    virtual void unexecute();

private:
    ECTextViewImp* _TextViewImp;
    ECController* _Controller;
};

#endif /* ECCOMMAND_H */
