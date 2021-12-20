#include "gtest/gtest.h"

class TestQueue : public ::testing::Test
{
protected:
    TestQueue();
    virtual ~TestQueue();
    virtual void SetUp();
    virtual void TearDown();
};
