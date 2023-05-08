#ifndef ECMODEL_h
#define ECMODEL_h

#include "ECTextViewImp.h"
#include "ECController.h"
#include <string>
class ECModel : public ECObserver {

public:
    ECModel(ECTextViewImp *TextViewImp, ECController *Controller);

    void Update();



private:
    ECTextViewImp * _TextViewImp;
    ECController * _Controller;

};

#endif