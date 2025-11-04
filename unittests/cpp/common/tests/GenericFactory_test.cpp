/*
 * Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief This file is for testing the GenericFactory.h file.
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 25/11/04
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// common
#include <patterns/GenericFactory.h>
#include <Logger.h>
#include "SharedFlags.h"

// gtest
#include <gtest/gtest.h>

// std
#include <memory>
#include <string>

using namespace catena::patterns;

/**
 * Test fixture class for GenericFactory tests
 */
class GenericFactoryTest : public ::testing::Test {
protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        absl::SetFlag(&FLAGS_log_dir, UNITTEST_LOG_DIR);
        Logger::init("GenericFactoryTest");
    }

    static void TearDownTestSuite() {
    }
};

// Test product base class
class TestProduct {
public:
    TestProduct() = default;
    virtual ~TestProduct() = default;
    virtual int getValue() const = 0;
};

// Test product implementations
class ConcreteProductA : public TestProduct {
private:
    int _value;
public:
    ConcreteProductA(int value) : _value(value) {}
    int getValue() const override { return _value; }
    static TestProduct* make(int value) { return new ConcreteProductA(value); }
};

class ConcreteProductB : public TestProduct {
private:
    int _value;
public:
    ConcreteProductB(int value) : _value(value) {}
    int getValue() const override { return _value; }
    static TestProduct* make(int value) { return new ConcreteProductB(value); }
};

// Factory type alias for tests
using TestFactory = GenericFactory<TestProduct, std::string, int>;

/*
 * ============================================================================
 *                               GenericFactory tests
 * ============================================================================
 */

/**
 * TEST 1 - Register a product with the factory
 */
TEST_F(GenericFactoryTest, AddProduct_Success) {
    TestFactory& factory = TestFactory::getInstance();
    EXPECT_TRUE(factory.addProduct("test1_productA", ConcreteProductA::make));
}

/**
 * TEST 2 - Register duplicate product key should throw exception
 */
TEST_F(GenericFactoryTest, AddProduct_DuplicateKey) {
    TestFactory& factory = TestFactory::getInstance();
    factory.addProduct("test2_productB", ConcreteProductB::make);
    EXPECT_THROW(factory.addProduct("test2_productB", ConcreteProductB::make), std::runtime_error);
}

/**
 * TEST 3 - Create a product using makeProduct
 */
TEST_F(GenericFactoryTest, MakeProduct_Success) {
    TestFactory& factory = TestFactory::getInstance();
    // Only add if not already registered (since factory is singleton)
    if (!factory.canMake("test3_productA")) {
        factory.addProduct("test3_productA", ConcreteProductA::make);
    }
    
    std::unique_ptr<TestProduct> product = factory.makeProduct("test3_productA", 42);
    ASSERT_NE(product, nullptr);
    EXPECT_EQ(product->getValue(), 42);
}

/**
 * TEST 4 - Create product with non-existent key should throw exception
 */
TEST_F(GenericFactoryTest, MakeProduct_NonExistentKey) {
    TestFactory& factory = TestFactory::getInstance();
    EXPECT_THROW(factory.makeProduct("test4_nonExistent", 42), std::runtime_error);
}

/**
 * TEST 5 - Check if factory can make a product with canMake
 */
TEST_F(GenericFactoryTest, CanMake_Exists) {
    TestFactory& factory = TestFactory::getInstance();
    // Only add if not already registered (since factory is singleton)
    if (!factory.canMake("test5_productA")) {
        factory.addProduct("test5_productA", ConcreteProductA::make);
    }
    EXPECT_TRUE(factory.canMake("test5_productA"));
}

/**
 * TEST 6 - Check if factory cannot make a product that doesn't exist
 */
TEST_F(GenericFactoryTest, CanMake_NotExists) {
    TestFactory& factory = TestFactory::getInstance();
    EXPECT_FALSE(factory.canMake("test6_nonExistent"));
}

