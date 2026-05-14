#pragma once

#include "quant_engine/types.hpp"

#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace qe {

class OrderBook {
public:
    std::vector<Trade> apply(const OrderEvent& event);
    bool cancel(OrderId order_id);
    TopOfBook top() const;
    std::optional<Price> mid_price() const;
    std::size_t resting_order_count() const;

private:
    struct RestingOrder {
        OrderId id{};
        Side side{};
        Price price{};
        Quantity quantity{};
        Timestamp timestamp_ns{};
    };

    using Level = std::deque<RestingOrder>;
    using BidBook = std::map<Price, Level, std::greater<Price>>;
    using AskBook = std::map<Price, Level>;

    struct OrderLocation {
        Side side{};
        Price price{};
    };

    std::vector<Trade> add_limit(const OrderEvent& event);
    std::vector<Trade> add_market(const OrderEvent& event);
    void rest(const OrderEvent& event, Quantity remaining);
    void remove_empty_level(Side side, Price price);

    template <typename Book>
    std::vector<Trade> match_against(Book& opposite_book, const OrderEvent& event, Quantity& remaining);

    BidBook bids_;
    AskBook asks_;
    std::unordered_map<OrderId, OrderLocation> order_index_;
};

}  // namespace qe
