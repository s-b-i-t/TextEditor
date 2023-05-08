//
//  ECTextViewImp.h
//  
//
//  Created by Yufeng Wu on 2/27/20.
//  Last modified: 4/9/2021
//

#ifndef ECTextViewImp_h
#define ECTextViewImp_h

#include <string>
#include <vector>
#include <map>
#include "ECObserver.h"

//***********************************************************
//          TextView version 1.0.0 beta
//                  April 9, 2021
//                  By Yufeng Wu (all rights reserved)
//    Disclaimer: the following code is largely a porting of
//              the Kilo editor from C to C++
//***********************************************************


//***********************************************************
// Key code. Use this to match the keycode you get from ECTextViewImp

enum KEY_ACTION{
    KEY_NULL = 0,       /* NULL */
    CTRL_A = 1,         /* Ctrl-a */
    CTRL_B = 2,         /* Ctrl-b */
    CTRL_C = 3,         /* Ctrl-c */
    CTRL_D = 4,         /* Ctrl-d */
    CTRL_E = 5,         /* Ctrl-e */
    CTRL_F = 6,         /* Ctrl-f */
    CTRL_G = 7,         /* Ctrl-g */
    CTRL_H = 8,         /* Ctrl-h */
    TAB = 9,            /* Tab */
    CTRL_L = 12,        /* Ctrl+l */
    ENTER = 13,         /* Enter */
    CTRL_Q = 17,        /* Ctrl-q */
    CTRL_R = 18,        /* Ctrl-r */
    CTRL_S = 19,        /* Ctrl-s */
    CTRL_U = 21,        /* Ctrl-u */
    CTRL_V = 22,        /* Ctrl-u */
    CTRL_Y = 25,        /* Ctrl-y */
    CTRL_Z = 26,        /* Ctrl-z */
    ESC = 27,           /* Escape */
    SLASH = 47,         /* Forward slash */
    BACKSPACE =  127,   /* Backspace */
    /* The following are just soft codes, not really reported by the
     * terminal directly. */
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

//***********************************************************
// Text color.

enum TEXT_COLOR{
    TEXT_COLOR_BLACK = 30,     /* black */
    TEXT_COLOR_WHITE = 37,     /* white */
    TEXT_COLOR_CYAN = 36,      /* cyan */
    TEXT_COLOR_YELLOW = 33,    /* yellow */
    TEXT_COLOR_GREEN = 32,     /* green */
    TEXT_COLOR_MAGENTA = 35,   /* magenta */
    TEXT_COLOR_RED = 31,       /* red */
    TEXT_COLOR_BLUE = 34,      /* blue */
    TEXT_COLOR_DEF = -1        /* no-color */
};

//***********************************************************
// Text view configuration
// This is the internal status of ECTextViewImp. You don't need to use this in your code.

class ECTextViewConfig {
public:
    ECTextViewConfig();
    int GetWndRowNum() const { return screenrows-(int)listStatusMsgsLeft.size(); }
    int GetWndColNum() const { return screencols; }
    int GetCursorX() const { return cx; }
    int GetCursorY() const { return cy; }
    void SetCursorX(int pos) { cx = pos; }
    void SetCursorY(int pos) { cy = pos; }
    int GetNumStatusRows() const { return listStatusMsgsLeft.size(); }
    std::string GetStatusRowLeft(int r) const { return listStatusMsgsLeft[r]; }
    std::string GetStatusRowRight(int r) const { return listStatusMsgsRight[r]; }
    bool IsStatusRowBlkBackground(int r) const { return listStatusBlackBackground[r]; }
    void ClearStatusRows() { listStatusMsgsLeft.clear(); listStatusMsgsRight.clear(); listStatusBlackBackground.clear(); }
    void AddStatusRow( const std::string &statusMsgLeft, const std::string &statusMsgRight, bool fBlackBackground );
    void Dump() const;
    
private:
    void InitParams();
    void InitWndSize();
    int GetCursorPosition(int ifd, int ofd, int *rows, int *cols);
    
    int cx;
    int cy;  /* Cursor x and y position in characters */
    int screenrows; /* Number of rows that we can show */
    int screencols; /* Number of cols that we can show */
    std::vector<std::string> listStatusMsgsLeft;    // status msgs (left aligned)
    std::vector<std::string> listStatusMsgsRight;   // right aligned
    std::vector<bool> listStatusBlackBackground;    // set to true if use black background
};

//***********************************************************
// A textview implementation
// This is based on the simple Kilo code from GitHub
//
// ECTextViewImp provides basci functionalities for a text view
// (1) Display a number of rows onto scrreen (and refresh the current view after updates)
// (2) Cursor management: display cursor, and allow cursor movement
// (3) Key stroke event handling: notify clients (observers) that a key has been pressed (see below)
//
//  Please note: ECTextViewImp only provides very basic error handling
//  Also note: you are responsible for what text to show on the screen and where to place cursor
//  ECTextViewImp has no concerns about formatting at all. You need to figure out how many
//  rows you are going to show on the screen
//
// Note: ECTextViewImp implements *** Observer *** pattern
// It is the subject that accepts observers.
// Whenver something happens (i.e., a key is pressed), all observers
// are notified through Observer's Notify function
// then an observer would check for update (in this case, what key is pressed)

class ECTextViewImp : public ECObserverSubject
{
public:
    ECTextViewImp();
    virtual ~ECTextViewImp();
    
