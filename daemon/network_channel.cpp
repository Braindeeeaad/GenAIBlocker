#include "network_channel.h"
#include <cstdint>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <iterator>

using namespace std;

// Helper function to handle short-writes on socket sends safely
static bool send_all(int fd, const void* data, size_t len) {
    const char* ptr = static_cast<const char*>(data);
    while (len > 0) {
        ssize_t sent = send(fd, ptr, len, 0);
        if (sent < 0) return false;
        ptr += sent;
        len -= sent;
    }
    return true;
}

NetworkRequestChannel::NetworkRequestChannel(const std::string& ip, int port, Side side) 
    : my_side(side), client_addr_len(sizeof(client_addr)) {
    
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    
    if (side == SERVER_SIDE) {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            cerr << "Error making server-side socket" << endl;
            exit(1);
        }
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            cerr << "Error setting socket options" << endl;
            exit(1);
        }

        if (bind(sockfd, (const sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            cerr << "Error binding server-side fd " << endl;
            exit(1);
        }
        if (listen(sockfd, 10) < 0) {
            cerr << "Error while listening" << endl;
            exit(1);
        }
        peer_ip = "0.0.0.0";
        peer_port = port;
        cout << "Server listening on port " << port << endl;
    } else {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        string resolved_ip = ip;
        if (ip == "localhost") resolved_ip = "127.0.0.1";
            
        int result = inet_pton(AF_INET, resolved_ip.c_str(), &server_addr.sin_addr);
        if (result <= 0) {
            cerr << "Could not resolve address" << endl;
            exit(1);
        }
        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            cerr << "Error making client-side socket" << endl;
            exit(1);
        }
            
        if (connect(sockfd, (const sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Error connecting " << endl;
            exit(1);
        }
        peer_ip = ip;
        peer_port = port;
        cout << "Connected to server at " << ip << ":" << port << endl;
    }
}

NetworkRequestChannel::NetworkRequestChannel(int fd) 
    : my_side(SERVER_SIDE), sockfd(fd), client_addr_len(sizeof(client_addr)) {
    if (getpeername(sockfd, (struct sockaddr*)&client_addr, &client_addr_len) < 0) {
        cerr << "Error getting peer name" << endl;
        exit(1);
    }
    peer_port = ntohs(client_addr.sin_port);
    peer_ip = inet_ntoa(client_addr.sin_addr);
}

NetworkRequestChannel::~NetworkRequestChannel() {
    close(sockfd);
}

int NetworkRequestChannel::accept_connection() {
    client_addr_len = sizeof(client_addr); // Reset size explicitly
    int fd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len); 
    if (fd < 0) {
        cerr << "Error accepting connection" << endl;
        exit(1); 
    }
    cout << "Accepted connection from " 
         << inet_ntoa(client_addr.sin_addr) << ":" 
         << ntohs(client_addr.sin_port) << endl;
    
    return fd; 
}

string NetworkRequestChannel::get_peer_address() const {
    return peer_ip + ":" + to_string(peer_port);
}

int NetworkRequestChannel::get_socket_fd() const {
    return sockfd;
}

Response NetworkRequestChannel::send_request(const Request& req) {
    stringstream ss;
    ss << req.command << "|"
       << req.filepath << "|" 
       << req.line << "|" 
       << req.window_size;
    
    string request_str = ss.str();
    
    uint32_t length = htonl(request_str.length());
    char length_buf[4];
    memcpy(length_buf, &length, 4);

    if (!send_all(sockfd, length_buf, 4)) {
        cerr << "Failed to send request header" << endl;
        exit(1); 
    }
    if (!send_all(sockfd, request_str.c_str(), request_str.length())) {
        cerr << "Failed to send request" << endl;
        exit(1);
    }

    char buf[4]; 
    int buf_len = 4;
    int num_chars = 0; 
    while (num_chars < 4) {
        int n = recv(sockfd, &buf[num_chars], buf_len, 0);
        if (n < 0) { cerr << "Error receiving header" << endl; exit(1); }
        if (n == 0) { cerr << "Error receiving header: connection closed" << endl; exit(1); }
        num_chars += n;
        buf_len -= n;
    }

    uint32_t net_length; 
    memcpy(&net_length, buf, 4);
    int mssg_length = ntohl(net_length);

    num_chars = 0;
    buf_len = mssg_length;
    vector<char> recv_buf(buf_len); 
    while (num_chars < mssg_length) {
        int n = recv(sockfd, &recv_buf[num_chars], buf_len, 0);
        if (n < 0) { cerr << "Error receiving body" << endl; exit(1); }
        if (n == 0) { cerr << "Error receiving body: connection closed" << endl; exit(1); }
        num_chars += n;
        buf_len -= n;
    }

    string received_mssg(recv_buf.begin(), recv_buf.end());
    stringstream sr(received_mssg);
    
    Response res(false, "", "", "");
    string token;
    
    getline(sr, token, '|');
    res.success = (token == "1");
    
    getline(sr, token, '|');
    res.data = token;
    
    getline(sr, token, '|');
    res.message = token;
    
    // SAFE PARSE: Grabs ALL remaining multi-line buffer strings entirely
    if (sr.peek() != EOF) {
        res.decrypted_window = string(istreambuf_iterator<char>(sr), {});
    }

    return res;
}

Request NetworkRequestChannel::receive_request() {
    char buf[4]; 
    int buf_len = 4;
    int num_chars = 0; 
    while (num_chars < 4) {
        int n = recv(sockfd, &buf[num_chars], buf_len, 0);
        if (n < 0) { cerr << "Error receiving header" << endl; exit(1); }
        if (n == 0) { cerr << "Error receiving header: connection closed" << endl; exit(1); }
        num_chars += n;
        buf_len -= n;
    }

    uint32_t net_length; 
    memcpy(&net_length, buf, 4);
    int mssg_length = ntohl(net_length);

    num_chars = 0;
    buf_len = mssg_length;
    vector<char> recv_buf(buf_len); 
    while (num_chars < mssg_length) {
        int n = recv(sockfd, &recv_buf[num_chars], buf_len, 0);
        if (n < 0) { cerr << "Error receiving body" << endl; exit(1); }
        if (n == 0) { cerr << "Error receiving body: connection closed" << endl; exit(1); }
        num_chars += n;
        buf_len -= n;
    }

    string received_mssg(recv_buf.begin(), recv_buf.end());
    stringstream sr(received_mssg);
    
    Request req("", "", 0, 0);
    string token;
    getline(sr, token, '|');
    req.command = token;
    getline(sr, token, '|');
    req.filepath = token;
    getline(sr, token, '|');
    req.line = stoi(token);
    getline(sr, token, '|');
    req.window_size = stoi(token);

    return req;
}

void NetworkRequestChannel::send_response(const Response& resp) {
    stringstream ss;
    ss << (resp.success ? "1" : "0") << "|"
       << resp.data << "|"
       << resp.message << "|";
       
    if (!resp.decrypted_window.empty()) {
        ss << resp.decrypted_window;
    }
    
    string response_str = ss.str();
    
    char length_buf[4];
    uint32_t response_len = htonl(response_str.length());
    memcpy(&length_buf, &response_len, 4);

    if (!send_all(sockfd, length_buf, 4)) {
        cerr << "Failed to send response header" << endl;
        exit(1); 
    }
    if (!send_all(sockfd, response_str.c_str(), response_str.length())) {
        cerr << "Failed to send response" << endl;
        exit(1);
    }
}