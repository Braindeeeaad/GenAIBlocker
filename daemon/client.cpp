#include "network_channel.h"
#include <signal.h>
#include <stdio.h>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

int main(int argc, char *argv[]){
    if(argc<3){
        std::cerr<< "Usage:\n" 
                    <<" cblocker encrypt <file>\n" 
                    <<" cblocker decrypt <file>\n";
        return 1;
    }


    if((argc-1)%2!=0){
        std::cerr<<"Missing commands"<<std::endl;
        return 1;
    }

    NetworkRequestChannel channel("", 12345, NetworkRequestChannel::CLIENT_SIDE);
    channel.accept_connection();
    int i = 1;
    while(i<argc){

        std::string command = argv[i]; 
        std::string filepath = argv[i+1];    
        if(!fs::exists(filepath)){
            std::cerr<<"Error: file not found" << filepath << "\n";
            return 1;
        }

        if(command=="encrypt"){
            Request req("encrypt", filepath,0,0);
            channel.send_request(req);
        }
        else if(command=="decrypt"){
            Request req("decrypt", filepath,0,0);
            channel.send_request(req);
        }

        i+=2;
    }

    return 0;
}