//
// Created by edward on 2021/7/29.
//

#ifndef LINUX_SERVER_11_3_H
#define LINUX_SERVER_11_3_H

#include <ctime>
#include <functional>
#include <set>
#include <unordered_map>
#include <string>

namespace data_structure {
    class Timer;
    class ClientData {
    public:
        int sockfd;
        std::string ip;
        int port;
        std::set<Timer>::iterator timer;
        ClientData(int _sockfd, const std::string &_ip, int _port)
        : sockfd(_sockfd)
        , ip(_ip)
        , port(_port) {}
    };
    std::unordered_map<int, ClientData> clientMap;
    class Timer {
    public:
        std::time_t expire;
        std::function<void (int)> cb;
        ClientData *clientData;
        Timer(std::time_t _expire, std::function<void (int)>_cb, ClientData *_clientData)
        : expire(_expire)
        , cb(_cb)
        , clientData(_clientData) {}
    };
    bool operator < (const Timer &lhs, const Timer &rhs) {
        return lhs.expire < rhs.expire;
    }
    std::set<Timer> timerSet;
}

#endif //LINUX_SERVER_11_3_H
