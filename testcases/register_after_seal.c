/*
 * register_after_seal.c - guard_register() after guard_seal() must
 * terminate the process.
 *
 * Detect test: the guarded build must stop with a "guard:" error.
 */
#include "../libguard/guard.h"
#include <stdio.h>

static void a(void) { }
static void b(void) { }

int main(void)
{
    guard_register(a);
    guard_seal();

    /* An attacker who can call guard_register late could whitelist
     * any target. This must not succeed. */
    guard_register(b);

    printf("register after seal was accepted\n");
    return 0;
}
