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


    6. Need to turn cryptopp functions from a stream into another cryptographic alg

*/


class MemFsDirEntry {
private: 
    std::filesystem::path filepath;
public:
    MemFsDirEntry(const std::filesystem::path& path) : filepath(path) {}

    std::filesystem::path path() const {return filepath;}

    virtual void save() = 0;
    virtual void load() = 0; 
    virtual bool is_directory() = 0;
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
    bool is_directory() override; 

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
    bool is_directory() override;


    void readFile();
    void writeLine();
};