#include "quant_engine/market_data.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace qe {
namespace {

std::optional<std::string> json_string_value(const std::string& row, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    const auto key_pos = row.find(needle);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }

    auto value_pos = key_pos + needle.size();
    while (value_pos < row.size() && std::isspace(static_cast<unsigned char>(row[value_pos]))) {
        ++value_pos;
    }
    if (value_pos >= row.size() || row[value_pos] != '"') {
        return std::nullopt;
    }

    const auto start = value_pos + 1;
    const auto end = row.find('"', start);
    if (end == std::string::npos) {
        return std::nullopt;
    }

    return row.substr(start, end - start);
}

std::optional<std::string> json_array_region(const std::string& row, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    const auto key_pos = row.find(needle);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }

    const auto start = row.find('[', key_pos + needle.size());
    if (start == std::string::npos) {
        return std::nullopt;
    }

    int depth = 0;
    for (auto pos = start; pos < row.size(); ++pos) {
        if (row[pos] == '[') {
            ++depth;
        } else if (row[pos] == ']') {
            --depth;
            if (depth == 0) {
                return row.substr(start, pos - start + 1);
            }
        }
    }

    return std::nullopt;
}

void append_snapshot_levels(
    std::vector<CoinbaseMessage>& messages,
    const std::string& row,
    const std::string& key,
    Side side) {
    const auto region = json_array_region(row, key);
    if (!region) {
        return;
    }

    static const std::regex pair_pattern("\\[\\s*\"([0-9.]+)\"\\s*,\\s*\"([0-9.]+)\"\\s*\\]");
    for (std::sregex_iterator it(region->begin(), region->end(), pair_pattern), end; it != end; ++it) {
        messages.push_back(CoinbaseMessage{
            CoinbaseMessage::Kind::LevelUpdate,
            side,
            decimal_to_ticks((*it)[1].str()),
            decimal_to_quantity((*it)[2].str())
        });
    }
}

void append_l2_changes(std::vector<CoinbaseMessage>& messages, const std::string& row) {
    const auto region = json_array_region(row, "changes");
    if (!region) {
        return;
    }

    static const std::regex change_pattern("\\[\\s*\"(buy|sell)\"\\s*,\\s*\"([0-9.]+)\"\\s*,\\s*\"([0-9.]+)\"\\s*\\]");
    for (std::sregex_iterator it(region->begin(), region->end(), change_pattern), end; it != end; ++it) {
        messages.push_back(CoinbaseMessage{
            CoinbaseMessage::Kind::LevelUpdate,
            (*it)[1].str() == "buy" ? Side::Buy : Side::Sell,
            decimal_to_ticks((*it)[2].str()),
            decimal_to_quantity((*it)[3].str())
        });
    }
}

void observe_depth(MarketReplayStats& stats, const MarketDepth& depth) {
    const auto metrics = depth.metrics();
    if (!metrics.has_bid || !metrics.has_ask || metrics.spread < 0) {
        return;
    }

    if (stats.depth_observations == 0) {
        stats.min_spread = metrics.spread;
        stats.max_spread = metrics.spread;
    } else {
        stats.min_spread = std::min(stats.min_spread, metrics.spread);
        stats.max_spread = std::max(stats.max_spread, metrics.spread);
    }

    ++stats.depth_observations;
    stats.spread_sum += metrics.spread;
    stats.abs_imbalance_sum += std::abs(metrics.top_imbalance);
}

}  // namespace

void MarketDepth::apply_level(Side side, Price price, Quantity quantity) {
    if (side == Side::Buy) {
        if (quantity <= 0) {
            bids_.erase(price);
        } else {
            bids_[price] = quantity;
        }
        return;
    }

    if (quantity <= 0) {
        asks_.erase(price);
    } else {
        asks_[price] = quantity;
    }
}

void MarketDepth::clear() {
    bids_.clear();
    asks_.clear();
}

