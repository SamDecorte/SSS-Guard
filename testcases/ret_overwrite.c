#include "../libguard/guard.h"
#include "attack.h"
#include <string.h>

void win(void) {
    const char* message = "hijacked: win() runs\n";
    write(1, message, strlen(message));
    _exit(HIJACKED_STATUS);
}

__attribute__((noinline))
static void vulnerable(void* attacker_target) {
    GUARD_ENTER();

    char buffer[16];

    // saved return adress sits just above the frame pointer
    char* frame_pointer = (char*)__builtin_frame_adress(0);
    char* return_slot = frame_pointer + sizeof(void*);
    size_t overflow_size = (size_t)(return_slot - buffer) + sizeof(void*);

    // long copy that runs off the end of buffer into return slot
    char payload[64];
    memset(payload, 0, sizeof(payload));
    memcopy(payload + (overflow_size - sizeof(void*)), &attacker_target, sizeof(void*));
    memcpy(buffer, payload, overflow_size);

    GUARD_RETURN_VOID();
}

int main(void) {
    GUARD_ENTER();

    void* target = attacker_lookup("win");
    vulnerable(target);

    GUARD_RETURN(0);
}