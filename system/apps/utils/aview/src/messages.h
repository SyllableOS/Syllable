#ifndef MESSAGES_H
#define MESSAGES_H

enum Menus{
    ID_FILE_LOAD,
    ID_VOID,
    ID_EXIT,
    ID_ABOUT,
    ID_HELP,
    ID_SCROLL_VERTICAL,
    ID_SCROLL_HORIZONTAL,
    ID_SETTINGS
};

enum App {
    M_MESSAGE_PASSED = 0x00345
};

enum SetWindowMessage{
    ID_APPLY = 0x00543,
    ID_CANCEL = 0x00545,
    ID_SEARCH = 0x00547
};


const unsigned int MENU_OFFSET = 20;

#endif