DepthMetrics MarketDepth::metrics() const {
    DepthMetrics result{};
    if (!bids_.empty()) {
        result.has_bid = true;
        result.best_bid = bids_.begin()->first;
        result.best_bid_size = bids_.begin()->second;
    }
    if (!asks_.empty()) {
        result.has_ask = true;
        result.best_ask = asks_.begin()->first;
        result.best_ask_size = asks_.begin()->second;
    }
    if (result.has_bid && result.has_ask) {
        result.spread = result.best_ask - result.best_bid;
        result.mid = (result.best_ask + result.best_bid) / 2;
        const auto denom = static_cast<double>(result.best_bid_size + result.best_ask_size);
        if (denom > 0.0) {
            result.top_imbalance = static_cast<double>(result.best_bid_size - result.best_ask_size) / denom;
        }
    }
    return result;
}

std::size_t MarketDepth::bid_levels() const {
    return bids_.size();
}

std::size_t MarketDepth::ask_levels() const {
    return asks_.size();
}

Price decimal_to_ticks(const std::string& value, int scale) {
    return static_cast<Price>(std::llround(std::stold(value) * scale));
}

Quantity decimal_to_quantity(const std::string& value, int scale) {
    return static_cast<Quantity>(std::llround(std::stold(value) * scale));
}

std::vector<CoinbaseMessage> parse_coinbase_jsonl_messages(const std::string& row) {
    std::vector<CoinbaseMessage> messages;
    const auto type = json_string_value(row, "type");
    if (!type) {
        return messages;
    }

    if (*type == "snapshot") {
        messages.push_back(CoinbaseMessage{CoinbaseMessage::Kind::Snapshot});
        append_snapshot_levels(messages, row, "bids", Side::Buy);
        append_snapshot_levels(messages, row, "asks", Side::Sell);
        return messages;
    }

    if (*type == "l2update") {
        append_l2_changes(messages, row);
        return messages;
    }

    if (*type == "match" || *type == "last_match") {
        const auto side = json_string_value(row, "side");
        const auto price = json_string_value(row, "price");
        const auto size = json_string_value(row, "size");
        if (!side || !price || !size) {
            return messages;
        }

        messages.push_back(CoinbaseMessage{
            CoinbaseMessage::Kind::Match,
            *side == "buy" ? Side::Buy : Side::Sell,
            decimal_to_ticks(*price),
            decimal_to_quantity(*size)
        });
        return messages;
    }

    messages.push_back(CoinbaseMessage{CoinbaseMessage::Kind::Ignore});
    return messages;
}

std::optional<CoinbaseMessage> parse_coinbase_jsonl(const std::string& row) {
    const auto messages = parse_coinbase_jsonl_messages(row);
    if (messages.empty()) {
        return std::nullopt;
    }
    return messages.front();
}

MarketReplayStats replay_coinbase_jsonl(std::istream& input) {
    MarketDepth depth;
    MarketReplayStats stats;
    std::string row;

    while (std::getline(input, row)) {
        const auto messages = parse_coinbase_jsonl_messages(row);
        if (messages.empty()) {
            continue;
        }

        ++stats.messages;
        const bool snapshot_row = messages.front().kind == CoinbaseMessage::Kind::Snapshot;
        for (const auto& message : messages) {
            switch (message.kind) {
            case CoinbaseMessage::Kind::Snapshot:
                depth.clear();
                ++stats.snapshots;
                break;
            case CoinbaseMessage::Kind::LevelUpdate:
                depth.apply_level(message.side, message.price, message.quantity);
                ++stats.level_updates;
                if (snapshot_row) {
                    ++stats.snapshot_levels;
                }
                observe_depth(stats, depth);
                break;
            case CoinbaseMessage::Kind::Match:
                ++stats.trades;
                stats.executed_quantity += message.quantity;
                stats.notional += static_cast<long double>(message.price) * static_cast<long double>(message.quantity);
                break;
            case CoinbaseMessage::Kind::Ignore:
                break;
            }
        }
    }

    stats.final_depth = depth.metrics();
    return stats;
}

}  // namespace qe
