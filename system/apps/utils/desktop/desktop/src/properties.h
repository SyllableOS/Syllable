#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "include.h"
#include "crect.h"
#include "properties_messages.h"
#include "loadbitmap.h"
#include "bitmapscale.h"
#include "coloredit.h"
#include "settings.h"
using namespace os;

typedef vector <ListViewStringRow*> t_ListRows;
typedef vector <std::string> t_List;


class MiscView : public View
{

    public:
        MiscView(const Rect & cFrame);
        CheckBox* pcLoginCheck;
        CheckBox* pcShowVerCheck;
};


class BackView : public View
{
    public:
        BackView(const Rect & cFrame);
        ~BackView();
        ListView* pcList;
        DropdownMenu* pcSizeDrop;
        t_List ImageList();
        Bitmap* pcScreenBmp;
        virtual void Paint(const Rect& cUpdate);
        void ListFiles();



    private:

        t_ListRows vFolderRow;
        StringView* pcString;
        void Defaults();

};



class ColorView : public View
{
    public:
        ColorView(const Rect& cFrame);
        ~ColorView();
        DropdownMenu* pcColorDrop;
        ListView* pcListTheme;
        ColorEdit* pcColorEdit;
        virtual void Paint(const Rect& cUpdate);
        Bitmap* pcScreenBmp;
		t_List PopulateFolderList();
		t_ListRows vFolderRow;
		void ListFiles();
		void SwitchList();
};


class PropTab : public TabView
{
    public:
        PropTab(const Rect & cFrame);
        ColorView* pcColor;
        MiscView* pcMisc;
        BackView* pcBack;
    private:
        void Defaults();

};


class PropWin : public Window
{
    public:
        PropWin();
        PropTab* pcPropTab;

    private:
        Button* pcCancel;
        Button* pcSave;
        Button* pcClose;
        void HandleMessage(Message* pcMessage);
        void Defaults();
        void ReadLoginConfig();
        char zLoginName[1024];
        void Save();
        void SaveLoginConfig(bool b_Login, const char* zName);
        bool OkToQuit();
        void LoadPrefs();
        void SavePrefs(bool bShow, bool bLogin, string zPic, int32 nNewImageSize);
		DeskSettings* pcSettings;
        bool bShwVr;
        const char* dImage;
        string zImage;
};

#endif

























































