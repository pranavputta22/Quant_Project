#include "quant_engine/order_book.hpp"

#include <cassert>
#include <iostream>

namespace {

qe::OrderEvent add(qe::Timestamp ts, qe::OrderId id, qe::Side side, qe::Price price, qe::Quantity qty) {
    return qe::OrderEvent{ts, qe::EventType::Add, id, side, price, qty};
}

qe::OrderEvent market(qe::Timestamp ts, qe::OrderId id, qe::Side side, qe::Quantity qty) {
    return qe::OrderEvent{ts, qe::EventType::Market, id, side, 0, qty};
}

void test_limit_crosses_best_price() {
    qe::OrderBook book;
    book.apply(add(1, 1, qe::Side::Sell, 10100, 10));
    book.apply(add(2, 2, qe::Side::Sell, 10200, 10));

    const auto trades = book.apply(add(3, 3, qe::Side::Buy, 10100, 4));

    assert(trades.size() == 1);
    assert(trades[0].price == 10100);
    assert(trades[0].quantity == 4);

    const auto top = book.top();
    assert(top.has_ask);
    assert(top.ask_price == 10100);
    assert(top.ask_quantity == 6);
}

void test_price_time_priority() {
    qe::OrderBook book;
    book.apply(add(1, 1, qe::Side::Sell, 10100, 5));
    book.apply(add(2, 2, qe::Side::Sell, 10100, 5));

    const auto trades = book.apply(market(3, 3, qe::Side::Buy, 7));

    assert(trades.size() == 2);
    assert(trades[0].resting_order_id == 1);
    assert(trades[0].quantity == 5);
    assert(trades[1].resting_order_id == 2);
    assert(trades[1].quantity == 2);
}

void test_cancel_removes_resting_order() {
    qe::OrderBook book;
    book.apply(add(1, 1, qe::Side::Buy, 10000, 8));

    assert(book.cancel(1));
    assert(!book.top().has_bid);
    assert(book.resting_order_count() == 0);
}

void test_uncrossed_limit_rests() {
    qe::OrderBook book;
    const auto trades = book.apply(add(1, 1, qe::Side::Buy, 9900, 8));

    assert(trades.empty());
    const auto top = book.top();
    assert(top.has_bid);
    assert(top.bid_price == 9900);
    assert(top.bid_quantity == 8);
}

}  // namespace

int main() {
    test_limit_crosses_best_price();
    test_price_time_priority();
    test_cancel_removes_resting_order();
    test_uncrossed_limit_rests();
    std::cout << "order_book_tests passed\n";
    return 0;
}
