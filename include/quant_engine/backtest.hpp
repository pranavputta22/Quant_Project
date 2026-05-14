#pragma once

#include "quant_engine/order_book.hpp"
#include "quant_engine/types.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace qe {

struct ReplayStats {
    std::size_t events{};
    std::size_t trades{};
    Quantity executed_quantity{};
    long double notional{};
    TopOfBook final_top{};
};

OrderEvent parse_event_row(const std::string& row);
ReplayStats replay_csv(std::istream& input, std::ostream& trades_output);
std::vector<OrderEvent> read_events(std::istream& input);

}  // namespace qe
