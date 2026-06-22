#include "network_channel.h"
#include <signal.h>
#include <stdio.h>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char *argv[]){
    if(argc < 3){
        std::cerr << "Usage:\n" 
                  << " cblocker encrypt <file>\n" 
                  << " cblocker decrypt <file>\n"
                  << " cblocker decrypt_window <file> <line_num> <window_size>\n";
        return 1;
    }

    NetworkRequestChannel channel("127.0.0.1", 12345, NetworkRequestChannel::CLIENT_SIDE);
    int i = 1;
    
    while(i < argc){
        std::string command = argv[i]; 
        
        // Ensure there is at least a filepath argument following the command
        if (i + 1 >= argc) {
            std::cerr << "Error: Missing filepath for command " << command << "\n";
            return 1;
        }
        
        std::string filepath = argv[i+1];    
        
        if(!fs::exists(filepath)){
            std::cerr << "Error: file not found " << filepath << "\n";
            return 1;
        }
        fs::path full_path = fs::absolute(filepath);

        if(command == "encrypt"){
            std::cout << "Command: encrypt, Filepath: " << filepath << std::endl;
            Request req("encrypt", full_path.string(), 0, 0);
            Response resp = channel.send_request(req);
            std::cout << "Server Response: " << resp.message << std::endl;
            i += 2; // Move past command and filepath
        }
        else if(command == "decrypt"){
            std::cout << "Command: decrypt, Filepath: " << filepath << std::endl;
            Request req("decrypt", full_path.string(), 0, 0);
            Response resp = channel.send_request(req);
            std::cout << "Server Response: " << resp.message << std::endl;
            i += 2; // Move past command and filepath
        }
        else if(command == "decrypt_window"){
            // Verify we have all 4 required arguments for this command
            if (i + 3 >= argc) {
                std::cerr << "Error: decrypt_window requires <file> <line_num> <window_size>\n";
                return 1;
            }
            
            size_t line_num = std::stoull(argv[i+2]);
            size_t window_size = std::stoull(argv[i+3]);
            
            std::cout << "Command: decrypt_window, Filepath: " << filepath 
                      << ", Line: " << line_num << ", Window Size: " << window_size << std::endl;
            
            Request req("decrypt_window", full_path.string(), line_num, window_size);
            Response resp = channel.send_request(req);
            
            if (resp.success) {
                std::cout << "\n--- Decrypted Window Output ---\n";
                std::cout << resp.decrypted_window;
                std::cout << "-------------------------------\n";
            } else {
                std::cerr << "Server Error: " << resp.message << std::endl;
            }
            
            i += 4; // Move past command, filepath, line_num, and window_size
        }
        else {
            std::cerr << "Error: Unknown command " << command << "\n";
            return 1;
        }
    }

    return 0;
}