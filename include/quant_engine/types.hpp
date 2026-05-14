#pragma once

#include <cstdint>
#include <string>

namespace qe {

using OrderId = std::uint64_t;
using Timestamp = std::uint64_t;
using Quantity = std::int64_t;
using Price = std::int64_t;

enum class Side {
    Buy,
    Sell
};

enum class EventType {
    Add,
    Cancel,
    Market
};

struct OrderEvent {
    Timestamp timestamp_ns{};
    EventType type{};
    OrderId order_id{};
    Side side{};
    Price price{};
    Quantity quantity{};
};

struct Trade {
    Timestamp timestamp_ns{};
    OrderId aggressor_order_id{};
    OrderId resting_order_id{};
    Side aggressor_side{};
    Price price{};
    Quantity quantity{};
};

struct TopOfBook {
    bool has_bid{};
    bool has_ask{};
    Price bid_price{};
    Quantity bid_quantity{};
    Price ask_price{};
    Quantity ask_quantity{};
};

Side parse_side(const std::string& value);
EventType parse_event_type(const std::string& value);
std::string to_string(Side side);

}  // namespace qe
