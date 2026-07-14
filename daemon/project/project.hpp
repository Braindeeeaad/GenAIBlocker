#pragma once
#include "../crypto/crypto.hpp"
#include "../memfs/memfs.hpp"
#include <filesystem>
#include <iostream> 
#include <fstream>
#include <memory>
#include <vector>
#include <unordered_set>

/*

    1.X Need to check if a .cblocker file exists within current directory 
    2.X Need to make .cblocker file if it doesn't exist 
    3.X Encrypt all of the files that are not in .cblockerignore
       Do I need a way to check if files/project has been already encrypted 
       I don't think a simple .cblocker check would work
    4.X Need to make make a file in .cblocker that holds our generated key
    5.X Need to modify constructor of crypto to check and read key from the .cblocker file
    6.X Need to make a cblocker method where we can run files from memory 
    

*/
namespace fs = std::filesystem;


class Project{
    private: 
        
        std::string name;
        fs::path projectPath;
        fs::path dotFolderPath; 
        std::shared_ptr<char[]> sharedKey;

        std::unordered_set<fs::path> ignoredFiles;
        std::unique_ptr<MemFsDirectory> project; 



    public:
        Project(const fs::path& filepath);
        void init();

        std::string readFile(fs::path& fp,size_t line_num, size_t window);
        bool writeLine(fs::path& fp, size_t line_num,std::string new_line);
        bool deleteFile(fs::path& fp);
        bool addFile(fs::path& fp,bool is_dir);        

    private: 
        
        void readIgnoreFile();
        void makeDotFolder();
        void loadDotFolder();


        
};
