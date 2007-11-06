#include "msgtotext.h"
#include <util/message.h>
#include <gui/rect.h>
#include <gui/gfxtypes.h>

using namespace os;

String DataTypeToText( int nType )
{
	String s;

	switch(nType) {
		case T_ANY_TYPE:	return s = "ANY_TYPE";
		case T_POINTER:		return s = "POINTER";
		case T_INT8:		return s = "INT8";
		case T_INT16:		return s = "INT16";
		case T_INT32:		return s = "INT32";
		case T_INT64:		return s = "INT64";
		case T_BOOL:		return s = "BOOL";
		case T_FLOAT:		return s = "FLOAT";
		case T_DOUBLE:		return s = "DOUBLE";
		case T_STRING:		return s = "STRING";
		case T_IRECT:		return s = "IRECT";
		case T_IPOINT:		return s = "IPOINT";
		case T_MESSAGE:		return s = "MESSAGE";
		case T_COLOR32:		return s = "COLOR32";
		case T_RECT:		return s = "RECT";
		case T_POINT:		return s = "POINT";
		case T_VARIANT:		return s = "VARIANT";
		case T_RAW:		return s = "RAW";
		default:		s.Format("Type:%d", nType);
	}

	return s;
}

String MsgTypeToText(const String &cName, Message *pcMsg)
{
	int nType, nCount;
	String s;

	pcMsg->GetNameInfo(cName.c_str(), &nType, &nCount);

	s = DataTypeToText( nType );
	
	if(nCount > 1) {
		String idx;
		idx.Format("[%d]", nCount);
		s+=idx;
	}
	
	return s;
}

String MsgDataToText(const String &cName, int nIndex, Message *pcMsg)
{
	String str = "(?)";
	int nType, nCount;

	pcMsg->GetNameInfo(cName.c_str(), &nType, &nCount);

	switch(nType) {
		case T_POINTER:
			{
				void *ptr;
				pcMsg->FindPointer(cName.c_str(), &ptr, nIndex);
				str.Format("0x%08lx", ptr);
			}
			break;
		case T_INT8:
			{
				int8 i;
				pcMsg->FindInt8(cName.c_str(), &i, nIndex);
				str.Format("%d", i);
			}
			break;
		case T_INT16:
			{
				int16 i;
				pcMsg->FindInt16(cName.c_str(), &i, nIndex);
				str.Format("%d", i);
			}
			break;
		case T_INT32:
			{
				int32 i;
				pcMsg->FindInt32(cName.c_str(), &i, nIndex);
				str.Format("%ld", i);
			}
			break;
		case T_INT64:
			{
				int64 i;
				pcMsg->FindInt64(cName.c_str(), &i, nIndex);
				str.Format("0x%016llx", i);
			}
			break;
		case T_BOOL:
			{
				bool b;
				pcMsg->FindBool(cName.c_str(), &b, nIndex);
				str = b?"true":"false";
			}
			break;
		case T_FLOAT:
			{
				float f;
				pcMsg->FindFloat(cName.c_str(), &f, nIndex);
				str.Format("%f", f);
			}
			break;
		case T_DOUBLE:
			{
				double d;
				pcMsg->FindDouble(cName.c_str(), &d, nIndex);
				str.Format("%lf", d);
			}
			break;
		case T_STRING:
			{
				const char *ptr;
				if(!pcMsg->FindString(cName.c_str(), &ptr, nIndex)) {
					//str = "\"" + (string)ptr + "\"";
					str = (String)ptr;
				}
			}
			break;
		case T_IRECT:
			{
				IRect r;
				pcMsg->FindIRect(cName.c_str(), &r, nIndex);
				str.Format("%d, %d, %d, %d", r.left, r.top, r.right, r.bottom);
			}
			break;
		case T_IPOINT:
			{
				IPoint p;
				pcMsg->FindIPoint(cName.c_str(), &p, nIndex);
				str.Format("%d, %d", p.x, p.y);
			}
			break;
		case T_MESSAGE:
			str = "(...)";
			break;
		case T_COLOR32:
			{
				Color32_s c;
				pcMsg->FindColor32(cName.c_str(), &c, nIndex);
				str.Format("%d, %d, %d", c.red, c.green, c.blue);
			}
			break;
		case T_RECT:
			{
				Rect r;
				pcMsg->FindRect(cName.c_str(), &r, nIndex);
				str.Format("%f, %f, %f, %f", r.left, r.top, r.right, r.bottom);
			}
			break;
		case T_POINT:
			{
				Point p;
				pcMsg->FindPoint(cName.c_str(), &p, nIndex);
				str.Format("%f, %f", p.x, p.y);
			}
			break;
		case T_VARIANT:
			break;
		case T_RAW:
			str = "(...)";
			break;
	}

	return str;
}


