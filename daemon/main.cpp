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


int main(int argc, char *argv[]){

    crypto cr;

    if((argc-1)%2!=0){
        std::cerr<<"Missing commands"<<std::endl;
        return 1;
    }

    daemonpp::daemon dm("cblocker");
    dm.daemonize();
    NetworkRequestChannel channel("", 12345, NetworkRequestChannel::SERVER_SIDE);
    channel.accept_connection();


    while(true){
        Request req = channel.receive_request();
        std::string command = req.command;
        std::string filepath = req.filepath;
        if(command=="encrypt"){
            cr.encrypt_directory(filepath);
        }
        else if(command=="decrypt"){
            cr.decrypt_directory(filepath);
        }
        channel.send_response(Response(true,"",""));
    }

    return 0;
}