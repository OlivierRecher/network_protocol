#include "routing_table.hpp"

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
