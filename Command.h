// Command.h

#ifndef COMMAND_H
#define COMMAND_H

class Controller;

class Command {
public:
    virtual ~Command() {}
    virtual void execute() = 0;
    virtual void unexecute() = 0;
protected:

    int current_x = 0; // doc column
    int current_y = 0; // doc row
    int current_line = 0; // helper storage (e.g., split position)
};



class InsertTextCommand : public Command {
public:
    InsertTextCommand(Controller* Controller, char ch);
    virtual void execute();
    virtual void unexecute();

private:
    Controller* _Controller;
    char _ch;
};



class RemoveTextCommand : public Command {
public:
    RemoveTextCommand(Controller* Controller);

    virtual void execute();
    virtual void unexecute();

private:
    Controller* _Controller;
    char _removedChar;
    int _removedPos = 0; // document column removed in execute()
};



class EnterCommand : public Command {
public:
    EnterCommand(Controller* Controller);
    virtual void execute();
    virtual void unexecute();

private:
    Controller* _Controller;
    int _split_pos = 0;
};




class MergeLinesCommand : public Command {
public:
    MergeLinesCommand(Controller* Controller);
    virtual void execute();
    virtual void unexecute();

private:
    Controller* _Controller;
};

class DeleteTextCommand : public Command {
public:
    DeleteTextCommand(Controller* Controller);
    virtual void execute();
    virtual void unexecute();

private:
    Controller* _Controller;
    char _removedChar;
    int _pos = 0; // document column removed in execute()
};

#endif /* COMMAND_H */
