#ifndef MESSAGES_H
#define MESSAGES_H


enum
{
    /* for SettingsWindow */
    M_APPLY,
    M_SAVE,
    M_CANCEL,
    M_REVERT,

    /* for TermView */
    M_MENU_ABOUT,
    M_MENU_SETTINGS,
    M_MENU_COPY,
    M_MENU_PASTE,

    /* for MyApp */
    M_SCROLL,

    /* from SettingsWindow to TermView (through MyApp) */
    M_REFRESH
};


#endif /* MESSAGES_H */
