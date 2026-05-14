#include "quant_engine/order_book.hpp"

#include <chrono>
#include <iostream>

int main(int argc, char** argv) {
    const int events = argc > 1 ? std::stoi(argv[1]) : 1000000;
    qe::OrderBook book;
    std::size_t trades = 0;

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < events; ++i) {
        const auto id = static_cast<qe::OrderId>(i + 1);
        const auto side = i % 2 == 0 ? qe::Side::Buy : qe::Side::Sell;
        const auto price = static_cast<qe::Price>(100000 + ((i % 101) - 50));
        const qe::OrderEvent event{
            static_cast<qe::Timestamp>(i),
            qe::EventType::Add,
            id,
            side,
            price,
            100
        };
        trades += book.apply(event).size();
    }
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration<double>(end - start).count();
    const auto rate = static_cast<double>(events) / elapsed;

    std::cout << "Synthetic replay benchmark\n";
    std::cout << "  Events: " << events << '\n';
    std::cout << "  Trades: " << trades << '\n';
    std::cout << "  Seconds: " << elapsed << '\n';
    std::cout << "  Events/sec: " << static_cast<long long>(rate) << '\n';
    std::cout << "  Resting orders: " << book.resting_order_count() << '\n';
    return 0;
}
