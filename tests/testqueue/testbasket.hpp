#include "gtest/gtest.h"

class TestBasket : public ::testing::Test
{
protected:
    TestBasket();
    virtual ~TestBasket();
    virtual void SetUp();
    virtual void TearDown();
};
