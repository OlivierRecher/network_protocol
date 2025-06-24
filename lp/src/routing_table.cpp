#include "routing_table.hpp"
#include <iostream>

void RoutingTable::update_route(const Route &route)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = routes_.find(route.destination_id);
    if (it == routes_.end() || route.cost < it->second.cost)
    {
        routes_[route.destination_id] = route;
    }
}

std::unordered_map<std::string, Route> RoutingTable::get_all_routes()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return routes_;
}

void RoutingTable::print_routes()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "--- Routing Table ---\n";
    for (const auto &[dest, route] : routes_)
    {
        std::cout << dest << " via " << route.next_hop_id << " (cost " << route.cost << ")";
        if (route.is_network)
            std::cout << " [network]";
        std::cout << "\n";
    }
    std::cout << "---------------------\n";
}
