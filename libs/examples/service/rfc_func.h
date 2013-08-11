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
#include <atlas/rpc/rpc.h>

namespace pioneer {
  namespace rpc {

    using std::string;
    using atlas::rpc::rpc_result;
    using atlas::rpc::rpc_context;

    // the implementation of this class may include many files in the other modules, for example, database access module
    // but the signature of the functions should be used by the client code, so just split the declaration and the
    // implementation
    class rpc_func {
    public:

      static rpc_result announce_inner_node(const string& ip, rpc_context c) noexcept;

      static rpc_result cannounce_inner_node(const string& ip_list, rpc_context c) noexcept;

      // execute on catalog only
      static rpc_result outer_node_quit(rpc_context c) noexcept;

      // execute on catalog only
      static rpc_result inner_node_quit(rpc_context c) noexcept;

      static rpc_result udp_test_received(int round, rpc_context c) noexcept;

      static rpc_result cstart_udp_test(int rounds, int test_count, int interval, int rest_time, rpc_context c) noexcept;

      static rpc_result start_udp_test(int rounds, int test_count, int interval, int rest_time, rpc_context c) noexcept;

    };

    REGISTER_REMOTE_FUNC(detect_catalog, 100);
    REGISTER_REMOTE_FUNC(announce_catalog, 102);
    REGISTER_REMOTE_FUNC(cannounce_catalog, 103);
    REGISTER_REMOTE_FUNC(announce_inner_node, 104);
    REGISTER_REMOTE_FUNC(cannounce_inner_node, 105);
    REGISTER_REMOTE_FUNC(announce_outer_node, 106);
    REGISTER_REMOTE_FUNC(cannounce_outer_node, 107);
    REGISTER_REMOTE_FUNC(outer_node_quit, 108);
    REGISTER_REMOTE_FUNC(inner_node_quit, 109);

    REGISTER_REMOTE_FUNC(udp_test_received, 110);
    REGISTER_REMOTE_FUNC(start_udp_test, 111);
    REGISTER_REMOTE_FUNC(cstart_udp_test, 112);
    REGISTER_REMOTE_FUNC(stop_udp_test, 113);

  } // rpc
} // pioneer

#endif /* RFC_SERVICE_RFC_FUNC_H_ */
