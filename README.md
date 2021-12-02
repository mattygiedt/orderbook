# orderbook
Call me Ishmael... Every software developer has that special problem domain that they keep getting drawn back to and cannot let go. Mine, sadly, (unfortunately?), is thinking about, executing against, and working with limit order exchange venues of varying degrees of sophistication and importance.

Put simply, the complex technology in play at the massive exchanges like the CME Globex, or even any full-scale limit order book that supports spread orders is mind-boggling. It simply _has_ to work, with complete redundancy, zero downtime and with deterministic execution. With that in mind ...

**Please Do Not Use This Code In Production**

This project is meant to be educational. It employs the tinyest of testing, which validates simple [add, modify, delete] limit order book semantics, and assumes best behavior of our market participants.

Still, I think it's pretty cool, and would love to hear your comments and / or suggested improvements to the `MapListContainer` and `IntrusiveListContainer` implementations!

### Building
Project uses VSCode `.devcontainer` support -- look in the directory for a sample `Dockerfile` if you want to roll your own.
```
git clone git@github.com:mattygiedt/orderbook.git
cd orderbook
code . <open project inside dev container>
mkdir build
cd build
cmake ..
make -j4 && ctest
```

### Running Client / Server Example
Start two separate `bash` shells, and run the client / server in each:
```
root@:/workspaces/orderbook/build# ./src/cpp/orderbook tcp://127.0.0.1:5555
```
```
root@:/workspaces/orderbook/build# ./src/cpp/client tcp://127.0.0.1:5555
```
Did your orders get filled?

### Limit Order Book Discussion
W.K. Selpf wrote a really cool [article](http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/) (2011!) on how to go about building a limit order book.
>The idea is to have a binary tree of Limit objects sorted by limitPrice, each of which is itself a doubly linked list of Order objects.  Each side of the book, the buy Limits and the sell Limits, should be in separate trees so that the inside of the book corresponds to the end and beginning of the buy Limit tree and sell Limit tree, respectively.  Each order is also an entry in a map keyed off idNumber, and each Limit is also an entry in a map keyed off limitPrice.

That's basically what you'll find here, written in my best attempt at modern C++20'ish concepts and syntax.

### Benchmarking
```
root@345a55e505f8:/workspaces/orderbook/build/benchmark/container# ./container_benchmark
2021-12-02T19:31:39+00:00
Running ./container_benchmark
Run on (6 X 2999.99 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 0.87, 0.66, 0.45
------------------------------------------------------------------------------------------------------
Benchmark                                                            Time             CPU   Iterations
------------------------------------------------------------------------------------------------------
BM_AddOrder<typename MapListTraits::BidContainerType>             5044 ns         5044 ns       126367
BM_AddOrder<typename MapListTraits::AskContainerType>             5137 ns         5137 ns       135224
BM_AddOrder<typename IntrusiveListTraits::BidContainerType>       4613 ns         4613 ns       151283
BM_AddOrder<typename IntrusiveListTraits::AskContainerType>       4476 ns         4476 ns       154313
```

### TODO
* `NewOrderSingle` message validation
* ~~`OrderCancelRequest` message validation~~
* ~~`OrderCancelReplaceRequest` message validation~~
* ~~Wrap `intrusive_ptr` around `LimitOrder` and allocate from an object_pool~~
* ~~Support `OrderCancelReplaceRequest` workflow~~
* Support cancel-on-disconnect
* Performance benchmarking
* FIX.4.2 Client Gateway
* Optimizing `orderbook::data` object members via flatbuffer API
* Marketdata?