    // Show the view. This would enter a forever loop, until quit is set
    void Show();
    
    // Write the text for the entire wiindow including cursor onto console
    // Invoke this function after you make changes to your document.
    // Refreshing entire window for any document update seems excessive; but this is how the code works
    void Refresh();
    
    // Get the key last pressed. Note: call this after getting a notification from observer
    int GetPressedKey();
    
    // Call when you want to quit from the program
    void Quit();
    
    // Clear out the content of the screen
    // A view consists multiple number of rows, where the maximum number of rows is equal
    // to the number of rows in the window of the view, minus the number of status rows (by default, none)
    // You don't have to specify all rows; ECTextViewImp aautomatically fills the remaining rows
    // with special characters
    void InitRows() { listRows.clear(); }
    
    // Add an entire row to the view
    void AddRow(const std::string &strRow);

    
    // Get/set
    // Get number of rows in view
    int GetRowNumInView() const { return status.GetWndRowNum(); }
    
    // Get number of columns in view
    int GetColNumInView() const { return status.GetWndColNum(); }
    
    // Get position of the cursor: x coordinate (horonzontal); position starts from 0
    int GetCursorX() const { return status.GetCursorX(); }
    
    // Get position of the cursor: y coordinate (vertical)
    int GetCursorY() const { return status.GetCursorY(); }
    
    // Set the cursor position x and y coordinate
    // Be careful: ECTextViewImp doesn't check whether the coordinates you specified
    // is valid or not; the code may CRASH if you provide an invalid cursor position
    // A cursor position is valid if the position is within the text range
    // Suppose the current line has 3 characters. Then the cursor can be at positions 0, 1, 2 and 3
    // where 3 is to the right of the last character.
    // You are responsible to ensure the cursor position is properly set
    void SetCursorX(int pos) { status.SetCursorX(pos); }
    void SetCursorY(int pos) { status.SetCursorY(pos); }
    
    // status rows
    // call this function to disable status rows
    void ClearStatusRows() { status.ClearStatusRows(); }
    
    // call to show a status row. A status row consists of two parts: the left message and the right message
    // set fBlackBackground to be true if you want to show a darkened background (for highlight)
    void AddStatusRow( const std::string &statusMsgLeft, const std::string &statusMsgRight, bool fBlackBackground ) { status.AddStatusRow(statusMsgLeft, statusMsgRight, fBlackBackground); }
    
    // call to change the text color for a segment of row (from colBegin to colEnd)
    // Note: make sure the range of row and columns are valid; this code wont' check it
    // Note: also avoid intersection of segments; don't set two different colors starting from the same column at the same row
    // Note: if clr == TEXT_COLOR_DEF, then this function removes any coloring starting at colBegin for row
    void SetColor(int row, int colBegin, int colEnd, TEXT_COLOR clr);

    // call to clear all color settings
    void ClearColor() { clrTextInfo.clear(); } 
   
private:
    // These are implementation specific functions; you don't need to call these functions
    // You can read the code if you want to know what they do
    
    // Must call Init before using ECTextViewImp
    void Init();
    void ClearBuffer();
    void AppendRowBuffer(int row, const std::string &strRow);
    void FinishRowsBuffer();
    int GetNumRows() const { return listRows.size(); }
    const char *GetRow(int r) const { return listRows[r].c_str();  }
    void AppendStatusMsg(int rs);
    bool ShouldQuit() const { return fQuit; }
    int ReadKey(int fd);
    
    // This is the buffer for the window that is used to refresh the view
    std::string bufferWnd;
    
    // List of rows being requested
    std::vector<std::string> listRows;
    
    // Status of the view
    ECTextViewConfig status;
    
    // save text color info <row, <col-start, <col-end, color> > >
    std::map<int, std::map<int, std::pair<int, TEXT_COLOR> > > clrTextInfo;
    
    // if set to true, would quit from loop of Show()
    bool fQuit;
    
    // what key is pressed? The value must be one of KEY_ACTION entries
    int keyLastPressed;
};




#endif /* ECTextViewImp_h */
