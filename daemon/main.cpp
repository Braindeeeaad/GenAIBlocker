#include "crypto.cpp"
#include "include/daemon.hpp"
#include "network_channel.h"
#include <signal.h>

namespace fs = std::filesystem;




/*
    What do we need to do? 
    
    - need to setup network channel in main after daeminization
    - Need to figure out terminating condition of the main loop
    - Need to provide signal handlers to properly clean up reasources 
    


*/


int main(int argc, char *argv[]) {
    crypto cr;

    if((argc-1)%2!=0){
        std::cerr << "Missing commands" << std::endl;
        return 1;
    }

    daemonpp::daemon dm("cblocker");
    dm.daemonize();


    NetworkRequestChannel listener("", 12345, NetworkRequestChannel::SERVER_SIDE);

    while(true) {
        int client_fd = listener.accept_connection();
        
        NetworkRequestChannel channel(client_fd);
        
        
        Request req = channel.receive_request();
        std::string command = req.command;
        std::string filepath = req.filepath;
        
        Response resp(false, "", "Unknown Command");

        if(command == "encrypt") {
            cr.encrypt_directory(filepath);
            resp = Response(true, "Success", "Directory encrypted");
        }
        else if(command == "decrypt") {
            cr.decrypt_directory(filepath);
            resp = Response(true, "Success", "Directory decrypted");
        }
        else if(command == "decrypt_window") {
            try {
                std::string decrypted_text = cr.decrypt_window(filepath, req.line, req.window_size);
                resp = Response(true, "Success", "Window decrypted", decrypted_text);
            } catch (const std::exception& e) {
                resp = Response(false, "Error", e.what());
            }
        }
        
        channel.send_response(resp);
    
    }

    return 0;
}