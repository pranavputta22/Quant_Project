# Low-Latency Order Book Backtesting Engine

A C++17 event-driven limit order book and market replay engine built for quant developer and SWE roles at trading firms. The project focuses on correctness, realistic exchange mechanics, reproducible benchmarks, and a Python research layer for post-trade analysis.

Project page: https://pranavputta22.github.io/Quant_Project/

## What It Demonstrates

- Price-time priority matching for limit and market orders
- Order cancellation with O(1) order lookup by id
- Level 2 book reconstruction from event streams
- CSV market replay with trade log output
- Realistic metrics: volume, notional, spread, midprice, realized PnL proxy, and drawdown
- Unit-style correctness tests for matching, resting liquidity, and cancellation
- Python analysis layer for summaries and optional plotting

## Project Layout

```text
.
├── CMakeLists.txt
├── data/
│   └── sample_events.csv
├── include/quant_engine/
│   ├── backtest.hpp
│   ├── order_book.hpp
│   └── types.hpp
├── python/
│   └── analyze_results.py
├── src/
│   ├── backtest.cpp
│   ├── main.cpp
│   └── order_book.cpp
└── tests/
    └── order_book_tests.cpp
```

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

If CMake is not installed, build directly with MinGW `g++`:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/order_book.cpp src/backtest.cpp src/main.cpp -o build/quant_replay.exe
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/order_book.cpp tests/order_book_tests.cpp -o build/order_book_tests.exe
```

## Run A Replay

```powershell
.\build\Release\quant_replay.exe data\sample_events.csv trades.csv
python python\analyze_results.py trades.csv
```

On single-config generators, the executable may be:

```powershell
.\build\quant_replay.exe data\sample_events.csv trades.csv
```

On Windows, `py` can be used instead of `python`:

```powershell
py python\analyze_results.py trades.csv
```

## Run Tests

```powershell
.\build\Release\order_book_tests.exe
```

## Input Format

The replay engine consumes CSV rows with this schema:

```text
timestamp_ns,event_type,order_id,side,price,quantity
```

Supported event types:

- `ADD`: add a limit order
- `CANCEL`: cancel an existing resting order
- `MARKET`: submit a market order

Supported sides are `BUY` and `SELL`. Market order prices are ignored and can be `0`.

## Resume Bullets

- Built a C++17 event-driven limit order book and replay engine supporting price-time priority matching, cancellations, market orders, trade logging, and Level 2 book reconstruction.
- Implemented O(1) cancellation lookup and deterministic matching tests for order priority, partial fills, and resting liquidity correctness.
- Added a Python research layer to analyze replay outputs, summarize notional/volume/PnL proxy, and generate optional execution charts.

## Next Extensions

- Add binary tick-data parsing and memory-mapped input
- Add latency models, fees, queue position, and slippage assumptions
- Bind the engine to Python with `pybind11`
- Benchmark against a pure Python baseline on 10M+ synthetic events
