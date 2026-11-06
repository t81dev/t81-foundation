// Infrastructure only: no ARCH-003 candidate, oracle, G1/G2 or sanitizer probe.
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

static void memory(unsigned mb) {
  size_t n = static_cast<size_t>(mb) * 1024 * 1024;
  volatile unsigned char* p = static_cast<volatile unsigned char*>(malloc(n));
  if (!p) _exit(90);
  for (size_t i = 0; i < n; i += 4096) p[i] = 1;
  free(const_cast<unsigned char*>(p));
}
static int child(const char* self, const char* mode) {
  pid_t p = fork();
  if (p == 0) {
    execl(self, self, mode, static_cast<char*>(nullptr));
    _exit(91);
  }
  if (p < 0) return 92;
  int status = 0;
  if (waitpid(p, &status, 0) != p) return 93;
  return WIFEXITED(status) ? WEXITSTATUS(status) : 94;
}
static void* maintenance(void*) {
  memory(8);
  return nullptr;
}
static void* thread_exec(void* arg) {
  char* self = static_cast<char*>(arg);
  execl(self, self, "short", static_cast<char*>(nullptr));
  return nullptr;
}
int main(int argc, char** argv) {
  struct rlimit core = {0, 0};
  setrlimit(RLIMIT_CORE, &core);
  if (argc != 2) return 2;
  const char* mode = argv[1];
  if (!strcmp(mode, "thread-exec")) {
    pthread_t t;
    if (pthread_create(&t, nullptr, thread_exec, argv[0])) return 98;
    return pthread_join(t, nullptr);
  }
  if (!strcmp(mode, "threads")) {
    pthread_t t;
    if (pthread_create(&t, nullptr, maintenance, nullptr)) return 98;
    return pthread_join(t, nullptr);
  }
  if (!strcmp(mode, "leaf")) {
    memory(12);
    return 0;
  }
  if (!strcmp(mode, "short")) return 0;
  if (!strcmp(mode, "driver")) return child(argv[0], "leaf");
  if (!strcmp(mode, "sequential")) {
    for (unsigned i = 0; i < 3; ++i)
      if (child(argv[0], "short")) return 95;
    return 0;
  }
  if (!strcmp(mode, "nested")) return child(argv[0], "driver");
  if (!strcmp(mode, "preexec")) {
    memory(24);
    execl(argv[0], argv[0], "short", static_cast<char*>(nullptr));
    return 96;
  }
  if (!strcmp(mode, "signal")) {
    memory(4);
    raise(SIGTERM);
    return 97;
  }
  if (!strcmp(mode, "sleep")) {
    sleep(2);
    return 0;
  }
  if (!strcmp(mode, "detach")) {
    setsid();
    return child(argv[0], "sleep");
  }
  return 2;
}
