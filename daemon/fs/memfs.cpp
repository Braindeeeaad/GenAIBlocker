#include "memfs.hpp"
#include <filesystem>
#include <memory>
#include <iostream> 
#include <fstream>
#include <sstream>
#include "../crypto/crypto.hpp"

MemFsDirectory::MemFsDirectory(const fs::path& filepath, std::shared_ptr<char> key) 
    : MemFsDirEntry(filepath,key) {
    for(auto& entry : fs::directory_iterator(filepath)){
        if(entry.is_directory()){
            auto dir = std::make_unique<MemFsDirectory>(entry.path(),key);    
            entries.push_back(std::move(dir));
        }
        else{
            auto file = std::make_unique<MemFsFile>(entry.path(),key);
            entries.push_back(std::move(file));
        }
    }
}

void MemFsDirectory::save() {
    for(auto& entry: entries){
        if(!entry->is_directory()){
            entry->save(); 
        }
        //TODO: finish save operation for directories
        //note doesnt actually fix the issue 
        fs::create_directories(entry->path());
        entry->save();
    }
}


void MemFsDirectory::load() {
    for(auto& entry: entries){
        entry->load();
    }
}




void MemFsDirectory::addEntry(std::unique_ptr<MemFsDirEntry> entry) {
    entries.push_back(std::move(entry));
}

void MemFsDirectory::deleteEntry(const fs::path& fpath) {
    size_t idx = -1, i = 0;
    for(auto& entry: entries){
        if(entry->path() == filepath){
            idx = i;
        }
        i++;
    }
    if(idx>0){
        entries.erase(entries.begin() + idx);
    }
}

MemFsFile::MemFsFile(const fs::path& filepath, std::shared_ptr<char> key) 
    : MemFsDirEntry(filepath, key) {
    
}

void MemFsFile::save() {
    std::ofstream file(filepath.generic_string());
    file << this->encrypt_text;

}

void MemFsFile::load(){
    std::ifstream file(filepath.generic_string());
    if(!file.is_open()){
        std::cerr << "Failed to open file "<<filepath<<std::endl;

        return; 
    }

    //crypto cr();
    std::string line; 
    this->encrypt_text = "";
    while(std::getline(file,line)){
        this->encrypt_text+=line+"\n";
        //TODO: include line by line decryption code here
        std::string decrypt_line = crypto::decryptLine(line,(const unsigned char *)(key.get()));
        this->plain_text = decrypt_line;
    }

    file.close();
}

std::string MemFsFile::readFile(size_t line_num) {
    std::ifstream file(filepath.generic_string());
    if(!file.is_open()){
        std::cerr << "Failed to open file for reading" << filepath<<std::endl;
    }
    std::string output = "";
    std::string line;
    while(std::getline(file,line)){
        
    }
}

void MemFsFile::writeLine() {

}