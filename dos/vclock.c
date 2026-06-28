#include <signal.h>
#include <setjmp.h>
#include "vclock.h"
#include <dpmi.h>
#include <dos.h>
#include <go32.h>
#include <stdlib.h>
#include <stdio.h>
#include <pc.h>
#include <libc/farptrgs.h>

// PIT constants
#define PIT_CONTROL    0x43                       // PIT control port
#define PIT_COUNTER0   0x40                       // PIT channel 0 counter port
#define PIT_INPUT      1193182UL
#define PIT_BIOS_DIVISOR 65536UL                  // BIOS timer divisor (~18.2Hz)
#define BIOS_TICK_ADDR 0x46CUL                    // BIOS Data Area tick counter
#define BIOS_MIDNIGHT_FLAG_ADDR 0x470UL           // BIOS midnight rollover flag
#define BIOS_TICKS_PER_DAY 0x1800B0UL             // 24h * 18.2065Hz
#define IRQ0_VECTOR    0x08                       // IRQ0 protected-mode interrupt vector
#define MAX_VHOOKS     16                         // Max number of simultaneous hooks

#if 1
#include <SDL3/SDL.h>
#define VCLOCK_DEBUG_LOG(fmt, ...) \
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[VCLOCK] " fmt, ##__VA_ARGS__)
#else
#define VCLOCK_DEBUG_LOG(fmt, ...) \
    printf("[VCLOCK] " fmt "\n", ##__VA_ARGS__)
#endif

// Runtime parameters
static int vclock_hz = VCLOCK_DEFAULT_HZ;         // Default 100Hz
static unsigned long vclock_pit_divisor = PIT_INPUT / VCLOCK_DEFAULT_HZ;
static unsigned long vclocks_per_tick = VCLOCKS_PER_SEC / VCLOCK_DEFAULT_HZ;

static int vclock_ready = 0;              // Fast-path init guard for non-ISR code
static int vclock_lazy_init_called = 0;   // Enforce lazy_init one-shot behavior
static volatile uint32_t bios_tick_pit_accum;
static volatile int vclock_inited;

static void vclock_setup_pit(void);
static void vclock_lazy_init(void);
static void vhook_recalc_intervals(void);

static inline void vclock_ensure_init(void) {
    if (!vclock_ready && !vclock_lazy_init_called) {
        vclock_lazy_init_called = 1;
        vclock_lazy_init();
    }
}

// Runtime clock setup
void vclock_setup(int freq) {
    if (freq < 1) freq = VCLOCK_DEFAULT_HZ;
    vclock_hz = freq;
    vclock_pit_divisor = PIT_INPUT / vclock_hz;
    if (vclock_pit_divisor == 0) vclock_pit_divisor = 1;
    vclocks_per_tick = VCLOCKS_PER_SEC / vclock_hz;
    vhook_recalc_intervals();
    vclock_ensure_init();
    if (vclock_inited) {
        vclock_setup_pit();
        bios_tick_pit_accum = 0;
    }
    VCLOCK_DEBUG_LOG("vclock_setup: hz=%d", vclock_hz);
}

/**
 * Internal representation of a periodic callback.
 * `interval_ticks` determines how often the function is called,
 * based on number of PIT-driven IRQ0 ticks.
 */
typedef struct {
    vhook_fn fn;             // Function pointer to invoke
    void *userdata;          // User context passed on each call
    uint16_t hz;             // Requested callback frequency
    uint16_t interval_ticks; // Interval between calls, in ticks
    uint16_t counter;        // Tick accumulator
} VHook;

static VHook vhooks[MAX_VHOOKS];  // Registered hook list
static int vhook_count = 0;       // Current number of active hooks

static void vhook_recalc_intervals(void)
{
    int i;

    for (i = 0; i < vhook_count; ++i) {
        VHook *h = &vhooks[i];
        uint16_t interval_ticks;

        if (h->hz == 0) {
            h->interval_ticks = 1;
            h->counter = 0;
            continue;
        }

        interval_ticks = (vclock_hz + h->hz / 2) / h->hz;
        if (interval_ticks == 0) interval_ticks = 1;

        h->interval_ticks = interval_ticks;
        if (h->counter >= interval_ticks) {
            h->counter %= interval_ticks;
        }
    }
}

// Timekeeping state
static volatile vclock_t tick_acc = 0;   // Accumulated time in microseconds
static volatile uint32_t tick_count = 0; // Number of ISR ticks since startup
static volatile uint32_t irq_call_count = 0; // Debug: count of IRQ handler calls
static volatile uint32_t bios_tick_count = 0; // Debug: 18.2Hz payload calls
static volatile uint32_t bios_tick_pit_accum = 0; // PIT-clock accumulator for 18.2Hz
static volatile int vclock_inited = 0;            // Whether module has been initialized

static _go32_dpmi_seginfo old_irq0;            // Original IRQ0 handler to restore

static inline void bios_tick_add(int step)
{
    long ticks = (long)_farpeekl(_dos_ds, BIOS_TICK_ADDR);
    unsigned char midnight_flag = _farpeekb(_dos_ds, BIOS_MIDNIGHT_FLAG_ADDR);

    ticks += step;
    while (ticks >= (long)BIOS_TICKS_PER_DAY) {
        ticks -= (long)BIOS_TICKS_PER_DAY;
        midnight_flag++;
    }
    while (ticks < 0) {
        ticks += (long)BIOS_TICKS_PER_DAY;
        if (midnight_flag != 0) {
            midnight_flag--;
        }
    }

    _farpokel(_dos_ds, BIOS_TICK_ADDR, (unsigned long)ticks);
    _farpokeb(_dos_ds, BIOS_MIDNIGHT_FLAG_ADDR, midnight_flag);
}

/**
 * IRQ0 handler (frequency set by vclock_hz).
 * Increments internal time counter and dispatches user hooks.
 * This function is called via assembly wrapper for proper interrupt handling.
 */
static void __attribute__((no_reorder)) vclock_irq0_handler(void)
{
    int i;

    irq_call_count++; // Debug counter
    tick_acc  += vclocks_per_tick; // Advance time
    tick_count += 1;
    for (i = 0; i < vhook_count; ++i) {
        VHook *h = &vhooks[i];
        h->counter += 1;
        if (h->counter >= h->interval_ticks) {
            h->counter = 0;
            h->fn(h->userdata); // Invoke hook with its userdata
        }
    }

    /*
     * Keep chained BIOS IRQ0 behavior but cancel its per-IRQ tick advance:
     * we will re-add only at emulated original bios tick cadence below.
     */
    bios_tick_add(-1);

    /* Add original tick at original frequency regardless of vclock_hz. */
    bios_tick_pit_accum += (uint32_t)vclock_pit_divisor;
    while (bios_tick_pit_accum >= PIT_BIOS_DIVISOR) {
        bios_tick_pit_accum -= PIT_BIOS_DIVISOR;
        bios_tick_add(+1);
    }
}

static int __attribute__((no_reorder))
vclock_lockisr(void)
{
    size_t len = (void *)vclock_lockisr - (void *)vclock_irq0_handler;

    VCLOCK_DEBUG_LOG("VCLOCK ISR code size is %zd bytes", len);

    /* Lock interrupt service routine. */
    if (_go32_dpmi_lock_code(vclock_irq0_handler, len)) {
        VCLOCK_DEBUG_LOG("Failed to lock VCLOCK ISR code (%zd bytes)", len);
        return -1;
    }

    return 0;
}

/**
 * Called on program termination via atexit().
 * Restores IRQ0 handler and resets PIT to BIOS-safe values.
 */
static void vclock_uninstall(void)
{
    VCLOCK_DEBUG_LOG("[vclock] vclock_uninstall() called via atexit");
    if (!vclock_inited) return;
    vclock_inited = 0;
    vclock_ready = 0;

    // Restore previous interrupt vector for IRQ0
    _go32_dpmi_set_protected_mode_interrupt_vector(IRQ0_VECTOR, &old_irq0);

    // Optionally reset PIT to BIOS default (18.2Hz)
    outportb(PIT_CONTROL, 0x34);
    outportb(PIT_COUNTER0, 0x00);
    outportb(PIT_COUNTER0, 0x00);

    // Clear internal state
    tick_acc = 0;
    tick_count = 0;
    irq_call_count = 0;
    bios_tick_count = 0;
    bios_tick_pit_accum = 0;
    vhook_count = 0;
}

static void vclock_setup_pit(void) {
    outportb(PIT_CONTROL, 0x34);
    outportb(PIT_COUNTER0, vclock_pit_divisor & 0xFF);
    outportb(PIT_COUNTER0, (vclock_pit_divisor >> 8) & 0xFF);
}

// Returns current time in microseconds
static inline uint64_t vclock_usec_now(void) {
    return tick_acc;
}


/**
 * Initializes PIT and installs IRQ0 handler.
 * Automatically triggered the first time any clock function is used.
 */
static void vclock_lazy_init(void)
{
    _go32_dpmi_seginfo new_vector;

    if (vclock_ready) return; // Already initialized (fast path)
    if (vclock_inited) {
        vclock_ready = 1;
        return;
    }
    vclock_setup_pit();
    tick_acc = 0;
    tick_count = 0;
    irq_call_count = 0;
    bios_tick_count = 0;
    bios_tick_pit_accum = 0;
    vhook_count = 0;
    // Set interrupt vector
    VCLOCK_DEBUG_LOG("[vclock] lazy_init(): starting");
    VCLOCK_DEBUG_LOG("[vclock] pit_divisor = %lu", vclock_pit_divisor);
    new_vector.pm_offset = (unsigned long)vclock_irq0_handler;
    new_vector.pm_selector = _go32_my_cs();
    _go32_dpmi_get_protected_mode_interrupt_vector(IRQ0_VECTOR, &old_irq0);
    VCLOCK_DEBUG_LOG("[vclock] IRQ0 vector got (sel=%x, addr=%lx)", (unsigned int)old_irq0.pm_selector, (unsigned long)old_irq0.pm_offset);
    if(vclock_lockisr() < 0) {
        VCLOCK_DEBUG_LOG("[vclock] Failed to lock VCLOCK ISR");
        vclock_inited = 0;
        vclock_ready = 0;
        return;
    }
    if (_go32_dpmi_chain_protected_mode_interrupt_vector(IRQ0_VECTOR, &new_vector)) {
        VCLOCK_DEBUG_LOG("[vclock] Failed to install IRQ0 vector");
        vclock_inited = 0;
        vclock_ready = 0;
        return;
    }
    vclock_inited = 1;
    vclock_ready = 1;
    VCLOCK_DEBUG_LOG("[vclock] IRQ0 vector installed (sel=%x, addr=%lx)", (unsigned int)new_vector.pm_selector, (unsigned long)new_vector.pm_offset);
    atexit(vclock_uninstall);
}

/**
 * Returns current timestamp in microseconds.
 */
vclock_t vclock(void)
{
    //vclock_ensure_init();
    return vclock_usec_now();
}

/**
 * Blocks for the specified number of milliseconds.
 * Uses high-resolution busy loop.
 */
void vclock_delay(uint32_t ms)
{
    vclock_t target;
    int timeout_counter = 0;
    //vclock_ensure_init();
    target = tick_acc + ((vclock_t)ms * (VCLOCKS_PER_SEC / 1000UL));
    while (tick_acc < target && timeout_counter < (ms * 10000)) {
        asm volatile ("nop");
        __dpmi_yield();
        //delay(0);
    }
}

/**
 * Registers a periodic hook callback.
 * Hook will be called every (1000 / hz) milliseconds.
 *
 * @param fn       Function pointer to invoke
 * @param hz       Frequency in Hz (calls per second)
 * @param userdata User-specified context pointer returned on each call
 * @return         0 on success, -1 on failure
 */
int vhook_register(vhook_fn fn, uint16_t hz, void *userdata)
{
    if (vhook_count >= MAX_VHOOKS || hz == 0) return -1;

    vhooks[vhook_count].fn             = fn;
    vhooks[vhook_count].hz             = hz;
    vhooks[vhook_count].interval_ticks = 1;
    vhooks[vhook_count].counter        = 0;
    vhooks[vhook_count].userdata       = userdata;
    vhook_count += 1;
    vhook_recalc_intervals();

    return 0;
}

/**
 * Unregisters a previously registered hook by function pointer.
 * Only matches on `fn`, not userdata.
 *
 * @param fn Hook function to remove
 * @return   0 if removed, -1 if not found
 */
int vhook_unregister(vhook_fn fn)
{
    int i, j;

    for (i = 0; i < vhook_count; ++i) {
        if (vhooks[i].fn == fn) {
            // Shift remaining hooks to fill the gap
            for (j = i; j < vhook_count - 1; ++j) {
                vhooks[j] = vhooks[j + 1];
            }
            vhook_count -= 1;
            return 0;
        }
    }

    return -1;
}