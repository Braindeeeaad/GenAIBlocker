#include "crypto.cpp"
#include "include/daemon.hpp"
namespace fs = std::filesystem;

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