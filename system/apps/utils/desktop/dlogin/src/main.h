#ifndef MAIN_H
#define MAIN_H

#include <util/application.h>
#include "mainwindow.h"
using namespace os;

class LoginApp : public Application
{
public:
    LoginApp();
private:
    LoginWindow* pcWindow;
};
#endif
