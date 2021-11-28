#include "gtest/gtest.h"
#include "orderbook/application_traits.h"

using namespace orderbook::data;

class PoolFixture : public ::testing::Test {
 private:
 public:
  static auto SimpleTest() -> void {
    constexpr std::size_t kPoolSize = 4;
    using LimitOrderType = orderbook::data::PooledLimitOrder<kPoolSize>;
    using LimitOrderPtr = boost::intrusive_ptr<LimitOrderType>;
    using Pool = orderbook::data::ObjectPool<LimitOrderType,
                                             LimitOrderType::GetPoolSize()>;
    using List = std::list<LimitOrderPtr>;

    Pool& pool = Pool::Instance();
    ASSERT_TRUE(pool.Capacity() == kPoolSize);
    ASSERT_TRUE(pool.Capacity() == pool.Available());

    auto&& order = pool.MakeIntrusive();
    ASSERT_TRUE(1 == order->GetRef());
    ASSERT_TRUE(pool.Available() == (pool.Capacity() - 1));

    List list;
    list.insert(list.end(), order);
    ASSERT_TRUE(2 == order->GetRef());  // NOLINT

    list.clear();
    ASSERT_TRUE(1 == order->GetRef());
    ASSERT_TRUE(pool.Available() == (pool.Capacity() - 1));
  }
};

TEST_F(PoolFixture, simple_test) { SimpleTest(); }  // NOLINT
