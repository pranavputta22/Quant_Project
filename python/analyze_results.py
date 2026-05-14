from __future__ import annotations

import csv
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Trade:
    timestamp_ns: int
    aggressor_order_id: int
    resting_order_id: int
    aggressor_side: str
    price: int
    quantity: int


def read_trades(path: Path) -> list[Trade]:
    with path.open(newline="") as handle:
        return [
            Trade(
                timestamp_ns=int(row["timestamp_ns"]),
                aggressor_order_id=int(row["aggressor_order_id"]),
                resting_order_id=int(row["resting_order_id"]),
                aggressor_side=row["aggressor_side"],
                price=int(row["price"]),
                quantity=int(row["quantity"]),
            )
            for row in csv.DictReader(handle)
        ]


def signed_cashflow(trade: Trade) -> int:
    notional = trade.price * trade.quantity
    return notional if trade.aggressor_side == "SELL" else -notional


def summarize(trades: list[Trade]) -> None:
    total_qty = sum(trade.quantity for trade in trades)
    notional = sum(trade.price * trade.quantity for trade in trades)
    cashflow = sum(signed_cashflow(trade) for trade in trades)
    buy_qty = sum(trade.quantity for trade in trades if trade.aggressor_side == "BUY")
    sell_qty = total_qty - buy_qty
    vwap = notional / total_qty if total_qty else 0.0

    print("Trade summary")
    print(f"  Trades: {len(trades)}")
    print(f"  Executed quantity: {total_qty}")
    print(f"  Buy quantity: {buy_qty}")
    print(f"  Sell quantity: {sell_qty}")
    print(f"  Notional: {notional:,.0f}")
    print(f"  VWAP: {vwap:,.2f}")
    print(f"  Aggressor cashflow proxy: {cashflow:,.0f}")


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: python python/analyze_results.py <trades.csv>", file=sys.stderr)
        return 1

    path = Path(sys.argv[1])
    trades = read_trades(path)
    summarize(trades)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
