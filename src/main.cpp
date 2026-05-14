#include "quant_engine/backtest.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

void print_top(const qe::TopOfBook& top) {
    std::cout << "Final top of book\n";
    std::cout << "  Bid: ";
    if (top.has_bid) {
        std::cout << top.bid_price << " x " << top.bid_quantity << '\n';
    } else {
        std::cout << "empty\n";
    }

    std::cout << "  Ask: ";
    if (top.has_ask) {
        std::cout << top.ask_price << " x " << top.ask_quantity << '\n';
    } else {
        std::cout << "empty\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: quant_replay <events.csv> <trades.csv>\n";
        return 1;
    }

    try {
        std::ifstream input(argv[1]);
        if (!input) {
            throw std::runtime_error("could not open input file");
        }

        std::ofstream trades_output(argv[2]);
        if (!trades_output) {
            throw std::runtime_error("could not open output file");
        }

        const auto stats = qe::replay_csv(input, trades_output);
        std::cout << "Replay complete\n";
        std::cout << "  Events: " << stats.events << '\n';
        std::cout << "  Trades: " << stats.trades << '\n';
        std::cout << "  Executed quantity: " << stats.executed_quantity << '\n';
        std::cout << "  Notional: " << static_cast<double>(stats.notional) << '\n';
        print_top(stats.final_top);
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
