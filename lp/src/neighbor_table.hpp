#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>

struct Neighbor
{
    std::string id;
    std::string ip;
    std::chrono::steady_clock::time_point last_seen;
};

class NeighborTable
{
public:
    void update_neighbor(const std::string &id, const std::string &ip);
    void remove_stale_neighbors();
    void print_neighbors();
    std::string get_ip(const std::string &id);

private:
    std::unordered_map<std::string, Neighbor> neighbors_;
    std::mutex mutex_;
    int timeout_seconds_ = 15;
};
