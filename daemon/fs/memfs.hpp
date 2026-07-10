#pragma once
#include <string> 
#include <vector>
#include <memory>
#include <filesystem>


namespace fs = std::filesystem;


/*
    TODO LIST: 

    
    2. Make a readfile command for MemFsFile which can provide a hybrid string view based off of the 
       window size and line num provided to as arguments 

    3. Make a write line function that when given a line will modify the proper line of code in plain_text  
       and changes the respective lines in both plain_text and encrypt_text 

    4. Make a save function that rewrites the current directory/file back into its correct file.

*/


class MemFsDirEntry {
private: 
    std::filesystem::path filepath;
protected: 
    std::shared_ptr<char[]> key;

public:
    MemFsDirEntry(const std::filesystem::path& path,std::shared_ptr<char[]> key) : filepath(path), key(key) {}

    std::filesystem::path path() const {return filepath;}

    virtual void save() = 0;
    virtual void load() = 0; 
    virtual bool is_directory() = 0;
    virtual bool find(fs::path filepath);

    virtual ~MemFsDirEntry() = default;
};

class MemFsDirectory : public MemFsDirEntry {
private:
    fs::path filepath;
    std::vector<std::unique_ptr<MemFsDirEntry>> entries;
public:
    MemFsDirectory(const fs::path& filepath,std::shared_ptr<char[]> key);
    void save() override;
    void load() override; 
    bool is_directory() override; 
    bool find(fs::path filepath) override;
    
    void addEntry(std::unique_ptr<MemFsDirEntry> entry);
    void deleteEntry(const fs::path& filepath);
};

class MemFsFile : public MemFsDirEntry {
private:
    fs::path filepath;
    std::string encrypt_text;
    std::string plain_text;
    std::vector<off_t> plain_offset;
    std::vector<off_t> encrypt_offset;
public:
    MemFsFile(const fs::path& filepath, std::shared_ptr<char[]> key);
    
    void save() override;
    void load() override; 
    bool is_directory() override;
    bool find(fs::path filepath) override;
    


    std::string readFile(size_t line,size_t window_size=0);
    void writeLine(size_t line_num, std::string new_line);
};