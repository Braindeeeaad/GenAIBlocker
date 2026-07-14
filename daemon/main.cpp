#include "project/project.hpp"
#include "include/daemon.hpp"
#include "network/network_channel.hpp"
#include <exception>
#include <filesystem>
#include <unordered_map>
#include <utility>

namespace fs = std::filesystem;




/*
    What do we need to do? 
    
    - need to setup network channel in main after daeminization
    - Need to figure out terminating condition of the main loop
    - Need to provide signal handlers to properly clean up reasources 
    


*/
fs::path find_project_dir(fs::path& filepath){
    fs::path curr_path = filepath;
    std::cout<<"Trying to find project dir"<<std::endl<<std::flush; 
    while(curr_path!=fs::path("/")){
        fs::path cblocker_path = curr_path / ".cblocker";
        if(fs::exists(cblocker_path))
            return curr_path;    
        curr_path = curr_path.parent_path();
    }
    
    return filepath;
}

int main(int argc, char *argv[]) {
    crypto cr;

    if((argc-1)%2!=0){
        std::cerr << "Missing commands" << std::endl;
        return 1;
    }

    daemonpp::daemon dm("cblocker");
    dm.daemonize();


    NetworkRequestChannel listener("", 12345, NetworkRequestChannel::SERVER_SIDE);
    
    std::unordered_map<fs::path, Project*> project_registry;
    std::cout<<"Project registry made"<<std::endl;

    while(true) {
        int client_fd = listener.accept_connection();
        
        NetworkRequestChannel channel(client_fd);
        
        
        Request req = channel.receive_request();
        const std::string command = req.command;
        fs::path filepath = fs::path(req.filepath);
        
        fs::path project_dir = find_project_dir(filepath);

        Response resp(false, "", "Unknown Command");

        //Okay so the project stays in memory for entire daemon process, kinda cooked 
        //Need some way to close it, i.e make a method to save and clear the proj from mem 
        
        Project* curr_project = nullptr;
        auto it = project_registry.find(filepath);
        if(it != project_registry.end()){
            curr_project = it->second;
            std::cout<<"Project found"<<std::endl;
        }
        
        std::cout << "Start of command check:"<<command<<std::endl;
        if(command == "init") {
            try{
                if(curr_project)
                    resp = Response(false, "Fail", "Project already initalized");
                else{
                    std::cout<<"Making Project"<<std::endl;
                    curr_project = new Project(project_dir);
                    project_registry.insert(std::pair<fs::path, Project*>(project_dir,curr_project));
                    std::cout<<"Project Made"<<std::endl;
                    resp = Response(true,"Success", "Project initalized");
                }  
            } catch(const std::exception& e){
                 resp = Response(false, "Error initalizing project:", e.what());
            } 
        }
        else if(!curr_project){
            resp = Response(false, "Fail", "Please initalize project");
        }
        else if(command == "mkdir") {
            try {
                curr_project->addFile(filepath, true);
                //std::string decrypted_text = cr.decrypt_window(filepath, req.line, req.window_size);
                resp = Response(true, "Success", "Directory added");
            } catch (const std::exception& e) {
                resp = Response(false, "Error making directory:", e.what());
            }
        }
        else if(command == "mkfile") {
            try {
                curr_project->addFile(filepath, false);
                //std::string decrypted_text = cr.decrypt_window(filepath, req.line, req.window_size);
                resp = Response(true, "Success", "File added");
            } catch (const std::exception& e) {
                resp = Response(false, "Error making file:", e.what());
            }
        }
        else if(command == "read"){
            try{
                std::string outstr = curr_project->readFile(filepath, req.line, req.window_size);
                resp = Response(true,"Success","File read",outstr);
            }catch(const std::exception& e){
                resp = Response(false,"Error reading file:",e.what());
            }
        }
        else if(command == "write_line"){
            try{
                bool is_file = curr_project->writeLine(filepath, req.line, req.new_line);
                if(!is_file)
                    resp = Response(false,"Fail","can't write to directory");
                else
                    resp = Response(true,"Success","Line written");
            }catch(const std::exception& e){
                resp = Response(false,"Error writing line",e.what());
            }
        }
        channel.send_response(resp);
    
    }

    //deletes all projects in mem registry
    for(auto pair: project_registry){
        delete pair.second;
    }

    return 0;
}