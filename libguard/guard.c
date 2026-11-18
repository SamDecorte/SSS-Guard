/*
 * guard.c - Guard runtime library.
 *
 * Every function below is a stub. Implement them in Part 1.
 */
#include "guard.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <unistd.h>


extern char __executable_start;
extern char _etext;

static const unsigned int SLOT_SIZE_BYTES = 16;
static char *bitmap = NULL;

static unsigned int is_sealed = 0;

// Variables for 1.3
#define SHADOW_STACK_ENTRIES 65536 // Room for 512KB

static void **shadow_stack_start = NULL;
static void **shadow_stack_end = NULL;
static void **shadow_stack_top = NULL;


__attribute__((constructor)) static void guard_init(void)
{
    // You can use this function to initialize the libguard library before any other code runs.

    // Make the bitmap for all valid call targets.
    size_t number_of_bits = (&_etext - &__executable_start) / SLOT_SIZE_BYTES;
    // 8 bits per byte. Round up to the nearest byte.
    size_t size = (number_of_bits + 7) / 8;
    bitmap = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void guard_register(void *fn)
{
    // Set the bit for fn in the bitmap.

    if (is_sealed) {
        // Give this abort a better error message than just "segmentation fault".
        guard_abort("failed to register function: guard is sealed");
    }

    size_t offset = (char *)fn - &__executable_start;
    // Set its corresponding bit
    size_t corresponding_bit = offset / SLOT_SIZE_BYTES;   // 16 byte slots; one bit in the bitmap corresponds with a 16-byte region
    size_t corresponding_byte = corresponding_bit / 8;
    bitmap[corresponding_byte] |= 1 << (corresponding_bit % 8);
}

void guard_seal(void)
{
    // mprotect the bitmap read-only.
    size_t number_of_bits = (&_etext - &__executable_start) / SLOT_SIZE_BYTES;
    // 8 bits per byte. Round up to the nearest byte.
    size_t size = (number_of_bits + 7) / 8;
    int result = mprotect(bitmap, size, PROT_READ);
    if (result != 0) {
        guard_abort("failed to mprotect the bitmap");
    }

    is_sealed = 1;
}

int guard_is_valid(const void *addr)
{
    // Returning non-zero means OK
    // O(1) bitmap lookup.

    // Check: outside of text segment?
    if (((char *)addr < &__executable_start) || ((char *)addr >= &_etext)) {
        return 0;
    }

    // Check: has a hit?
    size_t offset = (char *)addr - &__executable_start;
    size_t corresponding_bit = offset / SLOT_SIZE_BYTES;
    size_t corresponding_byte = corresponding_bit / 8;

    size_t bit_at_position = bitmap[corresponding_byte] & (1 << (corresponding_bit % 8));

    if (bit_at_position == 0) {
        return 0;
    }

    // Check: address inside a function, but not at the start?
    if (offset % SLOT_SIZE_BYTES != 0) {
        return 0;
    }

    return 1;
}

_Noreturn void guard_abort(const char *reason)
{
    fprintf(stderr, "guard: %s\n", reason);
    //free(bitmap);
    abort();
}

_Noreturn void guard_violation(const char *what, const void *addr,
                               const char *file, int line)
{
    fprintf(stderr, "guard: %s violation at %s:%d (target %p)\n", what, file, line, addr);
    //free(bitmap);
    abort();
}

static void shadow_stack_init(void)
{
    size_t page_size = sysconf(_SC_PAGESIZE); // Usually 4 KB
    size_t stack_size = SHADOW_STACK_ENTRIES * sizeof(void *);
    size_t total_size = page_size + stack_size + page_size;

    char *memory = mmap(NULL, total_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); // Reserve one extra page before and after it + everything starts with no access
    if (memory == MAP_FAILED) {
        guard_abort("failed to allocate the shadow stack");
    }

    char *usable_part = memory + page_size;
    int result = mprotect(usable_part, stack_size, PROT_READ | PROT_WRITE); // Only make the middle readable and writable
    if (result != 0) {
        guard_abort("failed to mprotect the shadow stack");
    }
    // Now there are 2 guard pages in place (before and after) that causes a crash if anything runs past the ends

    shadow_stack_start = (void **)usable_part;
    shadow_stack_end = shadow_stack_start + SHADOW_STACK_ENTRIES;
    shadow_stack_top = shadow_stack_start;
}

void guard_ss_push(void *ret)
{
    if (shadow_stack_start == NULL) {
        shadow_stack_init();
    }

    if (shadow_stack_top == shadow_stack_end) {
        guard_abort("shadow stack overflow");
    }

    *shadow_stack_top = ret;
    shadow_stack_top++;
}


void guard_ss_check_pop(void *ret, const char *file, int line)
{
    if (shadow_stack_top == shadow_stack_start) {
        guard_abort("shadow stack underflow");
    }

    shadow_stack_top--;

    if (*shadow_stack_top != ret) {
        guard_violation("return address", ret, file, line);
    }
}

