#pragma once
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <signal.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
namespace fs = std::filesystem;

namespace daemonpp {

enum DaemonStatus {
  STATUS_INIT_SUCCESS = 0x0,
  STATUS_CANNOT_ATTACH_STD_FDS_TO_NULL = 0x1,
  STATUS_CANNOT_CHDIR = 0x2,
  STATUS_CANNOT_SET_SID = 0xc,
  STATUS_CANNOT_FORK_DAEMON_PROCESS = 0xd,
  STATUS_UNKNOWN_DAEMON_ERROR = 0xe

};
class daemon {

  // should make a terminate function that cleanly removes the .pid and other
  // reasources its occupying
private:
  static int close_non_standard_file_descriptors(void) {
    unsigned int i;

    struct rlimit rlim;
    int num_of_fds = getrlimit(RLIMIT_NOFILE, &rlim);

    if (num_of_fds == -1)
      return false;

    for (i = 3; i < num_of_fds; i++)
      close(i);

    return true;
  }
  static int reset_signal_handlers_to_default(void) {
#if defined _NSIG

    unsigned int i;

    for (i = 1; i < _NSIG; i++) {
      if (i != SIGKILL && i != SIGSTOP)
        signal(i, SIG_DFL);
    }

#endif
    return true;
  }

  int clear_signal_mask(void) {
    sigset_t set;

    return ((sigemptyset(&set) == 0) &&
            (sigprocmask(SIG_SETMASK, &set, NULL) == 0));
  }

  int create_pid_file() {
    pid_t my_pid = getpid();
    char my_pid_str[10];
    int fd;
    std::string pid_file =  (this->name+ ".pid");
    sprintf(my_pid_str, "%d", my_pid);

    if ((fd = open(pid_file.data(), O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR)) ==
        -1)
      return false;

    if (write(fd, my_pid_str, strlen(my_pid_str)) == -1)
      return false;

    close(fd);

    return true;
  }
  int create_log_file() {
    FILE *log_file;
    std::string log_file_path = this->name + ".log";
    if ((log_file = fopen(log_file_path.data(), "a")) == NULL)
      return false;
    if ((log_file = freopen(nullptr, "w",log_file)) == NULL)
      return false;
    assert(log_file != NULL);
    setlinebuf(log_file);
    setlinebuf(stdout);
    setlinebuf(stderr);
    int log_fd = fileno(log_file);
    dup2(log_fd, STDOUT_FILENO);
    dup2(log_fd, STDERR_FILENO);

    return true;
  }
  int init_service() {
    if(close_non_standard_file_descriptors() == false){
        fprintf(stderr,"Failed to close non standard file descriptors\n");
        exit(1);
    }
    if(reset_signal_handlers_to_default() == false){
        fprintf(stderr,"Failed to reset signal handlers to default\n");
        exit(1);
    }
    if(clear_signal_mask() == false){
        fprintf(stderr,"Failed to clear signal mask\n");
        exit(1);
  
    }
    if(create_pid_file() == false){
        fprintf(stderr,"Failed to create pid file\n");
        exit(1);
    }
    if(create_log_file() == false){
        fprintf(stderr,"Failed to create log file\n");
        exit(1);  
    }
    return 0;
  }
  
  static daemon* instance; // Static tracker slot

  bool perform_cleanup() {
      std::string pid_file = (this->name + ".pid");
      if (remove(pid_file.data()) != 0) {
          fprintf(stderr, "Failed to remove pid file\n");
          return false;
      }
      return true;
  }
  static void signal_handler(int signum) {
      fprintf(stderr, "Received signal %d, performing cleanup and exiting\n", signum);
      if (instance != nullptr) {
          if (!instance->perform_cleanup()) {
              fprintf(stderr, "Cleanup failed, exiting with error\n");
              exit(1);
          }
      }
      exit(0);
  }
  void setup_signal_handlers() {
        struct sigaction sa;
        sa.sa_handler = signal_handler; 
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGINT,  &sa, nullptr);
        sigaction(SIGQUIT, &sa, nullptr);
        sigaction(SIGTSTP, &sa, nullptr);
    }

public:
  void daemonize() {
    pid_t child_pid = fork();
    assert(child_pid != -1);
    if (child_pid > 0)
      exit(EXIT_SUCCESS);
    // creates new session
    setsid();
    child_pid = fork();
    assert(child_pid != -1);
    if (child_pid > 0)
      exit(EXIT_SUCCESS);

    // daemon is now detached from terminal
    this->init_service();
    this->setup_signal_handlers();
  }

  daemon(const std::string &name) : name(name){instance = this;};

private:
  std::string name;
};
} // namespace daemonpp


daemonpp::daemon* daemonpp::daemon::instance = nullptr;