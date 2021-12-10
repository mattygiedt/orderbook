# orderbook
Call me Ishmael... Every software developer has that special problem domain that they keep getting drawn back to and cannot let go. Mine, sadly, (unfortunately?), is thinking about, executing against, and working with limit order exchange venues of varying degrees of sophistication and importance.

Put simply, the complex technology in play at the massive exchanges like the CME Globex, or even any full-scale limit order book that supports spread orders is mind-boggling. It simply _has_ to work, with complete redundancy, zero downtime and with deterministic execution. With that in mind ...

**Please Do Not Use This Code In Production**

This project is meant to be educational. It employs the tinyest of testing, which validates simple [add, modify, delete] limit order book semantics, and assumes best behavior of our market participants.

Still, I think it's pretty cool, and would love to hear your comments and / or suggested improvements to the `IntrusivePtr` and `IntrusiveList` order book implementations!

### Building
This project uses VSCode `.devcontainer` support -- look in the directory for a sample `Dockerfile` if you want to roll your own.

```
git clone git@github.com:mattygiedt/orderbook.git
cd orderbook
code . <open project inside dev container>
mkdir build
cd build
cmake ..
make -j4 && ctest
Test project /workspaces/orderbook/build
    Start 1: book_test_suite
1/6 Test #1: book_test_suite ..................   Passed    0.02 sec
    Start 2: container_test_suite
2/6 Test #2: container_test_suite .............   Passed    0.02 sec
    Start 3: data_test_suite
3/6 Test #3: data_test_suite ..................   Passed    0.00 sec
    Start 4: pool_test_suite
4/6 Test #4: pool_test_suite ..................   Passed    0.00 sec
    Start 5: book_benchmark_test_suite
5/6 Test #5: book_benchmark_test_suite ........   Passed    0.00 sec
    Start 6: container_benchmark_test_suite
6/6 Test #6: container_benchmark_test_suite ...   Passed    4.81 sec

100% tests passed, 0 tests failed out of 6

Total Test time (real) =   4.86 sec
```

### Running Client / Gateway / Orderbook

There are three binary executables created under `/src/cpp`:

* `client` -- Example application that sends FIX.4.2 messages to the order book
* `gateway` -- FIX.4.2 gateway application that proxies FIX and FlatBuffer between the client and the order book
* `orderbook` -- The order book application

Start three separate `bash` shells, and run the client / gateway / orderbook in each:
```
root@:/workspaces/orderbook/build# ./src/cpp/orderbook tcp://127.0.0.1:5555
```
```
root@:/workspaces/orderbook/build# ./src/cpp/gateway ../config/gateway.ini
```
```
root@:/workspaces/orderbook/build# ./src/cpp/client ../config/citadel.ini
```

### Limit Order Book Discussion

W.K. Selpf wrote a really cool [article](http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/) (2011!) on how to go about building a limit order book.

>The idea is to have a binary tree of Limit objects sorted by limitPrice, each of which is itself a doubly linked list of Order objects.  Each side of the book, the buy Limits and the sell Limits, should be in separate trees so that the inside of the book corresponds to the end and beginning of the buy Limit tree and sell Limit tree, respectively.  Each order is also an entry in a map keyed off idNumber, and each Limit is also an entry in a map keyed off limitPrice.

That's basically what you'll find here, written in my best attempt at modern C++20'ish concepts and syntax.

### Boost Intrusive
There are two limit order book containers:

* `IntrusivePtrContainer` -- read up on intrusive pointers [here](https://www.boost.org/doc/libs/1_78_0/libs/smart_ptr/doc/html/smart_ptr.html#intrusive_ptr).
* `IntrusiveListContainer` -- read up on the intrusive list data structure [here](https://www.boost.org/doc/libs/1_78_0/doc/html/intrusive.html).

Each implementation shares the same interface defined in the `ContainerConcept` and, frankly, I wish I could figure out how to provide each implementation with a common base class as there is way too much code duplication between the two.


### Benchmarking
```
root@345a55e505f8:/workspaces/orderbook/build/benchmark/container# ./container_benchmark
2021-12-02T20:35:57+00:00
Running ./container_benchmark
Run on (6 X 2999.99 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 0.54, 0.82, 0.61
------------------------------------------------------------------------------------------------------
Benchmark                                                            Time             CPU   Iterations
------------------------------------------------------------------------------------------------------
BM_AddOrder<typename MapListTraits::BidContainerType>             4643 ns         4642 ns       150282
BM_AddOrder<typename MapListTraits::AskContainerType>             4924 ns         4924 ns       147072
BM_AddOrder<typename IntrusiveListTraits::BidContainerType>       4197 ns         4197 ns       167427
BM_AddOrder<typename IntrusiveListTraits::AskContainerType>       4185 ns         4184 ns       162740
```

### TODO
* ~~`NewOrderSingle` message validation~~
* ~~`OrderCancelRequest` message validation~~
* ~~`OrderCancelReplaceRequest` message validation~~
* ~~Wrap `intrusive_ptr` around `LimitOrder` and allocate from an object_pool~~
* ~~Support `OrderCancelReplaceRequest` workflow~~
* ~~Support cancel-on-disconnect~~
* ~~Performance benchmarking~~
* ~~FIX.4.2 Client Gateway~~
* Optimizing `orderbook::data` object members via flatbuffer API
* Marketdata?
