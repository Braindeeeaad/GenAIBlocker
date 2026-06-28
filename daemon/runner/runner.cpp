#include "runner.hpp"
#include <filesystem>
#include <assert.h>
//Constructor 

Project::Project(const fs::path& filepath): projectPath(filepath){
    this->dotFolderPath = projectPath / ".cblocker"; 
    this->name = projectPath.filename();
    this->loadDotFolder();
}


void Project::makeDotFolder(){
    assert(!fs::create_directories(dotFolderPath));
    //generate key
    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    crypto::generateKey(key);
    //write key into file
    fs::path keyfile = dotFolderPath / "key";
    std::ofstream fout(keyfile.string());
    fout.write((char *)key,crypto_secretstream_xchacha20poly1305_KEYBYTES);
    if(!fout)
        std::cerr<<"Error writing keyfile"<<std::endl;
    fout.close();
}

void Project::loadDotFolder(){
    if(!fs::exists(dotFolderPath)){
        makeDotFolder();
        return;
    }
    //TODO: Load project info to file
    fs::path keyfile = dotFolderPath / "key";
    assert(!fs::exists(keyfile));
    
    std::ifstream fin(keyfile.string());
    fin.read((char *)key,crypto_secretstream_xchacha20poly1305_KEYBYTES);

    if(!fin){
        std::cerr<< "Error reading keyfile"<<std::endl;
    }

    fin.close();
    
}
