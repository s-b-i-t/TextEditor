// Test code for editor
#include "ECTextViewImp.h"
#include "ECModel.h"
#include "ECController.h"
#include <iostream>

int main(int argc, char *argv[])
{
    ECTextViewImp wndTest;

    if (argc < 2) {
        cerr << "Please provide a filename" << endl;
        return 1;
    }

    ECController controller(&wndTest, argv[1]);
    ECModel model(&wndTest , &controller);
    
    wndTest.Attach(&model);
    wndTest.AddStatusRow("ctrl-h for help", "mode: " + controller.getCurStatus(), true);
    wndTest.Refresh();
    wndTest.Show();
    
    return 0;
}