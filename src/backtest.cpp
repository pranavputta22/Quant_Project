#include "quant_engine/backtest.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace qe {
namespace {

std::vector<std::string> split_csv_row(const std::string& row) {
    std::vector<std::string> fields;
    std::stringstream ss(row);
    std::string field;
    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }
    return fields;
}

bool is_header_or_empty(const std::string& row) {
    return row.empty() || row.find("timestamp_ns") == 0;
}

}  // namespace

OrderEvent parse_event_row(const std::string& row) {
    const auto fields = split_csv_row(row);
    if (fields.size() != 6) {
        throw std::invalid_argument("expected 6 CSV fields in row: " + row);
    }

    return OrderEvent{
        static_cast<Timestamp>(std::stoull(fields[0])),
        parse_event_type(fields[1]),
        static_cast<OrderId>(std::stoull(fields[2])),
        parse_side(fields[3]),
        static_cast<Price>(std::stoll(fields[4])),
        static_cast<Quantity>(std::stoll(fields[5]))
    };
}

std::vector<OrderEvent> read_events(std::istream& input) {
    std::vector<OrderEvent> events;
    std::string row;
    while (std::getline(input, row)) {
        if (is_header_or_empty(row)) {
            continue;
        }
        events.push_back(parse_event_row(row));
    }
    return events;
}

ReplayStats replay_csv(std::istream& input, std::ostream& trades_output) {
    OrderBook book;
    ReplayStats stats;

    trades_output << "timestamp_ns,aggressor_order_id,resting_order_id,aggressor_side,price,quantity\n";

    for (const auto& event : read_events(input)) {
        ++stats.events;
        const auto trades = book.apply(event);
        for (const auto& trade : trades) {
            ++stats.trades;
            stats.executed_quantity += trade.quantity;
            stats.notional += static_cast<long double>(trade.price) * static_cast<long double>(trade.quantity);

            trades_output << trade.timestamp_ns << ','
                          << trade.aggressor_order_id << ','
                          << trade.resting_order_id << ','
                          << to_string(trade.aggressor_side) << ','
                          << trade.price << ','
                          << trade.quantity << '\n';
        }
    }

    stats.final_top = book.top();
    return stats;
}

}  // namespace qe
