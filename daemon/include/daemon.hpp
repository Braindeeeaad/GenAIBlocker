#pragma once
#include <unistd.h>
#include <assert.h>

#include <cstring>
#include <string>

#include <sys/stat.h>

#include <fstream>
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
                assert(!fs::exists((name+"pid")));
                std::ofstream pid_save(name+".pid");
                
                if(!pid_save.is_open()){
                    exit(EXIT_FAILURE);
                }
                pid_t pid  = getpid();
                pid_save.write((char *)&pid,sizeof(pid));


                //setting up log file
                std::string log_file_name = name+".log";
                const char *log_file_path = log_file_name.data();
                FILE *log_file = fopen(log_file_path,"a");
                assert(log_file!=NULL);
                setlinebuf(log_file);
                setlinebuf(stdout);
                setlinebuf(stderr); 
                int log_fd =fileno(log_file);
                dup2(log_fd,STDOUT_FILENO);
                dup2(log_fd,STDERR_FILENO);

            }
        
        //should make a terminate function that cleanly removes the .pid and other reasources its occupying
        public:
            daemon(const std::string& name):name(name){
                this->daemonize();
            };
    
        private: 
            std::string name;
    };
}