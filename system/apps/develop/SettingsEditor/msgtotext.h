
#ifndef MSGTOTEXT_H
#define MSGTOTEXT_H

#include <util/string.h>
#include <util/message.h>

using namespace os;

String MsgTypeToText(const String &cName, Message *pcMsg);
String MsgDataToText(const String &cName, int nIndex, Message *pcMsg);
String DataTypeToText( int nType );

#endif /* MSGTOTEXT_H */
