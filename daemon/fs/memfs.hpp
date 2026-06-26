#include <string> 
#include <vector>
#include <memory>
#include <filesystem>



namespace fs = std::filesystem;


/*
    TODO LIST: 

    1. Make constructors for MemFsDirectory and MemFsFile
       the constructor for the MemFsDirectory needs to recursively initalize 
       all of the dir_entries under the actual file system. The constructor for the MemFsFile 
       should open the current file and copy its contents into memory via string 
    
    2. Make a readfile command for MemFsFile which can provide a hybrid string view based off of the 
       window size and line num provided to as arguments 

    3. Make a write line function that when given a line will modify the proper line of code in plain_text  
       and changes the respective lines in both plain_text and encrypt_text 

    4. Make a save function that rewrites the current directory/file back into its correct file.

    5. Make a deleteEntry function thats a linear search through the dir_entries and deletes the matching file

*/


class MemFsDirEntry {
public:
    virtual void save() = 0;
    virtual void load() = 0; 
    virtual ~MemFsDirEntry() = default;
};

class MemFsDirectory : public MemFsDirEntry {
private:
    fs::path filepath;
    std::vector<std::unique_ptr<MemFsDirEntry>> entries;
public:
    MemFsDirectory(const fs::path& filepath);
    void save() override;
    void load() override; 

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
    MemFsFile(const fs::path& filepath);
    
    void save() override;
    void load() override; 
    
    void readFile();
    void writeLine();
};