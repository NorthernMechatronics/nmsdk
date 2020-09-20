#include "build_timestamp.h"

char *g_build_timestamp(void)
{
    static char build_timestamp[32] = __DATE__ " " __TIME__;

    return build_timestamp;
}
