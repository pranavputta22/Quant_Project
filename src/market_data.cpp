#include "quant_engine/market_data.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
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

std::vector<std::string> extract_changes(const std::string& row) {
    const auto changes_pos = row.find("\"changes\":");
    if (changes_pos == std::string::npos) {
        return {};
    }

    std::vector<std::string> values;
    auto pos = changes_pos;
    while ((pos = row.find('[', pos + 1)) != std::string::npos) {
        const auto end = row.find(']', pos);
        if (end == std::string::npos) {
            break;
        }
        const auto segment = row.substr(pos, end - pos);
        if (segment.find("\"buy\"") != std::string::npos || segment.find("\"sell\"") != std::string::npos) {
            values.push_back(segment);
        }
        pos = end;
    }
    return values;
}

std::vector<std::string> quoted_values(const std::string& text) {
    std::vector<std::string> values;
    std::size_t pos = 0;
    while ((pos = text.find('"', pos)) != std::string::npos) {
        const auto start = pos + 1;
        const auto end = text.find('"', start);
        if (end == std::string::npos) {
            break;
        }
        values.push_back(text.substr(start, end - start));
        pos = end + 1;
    }
    return values;
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

std::optional<CoinbaseMessage> parse_coinbase_jsonl(const std::string& row) {
    const auto type = json_string_value(row, "type");
    if (!type) {
        return std::nullopt;
    }

    if (*type == "snapshot") {
        return CoinbaseMessage{CoinbaseMessage::Kind::Snapshot};
    }

    if (*type == "l2update") {
        const auto changes = extract_changes(row);
        if (changes.empty()) {
            return std::nullopt;
        }

        const auto fields = quoted_values(changes.front());
        if (fields.size() < 3) {
            return std::nullopt;
        }

        return CoinbaseMessage{
            CoinbaseMessage::Kind::LevelUpdate,
            fields[0] == "buy" ? Side::Buy : Side::Sell,
            decimal_to_ticks(fields[1]),
            decimal_to_quantity(fields[2])
        };
    }

    if (*type == "match" || *type == "last_match") {
        const auto side = json_string_value(row, "side");
        const auto price = json_string_value(row, "price");
        const auto size = json_string_value(row, "size");
        if (!side || !price || !size) {
            return std::nullopt;
        }

        return CoinbaseMessage{
            CoinbaseMessage::Kind::Match,
            *side == "buy" ? Side::Buy : Side::Sell,
            decimal_to_ticks(*price),
            decimal_to_quantity(*size)
        };
    }

    return CoinbaseMessage{CoinbaseMessage::Kind::Ignore};
}

MarketReplayStats replay_coinbase_jsonl(std::istream& input) {
    MarketDepth depth;
    MarketReplayStats stats;
    std::string row;

    while (std::getline(input, row)) {
        const auto message = parse_coinbase_jsonl(row);
        if (!message) {
            continue;
        }

        ++stats.messages;
        switch (message->kind) {
        case CoinbaseMessage::Kind::Snapshot:
            depth.clear();
            ++stats.snapshots;
            break;
        case CoinbaseMessage::Kind::LevelUpdate:
            depth.apply_level(message->side, message->price, message->quantity);
            ++stats.level_updates;
            break;
        case CoinbaseMessage::Kind::Match:
            ++stats.trades;
            stats.executed_quantity += message->quantity;
            stats.notional += static_cast<long double>(message->price) * static_cast<long double>(message->quantity);
            break;
        case CoinbaseMessage::Kind::Ignore:
            break;
        }
    }

    stats.final_depth = depth.metrics();
    return stats;
}

}  // namespace qe
