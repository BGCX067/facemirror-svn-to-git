#include "utils.h"

using namespace FACE;

char p_names[4][5] = {"MSG\0", "ERR\0", "INF\0", "RPC\0"};

#ifdef WIN32
int gettimeofday (struct timeval *tv, void* tz)
{
    union {
        long long ns100;        /*time since 1 Jan 1601 in 100ns units */
        FILETIME  ft;
    } now;

    GetSystemTimeAsFileTime (&now.ft);
    tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
    tv->tv_sec  = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
    return (0);
}
#endif

namespace FACE{
char* fmttime(char* timestr, int time){
    struct timeval tv;
    //struct timezone tz;
    gettimeofday(&tv, NULL);
    if (time){
        tv.tv_sec = time;
        tv.tv_usec = 0;
    }
    struct tm lt;
#ifdef WIN32

    struct tm *suck;
    time_t     long_time;

    ::time( &long_time );       /* Get time as long integer. */
    suck = localtime( &long_time ); /* Convert to local time. */
    lt   = *suck;

#else
    localtime_r(&(tv.tv_sec), &lt);
#endif
    sprintf( timestr, "%d-%02d-%02d %02d:%02d:%02d.%ld", 1900+lt.tm_year,
            1+lt.tm_mon, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, tv.tv_usec);
    return timestr;
}

void _p(FILE* pstream, int debug_lv, char *str, va_list ap){
    if (debug_lv > MAX_DEBUG_LV )
        return;
    char timestr[27];
    bzero(timestr, sizeof(timestr));
    fmttime(timestr, 0);
    if (0 <= debug_lv)
        fprintf(pstream, "[%s %s] ", p_names[debug_lv], timestr);
/*    va_list ap; */
/*    va_start(ap, str); */
    vfprintf(pstream, str, ap);
/*    va_end(ap); */
}

void p(int debug_lv, char *str, ...){
    va_list ap;
    va_start(ap, str);
    _p(stdout, debug_lv, str, ap);
    va_end(ap);
    return;
}

int count_line(char *buf)
{
    int line = 0;
    unsigned int i;
    

    for(i = 0; i < strlen(buf); i++)
        if('\n' == buf[i])
            line++;

    return line;
}


int get_pos(char *str, char delim)
{
    int i = 0;
    
    while(*str){
        if(delim == *str++)
            return i;
        i++;
    }
    
    return -1;
}



}
