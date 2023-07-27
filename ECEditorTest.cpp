// Test code for editor
#include "ECTextViewImp.h"
#include "ECModel.h"
#include "ECController.h"
#include <iostream>
using namespace std;
int main(int argc, char *argv[])
{

    if (argc < 2) {
        cerr << "Please provide a filename" << endl;
        return 1;
    }

    ECTextViewImp View;
    ECController Controller(&View, argv[1]);
    ECModel Model(&View, &Controller);
    
    View.Attach(&Model);
    // View.AddStatusRow("ctrl-h for help", "mode: " + controller.getCurStatus(), true);
    View.Refresh();
    View.Show();
    
    return 0;
}