/* Synthetic Linux x86_64 feature/counterexample probe. No codec or timing code. */
#if defined(__linux__) && defined(__x86_64__)
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/magic.h>
#include <linux/sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#define LIMIT (32UL * 1024 * 1024)
#define SAMPLE (48UL * 1024 * 1024)
static void die(const char *s) { perror(s); exit(1); }
static long read_text(const char *path, char *out, size_t cap) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) die(path);
    ssize_t n = read(fd, out, cap - 1);
    if (n < 0) die("read");
    out[n] = 0;
    if (close(fd)) die("close");
    return n;
}
static unsigned long rss(void) {
    char buf[8192];
    read_text("/proc/self/smaps_rollup", buf, sizeof(buf));
    char *line = strstr(buf, "Rss:");
    if (!line) { errno = EINVAL; die("missing Rss in smaps_rollup"); }
    return strtoul(line + 4, NULL, 10) * 1024;
}
static unsigned long current(const char *cg) {
    char path[1024], buf[128];
    if (snprintf(path, sizeof(path), "%s/memory.current", cg) >= (int)sizeof(path)) exit(1);
    read_text(path, buf, sizeof(buf));
    return strtoul(buf, NULL, 10);
}
static void membership(const char *stage) {
    char buf[4096];
    read_text("/proc/self/cgroup", buf, sizeof(buf));
    printf("%s pid=%ld cgroup=%s", stage, (long)getpid(), buf);
    fflush(stdout);
}
static int exec_child(const char *cg, int fd) {
    membership("F1_POST_EXEC");
    volatile unsigned char *p = mmap(NULL, SAMPLE, PROT_READ, MAP_SHARED, fd, 0);
    if ((const void *)p == MAP_FAILED) die("child mmap shared");
    unsigned long sum = 0;
    for (size_t i = 0; i < SAMPLE; i += 4096) sum += p[i];
    unsigned long resident = rss(), charged = current(cg);
    printf("F2_SHARED rss_bytes=%lu cgroup_current_bytes=%lu limit_bytes=%lu checksum=%lu\n",
           resident, charged, LIMIT, sum);
    if (munmap((void *)p, SAMPLE)) die("child munmap");
    return resident > LIMIT && charged < LIMIT ? 0 : 2;
}
static int rss_limit_counterexample(void) {
    struct rlimit limit = { LIMIT, LIMIT };
    if (setrlimit(RLIMIT_RSS, &limit)) die("setrlimit RSS");
    volatile unsigned char *p = mmap(NULL, SAMPLE, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((const void *)p == MAP_FAILED) die("anonymous mmap");
    for (size_t i = 0; i < SAMPLE; i += 4096) p[i] = 0x33;
    unsigned long resident = rss();
    printf("F2_RLIMIT_RSS rss_bytes=%lu limit_bytes=%lu survived=1\n", resident, LIMIT);
    if (munmap((void *)p, SAMPLE)) die("anonymous munmap");
    return resident > LIMIT ? 0 : 2;
}
int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 2 && !strcmp(argv[1], "--rss-limit")) return rss_limit_counterexample();
    if (argc == 4 && !strcmp(argv[1], "--exec-child")) return exec_child(argv[2], atoi(argv[3]));
    if (argc != 2) { fputs("usage: containment_probe PREPROVISIONED_CGROUP_LEAF\n", stderr); return 1; }
    alarm(30); /* bounded diagnostic failure handling, not an elapsed-time observation */
    const char *cg = argv[1];
    int group = open(cg, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (group < 0) die("open cgroup");
    struct statfs fs;
    if (fstatfs(group, &fs)) die("fstatfs");
    if ((unsigned long)fs.f_type != CGROUP2_SUPER_MAGIC) { errno = EINVAL; die("not cgroup v2"); }
    char path[1024], buf[4096];
    if (snprintf(path, sizeof(path), "%s/memory.max", cg) >= (int)sizeof(path)) return 1;
    read_text(path, buf, sizeof(buf));
    if (strtoul(buf, NULL, 10) != LIMIT) { errno = EINVAL; die("expected 32 MiB memory.max"); }
    if (snprintf(path, sizeof(path), "%s/cgroup.procs", cg) >= (int)sizeof(path)) return 1;
    if (read_text(path, buf, sizeof(buf)) != 0) { errno = EBUSY; die("target not empty"); }
    membership("F1_PARENT_OUTSIDE");
    /* Precharge shmem in the PARENT cgroup; child inherits the descriptor. */
    int shared = syscall(SYS_memfd_create, "h04-synthetic-shared", 0);
    if (shared < 0) die("memfd_create");
    char page[4096];
    memset(page, 0x5a, sizeof(page));
    for (size_t off = 0; off < SAMPLE; off += sizeof(page))
        if (write(shared, page, sizeof(page)) != (ssize_t)sizeof(page)) die("precharge shared page");
    int ready[2], release[2];
    if (pipe(ready) || pipe(release)) die("pipe");
    char fd_string[32];
    snprintf(fd_string, sizeof(fd_string), "%d", shared);
    char *child_argv[] = { argv[0], "--exec-child", argv[1], fd_string, NULL };
    char *child_env[] = { "LC_ALL=C", NULL };
    struct clone_args args = {0};
    args.flags = CLONE_INTO_CGROUP;
    args.exit_signal = SIGCHLD;
    args.cgroup = (unsigned long)group;
    long pid = syscall(SYS_clone3, &args, sizeof(args));
    if (pid < 0) die("clone3 CLONE_INTO_CGROUP (no fallback)");
    if (pid == 0) {
        /* First marked child operations: raw syscalls, no malloc or stdio. */
        int f = syscall(SYS_openat, AT_FDCWD, "/proc/self/cgroup", O_RDONLY, 0);
        if (f < 0) _exit(90);
        long n = syscall(SYS_read, f, buf, sizeof(buf));
        if (n <= 0 || syscall(SYS_write, ready[1], buf, n) != n) _exit(91);
        syscall(SYS_close, f);
        char go;
        if (syscall(SYS_read, release[0], &go, 1) != 1) _exit(92);
        execve("/proc/self/exe", child_argv, child_env);
        _exit(93);
    }
    close(ready[1]); close(release[0]);
    ssize_t n = read(ready[0], buf, sizeof(buf) - 1);
    if (n <= 0) die("child readiness");
    buf[n] = 0;
    printf("F1_FIRST_CHILD_MARKER child=%ld cgroup=%s", pid, buf);
    snprintf(path, sizeof(path), "/proc/%ld/cgroup", pid);
    read_text(path, buf, sizeof(buf));
    printf("F1_PARENT_OBSERVATION child=%ld cgroup=%s", pid, buf);
    snprintf(path, sizeof(path), "%s/cgroup.procs", cg);
    read_text(path, buf, sizeof(buf));
    printf("F1_TARGET_PROCS %s", buf);
    if (write(release[1], "x", 1) != 1) die("release child");
    close(ready[0]); close(release[1]);
    int status;
    if (waitpid((pid_t)pid, &status, 0) != pid) die("waitpid");
    snprintf(path, sizeof(path), "/proc/%ld", pid);
    printf("F4_CHILD_WAIT exited=%d code=%d proc_absent=%d\n", WIFEXITED(status),
           WIFEXITED(status) ? WEXITSTATUS(status) : -1, access(path, F_OK) != 0 && errno == ENOENT);
    unsigned char *p = mmap(NULL, 4096, PROT_READ, MAP_SHARED, shared, 0), vector;
    if (p == MAP_FAILED || mincore(p, 4096, &vector)) die("parent shared mincore");
    printf("F4_SHARED_AFTER_EXIT resident=%d value=%u\n", vector & 1, p[0]);
    if (munmap(p, 4096) || close(shared) || close(group)) die("cleanup");
    return WIFEXITED(status) ? WEXITSTATUS(status) : 3;
}
#else
int main(void) { return 77; } /* unsupported host: NOT a successful Linux probe */
#endif
