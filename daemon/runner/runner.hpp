#include "../crypto/crypto.cpp"


/*

    1. Need to check if a .cblocker file exists within current directory 
    2. Need to make .cblocker file if it doesn't exist 
    3. Encrypt all of the files that are not in .cblockerignore
    4. Need to make make a file in .cblocker that holds our generated key
    5. Need to modify constructor of crypto to check and read key from the .cblocker file
    6. Need to make a cblocker method where we can run files from memory 
    

*/
namespace fs = std::filesystem;


class Project{
    private: 
        unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
        std::string name;
        fs::path projectPath;
        fs::path dotFolderPath; 



    public:
        Project(const fs::path& filepath);
        
    private: 
        void makeDotFolder();
        void loadDotFolder();
};


class Runner{

    private: 
        

    
    public: 




};