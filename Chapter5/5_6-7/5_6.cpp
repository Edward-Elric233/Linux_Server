// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/7/13
// Description: Client
#include "utils.h"
#include "wrap.h"
#include <cstring>
#include <string>

using namespace std;
using namespace C_std::Network;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s ip port\n", basename(argv[0]));
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);

    int cfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    Connect(cfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));

    string msg1 = "Hello Server";
    string msg2 = "123";
    string msg3 = "I have send a urgent message";
    send(cfd, msg1.c_str(), msg1.length(), 0);
    send(cfd, msg2.c_str(), msg2.length(), MSG_OOB);
    send(cfd, msg3.c_str(), msg3.length(), 0);

    shutdown(cfd, SHUT_RDWR);
    return 0;
}
