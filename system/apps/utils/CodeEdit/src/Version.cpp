#include "Version.h"

const char* Version(void)
{
    char zReturn[1024];
    if (Alpha)
        sprintf(zReturn, "%.1f Alpha",VERSION_NUM);
    if (Beta)
        sprintf(zReturn, "%.1f Beta",VERSION_NUM);
    if (Final)
        sprintf(zReturn, "%.1f",VERSION_NUM);

    return (zReturn);
}




