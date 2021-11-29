#pragma once

#include "boost/intrusive/list.hpp"
#include "boost/intrusive_ptr.hpp"
#include "orderbook/data/data_types.h"
#include "orderbook/data/object_pool.h"

/**
 * This is the object that represents resting orders, stored inside the
 * limit order book.
 */

namespace orderbook::data {
class LimitOrder : public BaseData {
 private:
 public:
  LimitOrder() : BaseData() {}

  auto operator<(const LimitOrder& o) const -> bool {
    return order_price_ < o.order_price_;
  }

  auto operator>(const LimitOrder& o) const -> bool {
    return order_price_ > o.order_price_;
  }
};

template <std::size_t PoolSize = 2048>
class IntrusiveLimitOrder : public LimitOrder {
 private:
 public:
  static constexpr std::size_t GetPoolSize() { return PoolSize; }

  IntrusiveLimitOrder() : LimitOrder(), ref_count_{1} {}

  auto AddRef() -> void { ++ref_count_; }
  auto DelRef() -> std::size_t { return --ref_count_; }
  auto GetRef() const -> std::size_t { return ref_count_; }
  auto Release() -> void {
    using Object = IntrusiveLimitOrder<PoolSize>;
    using ObjectPool = orderbook::data::IntrusivePool<Object, PoolSize>;
    ObjectPool::Instance().Offer(this);
  }

 private:
  std::size_t ref_count_;
};

template <std::size_t PoolSize = 2048>
class IntrusiveListLimitOrder : public LimitOrder,
                                public boost::intrusive::list_base_hook<> {
 private:
 public:
  static constexpr std::size_t GetPoolSize() { return PoolSize; }

  IntrusiveListLimitOrder() : LimitOrder() {}

  auto SetPos(const std::size_t& pos) -> void { pos_ = pos; }
  auto Release() -> void {
    if (pos_ == PoolSize) {
      delete this;
    }

    using Object = IntrusiveListLimitOrder<PoolSize>;
    using ObjectPool = orderbook::data::IntrusiveListPool<Object, PoolSize>;
    ObjectPool::Instance().Offer(pos_);
  }

 private:
  std::size_t pos_{0};
};

}  // namespace orderbook::data
