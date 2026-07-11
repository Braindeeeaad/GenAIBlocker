#include "project.hpp"
#include <cstring>
#include <filesystem>
#include <assert.h>
#include <fstream>
#include <string>
//Constructor 

Project::Project(const fs::path& filepath){
    this->projectPath = filepath;
    this->dotFolderPath = projectPath / ".cblocker"; 
    this->name = projectPath.filename();
    this->loadDotFolder();
    //making key a shared pntr
    std::shared_ptr<char[]> sharedKey(new char[crypto_secretstream_xchacha20poly1305_KEYBYTES]);
    memcpy(sharedKey.get(), this->key, crypto_secretstream_xchacha20poly1305_KEYBYTES);
    //initalizing memfs for current project
    this->readIgnoreFile();
    this->project = std::make_unique<MemFsDirectory>(
        fs::weakly_canonical(filepath),sharedKey,ignoredFiles,false
    );
}



void Project::init(){
    /*
        1. load files into memfs,
        2. Save files into memfs 

    */
    this->project->load(true);
    this->project->save();
}



void Project::readIgnoreFile(){
    fs::path ignoreFilePath = projectPath / ".cblockerignore";
    if(!fs::exists(ignoreFilePath))
        return;

    std::ifstream fin(ignoreFilePath.string());
    std::string line; 
    while(std::getline(fin,line)){
        ignoredFiles.insert(fs::weakly_canonical(projectPath/line));
    }
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

