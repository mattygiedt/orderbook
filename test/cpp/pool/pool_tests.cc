#include "gtest/gtest.h"
#include "orderbook/application_traits.h"

using namespace orderbook::data;

struct Foo {
  int a{1};
  int b{2};
  int c{3};
  auto Release() { return this; }
};

class PoolFixture : public ::testing::Test {
 private:
 public:
  static auto IntrusiveTest() -> void {
    constexpr std::size_t kPoolSize = 4;
    using LimitOrderType = orderbook::data::IntrusiveLimitOrder<kPoolSize>;
    using LimitOrderPtr = boost::intrusive_ptr<LimitOrderType>;
    using Pool = orderbook::data::IntrusivePool<LimitOrderType,
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

  static auto IntrusiveListTest() -> void {
    constexpr std::size_t kPoolSize = 4;
    using LimitOrderType = orderbook::data::IntrusiveListLimitOrder<kPoolSize>;
    using Pool =
        orderbook::data::IntrusiveListPool<LimitOrderType,
                                           LimitOrderType::GetPoolSize()>;
    using List = boost::intrusive::list<LimitOrderType>;

    Pool& pool = Pool::Instance();
    ASSERT_TRUE(pool.Capacity() == kPoolSize);
    ASSERT_TRUE(pool.Capacity() == pool.Available());

    List list;

    auto& order = pool.Take();
    auto iter = list.insert(list.end(), order);

    ASSERT_TRUE(pool.Available() == (pool.Capacity() - 1));
    ASSERT_TRUE(&list.front() == &order);

    list.erase(iter);
    ASSERT_TRUE(list.empty());

    order.Release();
    ASSERT_TRUE(pool.Capacity() == pool.Available());
  }

  static auto PointerTest() -> void {
    constexpr std::size_t kPoolSize = 4;
    std::array<Foo*, kPoolSize> buf;

    for (std::size_t i = 0; i < kPoolSize; ++i) {
      buf[i] = new Foo;
    }

    auto foo = *buf[0];
    auto& foo_ref = *buf[0];

    ASSERT_FALSE(&foo == &foo_ref);

    foo.a = 42;  // NOLINT

    ASSERT_TRUE(foo.a == 42);  // NOLINT
    ASSERT_FALSE(foo.a == foo_ref.a);

    foo_ref.a = 42;  // NOLINT
    ASSERT_TRUE(&foo_ref == buf[0]);
    ASSERT_TRUE(foo_ref.a == buf[0]->a);
    ASSERT_TRUE(foo_ref.Release() == buf[0]);
  }

  static auto ArrayTest() -> void {
    constexpr std::size_t kPoolSize = 4;
    std::array<Foo, kPoolSize> buf;

    auto foo = buf[0];
    auto& foo_ref = buf[0];

    ASSERT_FALSE(&foo == &foo_ref);

    foo.a = 42;  // NOLINT

    ASSERT_TRUE(foo.a == 42);  // NOLINT
    ASSERT_FALSE(foo.a == foo_ref.a);

    foo_ref.a = 42;  // NOLINT
    ASSERT_TRUE(&foo_ref == &buf[0]);
    ASSERT_TRUE(foo_ref.a == buf[0].a);
  }
};

TEST_F(PoolFixture, intrusive_test) { IntrusiveTest(); }           // NOLINT
TEST_F(PoolFixture, intrusive_list_test) { IntrusiveListTest(); }  // NOLINT
TEST_F(PoolFixture, pointer_test) { PointerTest(); }               // NOLINT
TEST_F(PoolFixture, array_test) { ArrayTest(); }                   // NOLINT
