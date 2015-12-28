#ifndef _UTILS_H_
#define _UTILS_H_

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#ifndef WIN32
#include <sys/time.h>           // for gettimeofday()
#else
#include <winsock2.h>


#define bzero(x, y) memset((x), 0, (y))
#define snprintf _snprintf_s

#endif

#define MAX_DEBUG_LV 2

#ifdef __cplusplus 
extern "C"
{
#endif

    namespace FACE{
        enum{
            MSG,
            ERR,
            INF,
            RPC
        };


        typedef int (*config_cb)(int lineno, char *key, char *value);


        void p(int debug_lv, char *str, ...);
        int get_pos(char *str, char delim);
        int count_line(char *buf);
        char* fmttime(char* timestr, int time);

    }
#ifdef __cplusplus    
}
#endif

#endif
