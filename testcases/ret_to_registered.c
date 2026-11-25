/*
 * ret_to_registered.c - backward-edge hijack to a *valid call target*.
 *
 * The return address is overwritten with the start of a function that
 * is registered in the call-target table. That makes it a legitimate
 * forward-edge target, but it is still not where this function was
 * called from, so the shadow stack must reject it. This checks that
 * the backward edge does not just reuse the forward-edge table.
 *
 * Plain: returns into admin_action() -> exit status 42.
 * Guard: blocked at GUARD_RETURN_VOID().
 */
#include "../libguard/guard.h"
#include "attack.h"
#include <string.h>

/* Legitimate, address-taken callback. Uses write()/_exit() only: we
 * arrive here by `ret`, so the stack is not 16-byte aligned. */
static void admin_action(void)
{
    const char *msg = "hijacked: admin_action() ran via ret\n";
    write(1, msg, strlen(msg));
    _exit(HIJACKED_STATUS);
}

static void user_action(void)
{
    printf("user action\n");
}

static void (*actions[])(void) = { user_action, admin_action };

__attribute__((noinline))
static void vulnerable(void *attacker_target)
{
    GUARD_ENTER();

    char buffer[16];

    /* x86-64 with frame pointers: saved return address is at rbp+8. */
    char *frame_pointer = (char *)__builtin_frame_address(0);
    char *return_slot = frame_pointer + sizeof(void *);
    size_t overflow_size = (size_t)(return_slot - buffer) + sizeof(void *);

    char payload[256];
    if (overflow_size > sizeof payload)
        _exit(3);  /* unexpected frame layout */
    memset(payload, 'A', sizeof payload);
    /* Keep the saved frame pointer intact; only the return address changes. */
    memcpy(payload + (frame_pointer - buffer), frame_pointer, sizeof(void *));
    memcpy(payload + (overflow_size - sizeof(void *)), &attacker_target,
           sizeof(void *));
    memcpy(buffer, payload, overflow_size);

    GUARD_RETURN_VOID();
}

int main(void)
{
    GUARD_ENTER();
    guard_register(user_action);
    guard_register(admin_action);
    guard_seal();

    GUARD_ICALL(actions[0])();

    /* The attacker picks a target that passes guard_is_valid(). */
    void *target = (void *)actions[1];
    if (!guard_is_valid(target)) {
        fprintf(stderr, "setup error: admin_action should be registered\n");
        GUARD_RETURN(2);
    }
    vulnerable(target);

    printf("vulnerable returned normally\n");
    GUARD_RETURN(0);
}
