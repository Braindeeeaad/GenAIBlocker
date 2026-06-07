#include "network_channel.h"
#include <asm-generic/socket.h>
#include <cstdint>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>

using namespace std;


/**
 * Creates a NetworkRequestChannel
 * 
 * @param ip IP address to connect to (client) or interface to bind to (server)
 *           Empty string for server side means bind to all interfaces
 * @param port Port number to use
 * @param side SERVER_SIDE to create a listening socket, CLIENT_SIDE to connect to a server
 * 
 * SERVER_SIDE behavior:
 * - Creates a socket and configures it for listening on the specified port
 * - Sets socket options to allow address reuse
 * 
 * CLIENT_SIDE behavior:
 * - Creates a socket and connects to the specified server address
 * - If the ip parameter is not a valid IP address, attempts to resolve it as a hostname
 * 
 * @throws Exits with error message if socket operations fail
 */

// Constructor for setting up a connection (server listening or client connecting)
NetworkRequestChannel::NetworkRequestChannel(const std::string& ip, int port, Side side) 
    : my_side(side), client_addr_len(sizeof(client_addr)) {
    
    // Initialize address structures to zero
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    
    if (side == SERVER_SIDE) {
        // TODO: Implement server-side socket creation and setup
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        sockfd = socket(PF_INET,SOCK_STREAM,0);
        if(sockfd<0){
            cerr<<"Error making server-side socket"<<std::endl;
            exit(1);
        }
        int opt =1;
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))<0){
            cerr<<"Error setting socket options"<< std::endl;
            exit(1);
        }

        if(bind(sockfd, (const sockaddr *)&server_addr, sizeof(server_addr))==-1){
            cerr<<"Error binding server-side fd "<<std::endl;
            exit(1);
        }
        if(listen(sockfd,10)<0){
            cerr<<"Error while listening"<<std::endl;
            exit(1);
        }
        // Set peer information for logging purposes
        peer_ip = "0.0.0.0";
        peer_port = port;
        cout << "Server listening on port " << port << endl;
    } else {
        // TODO: Implement client-side socket creation and connection
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        string resolved_ip = ip;
        if(ip == "localhost") resolved_ip = "127.0.0.1";
            
        int result = inet_pton(AF_INET, resolved_ip.c_str(), &server_addr.sin_addr);
        if(result <= 0){
            cerr << "Could not resolve address" << endl;
            exit(1);
        }
        sockfd = socket(PF_INET,SOCK_STREAM,0);
        if(sockfd<0){
            cerr<<"Error making client-side socket"<<std::endl;
            exit(1);
        }
            
        if(connect(sockfd,(const sockaddr * )&server_addr,sizeof(server_addr))<0){
            cerr<<"Error connecting "<<std::endl;
            exit(1);
        }
        // Store peer information for logging
        peer_ip = ip;
        peer_port = port;
        cout << "Connected to server at " << ip << ":" << port << endl;
    }
}

/**
 * Constructor for client connections accepted by a server
 * 
 * @param fd Socket file descriptor returned by accept()
 * 
 * This constructor initializes a NetworkRequestChannel using an existing
 * socket connection that was established by accepting a client connection.
 */
NetworkRequestChannel::NetworkRequestChannel(int fd) 
    : my_side(SERVER_SIDE), sockfd(fd), client_addr_len(sizeof(client_addr)) {
    
    // TODO: Implement this constructor function
    if(getpeername(sockfd , (struct sockaddr*)&client_addr, &client_addr_len)<0){
        cerr<<"Error getting peer name"<<std::endl;
        exit(1);
    }
    // Store the IP address and port in peer_ip and peer_port
    peer_port = ntohs(client_addr.sin_port);
    peer_ip = inet_ntoa(client_addr.sin_addr);
}

/**
 * Destructor
 * 
 * Cleans up resources used by this NetworkRequestChannel
 */
NetworkRequestChannel::~NetworkRequestChannel() {
    // TODO: Implement the destructor
    close(sockfd);
}

/**
 * Accepts a new client connection on a server socket
 */
