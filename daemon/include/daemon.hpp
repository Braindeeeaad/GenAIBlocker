#pragma once
#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <csignal>
#include <sys/stat.h>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace daemonpp{
    
    class daemon{

        private: 
            void daemonize(){
                pid_t pid = fork(); 
                
                //error when forking
                if(pid< 0) exit(EXIT_FAILURE); 
                //early terminating parent process so only child process runs
                if(pid>0) exit(EXIT_SUCCESS);

                //child process code 

                umask(0);
                chdir("/");
                
                close(STDIN_FILENO);
                close(STDOUT_FILENO); 
                close(STDERR_FILENO);

            }
        
        public:
            daemon(){};
    
    };
}