#include "memfs.hpp"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <iostream> 
#include <fstream>
#include <sstream>
#include <cassert>
#include <unordered_set>
#include "../crypto/crypto.hpp"

MemFsDirectory::MemFsDirectory(const fs::path& filepath, std::shared_ptr<char[]> key,const std::unordered_set<fs::path>& ignoreSet,bool ignore) 
    : MemFsDirEntry(filepath,key,ignore) {

    if(!fs::exists(filepath)){
        fs::create_directories(filepath);
    }    
    for(auto& entry : fs::directory_iterator(filepath)){
        bool childIgnored = ignore || ignoreSet.count(entry.path()) > 0;
        if(entry.is_directory()){
            auto dir = std::make_unique<MemFsDirectory>(entry.path(),key,ignoreSet,childIgnored);    
            entries.push_back(std::move(dir));
        }
        else{
            auto file = std::make_unique<MemFsFile>(entry.path(),key,false);
            entries.push_back(std::move(file));
        }
    }
}

void MemFsDirectory::save() {
    /*
        Runs under assumption that the current directory has already been created 
    
    */
    std::vector<fs::path> new_dirs, rm_dirs;
    for(auto& mem_entry: entries){
        bool match = false;
        for(auto& fs_entry: fs::directory_iterator(filepath)){
            if(mem_entry->path() == fs_entry)
                match = true;
        }
        if(!match && mem_entry->is_directory()){
            new_dirs.push_back(mem_entry->path());
        }
    }
    for(auto& fs_entry: fs::directory_iterator(filepath)){
        bool match = true;
        for(auto& mem_entry: entries){
            if(mem_entry->path() == fs_entry)
                match = false;
        }
        if(!match && fs_entry.is_directory()){
            rm_dirs.push_back(fs_entry);
        }
    }
    for(auto& entry: new_dirs){
        fs::create_directories(entry);
    }
    for(auto& entry: rm_dirs){
        fs::remove(entry);
    }
    for(auto& entry: entries){
        entry->save();
    }
}


void MemFsDirectory::load(bool firstTime) {
    for(auto& entry: entries){
        entry->load(firstTime);
    }
}


MemFsDirEntry* MemFsDirectory::find(fs::path fp){
    fs::path curr_path = fp; 
    while(curr_path.has_parent_path()){
        if(curr_path.parent_path()==this->filepath){
            break;
        }
    }
    if(curr_path.parent_path()!=this->filepath){
        return nullptr; 
    }

    for(auto& entry: entries){
        if(entry->path() == fp)
            return entry.get();
        if(entry->path() == curr_path){ 
            return entry->find(fp);
        }
    }
    return nullptr;
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


/*

    MemFsFile


*/


MemFsFile::MemFsFile(const fs::path& filepath, std::shared_ptr<char []> key,bool ignore) 
    : MemFsDirEntry(filepath, key, ignore) {
    if(!fs::exists(filepath)){
        std::ifstream file(filepath.string());
        file.close();
    }
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

void MemFsFile::load(bool firstTime){
    std::ifstream file(filepath.generic_string());
    if(!file.is_open()){
        std::cerr << "Failed to open file "<<filepath<<std::endl;

        return; 
    }

    //crypto cr();
    std::string line; 
    this->encrypt_text = "";
    while(std::getline(file,line)){
        std::string encrypt_line = line;
        std::string decrypt_line = line; 
        if(!firstTime){
            decrypt_line = crypto::decryptLine(line,(const unsigned char*)key.get());
        }
        else{
            encrypt_line = crypto::encryptLine(line, (const unsigned char *)key.get());
        }
        //TODO: include line by line decryption code here

        this->plain_text+= decrypt_line+"\n";
        this->encrypt_text+= encrypt_line+"\n";
    }

    //kinda dumb way to make sure extra \n isn't included
    this->plain_text = this->plain_text.substr(0,this->plain_text.size()-1);
    this->encrypt_text = this->encrypt_text.substr(0,this->encrypt_text.size()-1); 

    calculate_offset(this->plain_offset, this->plain_text);
    calculate_offset(this->encrypt_offset, this->encrypt_text);
    file.close();
}

MemFsDirEntry* MemFsFile::find(fs::path filep){return this;}

std::string MemFsFile::readFile(size_t line_num,size_t window_size) {
    assert(this->encrypt_offset.size()==this->plain_offset.size());
    std::stringstream out_string;
    
    for(size_t i = 0; i<plain_offset.size(); i++){
        
        //Decides if we will write out encrypted or decrypted line(based on line num and window size)
        std::vector<off_t> curr_offset = (i>=line_num-window_size && i<=line_num+window_size) ? plain_offset : encrypt_offset;
        std::string curr_string = (i>=line_num-window_size && i<=line_num+window_size) ? plain_text : encrypt_text;
        
        //writes entire string to stringstream if its the last line(no \n)
        if(curr_offset.size()-1==i){
            size_t idx = curr_offset.at(i);
            out_string << curr_string.substr(idx);
            continue;
        }
        
        size_t idx = curr_offset.at(i);
        size_t next_idx = curr_offset.at(i+1);
        
        //writes string + endl to the stringstream
        std::string sub_string = curr_string.substr(idx, next_idx-idx);
        out_string << sub_string << std::endl;
    
    }
    //returns string from stringstream
    return out_string.str();
}

void MemFsFile::writeLine(size_t line_num, std::string new_line) {
    //If ignored writes the same new_line, if not ignored encrypts the line
    std::string encrypted_line = (!ignored) ? crypto::encryptLine(new_line, (const unsigned char *)(key.get())) : new_line;
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