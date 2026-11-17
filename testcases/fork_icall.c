/*
 * fork_icall.c - guarding functions should work after a fork
 */
#include "../libguard/guard.h"
#include <unistd.h>

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
    (void)mul;

    guard_seal();

    fork();

    // Should not error
    int (*fp)(int, int) = add;
    GUARD_ICALL(fp)(2, 3);
    
    return 0;
}
