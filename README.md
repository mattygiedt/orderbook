# orderbook
Call me Ishmael... Every software developer as that special problem domain that they keep getting drawn back to and cannot let go. Mine, sadly, unfortunately, is thinking about, building, and working around limit order books. The technology in play at the massive exchanges like the CME Globex or any spread order book is simply mind-boggling. It simply _has_ to work, with complete redundancy, zero downtime over the span of trading hours, and with deterministic execution.

**Do Not Use This Code In Production**

This project is meant to be educational. It employs the tinyest of testing, which validates simple limit order book semantics, and assumes best behavior of our market participants.

Still, I think it's pretty cool, and would love to hear your comments and / or suggested improvements to the `MapListContainer`!

### Building
Project uses VSCode `.devcontainer` support -- look in the directory for a sample `Dockerfile` if you want to roll your own.
```
git clone git@github.com:mattygiedt/orderbook.git
cd orderbook
code . <open inside dev container>
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

### TODO
* `NewOrderSingle` message validation
* `OrderCancelRequest` message validation
* Wrap `intrusive_ptr` around `LimitOrder` and allocate from an object_pool
* Support `OrderCancelReplaceRequest` workflow
* Support cancel-on-disconnect
* Performance benchmarking
* FIX.4.2 Client Gateway
* Optimizing `orderbook::data` object members via flatbuffer API
* Marketdata?
