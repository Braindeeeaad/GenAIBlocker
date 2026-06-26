#include "memfs.hpp"
#include <filesystem>
#include <memory>

MemFsDirectory::MemFsDirectory(const fs::path& filepath) 
    : filepath(filepath) {
    for(auto& entry : fs::directory_iterator(filepath)){
        if(entry.is_directory()){
            auto dir = std::make_unique<MemFsDirectory>(entry.path());    
            entries.push_back(std::move(dir));
        }
        else{
            auto file = std::make_unique<MemFsFile>(entry.path());
            entries.push_back(std::move(file));
        }
    }
}

void MemFsDirectory::save() {

}
void MemFsDirectory::load() {
    for(auto& entry: entries){
        entry->load();
    }
}




void MemFsDirectory::addEntry(std::unique_ptr<MemFsDirEntry> entry) {
    entries.push_back(std::move(entry));
}

void MemFsDirectory::deleteEntry(const fs::path& filepath) {

}

MemFsFile::MemFsFile(const fs::path& filepath) 
    : filepath(filepath) {

}

void MemFsFile::save() {

}

void MemFsFile::load(){
    
}

void MemFsFile::readFile() {

}

void MemFsFile::writeLine() {

}