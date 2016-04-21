#ifndef _CHECK_H_
#define _CHECK_H_

#include "trace.h"
#include <stdlib.h>

#define SHOULD_NEVER_BE_HERE                            \
    ERROR("Reached illegal state - aborting");          \
    abort();

#define CHECK(cond_)                                        \
    if (! (cond_)) {                                        \
        ERROR("condition: %s FAILED.", #cond_);             \
        abort();                                            \
    }

#endif /* _CHECK_H_ */
