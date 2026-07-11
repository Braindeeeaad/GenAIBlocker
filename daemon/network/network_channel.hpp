#ifndef _NETWORK_CHANNEL_H_
#define _NETWORK_CHANNEL_H_

#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

class Request {
public: 
    std::string command;
    std::string filepath;
    size_t line; 
    size_t window_size; 

    Request(std::string cmnd, std::string fname, size_t line, size_t window_size)
        : command(cmnd), filepath(fname), line(line), window_size(window_size) {}
    static Request parseRequest(const std::string& buffer); 
};

class Response {
public: 
    bool success; 
    std::string message; 
    std::string data;
    std::string decrypted_window; // <-- Added to hold the decrypted file buffer

    // Updated constructor supporting the new string payload
    Response(bool s, std::string d, std::string mssg, std::string dec_win = "")
        : success(s), data(d), message(mssg), decrypted_window(dec_win) {}
};

class NetworkRequestChannel {
public:
    enum Side {SERVER_SIDE, CLIENT_SIDE};
    
    NetworkRequestChannel(const std::string& ip, int port, Side side);
    NetworkRequestChannel(int sockfd);
    ~NetworkRequestChannel();
    
    Response send_request(const Request& req);
    Request receive_request();
    void send_response(const Response& resp);
    
    int accept_connection(); 
    std::string get_peer_address() const;
    int get_socket_fd() const;
    
private:
    Side my_side;
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    std::string peer_ip;
    int peer_port;
};

#endif