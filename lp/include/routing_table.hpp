#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

struct Route
{
    std::string destination_id;
    std::string next_hop_id;
    int cost;
};

class RoutingTable
{
public:
    void update_route(const Route &route);
    std::unordered_map<std::string, Route> get_all_routes();

private:
    std::unordered_map<std::string, Route> routes_;
    std::mutex mutex_;
};
