// alert 2.0
// by jonas.sundstrom@kirilla.com 
// with enhancements by Rick Caudill(cau0730@cup.edu)

#include <util/application.h>
#include <gui/requesters.h>
#include <util/string.h>

int main(int argc, char* argv[]);

using namespace os;

class AlertApp : public Application
{
public:
    AlertApp(int argc, char* argv[]);
private:

};

int main(int argc, char* argv[])
{
    AlertApp * app = new AlertApp(argc, argv);
    app->Run();
    return(0);
}

AlertApp::AlertApp(int argc, char* argv[]) : Application( "application/x-vnd.rgc-alert")
{
    if(argc <= 1)  // no arguments
    {
        printf("Usage: alert title text alert_icon(Alert::ALERT_INFO is default) button1 [button2] [button3]\n");
        exit (EXIT_FAILURE);
    }

    String title    = "Untitled Alert";
    String text     = "Alert text goes here.";
    String bitmap  =  "Alert icon here(Alert::ALERT_INFO is default)";
    String button1  = "Ok";
    String button2  = "Ok";
    String button3  = "Ok";
    int    buttons  = 1;

    for(int32 i = 1; i < argc; i++)
    {

        switch (i)
        {
        case 1:
            title = argv[i];
            break;

        case 2:
            text = argv[i];
            break;

        case 3:
            bitmap = argv[i];
            break;

        case 4:
            button1 = argv[i];
            break;

        case 5:
            button2 = argv[i];
            buttons = 2;
            break;

        case 6:
            button3 = argv[i];
            buttons = 3;
            break;
        }
    }
    Alert * alert;

    switch (buttons)
    {
    case 1:
        {
            if (strcmp(bitmap.c_str(),"Alert::ALERT_INFO")==0)
            {
                alert = new Alert(title.c_str(), text.c_str(),Alert::ALERT_INFO, 0x00, button1.c_str(), NULL);
                break;
            }

            if (strcmp(bitmap.c_str(),"Alert::ALERT_WARNING")==0)
            {
                alert = new Alert(title.c_str(), text.c_str(),Alert::ALERT_WARNING, 0x00, button1.c_str(), NULL);
                break;
            }


            if (strcmp(bitmap.c_str(),"Alert::ALERT_QUESTION")==0)
            {
                alert = new Alert(title.c_str(), text.c_str(),Alert::ALERT_QUESTION, 0x00, button1.c_str(), NULL);
                break;
            }

            else
            {
                alert = new Alert(title.c_str(), text.c_str(),Alert::ALERT_INFO, 0x00, button1.c_str(), NULL);
                break;
            }
            break;
        }

    case 2:
        {
            if (strcmp(bitmap.c_str(),"Alert::ALERT_INFO")==0)
            {
                alert = new Alert(title.c_str(), text.c_str(),Alert::ALERT_INFO, 0x00, button1.c_str(),button2.c_str(), NULL);
                break;
            }

            if (strcmp(bitmap.c_str(),"Alert::ALERT_WARNING")==0)
            {
                alert = new Alert(title.c_str(), text.c_str(),Alert::ALERT_WARNING, 0x00, button1.c_str(),button2.c_str(), NULL);
                break;
            }

            if (strcmp(bitmap.c_str(),"Alert::ALERT_QUESTION")==0)
            {
                alert = new Alert(title.c_str(), text.c_str(),Alert::ALERT_QUESTION, 0x00, button1.c_str(),button2.c_str(), NULL);
                break;
            }

            else
            {
                alert = new Alert(title.c_str(), text.c_str(),Alert::ALERT_INFO, 0x00, button1.c_str(),button2.c_str(), NULL);
                break;
            }
            break;
        }
    case 3:
        {
            if (strcmp(bitmap.c_str(),"Alert::ALERT_INFO")==0)
            {
                alert = new Alert(title.c_str(), text.c_str(),os::Alert::ALERT_INFO, 0x00, button1.c_str(),button2.c_str(),button3.c_str(), NULL);
                break;
            }

            if (strcmp(bitmap.c_str(),"Alert::ALERT_WARNING")==0)
            {
                alert = new Alert(title.c_str(), text.c_str(),os::Alert::ALERT_WARNING, 0x00, button1.c_str(),button2.c_str(),button3.c_str(), NULL);
                break;
            }

            if (strcmp(bitmap.c_str(),"Alert::ALERT_QUESTION")==0)
            {
                alert = new Alert(title.c_str(), text.c_str(),Alert::ALERT_QUESTION, 0x00, button1.c_str(),button2.c_str(),button3.c_str(), NULL);
                break;
            }

            else
            {
                alert = new Alert(title.c_str(), text.c_str(),os::Alert::ALERT_INFO, 0x00, button1.c_str(),button2.c_str(),button3.c_str(), NULL);
                break;
            }
            break;
        }
    default:
        exit (EXIT_FAILURE);
        break;
    }

    int reply = -1;

    reply = alert->Go();

    switch (reply)
    {
    case 0:
        printf("%s\n", button1.c_str());
        break;

    case 1:
        printf("%s\n", button2.c_str());
        break;

    case 2:
        printf("%s\n", button3.c_str());
        break;

    default:
        exit (EXIT_FAILURE);
        break;
    }

    PostMessage(M_QUIT);
}


