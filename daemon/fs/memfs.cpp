#include "memfs.hpp"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <iostream> 
#include <fstream>
#include <sstream>
#include <cassert>
#include "../crypto/crypto.hpp"

MemFsDirectory::MemFsDirectory(const fs::path& filepath, std::shared_ptr<char[]> key) 
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

MemFsFile::MemFsFile(const fs::path& filepath, std::shared_ptr<char []> key) 
    : MemFsDirEntry(filepath, key) {
    
}

void MemFsFile::save() {
    std::ofstream file(filepath.generic_string());
    file << this->encrypt_text;

}


void calculate_offset(std::vector<off_t> offset, std::string file){
    offset.clear(); 
    std::string curr_substr = file;
    offset.push_back(0);
    while(curr_substr.find('\n')>-1){
        size_t idx = curr_substr.find('\n');
        offset.insert(offset.begin(), idx+1);
        curr_substr = curr_substr.substr(idx+1);
    }
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
        std::string decrypt_line = crypto::decryptLine(line,(const unsigned char*)key.get());
        this->plain_text = decrypt_line;
    }
    calculate_offset(this->plain_offset, this->plain_text);
    calculate_offset(this->encrypt_offset, this->encrypt_text);
    file.close();
}



std::string MemFsFile::readFile(size_t line_num,size_t window_size) {
    assert(this->encrypt_offset.size()==this->plain_offset.size());
    std::stringstream out_string;
    for(size_t i = 0; i<plain_offset.size(); i++){
        std::vector<off_t> curr_offset = (i>=line_num-window_size && i<=line_num+window_size) ? plain_offset : encrypt_offset;
        std::string curr_string = (i>=line_num-window_size && i<=line_num+window_size) ? plain_text : encrypt_text;
        if(curr_offset.size()-1==i){
            size_t idx = curr_offset.at(i);
            out_string << curr_string.substr(idx);
            continue;
        }
        size_t idx = curr_offset.at(i);
        size_t next_idx = curr_offset.at(i+1);
        std::string sub_string = curr_string.substr(idx, next_idx-idx);
        out_string << sub_string << std::endl;
    
    }
    return out_string.str();
}

void MemFsFile::writeLine(size_t line_num, std::string new_line) {
    std::string encrypted_line = crypto::encryptLine(new_line, (const unsigned char *)(key.get()));
    size_t enc_idx = encrypt_offset.at(line_num);
    size_t plain_idx = plain_offset.at(line_num);
        
    //erasing old line
    if(line_num == encrypt_offset.size()-1){
        std::string last_encrypt_line = encrypt_text.substr(enc_idx);
        std::string last_plain_line = plain_text.substr(plain_idx);
        encrypt_text.erase(enc_idx,last_encrypt_line.size());
        plain_text.erase(plain_idx,last_plain_line.size());

    }
    else{
        size_t next_enc_idx = encrypt_offset.at(line_num+1);
        size_t next_plain_idx = plain_offset.at(line_num+1);
        encrypt_text.erase(enc_idx,next_enc_idx);
        plain_text.erase(plain_idx,next_plain_idx);
    }

    encrypt_text.insert(enc_idx,encrypted_line+"\n");
    plain_text.insert(plain_idx,new_line+"\n");
    calculate_offset(this->plain_offset, this->plain_text);
    calculate_offset(this->encrypt_offset, this->encrypt_text);
}