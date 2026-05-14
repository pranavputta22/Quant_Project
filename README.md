# Market Microstructure Console

A live market microstructure dashboard with a C++17 order book/replay engine and a deployed browser console that ingests real Coinbase Exchange market events. The public app subscribes to live `level2_batch`, `matches`, and `ticker` WebSocket channels and displays order book depth, trades, spread, midprice, imbalance, VWAP, and short-horizon flow.

Public app: https://pranavputta22.github.io/Quant_Project/

## Browser App

The deployed app runs entirely in the browser:

- Connects to Coinbase Exchange public WebSocket market data
- Maintains live bid/ask depth from the unauthenticated `level2_batch` feed
- Displays real-time trades from the `matches` feed
- Tracks last price, spread, midprice, top-of-book imbalance, VWAP, volume, and notional
- Supports switching between liquid USD markets such as `BTC-USD`, `ETH-USD`, and `SOL-USD`
- Records raw exchange messages and exports them as JSONL for native replay

Open locally by launching:

```powershell
.\docs\index.html
```

or by serving the folder:

```powershell
python -m http.server 8000 -d docs
```

Then visit:

```text
http://localhost:8000
```

## C++ Replay Engine

The native engine is still available for deterministic local testing:

- Price-time priority matching for limit and market orders
- Order cancellation with O(1) lookup by id
- CSV market replay with trade log output
- Coinbase JSONL replay for recorded `level2_batch` and `match` messages
- Session analytics for average/min/max spread and average absolute imbalance
- Synthetic benchmark executable for events/sec throughput checks
- Unit-style correctness tests for matching, resting liquidity, and cancellation
- Python analysis layer for replay output summaries

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
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/order_book.cpp src/market_data.cpp src/coinbase_replay.cpp -o build/coinbase_replay.exe
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/order_book.cpp src/bench_replay.cpp -o build/bench_replay.exe
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

## Replay Real Recorded Events

1. Open the public app or local `docs/index.html`.
2. Connect to a market.
3. Click `Record`.
4. Let the feed run for a short session.
5. Click `Stop Recording`, then `Download JSONL`.
6. Replay the downloaded file locally:

```powershell
.\build\coinbase_replay.exe .\BTC-USD-recording.jsonl
```

The repository also includes a tiny sample:

```powershell
.\build\coinbase_replay.exe data\coinbase_sample.jsonl
```

Replay output includes message counts, snapshot depth, trade count, notional, final best bid/ask, spread, midprice, average spread, and average absolute top-of-book imbalance.

## Run Tests

```powershell
.\build\Release\order_book_tests.exe
```

## Run Benchmark

```powershell
.\build\bench_replay.exe 1000000
```

For a resume-scale benchmark:

```powershell
.\build\bench_replay.exe 10000000
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

## Next Extensions

- Add a backend collector for historical event storage
- Add replay controls for recorded real market sessions
- Add latency models, fees, queue position, and slippage assumptions
- Bind the engine to Python with `pybind11`
- Benchmark against a pure Python baseline on 10M+ synthetic events
