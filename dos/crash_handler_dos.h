#ifndef CRASH_HANDLER_DOS_H
#define CRASH_HANDLER_DOS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_crash_handler(void);

#ifdef CRASH_HANDLER_DOS_IMPLEMENTATION
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/exceptn.h>

static const char *crash_sig_name(int sig)
{
    switch (sig) {
        case SIGSEGV: return "SIGSEGV (Segmentation fault)";
        case SIGFPE:  return "SIGFPE (Floating point exception)";
        case SIGILL:  return "SIGILL (Illegal instruction)";
        case SIGABRT: return "SIGABRT (Abort)";
        case SIGTRAP: return "SIGTRAP (Trap)";
#ifdef SIGBUS
        case SIGBUS:  return "SIGBUS (Bus error)";
#endif
#ifdef SIGSYS
        case SIGSYS:  return "SIGSYS (Bad system call)";
#endif
        default:      return "UNKNOWN";
    }
}

static int crash_get_traceback(void *eip_list[], int size)
{
    struct __jmp_buf *state = (struct __jmp_buf *)__djgpp_exception_state_ptr;
    unsigned long ebp = state->__ebp;
    unsigned long eip = state->__eip;

    int count = 0;
    if (size > 0) {
        eip_list[count++] = (void *)eip;
    }

    unsigned long *frame = (unsigned long *)ebp;
    while (frame && count < size) {
        unsigned long retaddr = frame[1];
        if (retaddr == 0) {
            break;
        }
        eip_list[count++] = (void *)retaddr;
        frame = (unsigned long *)frame[0];
        if ((unsigned long)frame < 0x1000) {
            break;
        }
    }
    return count;
}

static void crash_handler(int sig)
{
    int fd = open("crash.log", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        _exit(1);
    }

    char crash_buf[4096];
    char *p = crash_buf;
#if PAL_HAS_GIT_REVISION
    p += sprintf(p, "%s\n", PAL_GIT_REVISION);
#endif
    p += sprintf(p, "Crash: %s (signal %d)\n", crash_sig_name(sig), sig);
    p += sprintf(p, "Call frame traceback EIPs:\n");

    void *eip_array[32];
    int n = crash_get_traceback(eip_array, 32);
    for (int i = 0; i < n; i++) {
        p += sprintf(p, "  0x%08lx\n", (unsigned long)eip_array[i]);
    }

    write(fd, crash_buf, (size_t)(p - crash_buf));
    close(fd);
    _exit(1);
}

void init_crash_handler(void)
{
    const int signals[] = {
        SIGSEGV, SIGFPE, SIGILL, SIGABRT, SIGTRAP,
#ifdef SIGBUS
        SIGBUS,
#endif
#ifdef SIGSYS
        SIGSYS,
#endif
    };
    const size_t num_signals = sizeof(signals) / sizeof(signals[0]);

    for (size_t i = 0; i < num_signals; i++) {
        signal(signals[i], crash_handler);
    }
}

#endif

#ifdef __cplusplus
}
#endif

#endif
