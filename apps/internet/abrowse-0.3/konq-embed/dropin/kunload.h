#include <qglobal.h>
#include <qobjectdict.h>

#define _UNLOAD(p) \
static void unload_##p (const char **list) \
{ \
    if (!objectDict) return; \
    /*qDebug("removing meta classes for %s", #p);*/ \
    const char **n = list; \
    for (; *n; n++) { \
        /*qDebug("  removing metaclass %s", *n);*/ \
	/* Because objectDict was set to AutoDelete the following does the \
           right thing. */ \
        while (objectDict->remove(*n)) ; \
    } \
    /*qDebug("removing done");*/ \
} \
 \
class _KUnloadMetaClass_Helper##p { \
public: \
    _KUnloadMetaClass_Helper##p () {} \
    ~_KUnloadMetaClass_Helper##p () { \
        unload_##p (_metalist_##p); \
    } \
}; \
 \
static _KUnloadMetaClass_Helper##p _helper_object_##p;
