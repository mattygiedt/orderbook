#pragma once

#include "orderbook/application_traits.h"
#include "quickfix/Application.h"
#include "quickfix/Message.h"

namespace orderbook::gateway {

class GatewayApplication : public FIX::Application {
 private:
  using EventType = orderbook::data::EventType;
  using EventData = orderbook::data::EventData;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;
  using EventDispatcherPtr = std::shared_ptr<EventDispatcher>;

 public:
  GatewayApplication(EventDispatcherPtr dispatcher)
      : dispatcher_(std::move(dispatcher)) {}

  auto onCreate(const FIX::SessionID& session_id) -> void override {
    spdlog::info("session created: {}", session_id.toString());
  }

  auto onLogon(const FIX::SessionID& session_id) -> void override {
    spdlog::info("session logon: {}", session_id.toString());
  }

  auto onLogout(const FIX::SessionID& session_id) -> void override {
    spdlog::info("session logout: {}", session_id.toString());
  }

  auto toAdmin(FIX::Message& message, const FIX::SessionID& /*unused*/)
      -> void override {
    spdlog::info("toAdmin: {}", message.toString());
  }

  auto fromAdmin(const FIX::Message& message, const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::RejectLogon) -> void override {
    spdlog::info("fromAdmin: {}", message.toString());
  }

  auto toApp(FIX::Message& message, const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::DoNotSend) -> void override {
    spdlog::info("toApp: {}", message.toString());
  }

  auto fromApp(const FIX::Message& message, const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::UnsupportedMessageType)
          -> void override {
    spdlog::info("fromApp: {}", message.toString());
  }

 private:
  EventDispatcherPtr dispatcher_;
};

}  // namespace orderbook::gateway
