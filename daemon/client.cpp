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

    
    NetworkRequestChannel channel("127.0.0.1", 12345, NetworkRequestChannel::CLIENT_SIDE);
    int i = 1;
    while(i<argc){

        std::string command = argv[i]; 
        std::string filepath = argv[i+1];    
        std::cout << "Command: " << command << ", Filepath: " << filepath << std::endl;
        if(!fs::exists(filepath)){
            std::cerr<<"Error: file not found" << filepath << "\n";
            return 1;
        }
        fs::path full_path = fs::absolute(filepath);
        if(command=="encrypt"){
            Request req("encrypt", full_path.string(),0,0);
            channel.send_request(req);
        }
        else if(command=="decrypt"){
            Request req("decrypt", full_path.string(),0,0);
            channel.send_request(req);
        }

        i+=2;
    }

    return 0;
}