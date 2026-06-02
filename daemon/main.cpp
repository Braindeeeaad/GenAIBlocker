#include "include/crypto.hpp"

namespace fs = std::filesystem;

int main(int argc, char *argv[]){
    if(argc<3){
        std::cerr<< "Usage:\n" 
                    <<" cblocker encrypt <file>\n" 
                    <<" cblocker decrypt <file>\n";
        return 1;
    }

    std::string command = argv[1]; 
    std::string filepath = argv[2];

    if(!fs::exists(filepath)){
        std::cerr<<"Error: file not found" << filepath << "\n";
        return 1;
    }
    
    crypto cr;

    if(command=="encrypt"){
        cr.encrypt_directory(filepath);
    }
    else if(command=="decrypt"){
        cr.decrypt_directory(filepath);
    }
    else{
        std::cerr<< "Unknown command: "<< command << "\n"
            <<"Valid commands: encrypt, decrypt\n";
        return 1;
    }


    
    return 0;
}