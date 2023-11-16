#include "testbasket.hpp"
#include "include/kbasket.hpp"
#include "gmock/gmock.h"

using ::testing::Return;

TestBasket::TestBasket() {};
TestBasket::~TestBasket() {};
void TestBasket::SetUp() {};
void TestBasket::TearDown() {};

TEST_F(TestBasket, isKSizeSetCorrectly)
{
    KBasketFAI basket{12};
    EXPECT_EQ(basket.size_k, 12);
}
