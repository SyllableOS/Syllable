#ifndef ICONMENU_PROP_H
#define ICONMENU_PROP_H

#include "include.h"
#include "crect.h"
using namespace os;

class IconProp : public Window
{
    public:
        IconProp(string cIconName, string cExec);

    private:
        TextView* pcIconNameText;
        
        StringView* pcIconNameString;
        Button*    pcIconOkBut;
        Button*    pcIconRenameBut;
        
        Button*    pcIconExecBut;
        TextView*  pcIconExecTxt;
        StringView* pcIconExecStr;
        
        
        virtual void HandleMessage(Message* pcMessage);
};

#endif





