///Get IPC working(shadow file & sockets)
#include <asm-generic/socket.h>
#include <cstdint>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <vector>

using namespace std;
