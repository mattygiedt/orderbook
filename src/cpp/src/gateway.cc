#include <iostream>
#include <string>

#include "orderbook/data/event_types.h"
#include "orderbook/gateway/application.h"
#include "orderbook/gateway/orderbook_client.h"
#include "quickfix/FileLog.h"
#include "quickfix/FileStore.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/SocketAcceptor.h"
#include "quickfix/config.h"

class FixGateway {
 private:
  using GatewayApplication = orderbook::gateway::GatewayApplication;
  using OrderbookClient = orderbook::gateway::OrderbookClient;
  using EventType = orderbook::data::EventType;
  using EventData = orderbook::data::EventData;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;
  using EventDispatcherPtr = std::shared_ptr<EventDispatcher>;

 public:
  FixGateway(std::string config)
      : config_(std::move(config)),
        dispatcher_(std::make_shared<EventDispatcher>()),
        application_{dispatcher_},
        orderbook_client_{dispatcher_},
        acceptor_{nullptr} {}

  auto Initialize() -> void {
    FIX::SessionSettings settings(config_);
    FIX::FileStoreFactory store_factory(settings);
    FIX::ScreenLogFactory log_factory(settings);

    acceptor_ = std::make_unique<FIX::SocketAcceptor>(
        application_, store_factory, settings, log_factory);

    // TODO: orderbook configuration
    orderbook_client_.Connect("tcp://127.0.0.1:5555");
  }

  auto Start() -> void {
    orderbook_client_.ProcessMessages();
    acceptor_->block();
  }

  auto Stop() -> void {
    acceptor_->stop();
    orderbook_client_.Close();
  }

 private:
  std::string config_;
  EventDispatcherPtr dispatcher_;
  GatewayApplication application_;
  OrderbookClient orderbook_client_;
  std::unique_ptr<FIX::Acceptor> acceptor_;
};

auto main(int argc, char** argv) -> int {
  if (argc < 2) {
    std::cout << "usage: " << argv[0] << " FILE." << std::endl;
    return 1;
  }

  std::string file = argv[1];
  spdlog::info("gateway config file: {}", file);

  FixGateway gateway(file);
  gateway.Initialize();
  gateway.Start();
  gateway.Stop();

  return 0;
}
