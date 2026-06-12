#include "crypto.cpp"
#include "include/daemon.hpp"
#include "network_channel.h"
#include <signal.h>

namespace fs = std::filesystem;




/*
    What do we need to do? 
    
    - need to setup network channel in main after daeminization
    - Need to figure out terminating condition of the main loop
    - Need to provide signal handlers to properly clean up reasources 
    


*/

static void handle_termination(){
    
}

static void init_service(){

}

static void run_main_loop(){

}


int main(int argc, char *argv[]){
    if(argc<3){
        std::cerr<< "Usage:\n" 
                    <<" cblocker encrypt <file>\n" 
                    <<" cblocker decrypt <file>\n";
        return 1;
    }

    crypto cr;

    if((argc-1)%2!=0){
        std::cerr<<"Missing commands"<<std::endl;
        return 1;
    }

    daemonpp::daemon dm("cblocker");
    dm.daemonize();

    int i = 1;
    while(i<argc){

        std::string command = argv[i]; 
        std::string filepath = argv[i+1];    
        if(!fs::exists(filepath)){
            std::cerr<<"Error: file not found" << filepath << "\n";
            return 1;
        }

        if(command=="encrypt"){
            cr.encrypt_directory(filepath);
        }
        else if(command=="decrypt"){
            cr.decrypt_directory(filepath);
        }

        i+=2;
    }

    return 0;
}