// /*
//  * Copyright 2025 Ross Video Ltd
//  *
//  * Redistribution and use in source and binary forms, with or without
//  * modification, are permitted provided that the following conditions are met:
//  *
//  * 1. Redistributions of source code must retain the above copyright notice,
//  * this list of conditions and the following disclaimer.
//  *
//  * 2. Redistributions in binary form must reproduce the above copyright notice,
//  * this list of conditions and the following disclaimer in the documentation
//  * and/or other materials provided with the distribution.
//  *
//  * 3. Neither the name of the copyright holder nor the names of its
//  * contributors may be used to endorse or promote products derived from this
//  * software without specific prior written permission.
//  *
//  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  * POSSIBILITY OF SUCH DAMAGE.
//  */

// #include "../../common/tests/CommonMockClasses.h"

// // REST
// #include "controllers/Connect.h"
// #include "SocketWriter.h"

// using namespace catena::common;
// using namespace catena::REST;

// // Fixture
// class RESTConnectTests : public ::testing::Test, public SocketHelper {
//   protected:
//     RESTConnectTests() : SocketHelper(&serverSocket, &clientSocket) {}

//     void SetUp() override {
//         // Redirecting cout to a stringstream for testing.
//         oldCout = std::cout.rdbuf(MockConsole.rdbuf());

//         // Creating Connect object.
//         EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
//         EXPECT_CALL(context, getSubscriptionManager()).Times(1).WillOnce(::testing::ReturnRef(subscriptionManager));
//         connect = Connect::makeOne(serverSocket, context, dm);
//     }

//     void TearDown() override {
//         std::cout.rdbuf(oldCout); // Restoring cout
//         // Cleanup code here
//         if (connect) {
//             delete connect;
//         }
//     }

//     std::stringstream MockConsole;
//     std::streambuf* oldCout;
    
//     MockSocketReader context;
//     MockDevice dm;
//     MockSubscriptionManager subscriptionManager;
//     catena::REST::ICallData* connect = nullptr;
// };

// /*
//  * ============================================================================
//  *                               Connect tests
//  * ============================================================================
//  * 
//  * TEST 1 - Creating a Connect object with makeOne.
//  */
// TEST_F(RESTConnectTests, Connect_create) {
//     // Making sure connect is created from the SetUp step.
//     ASSERT_TRUE(connect);
// }

// /* 
//  * TEST 2 - Normal case for Connect proceed() with socket open.
//  */
// TEST_F(RESTConnectTests, Connect_proceedNormal) {
//     // Setup expectations
//     EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(jwsToken));
//     EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
//     EXPECT_CALL(context, detailLevel()).Times(1).WillOnce(::testing::Return(catena::Device_DetailLevel_FULL));
//     EXPECT_CALL(dm, detail_level(catena::Device_DetailLevel_FULL)).Times(1);
//     EXPECT_CALL(dm, slot()).Times(1).WillOnce(::testing::Return(1));

//     // Call proceed
//     connect->proceed();

//     // Verify initial response
//     catena::PushUpdates expectedResponse;
//     expectedResponse.set_slot(1);
//     EXPECT_EQ(readResponse(), expectedResponse);
// }

// /* 
//  * TEST 3 - Connect proceed() with socket closed.
//  */
// TEST_F(RESTConnectTests, Connect_proceedSocketClosed) {
//     // Close the socket
//     serverSocket.close();

//     // Setup expectations
//     EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(jwsToken));
//     EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
//     EXPECT_CALL(context, detailLevel()).Times(1).WillOnce(::testing::Return(catena::Device_DetailLevel_FULL));
//     EXPECT_CALL(dm, detail_level(catena::Device_DetailLevel_FULL)).Times(1);

//     // Call proceed
//     connect->proceed();

//     // Verify no response is sent
//     EXPECT_EQ(readResponse(), "");
// }

// /* 
//  * TEST 4 - Connect proceed() with authorization enabled.
//  */
// TEST_F(RESTConnectTests, Connect_proceedWithAuthz) {
//     // Setup expectations
//     EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(jwsToken));
//     EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
//     EXPECT_CALL(context, detailLevel()).Times(1).WillOnce(::testing::Return(catena::Device_DetailLevel_FULL));
//     EXPECT_CALL(dm, detail_level(catena::Device_DetailLevel_FULL)).Times(1);
//     EXPECT_CALL(dm, slot()).Times(1).WillOnce(::testing::Return(1));

//     // Call proceed
//     connect->proceed();

//     // Verify initial response
//     catena::PushUpdates expectedResponse;
//     expectedResponse.set_slot(1);
//     EXPECT_EQ(readResponse(), expectedResponse);
// }

// /* 
//  * TEST 5 - Writing to console with Connect finish().
//  */
// TEST_F(RESTConnectTests, Connect_finish) {
//     // Calling finish and expecting the console output.
//     connect->finish();
//     // Verify console output contains finish message
//     ASSERT_TRUE(MockConsole.str().find("Connect[0] finished\n") != std::string::npos);
// } 