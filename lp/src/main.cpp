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
#include <net/if.h>
#include <cstdio>
#include <vector>

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
            std::string ip = inet_ntoa(((sockaddr_in *)ifa->ifa_addr)->sin_addr);
            if (ip.find("127.") == 0 || ip == "10.0.2.15")
                continue;
            freeifaddrs(ifs);
            return ip;
        }
    }
    freeifaddrs(ifs);
    return "0.0.0.0";
}

std::vector<std::string> get_local_networks()
{
    std::vector<std::string> networks;
    FILE *fp = popen("ip route", "r");
    if (!fp)
        return networks;

    char line[256];
    while (fgets(line, sizeof(line), fp))
    {
        std::string l(line);
        std::istringstream iss(l);
        std::string net;
        iss >> net;
        if (net.find('.') != std::string::npos && net.find('/') != std::string::npos)
            networks.push_back(net);
    }
    pclose(fp);
    return networks;
}

std::string build_hello()
{
    std::string msg = "HELLO from " + ROUTER_ID + " at " + LOCAL_IP + "\n";
    return msg + "--SIGNATURE " + compute_hmac(msg, SHARED_KEY);
}

void send_broadcast_all_interfaces(int s, const std::string &message)
{
    struct ifaddrs *ifaddr;
    getifaddrs(&ifaddr);
    for (auto *ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if (!(ifa->ifa_flags & IFF_BROADCAST) || !(ifa->ifa_flags & IFF_UP))
            continue;

        std::string ip = inet_ntoa(((sockaddr_in *)ifa->ifa_addr)->sin_addr);
        if (ip.find("127.") == 0 || ip == "10.0.2.15")
            continue;

        sockaddr_in *bcast = (sockaddr_in *)ifa->ifa_broadaddr;
        if (!bcast)
            continue;

        bcast->sin_port = htons(PORT);
        sendto(s, message.c_str(), message.size(), 0, (sockaddr *)bcast, sizeof(sockaddr_in));
        std::cout << "[HELLO] via " << ifa->ifa_name << " to " << inet_ntoa(bcast->sin_addr) << "\n";
    }
    freeifaddrs(ifaddr);
}

void sender_hello()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    while (true)
    {
        send_broadcast_all_interfaces(s, build_hello());
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void sender_update()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    while (true)
    {
        std::ostringstream oss;
        oss << "UPDATE from " << ROUTER_ID << "\n";
        for (const auto &[dest, route] : routing_table.get_all_routes())
        {
            if (route.is_network)
                oss << "ROUTER_ROUTE " << dest << "\n";
        }
        std::string msg = oss.str() + "--SIGNATURE " + compute_hmac(oss.str(), SHARED_KEY);
        send_broadcast_all_interfaces(s, msg);
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void receiver()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{AF_INET, htons(PORT), INADDR_ANY};
    bind(s, (sockaddr *)&addr, sizeof(addr));

    char buf[1024];
    while (true)
    {
        int n = recvfrom(s, buf, sizeof(buf) - 1, 0, NULL, NULL);
        if (n <= 0)
            continue;
        buf[n] = '\0';

        std::string msg(buf);
        size_t sig_pos = msg.find("--SIGNATURE ");
        if (sig_pos == std::string::npos)
            continue;

        std::string body = msg.substr(0, sig_pos);
        std::string sig = msg.substr(sig_pos + 12);
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
                if (line.rfind("ROUTER_ROUTE ", 0) == 0)
                {
                    std::string net = line.substr(13);
                    routing_table.update_route({net, id, 1, true});
                }
            }
            std::cout << "[RECV UPDATE] " << id << "\n";
        }
    }
}

void cleaner()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        neighbor_table.remove_stale_neighbors();
        neighbor_table.print_neighbors();
        routing_table.print_routes();
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./lp <RouterID>\n";
        return 1;
    }

    ROUTER_ID = argv[1];
    LOCAL_IP = detect_local_ip();
    std::cout << "RouterID: " << ROUTER_ID << ", IP: " << LOCAL_IP << "\n";

    if (LOCAL_IP == "0.0.0.0" || LOCAL_IP == "10.0.2.15")
    {
        std::cerr << "[ERROR] Invalid local IP for router execution.\n";
        return 1;
    }

    routing_table.update_route({ROUTER_ID, ROUTER_ID, 0, false});

    // Ajouter les routes locales détectées
    for (const auto &net : get_local_networks())
    {
        routing_table.update_route({net, ROUTER_ID, 0, true});
    }

    std::thread(sender_hello).detach();
    std::thread(sender_update).detach();
    std::thread(receiver).detach();
    std::thread(cleaner).detach();

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}
