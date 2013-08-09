/*
 * rfc_func.h
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
#include <atlas/func_wrapper.h>
#include <pioneer/rfc/rfc.h>

namespace pioneer {
  namespace rfc {

    using std::string;

    class rfc_func {
    public:

      static rfc_result announce_inner_node(const string& ip, rfc_context c);

      static rfc_result cannounce_inner_node(const string& ip_list, rfc_context c);

      // execute on catalog only
      static rfc_result outer_node_quit(rfc_context c);

      // execute on catalog only
      static rfc_result inner_node_quit(rfc_context c);

      static rfc_result udp_test_received(int round, rfc_context c);

      static rfc_result cstart_udp_test(int rounds, int test_count, int interval, int rest_time, rfc_context c);

      static rfc_result start_udp_test(int rounds, int test_count, int interval, int rest_time, rfc_context c);

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

    using atlas::func_wrapper;

    class customer_dispatcher {
    public:

      static rfc_result dispatch(int fn_id, const std::string& message, const rfc_context& context) {
        std::istringstream iss(message);
        rfc_iarchive ia(iss);

        // LOG(INFO) << "dispatch " << method << " for " << context.session_id() << " from " << context.source_ip();

        rfc_result result(false); // assume i am not responsible to this function call
        switch (fn_id) {
        case fn_ids::announce_inner_node: {
          func_wrapper<decltype(rfc_func::announce_inner_node)> announce_inner_node(rfc_func::announce_inner_node, ia, context);
          result = announce_inner_node();
        }
        break;
        case fn_ids::cannounce_inner_node: {
          func_wrapper<decltype(rfc_func::cannounce_inner_node)> cannounce_inner_node(rfc_func::cannounce_inner_node, ia, context);
          result = cannounce_inner_node();
        }
        break;
        case fn_ids::udp_test_received: {
          func_wrapper<decltype(rfc_func::udp_test_received)> udp_test_received(rfc_func::udp_test_received, ia, context);
          result = udp_test_received();
        }
        break;
        case fn_ids::start_udp_test: {
          func_wrapper<decltype(rfc_func::start_udp_test)> start_udp_test(rfc_func::start_udp_test, ia, context);
          result = start_udp_test();
        }
        break;
        case fn_ids::cstart_udp_test: {
          func_wrapper<decltype(rfc_func::cstart_udp_test)> cstart_udp_test(rfc_func::cstart_udp_test, ia, context);
          result = cstart_udp_test();
        }
        break;
          default:
          // not be responsible to this rfc
          // DLOG(INFO) << "no method " << method;
        break;
        } // switch

        return result;
      }

    };

  } // rfc
} // pioneer

#endif /* RFC_SERVICE_RFC_FUNC_H_ */
