// routing_protocol.cpp
// Prototype d'un protocole de routage dynamique simple, en C++

#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define HELLO_INTERVAL 5 // secondes
#define ROUTE_EXPIRY 15  // secondes

using namespace std;
using Clock = chrono::steady_clock;

struct Route
{
    string destination;
    int cost;
    string via;
    Clock::time_point lastUpdate;
};

struct Neighbor
{
    string id;
    string ip;
    Clock::time_point lastHello;
};

unordered_map<string, Route> routingTable;
unordered_map<string, Neighbor> neighbors;

// --- ENVOI DE MESSAGES HELLO ---
void sendHello(int sockfd, sockaddr_in &broadcastAddr, const string &routerId)
{
    string message = "HELLO " + routerId;
    sendto(sockfd, message.c_str(), message.size(), 0,
           (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr));
}

// --- RÉCEPTION ET TRAITEMENT DES MESSAGES ---
void handleMessage(const string &msg, const string &senderIp)
{
    if (msg.rfind("HELLO", 0) == 0)
    {
        string id = msg.substr(6);
        neighbors[id] = Neighbor{id, senderIp, Clock::now()};
        cout << "[INFO] Reçu HELLO de " << id << " (" << senderIp << ")\n";
    }
    // Étapes futures : gérer les messages de routes ici
}

// --- RÉCEPTION EN CONTINU ---
void receiveLoop(int sockfd)
{
    char buffer[BUFFER_SIZE];
    sockaddr_in senderAddr;
    socklen_t addrLen = sizeof(senderAddr);

    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                           (struct sockaddr *)&senderAddr, &addrLen);
        if (len > 0)
        {
            buffer[len] = '\0';
            string msg(buffer);
            string senderIp = inet_ntoa(senderAddr.sin_addr);
            handleMessage(msg, senderIp);
        }
    }
}

// --- FONCTION PRINCIPALE ---
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <router-id>\n";
        return 1;
    }
    string routerId = argv[1];

    // Création socket UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    // Activation du broadcast
    int broadcastEnable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    // Lier le socket à une adresse
    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(PORT);
    localAddr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sockfd, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0)
    {
        perror("bind");
        return 1;
    }

    // Adresse de broadcast
    sockaddr_in broadcastAddr = {};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(PORT);
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    // Lancer la réception dans un thread séparé
    thread recvThread(receiveLoop, sockfd);

    // Boucle d'envoi HELLO
    while (true)
    {
        sendHello(sockfd, broadcastAddr, routerId);
        this_thread::sleep_for(chrono::seconds(HELLO_INTERVAL));
    }

    close(sockfd);
    return 0;
}
