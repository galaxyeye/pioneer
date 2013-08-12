/*
 * rpc_func.ipp
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

#ifndef RFC_SERVICE_RFC_FUNC_CLIENT_H_
#define RFC_SERVICE_RFC_FUNC_CLIENT_H_

namespace pioneer {
  namespace rpc {

    // dummy implementation.

    // empty functions only, for client side, we just need the function's signature,
    // in order to avoid linkage error, just provide empty implementations

    rpc_result rpc_func::announce_inner_node(const string& ip, rpc_context c) noexcept {
      return nullptr;
    }

    rpc_result rpc_func::cannounce_inner_node(const string& ip_list, rpc_context c) noexcept {
      return nullptr;
    }

    rpc_result rpc_func::accumulate(const std::vector<int>& numbers, rpc_context c) noexcept {
      return nullptr;
    }

    rpc_result rpc_func::udp_test_received(int round, rpc_context c) noexcept {
      return nullptr;
    }

    rpc_result rpc_func::cstart_udp_test(int rounds, int test_count, int interval, int rest_time, rpc_context c) noexcept {
      return nullptr;
    }

    rpc_result rpc_func::start_udp_test(int rounds, int test_count, int interval, int rest_time, rpc_context c) noexcept {
      return nullptr;
    }

  } // rpc
} // mevo

#endif // RFC_SERVICE_RFC_FUNC_CLIENT_H_

