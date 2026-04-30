// Test code for editor
#include "TextView.h"
#include "Controller.h"
#include <iostream>
using std::cerr, std::endl;
int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        cerr << "Usage: text-editor <filename>" << endl;
        return 1;
    }

    TextView View;
    if (!View.IsOk())
    {
        std::cerr << View.GetInitError() << std::endl;
        return 1;
    }
    Controller Controller(&View, argv[1]);

    // Main event loop (no Observer): pull keys from the view and route to controller.
    while (true)
    {
        const int key = View.ReadKey();
        if (key == KEY_RESIZE)
        {
            View.SyncWindowSize();
            Controller.HandleResize();
            continue;
        }
        if (key == CTRL_Q)
        {
            View.Quit();
            break;
        }
        Controller.HandleInput(key);
    }

    return 0;
}
