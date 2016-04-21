#include "trace.h"
#include "check.h"

int main() {
    TRACE_INIT(scheme);
    TRACE("Starting...");

    SHOULD_NEVER_BE_HERE;
    CHECK(1 == 0);
    return 0;
}
