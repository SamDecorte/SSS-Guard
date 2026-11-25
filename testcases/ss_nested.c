/*
 * ss_nested.c - the shadow stack must stay balanced in normal code.
 *
 * Benign: same output with and without Guard. Exercises recursion,
 * mutual recursion, several return paths per function, early returns
 * from void functions, returns inside loops and switches, and
 * instrumented functions reached through guarded indirect calls.
 */
#include "../libguard/guard.h"
#include <stdio.h>

static int fib(int n)
{
    GUARD_ENTER();
    if (n < 2)
        GUARD_RETURN(n);
    GUARD_RETURN(fib(n - 1) + fib(n - 2));
}

static int is_odd(int n);

static int is_even(int n)
{
    GUARD_ENTER();
    if (n == 0)
        GUARD_RETURN(1);
    GUARD_RETURN(is_odd(n - 1));
}

static int is_odd(int n)
{
    GUARD_ENTER();
    if (n == 0)
        GUARD_RETURN(0);
    GUARD_RETURN(is_even(n - 1));
}

/* Return from inside a loop. */
static int find_first(const int *a, int n, int key)
{
    GUARD_ENTER();
    for (int i = 0; i < n; i++) {
        if (a[i] == key)
            GUARD_RETURN(i);
    }
    GUARD_RETURN(-1);
}

/* Return from every case of a switch. */
static const char *classify(int x)
{
    GUARD_ENTER();
    switch (x % 3) {
    case 0:  GUARD_RETURN("fizz");
    case 1:  GUARD_RETURN("one");
    default: GUARD_RETURN("two");
    }
}

/* Void function with an early return. */
static void print_if_positive(int x)
{
    GUARD_ENTER();
    if (x <= 0)
        GUARD_RETURN_VOID();
    printf("positive: %d\n", x);
    GUARD_RETURN_VOID();
}

/* Instrumented functions reached through guarded indirect calls,
 * which themselves recurse. */
typedef int (*unop)(int);

static int apply_n(unop f, int n, int x)
{
    GUARD_ENTER();
    if (n == 0)
        GUARD_RETURN(x);
    GUARD_RETURN(apply_n(f, n - 1, GUARD_ICALL(f)(x)));
}

static int twice(int x) { GUARD_ENTER(); GUARD_RETURN(2 * x); }
static int inc(int x)   { GUARD_ENTER(); GUARD_RETURN(x + 1); }

int main(void)
{
    GUARD_ENTER();
    guard_register(twice);
    guard_register(inc);
    guard_seal();

    printf("fib(20) = %d\n", fib(20));
    printf("is_even(101) = %d\n", is_even(101));

    int a[] = { 4, 8, 15, 16, 23, 42 };
    printf("find 23 -> %d, find 7 -> %d\n",
           find_first(a, 6, 23), find_first(a, 6, 7));

    for (int i = 0; i < 4; i++)
        printf("classify(%d) = %s\n", i, classify(i));

    print_if_positive(-5);
    print_if_positive(5);

    printf("twice^10(1) = %d\n", apply_n(twice, 10, 1));
    printf("inc^500(0)  = %d\n", apply_n(inc, 500, 0));

    GUARD_RETURN(0);
}
