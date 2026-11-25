/*
 * fptr_libc.c - forward-edge hijack into a shared library.
 *
 * The attacker overwrites a callback with the address of a libc
 * function (_exit). That target lies outside the executable's .text,
 * so the table lookup must reject it without ever touching the bitmap.
 *
 * Plain: the callback becomes _exit(42) -> exit status 42.
 * Guard: blocked before the call.
 */
#include "../libguard/guard.h"
#include "attack.h"
#include <string.h>

static void log_status(int code)
{
    printf("status %d\n", code);
}

struct job {
    char  tag[8];
    void (*report)(int);
};

int main(void)
{
    GUARD_ENTER();
    guard_register(log_status);
    guard_seal();

    struct job j;
    j.report = log_status;

    /* The "attacker" overflows tag into report. */
    void *target = attacker_lookup("_exit");
    memcpy(&j.report, &target, sizeof target);

    GUARD_ICALL(j.report)(HIJACKED_STATUS);

    printf("report returned normally\n");
    GUARD_RETURN(0);
}
