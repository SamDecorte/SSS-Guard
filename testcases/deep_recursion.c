//recurse until the shadow stack is full

#include "../libguard/guard.h"
#include <stdio.h>

static int count_down(int n) {
    GUARD_ENTER();

    if (n == 0) {
        GUARD_RETURN(0);
    }

    int result = count_down(n-1);
    GUARD_RETURN(result + 1);
}

int main(void) {
    GUARD_ENTER();

    int depth = count_down(100000); // More than 65536

    printf("returned normally from depth %d\n", depth);

    GUARD_RETURN(0);
}