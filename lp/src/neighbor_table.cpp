#include "neighbor_table.hpp"
#include <iostream>

void NeighborTable::update_neighbor(const std::string &id, const std::string &ip)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();

    neighbors_[id] = Neighbor{id, ip, now};
}

void NeighborTable::remove_stale_neighbors()
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();

    for (auto it = neighbors_.begin(); it != neighbors_.end();)
    {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen).count();
        if (duration > timeout_seconds_)
        {
            std::cout << "[INFO] Neighbor " << it->second.id << " timed out." << std::endl;
            it = neighbors_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void NeighborTable::print_neighbors()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "=== Active Neighbors ===" << std::endl;
    for (const auto &[id, neighbor] : neighbors_)
    {
        std::cout << " - " << id << " (" << neighbor.ip << ")" << std::endl;
    }
    std::cout << "=========================" << std::endl;
}
