#ifndef __kdatastream__h
#define __kdatastream__h

#include <qdatastream.h>

inline QDataStream & operator << (QDataStream & str, bool b)
{
  str << Q_INT8(b);
  return str;
}

inline QDataStream & operator >> (QDataStream & str, bool & b)
{
  Q_INT8 l;
  str >> l;
  b = bool(l);
  return str;
}

#endif
