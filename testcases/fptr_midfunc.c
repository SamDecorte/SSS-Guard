/*
 * fptr_midfunc.c - redirect a pointer into the middle of a registered function.
 */
#include "../libguard/guard.h"
#include <stdio.h>

static int long_function(int a, int b) {
    int c = a + b;
    int d = a * c;
    int e = d ^ a;

    return e;
}

int main(void)
{
    guard_register(long_function);
    guard_seal();

    // // Is OK
    // int (*fp2)(int, int) = long_function;
    // int result = GUARD_ICALL(fp2)(2, 3);
    // printf("Result: %d\n", result);

    // Should abort on failing check
    size_t addr = (size_t)(&long_function);
    addr += 16;
    int (*fp)(int, int) = (int (*)(int, int))(addr);
    GUARD_ICALL(fp)(2, 3);

    return 0;
}