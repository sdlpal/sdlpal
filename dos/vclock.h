#ifndef VCLOCK_H
#define VCLOCK_H

#include <stdint.h>

#define VCLOCKS_PER_SEC 1000000UL  // Time resolution: 1 tick = 1 microsecond

typedef uint64_t vclock_t;

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define VCLOCK_DEFAULT_HZ 100

/**
 * User-defined periodic hook function type.
 * Each hook receives a user-provided pointer on each invocation.
 */
typedef void (*vhook_fn)(void *userdata);

void vclock_setup(int freq);
// Time accessors
vclock_t   vclock(void);                     // Current timestamp in microseconds

// Delay primitive
void       vclock_delay(uint32_t ms);        // Blocking delay using busy wait

// Hook API
int        vhook_register(vhook_fn fn, uint16_t hz, void *userdata); // Register callback at frequency
int        vhook_unregister(vhook_fn fn);                             // Unregister callback by function pointer

#ifdef __cplusplus
}
#endif
#endif  // VCLOCK_H