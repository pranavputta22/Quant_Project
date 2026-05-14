#pragma once

#include "quant_engine/types.hpp"

#include <cstddef>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>

namespace qe {

struct DepthMetrics {
    bool has_bid{};
    bool has_ask{};
    Price best_bid{};
    Price best_ask{};
    Quantity best_bid_size{};
    Quantity best_ask_size{};
    Price spread{};
    Price mid{};
    double top_imbalance{};
};

struct MarketReplayStats {
    std::size_t messages{};
    std::size_t snapshots{};
    std::size_t level_updates{};
    std::size_t trades{};
    Quantity executed_quantity{};
    long double notional{};
    DepthMetrics final_depth{};
};

struct CoinbaseMessage {
    enum class Kind {
        Ignore,
        Snapshot,
        LevelUpdate,
        Match
    };

    Kind kind{Kind::Ignore};
    Side side{Side::Buy};
    Price price{};
    Quantity quantity{};
};

class MarketDepth {
public:
    void apply_level(Side side, Price price, Quantity quantity);
    void clear();
    DepthMetrics metrics() const;
    std::size_t bid_levels() const;
    std::size_t ask_levels() const;

private:
    std::map<Price, Quantity, std::greater<Price>> bids_;
    std::map<Price, Quantity> asks_;
};

std::optional<CoinbaseMessage> parse_coinbase_jsonl(const std::string& row);
MarketReplayStats replay_coinbase_jsonl(std::istream& input);

Price decimal_to_ticks(const std::string& value, int scale = 100);
Quantity decimal_to_quantity(const std::string& value, int scale = 100000000);

}  // namespace qe
