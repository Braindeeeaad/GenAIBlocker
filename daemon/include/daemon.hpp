#pragma once
#include <iostream>
#include <unistd.h>
#include <assert.h>
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
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace daemonpp{
    
    class daemon{

        private: 
            void daemonize(){
                pid_t child_pid = fork(); 
                assert(child_pid != -1);
                if(child_pid>0)
                    exit(EXIT_SUCCESS);
                //creates new session
                setsid();
                child_pid = fork(); 
                assert(child_pid != -1);
                if(child_pid>0)
                    exit(EXIT_SUCCESS);

                //daemon is now detached from terminal  
                
                //saving pid locally in .pid

                //TODO: make the duplicate daemon spin up prevention better
                assert(!fs::exists(("cblocker_daemon.pid")));
                std::ofstream pid_save("cblocker_daemon.pid");
                
                if(!pid_save.is_open()){
                    exit(EXIT_FAILURE);
                }
                pid_t pid  = getpid();
                pid_save.write((char *)&pid,sizeof(pid));


                //setting up log file
                
                const char *log_file_path = "cblocker_daemon.log";
                FILE *log_file = fopen(log_file_path,"a");
                assert(log_file!=NULL);
                setlinebuf(log_file);
                setlinebuf(stdout);
                setlinebuf(stderr); 
                int log_fd =fileno(log_file);
                dup2(log_fd,STDOUT_FILENO);
                dup2(log_fd,STDERR_FILENO);

            }
        
        public:
            daemon(){};
    
    };
}