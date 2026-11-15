#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

/* Fixed correctness input; no timer, workload dispatcher, or dynamic sizing. */
int main(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "success") &&
                     strcmp(argv[1], "exit7") && strcmp(argv[1], "signal")))
        return 64;
    volatile unsigned char bytes[65536];
    unsigned long checksum = 0;
    for (unsigned long i = 0; i < sizeof bytes; ++i) {
        bytes[i] = (unsigned char)((i * 17 + 3) & 255);
        checksum += bytes[i];
    }
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage)) return 65;
    printf("{\"pid\":%ld,\"ppid\":%ld,\"bytes\":65536,"
           "\"checksum\":%lu,\"user_seconds\":%ld,\"user_microseconds\":%ld,"
           "\"system_seconds\":%ld,\"system_microseconds\":%ld,"
           "\"maxrss_raw\":%ld}\n",
           (long)getpid(), (long)getppid(), checksum,
           (long)usage.ru_utime.tv_sec, (long)usage.ru_utime.tv_usec,
           (long)usage.ru_stime.tv_sec, (long)usage.ru_stime.tv_usec,
           usage.ru_maxrss);
    if (fflush(stdout)) return 66;
    if (!strcmp(argv[1], "exit7")) return 7;
    if (!strcmp(argv[1], "signal")) {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGTERM);
        if (sigprocmask(SIG_UNBLOCK, &set, NULL) ||
            signal(SIGTERM, SIG_DFL) == SIG_ERR) return 67;
        raise(SIGTERM);
        return 68;
    }
    return checksum == 8355840UL ? 0 : 69;
}
