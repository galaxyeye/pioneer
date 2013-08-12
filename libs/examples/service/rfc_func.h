/*
 * rpc_func.h
 *
 *  Created on: Oct 10, 2011
 *      Author: Vincent Zhang, ivincent.zhang@gmail.com
 */

/*    Copyright 2011 ~ 2013 Vincent Zhang, ivincent.zhang@gmail.com
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef RFC_SERVICE_RFC_FUNC_H_
#define RFC_SERVICE_RFC_FUNC_H_

#include <string>

#include <boost/uuid/uuid.hpp>
#include <boost/serialization/vector.hpp>

#include <atlas/rpc/rpc.h>

namespace pioneer {
  namespace rpc {

    using std::string;
    using atlas::rpc::rpc_iarchive;
    using atlas::rpc::rf_wrapper;
    using atlas::rpc::rpc_result;
    using atlas::rpc::rpc_context;

    // the implementation of this class may include many files from the other modules
    // but the caller side just need the function signatures, so we split the declaration and the
    // implementation into different files
    class rpc_func {
    public:

      // illustrate a normal async, non-void return RPC
      // accumulate all the numbers in the vector and return a result to the client
      static rpc_result accumulate(const std::vector<int>& numbers, rpc_context c) noexcept;

      // illustrate a normal async, void return RPC
      static rpc_result announce_inner_node(const string& ip, rpc_context c) noexcept;

      // illustrate a multicast, async, void return RPC
      // c means cluster wide remote function call, we multicast the RPC, and execute it at each server
      static rpc_result cannounce_inner_node(const string& ip_list, rpc_context c) noexcept;

      static rpc_result udp_test_received(int round, rpc_context c) noexcept;

      static rpc_result cstart_udp_test(int rounds, int test_count, int interval, int rest_time, rpc_context c) noexcept;

      static rpc_result start_udp_test(int rounds, int test_count, int interval, int rest_time, rpc_context c) noexcept;
    };

    ATLAS_REGISTER_REMOTE_FUNC(accumulate, 121);

    ATLAS_REGISTER_REMOTE_FUNC(announce_inner_node, 104);
    ATLAS_REGISTER_REMOTE_FUNC(cannounce_inner_node, 105);

    ATLAS_REGISTER_REMOTE_FUNC(udp_test_received, 110);
    ATLAS_REGISTER_REMOTE_FUNC(start_udp_test, 111);
    ATLAS_REGISTER_REMOTE_FUNC(cstart_udp_test, 112);

  } // rpc
} // pioneer

#endif /* RFC_SERVICE_RFC_FUNC_H_ */
