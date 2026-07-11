#pragma once
#include "../crypto/crypto.hpp"
#include "../fs/memfs.hpp"
#include <filesystem>
#include <iostream> 
#include <fstream>
#include <memory>
#include <vector>
#include <unordered_set>

/*

    1.X Need to check if a .cblocker file exists within current directory 
    2.X Need to make .cblocker file if it doesn't exist 
    3. Encrypt all of the files that are not in .cblockerignore
       Do I need a way to check if files/project has been already encrypted 
       I don't think a simple .cblocker check would work
    4.X Need to make make a file in .cblocker that holds our generated key
    5.X Need to modify constructor of crypto to check and read key from the .cblocker file
    6.X Need to make a cblocker method where we can run files from memory 
    

*/
namespace fs = std::filesystem;


class Project{
    private: 
        unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
        std::string name;
        fs::path projectPath;
        fs::path dotFolderPath; 

        std::unordered_set<fs::path> ignoredFiles;
        std::unique_ptr<MemFsDirectory> project; 



    public:
        Project(const fs::path& filepath);
        
    private: 
        void init();
        void readIgnoreFile();
        void makeDotFolder();
        void loadDotFolder();
};
