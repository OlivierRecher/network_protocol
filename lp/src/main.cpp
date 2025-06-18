#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#include "neighbor_table.hpp"
#include "routing_table.hpp"
#include "crypto_utils.hpp"

NeighborTable neighbor_table;
RoutingTable routing_table;

// Port utilisé par le protocole LRRP
const int PORT = 8888;

const std::string ROUTER_ID = "RouterA";
const std::string SHARED_SECRET_KEY = "supersecretkey"; //A modifier

// Construire un message HELLO signé
std::string build_hello_message()
{
    std::string message = "HELLO from " + ROUTER_ID + "\n";
    std::string signature = compute_hmac(message, SHARED_SECRET_KEY);
    message += "--SIGNATURE " + signature;
    return message;
}

void sender_thread()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return;
    }

    int broadcastEnable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
    {
        perror("setsockopt (SO_BROADCAST)");
        close(sock);
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    while (true)
    {
        std::string message = build_hello_message();
        ssize_t sent = sendto(sock, message.c_str(), message.size(), 0, (sockaddr *)&addr, sizeof(addr));
        if (sent < 0)
        {
            perror("sendto");
        }
        else
        {
            std::cout << "[HELLO] Sent: " << message << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    close(sock);
}

void receiver_thread()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(sock);
        return;
    }

    char buffer[1024];
    while (true)
    {
        sockaddr_in sender_addr{};
        socklen_t sender_len = sizeof(sender_addr);
        ssize_t received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&sender_addr, &sender_len);
        if (received > 0)
        {
            buffer[received] = '\0';

            char sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);

            std::string raw_msg(buffer, received);

            size_t sig_pos = raw_msg.find("--SIGNATURE ");
            if (sig_pos == std::string::npos)
            {
                std::cout << "[SECURITY] Signature absente – message ignoré\n";
                continue;
            }

            std::string msg_body = raw_msg.substr(0, sig_pos);
            std::string received_sig = raw_msg.substr(sig_pos + 12); // longueur de "--SIGNATURE "

            // Vérification HMAC
            if (!verify_hmac(msg_body, SHARED_SECRET_KEY, received_sig))
            {
                std::cout << "[SECURITY] Signature invalide – message rejeté\n";
                continue;
            }

            // Signature valide, traitement du message
            std::istringstream iss(msg_body);
            std::string first_line;
            std::getline(iss, first_line);

            if (first_line.rfind("HELLO from ", 0) == 0)
            {
                std::string neighbor_id = first_line.substr(11);
                neighbor_table.update_neighbor(neighbor_id, sender_ip);
                std::cout << "[RECV][HELLO] From " << neighbor_id << " (" << sender_ip << ")\n";
            }
            else if (first_line.rfind("UPDATE from ", 0) == 0)
            {
                std::string from_id = first_line.substr(12);
                std::string route_line;
                while (std::getline(iss, route_line))
                {
                    if (route_line.empty())
                        continue;

                    std::istringstream r(route_line);
                    std::string dest_id;
                    int cost;
                    r >> dest_id >> cost;

                    Route new_route;
                    new_route.destination_id = dest_id;
                    new_route.next_hop_id = from_id;
                    new_route.cost = cost + 1;

                    routing_table.update_route(new_route);
                }
                std::cout << "[RECV][UPDATE] From " << from_id << "\n";
            }
            else
            {
                std::cout << "[RECV] Message inconnu: " << first_line << "\n";
            }
        }
    }

    close(sock);
}

void neighbor_cleaner_thread()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        neighbor_table.remove_stale_neighbors();
        neighbor_table.print_neighbors();
    }
}

void update_sender_thread()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return;
    }

    int broadcastEnable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    while (true)
    {
        std::ostringstream oss;
        oss << "UPDATE from " << ROUTER_ID << "\n";

        for (const auto &[dest, route] : routing_table.get_all_routes())
        {
            oss << route.destination_id << " " << route.cost << "\n";
        }

        std::string msg = oss.str();
        std::string signature = compute_hmac(msg, SHARED_SECRET_KEY);
        msg += "--SIGNATURE " + signature;

        ssize_t sent = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr *)&addr, sizeof(addr));
        if (sent < 0)
        {
            perror("sendto");
        }
        else
        {
            std::cout << "[UPDATE] Sent update\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    close(sock);
}

int main(int argc, char *argv[])
{
    routing_table.update_route(Route{ROUTER_ID, ROUTER_ID, 0});

    if (argc < 2)
    {
        std::cerr << "Usage: ./lp <RouterID>\n";
        return 1;
    }
    std::string router_id = argv[1];
    routing_table.update_route(Route{router_id, router_id, 0});

    std::thread sender(sender_thread);
    std::thread receiver(receiver_thread);
    std::thread cleaner(neighbor_cleaner_thread);
    std::thread updater(update_sender_thread);

    sender.join();
    receiver.join();
    cleaner.join();
    updater.join();

    return 0;
}
