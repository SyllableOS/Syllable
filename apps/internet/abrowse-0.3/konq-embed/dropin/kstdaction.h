#ifndef __kstdaction_h__
#define __kstdaction_h__

class QObject;

class KAction;

class KStdAction
{
public:

    static KAction *find( const QObject *receiver, const char *slot,
                          QObject *parent, const char *name = 0 );

    static KAction *selectAll( const QObject *receiver, const char *slot,
                               QObject *parent, const char *name = 0 );

};

#endif
