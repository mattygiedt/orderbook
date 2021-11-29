#pragma once

#include <array>
#include <atomic>
#include <iostream>
#include <mutex>

#include "boost/intrusive_ptr.hpp"
#include "spdlog/spdlog.h"

namespace orderbook::data {

// clang-format off
template <typename PooledT>
concept IntrusiveConcept = requires(PooledT t) {
  PooledT::GetPoolSize();
  t.AddRef();
  t.DelRef();
  t.GetRef();
  t.Release();
};


template <typename T, std::size_t PoolSize = 2048>
requires IntrusiveConcept<T>
class IntrusivePool {  // clang-format on
 private:
  using Buffer = std::array<T*, PoolSize>;
  using Counter = std::size_t;
  using AtomicCounter = std::atomic<Counter>;

  IntrusivePool() {
    for (std::size_t i = 0; i < PoolSize; ++i) {
      Offer(new T());
    }
  }

 public:
  static constexpr std::size_t kPoolSize = PoolSize;

  auto Offer(T* p) -> void {
    Counter curr = idx_.load(std::memory_order_relaxed);

    if (curr == PoolSize) {
      delete p;
      spdlog::warn("IntrusivePool {} deleted overflow pointer",
                   typeid(T).name());
      return;
    }

    Counter next = curr + 1;
    while (!idx_.compare_exchange_weak(curr, next)) {
      // We could not swap next into idx_, the current value of
      // idx_ is now in curr.
      if (curr == PoolSize) {
        delete p;
        spdlog::warn("IntrusivePool {} deleted overflow pointer",
                     typeid(T).name());
        return;
      }

      // try again
      next = curr + 1;
    }

    // next has successfully been placed into idx_, curr holds the previous
    // idx_ value before the swap.
    buf_[curr] = p;
  }

  auto Take() -> T* {
    Counter curr = idx_.load(std::memory_order_relaxed);

    if (curr == 0) {
      spdlog::warn("IntrusivePool {} exhausted, capacity: {}", typeid(T).name(),
                   Capacity());
      return new T;
    }

    Counter next = curr - 1;
    while (!idx_.compare_exchange_weak(curr, next)) {
      // We could not swap next into idx_, the current value of
      // idx_ is now in curr.
      if (curr == 0) {
        spdlog::warn("IntrusivePool exhausted, capacity: {}", Capacity());
        return new T;
      }

      // try again
      next = curr - 1;
    }

    // Have we reached a new max_depth mark?
    Counter depth = Capacity() - next;
    if (max_depth_ < depth) {
      max_depth_ = depth;
      spdlog::warn("IntrusivePool {} max_depth: {}", typeid(T).name(),
                   max_depth_);
    }

    // next has successfully been placed into idx_
    return buf_[next];
  }

  auto MakeIntrusive() -> boost::intrusive_ptr<T> { return {Take(), false}; }

  constexpr auto Capacity() const -> std::size_t { return PoolSize; }
  auto Available() const -> Counter {
    Counter curr = idx_.load(std::memory_order_relaxed);
    return curr;
  }
  auto Depth() const -> Counter { return Capacity() - Available(); }
  auto MaxDepth() const -> Counter { return max_depth_; }

  static auto& Instance() {
    static IntrusivePool<T, PoolSize> instance;
    return instance;
  }

 private:
  Buffer buf_{};
  Counter max_depth_{0};
  AtomicCounter idx_{0};
};

template <typename T>
void intrusive_ptr_add_ref(T* p) {
  p->AddRef();
}

template <typename T>
void intrusive_ptr_release(T* p) {
  if (p->DelRef() == 0) {
    // Remember: ref_count_ is initialized to 1 in our data objects
    p->AddRef();
    p->Release();
  }
}

// clang-format off
template <typename PooledT>
concept IntrusiveListConcept = requires(PooledT t, std::size_t p) {
  PooledT::GetPoolSize();
  t.SetPos(p);
  t.Release();
};

template <typename T, std::size_t PoolSize = 2048>
requires IntrusiveListConcept<T>
class IntrusiveListPool {  // clang-format on
 private:
  using Buffer = std::array<T, PoolSize>;
  using Counter = std::size_t;
  using Stack = std::vector<Counter>;
  using AtomicCounter = std::atomic<Counter>;

  IntrusiveListPool() {
    vec_.resize(PoolSize);
    for (std::size_t i = 0; i < PoolSize; ++i) {
      vec_[i] = i;
      buf_[i].SetPos(i);
    }
    idx_ = PoolSize;
  }

 public:
  static constexpr std::size_t kPoolSize = PoolSize;

  auto Offer(const Counter& pos) -> void {
    if (pos >= PoolSize) {
      spdlog::error("IntrusiveListPool {} Offer({}) >= PoolSize {}",
                    typeid(T).name(), pos, PoolSize);
      return;
    }

    ++idx_;
    vec_.push_back(pos);
  }

  auto Take() -> T& {
    Counter curr = idx_.load(std::memory_order_relaxed);

    if (curr == 0) {
      spdlog::warn("IntrusiveListPool exhausted, capacity: {}", Capacity());

      T* t = new T;
      t->SetPos(PoolSize);
      return *t;
    }

    Counter next = curr - 1;
    while (!idx_.compare_exchange_weak(curr, next)) {
      // We could not swap next into idx_, the current value of
      // idx_ is now in curr.
      if (curr == 0) {
        spdlog::warn("IntrusiveListPool exhausted, capacity: {}", Capacity());

        T* t = new T;
        t->SetPos(PoolSize);
        return *t;
      }

      // try again
      next = curr - 1;
    }

    // next has successfully been placed into idx_, and curr is greater than
    // zero, which means there is still the ability to pop the vector.
    auto pos = vec_.back();
    vec_.pop_back();

    return buf_[pos];
  }

  constexpr auto Capacity() const -> std::size_t { return PoolSize; }
  auto Available() const -> Counter { return vec_.size(); }
  auto Depth() const -> Counter { return Capacity() - Available(); }

  static auto& Instance() {
    static IntrusiveListPool<T, PoolSize> instance;
    return instance;
  }

 private:
  Buffer buf_{};
  Stack vec_{};
  AtomicCounter idx_{0};
};

}  // namespace orderbook::data
