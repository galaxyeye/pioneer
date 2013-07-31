/*
 *  dispatcher.h
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

#ifndef RFC_DISPATCHER_H_
#define RFC_DISPATCHER_H_

#include <glog/logging.h>
#include <atlas/func_wrapper.h>

#include <pioneer/rfc/rfc.h>
#include <pioneer/rfc/service/rfc_func.h>
#include <pioneer/system/context.h>

namespace pioneer {
  namespace rfc {

    using atlas::func_wrapper;

    // dispatchers
    typedef std::function<std::pair<rfc_result, bool>(int, const std::string&, const rfc_context&)> dispatcher_type;

    class dispatcher_chain : public atlas::singleton<dispatcher_chain> {
    public:

      void register_dispatcher(const dispatcher_type& dispatcher, int) {

      }

    private:

      std::vector<dispatcher_type> _dispatchers;
    };

    class builtin_dispatcher {
    public:

      static std::pair<rfc_result, bool> dispatch(int fn_id, const std::string& message, const rfc_context& context) {
        std::istringstream iss(message);
        rfc_iarchive ia(iss);

        // LOG(INFO) << "dispatch " << method << " for " << context.session_id() << " from " << context.source_ip();

        switch (fn_id) {
        case fn_ids::announce_inward_node: {
          func_wrapper<decltype(rfc_func::announce_inward_node)> announce_inward_node(rfc_func::announce_inward_node, ia, context);
          return announce_inward_node();
        }
        break;
        case fn_ids::cannounce_inward_node: {
          func_wrapper<decltype(rfc_func::cannounce_inward_node)> cannounce_inward_node(rfc_func::cannounce_inward_node, ia, context);
          return cannounce_inward_node();
        }
        break;
        case fn_ids::udp_test_received: {
          func_wrapper<decltype(rfc_func::udp_test_received)> udp_test_received(rfc_func::udp_test_received, ia, context);
          return udp_test_received();
        }
        break;
        case fn_ids::start_udp_test: {
          func_wrapper<decltype(rfc_func::start_udp_test)> start_udp_test(rfc_func::start_udp_test, ia, context);
          return start_udp_test();
        }
        break;
        case fn_ids::cstart_udp_test: {
          func_wrapper<decltype(rfc_func::cstart_udp_test)> cstart_udp_test(rfc_func::cstart_udp_test, ia, context);
          return cstart_udp_test();
        }
        break;
        case fn_ids::stop_udp_test: {
          func_wrapper<decltype(rfc_func::stop_udp_test)> stop_udp_test(rfc_func::stop_udp_test, ia, context);
          return stop_udp_test();
        }
        break;
        case fn_ids::resume_thread: {
          func_wrapper<decltype(builtin_rfc::resume_thread)> resume_thread(builtin_rfc::resume_thread, ia, context);
          return resume_thread();
        }
        break;
        case fn_ids::resume_task: {
          func_wrapper<decltype(builtin_rfc::resume_task)> resume_task(builtin_rfc::resume_task, ia, context);
          return resume_task();
        }
        break;
          default:
          // DLOG(INFO) << "no method " << method;
        break;
        } // switch

        return std::make_pair(nullptr, false);
      }

    };

  } // rfc
} // pioneer

#endif // RFC_DISPATCHER_H_
