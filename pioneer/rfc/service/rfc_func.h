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

#include <pioneer/rfc/rfc.h>

namespace pioneer {
  namespace rfc {

    using std::string;

    REGISTER_REMOTE_FUNC(detect_catalog, 100);
    REGISTER_REMOTE_FUNC(announce_catalog, 102);
    REGISTER_REMOTE_FUNC(cannounce_catalog, 103);
    REGISTER_REMOTE_FUNC(announce_inward_node, 104);
    REGISTER_REMOTE_FUNC(cannounce_inward_node, 105);
    REGISTER_REMOTE_FUNC(announce_outward_node, 106);
    REGISTER_REMOTE_FUNC(cannounce_outward_node, 107);
    REGISTER_REMOTE_FUNC(outward_node_quit, 108);
    REGISTER_REMOTE_FUNC(inward_node_quit, 109);

    REGISTER_REMOTE_FUNC(udp_test_received, 110);
    REGISTER_REMOTE_FUNC(start_udp_test, 111);
    REGISTER_REMOTE_FUNC(cstart_udp_test, 112);
    REGISTER_REMOTE_FUNC(stop_udp_test, 113);

    class rfc_func {
    public:

      static rfc_result detect_catalog(const string& source, rfc_context c);

      static rfc_result announce_inward_node(const string& ip, rfc_context c);

      static rfc_result cannounce_inward_node(const string& ip_list, rfc_context c);

      // execute on catalog only
      static rfc_result outward_node_quit(rfc_context c);

      // execute on catalog only
      static rfc_result inward_node_quit(rfc_context c);

      static rfc_result udp_test_received(int round, rfc_context c);

      static rfc_result cstart_udp_test(int rounds, int test_count, int interval, int rest_time, rfc_context c);

      static rfc_result start_udp_test(int rounds, int test_count, int interval, int rest_time, rfc_context c);

      static rfc_result stop_udp_test(rfc_context c);
    };

  } // rfc
} // pioneer

#endif /* RFC_SERVICE_RFC_FUNC_H_ */