int NetworkRequestChannel::accept_connection() {
    // TODO: Accept a new client connection
    int fd = accept(sockfd,(struct sockaddr*)&client_addr,&client_addr_len); 
    if(fd<0){
        cerr<<"Error accepting connection" << endl;
        exit(1); 
    }
    // Print connection information for logging purposes
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

/**
 * Sends a request to the server and waits for a response
 * 
 * @param req The Request object to send
 * @return Response from the server
 * 
 * This method:
 * Sends the length of the request followed by the request itself and receives the responses.
 * 
 * The wire format uses a 4-byte length header followed by the serialized data.
 * Request format: CMMND|FILEPATH|LINE|WINDOW_SIZE
 * Response format: SUCCESS|DATA|MESSAGE
 * 
 * @throws May throw exceptions on network errors
 */
Response NetworkRequestChannel::send_request(const Request& req) {
    // Format: TYPE|USER_ID|AMOUNT|FILENAME|DATA
    
    stringstream ss;

    ss << req.command << "|"
       << req.filepath << "|" 
       << req.line << "|" 
       << req.window_size;
    
    string request_str = ss.str();
    
    // Add message length as header (4 bytes)
    // Convert the request string length to network byte order
    uint32_t length = htonl(request_str.length());
    char length_buf[4];
    memcpy(length_buf, &length, 4);

    // TODO : Implement the send_request function
    //sending the request
    if(send(sockfd, length_buf,4,0)<0){
        cerr<<"Failed to send request header"<<std::endl;
        exit(1); 
    }
    if(send(sockfd,request_str.c_str(),request_str.length(),0)<0){
        cerr<<"Failed to send request"<<std::endl;
        exit(1);
    }

    //gathering header info
    char buf[4]; 
    int buf_len = 4;
    int num_chars = 0; 
    while(num_chars<4){
        int n = recv(sockfd,&buf[num_chars],buf_len,0);
        if(n<0){
            cerr << "Error recieving header"<<std::endl;
            exit(1);
        }
        if(n==0){
            cerr << "Error recieving header:connection closed"<<std::endl;
            exit(1);
        }
        num_chars+=n;
        buf_len-=n;
    }

    //recieveing message
    uint32_t net_length; 
    memcpy(&net_length,buf,4);
    int mssg_length = ntohl(net_length);

    num_chars = 0;
    buf_len = mssg_length;
    vector<char> recv_buf(buf_len); 
    while(num_chars<mssg_length){
        int n = recv(sockfd,&recv_buf[num_chars],buf_len,0);
        if(n<0){
            cerr << "Error recieving header"<<std::endl;
            exit(1);
        }
        if(n==0){
            cerr << "Error recieving header:connection closed"<<std::endl;
            exit(1);
        }
        num_chars+=n;
        buf_len-=n;
    }

    //Parsing response string
    string received_mssg(recv_buf.begin(),recv_buf.end());
    stringstream sr(received_mssg);
    
    Response res = Response(false,"","Not Implemented");
    string token;
    getline(sr,token,'|');
    res.success = (token=="1");
    getline(sr,token,'|');
    res.data = token;
    getline(sr,token,'|');
    res.message = token;

    


    // Returning a dummy Response for now (Fix this)
    return res;
}
    

/**
 * Receives a request from a client
 * 
 * @return The received Request object
 * 
 * This method:
 * Receives the length of the incoming request (4-byte header) and the actual data
 * 
 */
Request NetworkRequestChannel::receive_request() {
    // TODO: Implement the receive_request function
    
    //Catching header
    char buf[4]; 
    int buf_len = 4;
    int num_chars = 0; 
    while(num_chars<4){
        int n = recv(sockfd,&buf[num_chars],buf_len,0);
        if(n<0){
            cerr << "Error recieving header"<<std::endl;
            exit(1);
        }
        if(n==0){
            cerr << "Error recieving header:connection closed"<<std::endl;
            exit(1);
        }
        num_chars+=n;
        buf_len-=n;
    }

    //recieveing message
    uint32_t net_length; 
    memcpy(&net_length,buf,4);
    int mssg_length = ntohl(net_length);

    num_chars = 0;
    buf_len = mssg_length;
    vector<char> recv_buf(buf_len); 
    while(num_chars<mssg_length){
        int n = recv(sockfd,&recv_buf[num_chars],buf_len,0);
        if(n<0){
            cerr << "Error recieving header"<<std::endl;
            exit(1);
        }
        if(n==0){
            cerr << "Error recieving header:connection closed"<<std::endl;
            exit(1);
        }
        num_chars+=n;
        buf_len-=n;
    }

    //Parsing request string
    string received_mssg(recv_buf.begin(),recv_buf.end());
    stringstream sr(received_mssg);
    
    Request req = Request("","",0,0);
    string token;
    getline(sr,token,'|');
    req.command = (token=="1");
    getline(sr,token,'|');
    req.filepath = token;
    getline(sr,token,'|');
    req.line = stoi(token);
    getline(sr,token,'|');
    req.window_size = stoi(token);


    


    return req;
}

/**
 * Sends a response to a client
 * 
 * @param resp The Response object to send
 * 
 * This method:
 * Sends the length of the response followed by the response itself
 * 
 * The wire format uses a 4-byte length header followed by the serialized data.
 * Response format: SUCCESS|BALANCE|DATA|MESSAGE
 */
void NetworkRequestChannel::send_response(const Response& resp) {
    stringstream ss;
    ss << (resp.success ? "1" : "0") << "|"
       << resp.data << "|"
       << resp.message;
    
    string response_str = ss.str();
    
    // TODO: Implement the send_response function
    char length_buf[4];
    uint32_t response_len = htonl(response_str.length());
    memcpy(&length_buf, &response_len, 4);

    if(send(sockfd, length_buf,4,0)<0){
        cerr<<"Failed to send response header"<<std::endl;
        exit(1); 
    }
    if(send(sockfd,response_str.c_str(),response_str.length(),0)<0){
        cerr<<"Failed to send response"<<std::endl;
        exit(1);
    }
}