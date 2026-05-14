#include "quant_engine/market_data.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {

double price(qe::Price ticks) {
    return static_cast<double>(ticks) / 100.0;
}

double quantity(qe::Quantity quantity) {
    return static_cast<double>(quantity) / 100000000.0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: coinbase_replay <coinbase_messages.jsonl>\n";
        return 1;
    }

    try {
        std::ifstream input(argv[1]);
        if (!input) {
            throw std::runtime_error("could not open input file");
        }

        const auto stats = qe::replay_coinbase_jsonl(input);
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Coinbase JSONL replay complete\n";
        std::cout << "  Messages: " << stats.messages << '\n';
        std::cout << "  Snapshots: " << stats.snapshots << '\n';
        std::cout << "  Snapshot levels: " << stats.snapshot_levels << '\n';
        std::cout << "  Level updates: " << stats.level_updates << '\n';
        std::cout << "  Trades: " << stats.trades << '\n';
        std::cout << "  Executed quantity: " << std::setprecision(6) << quantity(stats.executed_quantity) << '\n';
        std::cout << std::setprecision(2);
        std::cout << "  Notional: " << (static_cast<double>(stats.notional) / 10000000000.0) << '\n';
        if (stats.final_depth.has_bid && stats.final_depth.has_ask) {
            std::cout << "  Best bid: " << price(stats.final_depth.best_bid) << '\n';
            std::cout << "  Best ask: " << price(stats.final_depth.best_ask) << '\n';
            std::cout << "  Spread: " << price(stats.final_depth.spread) << '\n';
            std::cout << "  Mid: " << price(stats.final_depth.mid) << '\n';
            std::cout << "  Top imbalance: " << std::setprecision(4) << stats.final_depth.top_imbalance << '\n';
        }
        if (stats.depth_observations > 0) {
            std::cout << std::setprecision(2);
            std::cout << "  Avg spread: " << price(static_cast<qe::Price>(stats.spread_sum / stats.depth_observations)) << '\n';
            std::cout << "  Min spread: " << price(stats.min_spread) << '\n';
            std::cout << "  Max spread: " << price(stats.max_spread) << '\n';
            std::cout << std::setprecision(4);
            std::cout << "  Avg abs imbalance: "
                      << static_cast<double>(stats.abs_imbalance_sum / stats.depth_observations) << '\n';
        }
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
