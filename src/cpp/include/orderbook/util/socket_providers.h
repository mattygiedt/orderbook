#pragma once

#define ZMQ_BUILD_DRAFT_API 1

#include <csignal>
#include <thread>

#include "eventpp/eventdispatcher.h"
#include "spdlog/spdlog.h"
#include "zmq.hpp"

namespace orderbook::util {

class SocketMonitor : public zmq::monitor_t {
 private:
 public:
  constexpr static auto kMonitorTimeout{100};

  auto on_event_connected(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_connect_delayed(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_connect_retried(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_listening(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_bind_failed(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_accepted(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_accept_failed(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_closed(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_close_failed(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_disconnected(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_handshake_failed_no_detail(const zmq_event_t& event,
                                           const char* addr) -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_handshake_failed_protocol(const zmq_event_t& event,
                                          const char* addr) -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_handshake_failed_auth(const zmq_event_t& event,
                                      const char* addr) -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }
  auto on_event_handshake_succeeded(const zmq_event_t& event, const char* addr)
      -> void override {
    dispatcher_.dispatch(event.event, event, addr);
    dispatcher_.dispatch(ZMQ_EVENT_ALL, event, addr);
  }

  template <typename Callback>
  auto AddListener(const std::uint16_t event_type, Callback&& callback)
      -> void {
    dispatcher_.appendListener(event_type, callback);
  }

 private:
  eventpp::EventDispatcher<std::uint16_t, void(const zmq_event_t&, const char*)>
      dispatcher_;
};

namespace internal {
std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }
}  // namespace internal

template <zmq::socket_type ZmqSocketType>
struct BaseProvider {
 private:
  using EventVector = std::vector<zmq::poller_event<>>;

 public:
  static constexpr zmq::send_flags kNone = zmq::send_flags::none;
  static constexpr std::chrono::milliseconds kPollTimeout{100};

  template <typename MessageCallback>
  auto ProcessMessages(MessageCallback&& callback) -> void {
    if (!running_) {
      spdlog::warn("ProcessMessages: running == false");
      return;
    }

    zmq::poller_t poller;
    poller.add(socket_, zmq::event_flags::pollin);
    EventVector events{poller.size()};

    while (running_) {
      const int num_events = poller.wait_all(events, kPollTimeout);
      for (int i = 0; i < num_events; ++i) {
        zmq::message_t msg;
        auto res = events[i].socket.recv(msg, zmq::recv_flags::dontwait);

        if (res) {
          callback(std::move(msg));
        }
      }
    }
  }

  template <typename MonitorCallback>
  auto Monitor(MonitorCallback&& callback,
               const std::uint16_t event_type = ZMQ_EVENT_ALL) -> void {
    socket_monitor_.AddListener(event_type,
                                std::forward<MonitorCallback>(callback));
  }

  auto Close() -> void {
    running_ = false;
    if (monitor_thr_.joinable()) {
      monitor_thr_.join();
    }
    socket_.close();
  }

  auto Socket() -> zmq::socket_ref { return socket_ref_; }

  constexpr auto SocketType() -> zmq::socket_type { return ZmqSocketType; }

 protected:
  BaseProvider(const bool monitor_flag)
      : running_{true},
        context_{1},
        socket_{context_, ZmqSocketType},
        socket_ref_{socket_} {
    std::signal(SIGTERM, internal::signal_handler);
    internal::shutdown_handler = [&](int /*unused*/) { Close(); };

    if (monitor_flag) {
      monitor_thr_ = std::thread([&]() { MonitorSockets(); });
    }
  }

  ~BaseProvider() { Close(); }

  auto MonitorSockets() -> void {
    socket_monitor_.init(socket_, "inproc://monitor");
    while (running_) {
      socket_monitor_.check_event(SocketMonitor::kMonitorTimeout);
    }
  }

  std::atomic<bool> running_;
  SocketMonitor socket_monitor_;
  zmq::context_t context_;
  zmq::socket_t socket_;
  zmq::socket_ref socket_ref_;
  std::thread monitor_thr_;
};

class ClientSocketProvider : public BaseProvider<zmq::socket_type::client> {
 public:
  ClientSocketProvider(const bool monitor_flag = true)
      : BaseProvider<zmq::socket_type::client>(monitor_flag) {}

  auto Connect(const std::string& addr) -> void {
    spdlog::info("socket_.connect({})", addr);
    socket_.connect(addr);
    running_ = true;
  }

  auto SendMessage(const std::string& str, const zmq::send_flags flag = kNone)
      -> zmq::send_result_t {
    return socket_.send(zmq::buffer(str), flag);
  }

  auto SendFlatBuffer(const std::uint8_t* buf, const std::size_t size,
                      const zmq::send_flags flag = zmq::send_flags::dontwait)
      -> zmq::send_result_t {
    zmq::message_t msg{buf, size};
    return socket_.send(msg, flag);
  }
};

class ServerSocketProvider : public BaseProvider<zmq::socket_type::server> {
 public:
  ServerSocketProvider(const bool monitor_flag = true)
      : BaseProvider<zmq::socket_type::server>(monitor_flag) {}

  auto Bind(const std::string& addr) -> void {
    spdlog::info("socket_.bind({})", addr);
    socket_.bind(addr);
  }

  auto SendMessage(const std::string& str, const std::uint32_t routing_id,
                   const zmq::send_flags flag = kNone) -> zmq::send_result_t {
    zmq::message_t msg{str};
    msg.set_routing_id(routing_id);
    return socket_.send(msg, flag);
  }

  auto SendFlatBuffer(const std::uint8_t* buf, const std::size_t& size,
                      const std::uint32_t& routing_id,
                      const zmq::send_flags flag = zmq::send_flags::dontwait)
      -> zmq::send_result_t {
    spdlog::info("SendFlatBuffer: sz {}, dest {}", size, routing_id);
    zmq::message_t msg{buf, size};
    msg.set_routing_id(routing_id);
    return socket_.send(msg, flag);
  }
};

class RadioSocketProvider : public BaseProvider<zmq::socket_type::radio> {
 public:
  RadioSocketProvider(const bool monitor_flag = true)
      : BaseProvider<zmq::socket_type::radio>(monitor_flag) {}

  auto Connect(const std::string& addr) -> void {
    spdlog::info("socket_.connect({})", addr);
    socket_.connect(addr);
  }

  auto SendMessage(const std::string& str, const std::string& group,
                   const zmq::send_flags flag = kNone) -> zmq::send_result_t {
    zmq::message_t msg{str};
    msg.set_group(group.c_str());
    return socket_.send(msg, flag);
  }
};

class DishSocketProvider : public BaseProvider<zmq::socket_type::dish> {
 public:
  DishSocketProvider(const bool monitor_flag = true)
      : BaseProvider<zmq::socket_type::dish>(monitor_flag) {}

  auto Join(const std::string& group) -> void {
    spdlog::info("socket_.join({})", group);
    socket_.join(group.c_str());
  }

  auto Bind(const std::string& addr) -> void {
    spdlog::info("socket_.bind({})", addr);
    socket_.bind(addr);
  }
};

class SubSocketProvider : public BaseProvider<zmq::socket_type::sub> {
 public:
  SubSocketProvider(const bool monitor_flag = true)
      : BaseProvider<zmq::socket_type::sub>(monitor_flag) {}

  auto Subscribe(const std::string& group) -> void {
    spdlog::info("socket_.subscribe({})", group);
    socket_.set(zmq::sockopt::subscribe, group.c_str());
  }

  auto Connect(const std::string& addr) -> void {
    spdlog::info("socket_.connect({})", addr);
    socket_.connect(addr);
  }
};

class PubSocketProvider : public BaseProvider<zmq::socket_type::pub> {
 public:
  PubSocketProvider(const bool monitor_flag = true)
      : BaseProvider<zmq::socket_type::pub>(monitor_flag) {}

  auto Bind(const std::string& addr) -> void {
    spdlog::info("socket_.bind({})", addr);
    socket_.bind(addr);
  }

  auto SendMessage(const std::string& str, const zmq::send_flags flag = kNone)
      -> zmq::send_result_t {
    zmq::message_t msg{str};
    return socket_.send(msg, flag);
  }
};

class PullSocketProvider : public BaseProvider<zmq::socket_type::pull> {
 public:
  PullSocketProvider(const bool monitor_flag = true)
      : BaseProvider<zmq::socket_type::pull>(monitor_flag) {}

  auto Subscribe(const std::string& group) -> void {
    spdlog::info("socket_.subscribe({})", group);
    socket_.set(zmq::sockopt::subscribe, group.c_str());
  }

  auto Connect(const std::string& addr) -> void {
    spdlog::info("socket_.connect({})", addr);
    socket_.connect(addr);
  }
};

class PushSocketProvider : public BaseProvider<zmq::socket_type::push> {
 public:
  PushSocketProvider(const bool monitor_flag = true)
      : BaseProvider<zmq::socket_type::push>(monitor_flag) {}

  auto Bind(const std::string& addr) -> void {
    spdlog::info("socket_.bind({})", addr);
    socket_.bind(addr);
  }

  auto SendMessage(const std::string& str, const zmq::send_flags flag = kNone)
      -> zmq::send_result_t {
    zmq::message_t msg{str};
    return socket_.send(msg, flag);
  }
};

enum class PairSocketType : std::uint8_t { CLIENT, SERVER };

template <PairSocketType Type>
class PairSocketProvider : public BaseProvider<zmq::socket_type::pair> {
 public:
  PairSocketProvider(const bool monitor_flag = true)
      : BaseProvider<zmq::socket_type::pair>(monitor_flag) {}

  auto CreateHandle(const std::string& addr) -> void {
    if constexpr (Type == PairSocketType::CLIENT) {
      spdlog::info("socket_.connect({})", addr);
      socket_.connect(addr);
    }
    if constexpr (Type == PairSocketType::SERVER) {
      spdlog::info("socket_.bind({})", addr);
      socket_.bind(addr);
    }
  }

  auto SendMessage(const std::string& str, const zmq::send_flags flag = kNone)
      -> zmq::send_result_t {
    zmq::message_t msg{str};
    return socket_.send(msg, flag);
  }
};

}  // namespace orderbook::util
