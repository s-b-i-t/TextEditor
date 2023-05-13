// ECCommand.h

#ifndef ECCOMMAND_H
#define ECCOMMAND_H

#include "ECTextViewImp.h"

class ECCommand
{
public:
    virtual ~ECCommand() {}
    virtual void execute() = 0;
    virtual void unexecute() = 0;
};

class InsertTextCommand : public ECCommand
{
public:
    InsertTextCommand(ECTextViewImp* TextViewImp, char ch)
        : _controller(controller), _ch(ch) {}
    virtual void execute();
    virtual void unexecute();

private:
    ECTextViewImp* _TextViewImp;
    char _ch;
};

class RemoveTextCommand : public ECCommand
{
public:
    RemoveTextCommand(ECTextViewImp* TextViewImp)
        :  _TextViewImp(TextViewImp) {}
    virtual void execute();
    virtual void unexecute();

private:
    ECController* _controller;
};

class EnterCommand : public ECCommand
{
public:
    EnterCommand(ECController* controller)
        : _controller(controller) {}
    virtual void execute();
    virtual void unexecute();

private:
    ECController* _controller;
};

#endif /* ECCOMMAND_H */
