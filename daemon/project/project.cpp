#include "project.hpp"
#include <cstring>
#include <filesystem>
#include <assert.h>
#include <fstream>
#include <memory>
#include <string>
//Constructor 

Project::Project(const fs::path& filepath){
    this->projectPath = filepath;
    this->dotFolderPath = projectPath / ".cblocker"; 
    this->name = projectPath.filename();
    this->sharedKey = std::shared_ptr<char[]>(new char[crypto_secretstream_xchacha20poly1305_KEYBYTES]);
    this->loadDotFolder();
    
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

std::string Project::readFile(fs::path& fp,size_t line_num, size_t window){
    MemFsDirEntry* fileEntry = this->project->find(fp);
    std::stringstream sout; 
    //Print list of children files if dir
    if(MemFsDirectory* dir = dynamic_cast<MemFsDirectory*>(fileEntry)){
        size_t idx = 0; 
        for(auto& entry : dir->entries){
            sout << entry.get();
            if(idx<dir->entries.size()-1){
                sout<<std::endl;
            }
            idx++;
        }
        return sout.str();
    }

    //print out entire file if file
    if(MemFsFile* file = dynamic_cast<MemFsFile*>(fileEntry)){
        sout << file->readFile(line_num,window);
    }
    return sout.str();
}


bool Project::writeLine(fs::path& fp,size_t line_num,std::string new_line){
    MemFsDirEntry* fileEntry = this->project->find(fp);
    std::stringstream sout; 
    //Print list of children files if dir
    if(MemFsDirectory* dir = dynamic_cast<MemFsDirectory*>(fileEntry)){
        return false;
    }

    //print out entire file if file
    if(MemFsFile* file = dynamic_cast<MemFsFile*>(fileEntry)){
        file->writeLine(line_num, new_line);
        return true;
    }
    return false;
}

bool Project::deleteFile(fs::path &fp){
    fs::path parentDir = fp.parent_path();
    MemFsDirEntry* folder = this->project->find(fp); 

    if(MemFsDirectory* dir = dynamic_cast<MemFsDirectory*>(folder)){
        dir->deleteEntry(fp);
        return true;
    }
    else
        return false;

}

bool Project::addFile(fs::path &fp, bool is_dir){
    fs::path parent_dir = fp.parent_path();
    MemFsDirEntry* folder = this->project->find(fp); 

    if(MemFsDirectory* dir = dynamic_cast<MemFsDirectory*>(folder)){
        std::unique_ptr<MemFsDirEntry> folder;
        bool is_ignored = ignoredFiles.count(fp) > 0;
        
        if(is_dir){
            folder = std::make_unique<MemFsDirectory>(fp,this->sharedKey,this->ignoredFiles,is_ignored); 
        }
        else
            folder = std::make_unique<MemFsFile>(fp,this->sharedKey,is_ignored);
        
        dir->addEntry(std::move(folder));
        return true;
    }
    else
        return false;

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
    crypto::generateKey((unsigned char *)sharedKey.get());
    //write key into file
    fs::path keyfile = dotFolderPath / "key";
    std::ofstream fout(keyfile.string());
    fout.write((char *)sharedKey.get(),crypto_secretstream_xchacha20poly1305_KEYBYTES);
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
    fin.read((char *)sharedKey.get(),crypto_secretstream_xchacha20poly1305_KEYBYTES);

    if(!fin){
        std::cerr<< "Error reading keyfile"<<std::endl;
    }

    fin.close();
    
}

