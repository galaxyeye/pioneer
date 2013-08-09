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

#include <deque>

#include <glog/logging.h>
#include <atlas/func_wrapper.h>

#include <pioneer/rfc/detail/rfc.h>
#include <pioneer/system/context.h>

namespace pioneer {
  namespace rfc {

    using atlas::func_wrapper;

    // dispatchers
    typedef std::function<rfc_result(int, const std::string&, const rfc_context&)> dispatcher_type;

    class builtin_dispatcher {
    public:

      static rfc_result dispatch(int fn_id, const std::string& message, const rfc_context& context) {
        std::istringstream iss(message);
        rfc_iarchive ia(iss);

        // LOG(INFO) << "dispatch " << method << " for " << context.session_id() << " from " << context.source_ip();

        rfc_result result(false);
        switch (fn_id) {
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
          // not be responsible to this rfc
          // DLOG(INFO) << "no method " << method;
        break;
        } // switch

        return result;
      }

    };

    class dispatcher_chain {
    public:

      dispatcher_chain() {
        _dispatchers.push_front(std::bind(builtin_dispatcher::dispatch));
      }

    public:

      // we invoke the dispatcher earlier if he comes later
      // TODO : employ priority queue
      static void register_dispatcher(dispatcher_type dispatcher) {
        _dispatchers.push_front(dispatcher);
      }

      static rfc_result dispatch(int fn_id, const std::string& message, const rfc_context& context) {
        std::istringstream iss(message);
        rfc_iarchive ia(iss);

        for (const dispatcher_type& dispatcher : _dispatchers) {
          rfc_result result = dispatcher(fn_id, message, context);

          if (!result) return nullptr; // null result means no response
          else if (result.is_final()) return result;
        }

        return nullptr;
      }

      static std::deque<dispatcher_type> _dispatchers;
    };

    std::deque<dispatcher_type> dispatcher_chain::_dispatchers;

  } // rfc
} // pioneer

#endif // RFC_DISPATCHER_H_
