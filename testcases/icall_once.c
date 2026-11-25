/*
 * icall_once.c - GUARD_ICALL must evaluate its argument exactly once.
 *
 * Benign: same output with and without Guard. If the macro evaluated
 * fp twice (once for the check, once for the call), the side effects
 * below would happen twice and the printed counters would differ from
 * the plain build.
 */
#include "../libguard/guard.h"
#include <stdio.h>

typedef int (*binop)(int, int);

static int add(int a, int b) { GUARD_ENTER(); GUARD_RETURN(a + b); }
static int sub(int a, int b) { GUARD_ENTER(); GUARD_RETURN(a - b); }
static int mul(int a, int b) { GUARD_ENTER(); GUARD_RETURN(a * b); }

static int picks = 0;

/* Returns a different function every time it is called. */
static binop next_op(void)
{
    GUARD_ENTER();
    static binop ops[] = { add, sub, mul };
    binop r = ops[picks % 3];
    picks++;
    GUARD_RETURN(r);
}

int main(void)
{
    GUARD_ENTER();
    guard_register(add);
    guard_register(sub);
    guard_register(mul);
    guard_seal();

    binop table[] = { add, sub, mul };
    int i = 0;

    /* table[i++]: i must advance by exactly one per call. */
    int r0 = GUARD_ICALL(table[i++])(7, 3);
    int r1 = GUARD_ICALL(table[i++])(7, 3);
    int r2 = GUARD_ICALL(table[i++])(7, 3);
    printf("table: %d %d %d, i=%d\n", r0, r1, r2, i);

    /* next_op(): must be called once per indirect call. */
    int s0 = GUARD_ICALL(next_op())(10, 4);
    int s1 = GUARD_ICALL(next_op())(10, 4);
    printf("next_op: %d %d, picks=%d\n", s0, s1, picks);

    /* Pointer-to-pointer, dereferenced inside the macro argument. */
    binop fp = mul;
    binop *pp = &fp;
    printf("deref: %d\n", GUARD_ICALL(*pp)(6, 7));

    if (i != 3 || picks != 2) {
        fprintf(stderr, "icall_once: side effects ran more than once\n");
        GUARD_RETURN(1);
    }
    GUARD_RETURN(0);
}
