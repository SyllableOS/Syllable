#ifndef __kxmlgui_h__
#define __kxmlgui_h__

class KInstance;

class KXMLGUIFactory
{
public:

    static QString readConfigFile( const QString &, KInstance * = 0 ) { return QString::null; }

};

#endif
