#include <iostream>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <string.h>

#define CGROUP_FOLDER "/sys/fs/cgroup/pids/container/"
#define concat(a, b) (a "" b)

void write_rule(const char *path, const char *value)
{
  int fp = open(path, O_WRONLY | O_APPEND | O_CREAT);
  write(fp, value, strlen(value));
  close(fp);
}

void limitProcessCreation()
{
  mkdir(CGROUP_FOLDER, S_IRUSR | S_IWUSR);
  const char *pid = std::to_string(getpid()).c_str();
  write_rule(concat(CGROUP_FOLDER, "cgroup.procs"), pid);
  write_rule(concat(CGROUP_FOLDER, "notify_on_release"), "1");
  write_rule(concat(CGROUP_FOLDER, "pids.max"), "5");
}

void setup_fs()
{
  system("mkdir -p root");
  system("cd root && curl -Ol https://dl-cdn.alpinelinux.org/alpine/v3.15/releases/x86_64/alpine-minirootfs-3.15.1-x86_64.tar.gz");
  system("cd root && tar -xzf alpine-minirootfs-3.15.1-x86_64.tar.gz");
}

char *stack_memory()
{
  const int stackSize = 65536;
  auto *stack = new (std::nothrow) char[stackSize];

  if (stack == nullptr)
  {
    printf("Cannot allocate memory\n");
    exit(EXIT_FAILURE);
  }
  return stack + stackSize;
}

template <typename... P>
int run(P... params)
{
  char *_args[] = {(char *)params..., (char *)0};
  return execvp(_args[0], _args);
}

template <typename Function>
void clone_process(Function &&function, int flags)
{
  auto pid = clone(function, stack_memory(), flags, 0);
  wait(nullptr);
}

void setHostName(std::string hostname)
{
  sethostname(hostname.c_str(), hostname.size());
}

void setup_root(const char *folder)
{
  chroot(folder);
  chdir("/");
}

void setup_variables()
{
  clearenv();
  setenv("TERM", "xterm-256color", 0);
  setenv("PATH", "/bin/:/sbin/:/usr/bin:/usr/sbin", 0);
}

char **arguments;
int jail(void *args)
{
  printf("child pid: %d\n", getpid());
  limitProcessCreation();
  setHostName("app");
  setup_variables();
  setup_root("./root");

  mount("proc", "/proc", "proc", 0, 0);
  write_rule("/etc/resolv.conf", "nameserver 172.23.0.1\n");
  write_rule("/etc/hosts", "127.0.0.1      app\n");
  write_rule("/etc/host.conf", "order, hosts,bind\n");
  write_rule("/etc/host.conf", "multi on\n");

  auto runThis = [](void *args) -> int
  { return run(arguments[1], arguments[2], arguments[3], arguments[4]); };
  clone_process(runThis, SIGCHLD);

  umount("/proc");
  return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
  setup_fs();
  arguments = argv;
  clone_process(jail, CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD);
  return EXIT_SUCCESS;
}
