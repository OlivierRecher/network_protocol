#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sstream>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "crypto_utils.hpp"
#include "neighbor_table.hpp"
#include "routing_table.hpp"

NeighborTable neighbor_table;
RoutingTable routing_table;

const int PORT = 8888;
std::string ROUTER_ID;
std::string LOCAL_IP;
const std::string SHARED_KEY = "supersecretkey";

std::string detect_local_ip()
{
    struct ifaddrs *ifs, *ifa;
    getifaddrs(&ifs);
    for (ifa = ifs; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
        {
            std::string name(ifa->ifa_name);
            if (name != "lo")
            {
                char buf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
                          buf, sizeof(buf));
                freeifaddrs(ifs);
                return buf;
            }
        }
    }
    freeifaddrs(ifs);
    return "0.0.0.0";
}

std::string build_hello()
{
    std::string m = "HELLO from " + ROUTER_ID + " at " + LOCAL_IP + "\n";
    auto sig = compute_hmac(m, SHARED_KEY);
    return m + "--SIGNATURE " + sig;
}

void sender_hello()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    int be = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &be, sizeof(be));
    sockaddr_in a{AF_INET, htons(PORT), INADDR_BROADCAST};
    while (1)
    {
        sendto(s, build_hello().c_str(), build_hello().size(), 0,
               (sockaddr *)&a, sizeof(a));
        std::cout << "[HELLO] sent\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void sender_update()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    int be = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &be, sizeof(be));
    sockaddr_in a{AF_INET, htons(PORT), INADDR_BROADCAST};
    while (1)
    {
        std::ostringstream oss;
        oss << "UPDATE from " << ROUTER_ID << "\n";
        for (auto &p : routing_table.get_all_routes())
            oss << p.first << " " << p.second.cost << "\n";
        std::string m = oss.str();
        std::string sig = compute_hmac(m, SHARED_KEY);
        sendto(s, (m + "--SIGNATURE " + sig).c_str(),
               m.size() + sig.size() + 13,
               0, (sockaddr *)&a, sizeof(a));
        std::cout << "[UPDATE] sent\n";
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void receiver()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in a{AF_INET, htons(PORT), INADDR_ANY};
    bind(s, (sockaddr *)&a, sizeof(a));
    char buf[1024];
    while (1)
    {
        int n = recvfrom(s, buf, sizeof(buf) - 1, 0, NULL, NULL);
        if (n <= 0)
            continue;
        buf[n] = 0;
        std::string r(buf);
        auto pos = r.find("--SIGNATURE ");
        if (pos == std::string::npos)
            continue;
        std::string body = r.substr(0, pos);
        std::string sig = r.substr(pos + 12);
        if (!verify_hmac(body, SHARED_KEY, sig))
            continue;

        std::istringstream iss(body);
        std::string line;
        std::getline(iss, line);

        if (line.rfind("HELLO from ", 0) == 0)
        {
            auto at = line.find(" at ");
            std::string id = line.substr(11, at - 11);
            std::string ip = line.substr(at + 4);
            neighbor_table.update_neighbor(id, ip);
            std::cout << "[RECV HELLO] " << id << " @ " << ip << "\n";
        }
        else if (line.rfind("UPDATE from ", 0) == 0)
        {
            std::string id = line.substr(12);
            while (std::getline(iss, line) && !line.empty())
            {
                std::istringstream ls(line);
                std::string d;
                int c;
                ls >> d >> c;
                routing_table.update_route({d, id, c + 1});
            }
            std::cout << "[RECV UPDATE] " << id << "\n";
        }
    }
}

void cleaner()
{
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        neighbor_table.remove_stale_neighbors();
        neighbor_table.print_neighbors();
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: ./lp RouterID\n";
        return 1;
    }
    ROUTER_ID = argv[1];
    LOCAL_IP = detect_local_ip();
    std::cout << "ID=" << ROUTER_ID << " IP=" << LOCAL_IP << "\n";
    routing_table.update_route({ROUTER_ID, ROUTER_ID, 0});

    std::thread(sender_hello).detach();
    std::thread(sender_update).detach();
    std::thread(receiver).detach();
    std::thread(cleaner).detach();

    while (1)
        std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}
