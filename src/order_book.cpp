#include "quant_engine/order_book.hpp"

#include <algorithm>
#include <stdexcept>

namespace qe {
namespace {

bool crosses(Side side, Price incoming_price, Price resting_price) {
    if (side == Side::Buy) {
        return incoming_price >= resting_price;
    }
    return incoming_price <= resting_price;
}

}  // namespace

std::vector<Trade> OrderBook::apply(const OrderEvent& event) {
    if (event.quantity < 0) {
        throw std::invalid_argument("quantity cannot be negative");
    }

    switch (event.type) {
    case EventType::Add:
        return add_limit(event);
    case EventType::Cancel:
        cancel(event.order_id);
        return {};
    case EventType::Market:
        return add_market(event);
    }

    return {};
}

bool OrderBook::cancel(OrderId order_id) {
    const auto index_it = order_index_.find(order_id);
    if (index_it == order_index_.end()) {
        return false;
    }

    const auto location = index_it->second;
    if (location.side == Side::Buy) {
        auto level_it = bids_.find(location.price);
        if (level_it == bids_.end()) {
            order_index_.erase(index_it);
            return false;
        }

        auto& level = level_it->second;
        const auto order_it = std::find_if(level.begin(), level.end(), [order_id](const RestingOrder& order) {
            return order.id == order_id;
        });

        if (order_it == level.end()) {
            order_index_.erase(index_it);
            return false;
        }

        level.erase(order_it);
        order_index_.erase(index_it);
        if (level.empty()) {
            bids_.erase(level_it);
        }
        return true;
    }

    auto level_it = asks_.find(location.price);
    if (level_it == asks_.end()) {
        order_index_.erase(index_it);
        return false;
    }

    auto& level = level_it->second;
    const auto order_it = std::find_if(level.begin(), level.end(), [order_id](const RestingOrder& order) {
        return order.id == order_id;
    });

    if (order_it == level.end()) {
        order_index_.erase(index_it);
        return false;
    }

    level.erase(order_it);
    order_index_.erase(index_it);
    if (level.empty()) {
        asks_.erase(level_it);
    }
    return true;
}

TopOfBook OrderBook::top() const {
    TopOfBook tob{};
    if (!bids_.empty()) {
        tob.has_bid = true;
        tob.bid_price = bids_.begin()->first;
        for (const auto& order : bids_.begin()->second) {
            tob.bid_quantity += order.quantity;
        }
    }
    if (!asks_.empty()) {
        tob.has_ask = true;
        tob.ask_price = asks_.begin()->first;
        for (const auto& order : asks_.begin()->second) {
            tob.ask_quantity += order.quantity;
        }
    }
    return tob;
}

std::optional<Price> OrderBook::mid_price() const {
    const auto tob = top();
    if (!tob.has_bid || !tob.has_ask) {
        return std::nullopt;
    }
    return (tob.bid_price + tob.ask_price) / 2;
}

std::size_t OrderBook::resting_order_count() const {
    return order_index_.size();
}

std::vector<Trade> OrderBook::add_limit(const OrderEvent& event) {
    if (event.quantity == 0) {
        return {};
    }
    if (order_index_.find(event.order_id) != order_index_.end()) {
        throw std::invalid_argument("duplicate order id");
    }

    Quantity remaining = event.quantity;
    auto trades = event.side == Side::Buy
        ? match_against(asks_, event, remaining)
        : match_against(bids_, event, remaining);

    if (remaining > 0) {
        rest(event, remaining);
    }

    return trades;
}

std::vector<Trade> OrderBook::add_market(const OrderEvent& event) {
    if (event.quantity == 0) {
        return {};
    }

    Quantity remaining = event.quantity;
    return event.side == Side::Buy
        ? match_against(asks_, event, remaining)
        : match_against(bids_, event, remaining);
}

void OrderBook::rest(const OrderEvent& event, Quantity remaining) {
    RestingOrder resting{
        event.order_id,
        event.side,
        event.price,
        remaining,
        event.timestamp_ns
    };

    if (event.side == Side::Buy) {
        bids_[event.price].push_back(resting);
    } else {
        asks_[event.price].push_back(resting);
    }

    order_index_[event.order_id] = OrderLocation{event.side, event.price};
}

void OrderBook::remove_empty_level(Side side, Price price) {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        if (it != bids_.end() && it->second.empty()) {
            bids_.erase(it);
        }
    } else {
        auto it = asks_.find(price);
        if (it != asks_.end() && it->second.empty()) {
            asks_.erase(it);
        }
    }
}

template <typename Book>
std::vector<Trade> OrderBook::match_against(Book& opposite_book, const OrderEvent& event, Quantity& remaining) {
    std::vector<Trade> trades;

    while (remaining > 0 && !opposite_book.empty()) {
        auto level_it = opposite_book.begin();
        const Price resting_price = level_it->first;
        const Side resting_side = event.side == Side::Buy ? Side::Sell : Side::Buy;

        if (event.type == EventType::Add && !crosses(event.side, event.price, resting_price)) {
            break;
        }

        auto& level = level_it->second;
        while (remaining > 0 && !level.empty()) {
            auto& resting = level.front();
            const Quantity fill_qty = std::min(remaining, resting.quantity);

            trades.push_back(Trade{
                event.timestamp_ns,
                event.order_id,
                resting.id,
                event.side,
                resting.price,
                fill_qty
            });

            remaining -= fill_qty;
            resting.quantity -= fill_qty;

            if (resting.quantity == 0) {
                order_index_.erase(resting.id);
                level.pop_front();
            }
        }

        if (level.empty()) {
            opposite_book.erase(level_it);
        } else {
            remove_empty_level(resting_side, resting_price);
        }
    }

    return trades;
}

Side parse_side(const std::string& value) {
    if (value == "BUY" || value == "B") {
        return Side::Buy;
    }
    if (value == "SELL" || value == "S") {
        return Side::Sell;
    }
    throw std::invalid_argument("unknown side: " + value);
}

EventType parse_event_type(const std::string& value) {
    if (value == "ADD") {
        return EventType::Add;
    }
    if (value == "CANCEL") {
        return EventType::Cancel;
    }
    if (value == "MARKET") {
        return EventType::Market;
    }
    throw std::invalid_argument("unknown event type: " + value);
}

std::string to_string(Side side) {
    return side == Side::Buy ? "BUY" : "SELL";
}

}  // namespace qe
