/*
 * api_demo.c - showcasing the implemented Guard APIs.
 */
#include "../libguard/guard.h"
#include <stdio.h>

static int add(int a, int b) {
    return a + b;
}
static int mul(int a, int b) {
    return a * b;
}
static int xor(int a, int b) {
    return a ^ b;
}


int main(void)
{
    // Registering a function
    guard_register(add);
    guard_register(xor);
    // Do not register mul; this will be our invalid function call

    // Sealing
    guard_seal();

    // // Should abort the program
    // guard_register(mul);

    // A valid call
    int is_valid_add = guard_is_valid((void *)add);
    if (is_valid_add == 0) {
        return 1;
    }

    // An invalid call; non-registered function
    int is_valid_mul = guard_is_valid((void *)mul);
    if (is_valid_mul != 0) {
        return 2;
    }

    // Invalid, because it's not in the .text region.
    int is_valid_outside = guard_is_valid((void *)0);
    if (is_valid_outside != 0) {
        return 3;
    }

    // Invalid, because it's in the middle of a function; not at a 16-byte boundary
    int is_valid_inside_function = guard_is_valid((void *)((char *)add + 1));
    if (is_valid_inside_function != 0) {
        return 4;
    }

    return 0;
}
