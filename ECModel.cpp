// ECMODEL.cpp
#include "ECModel.h"
#include "ECTextViewImp.h"
#include "ECController.h"

ECModel::ECModel(ECTextViewImp *TextViewImp, ECController *Controller) : _TextViewImp(TextViewImp), _Controller(Controller) {}

void ECModel::Update(){
    int key = _TextViewImp->GetPressedKey();
    

    if (key == ENTER){
        _TextViewImp->AddRow("");
        _TextViewImp->SetCursorY(_TextViewImp->GetCursorY() + 1);
        _TextViewImp->SetCursorX(0);
        _TextViewImp->Refresh();
    } else if (key == BACKSPACE){
        _Controller->RemoveText();
    } else if (key >= 32 && key <= 126 && _Controller->getCurStatus() == "insert"){
        char ch = static_cast<char>(key);
        _Controller->AddText(ch);
    } else {
        _Controller->HandleKey(key);
    }
}
