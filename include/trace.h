#ifndef _TRACE_H_
#define _TRACE_H_

#include <stdio.h>
#include <time.h>

static const char *trace_name = "";

#define TRACE_INIT(_name) trace_name = #_name

#define TRACE_SIMPLE(...) printf(__VA_ARGS__)
#define TRACE_NL TRACE_SIMPLE("\n")
#define TRACE_PROMPT TRACE_TIME; TRACE_SIMPLE("%s > ", trace_name);
#define TRACE_TIME do {                            \
        time_t t = time(NULL);                     \
        char buf[32];                              \
        strftime(buf, 32, "%T", localtime(&t));    \
        TRACE_SIMPLE("%s - ", buf);                \
    } while(0)
      
#define TRACE(...)                              \
    TRACE_PROMPT;                               \
    TRACE_SIMPLE(__VA_ARGS__);                  \
    TRACE_NL

#define ERROR(...)                                              \
    TRACE_PROMPT;                                               \
    TRACE_SIMPLE("%s:%s:%d - "                                  \
                 , __FILE__, __FUNCTION__, __LINE__);           \
    TRACE_SIMPLE(__VA_ARGS__);                                  \
    TRACE_NL

#endif /* _TRACE_H_ */
