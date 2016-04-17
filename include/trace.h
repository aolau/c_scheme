#ifndef _TRACE_H_
#define _TRACE_H_

#include <stdio.h>

static const char *trace_name = "";

#define TRACE_INIT(_name) trace_name = #_name

#define TRACE_SIMPLE(...) printf(__VA_ARGS__)
#define TRACE_NL TRACE_SIMPLE("\n")
#define TRACE_PROMPT TRACE_SIMPLE("%s > ", trace_name);
#define TRACE(...)                              \
    TRACE_PROMPT;                               \
    TRACE_SIMPLE(__VA_ARGS__);                  \
    TRACE_NL

#endif /* _TRACE_H_ */
