/*
 * context.h
 *
 *  Created on: Apr 23, 2011 ~ 2013
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

#ifndef PIONEER_SYSTEM_CONTEXT_H_
#define PIONEER_SYSTEM_CONTEXT_H_

#include <set>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace pioneer {
  namespace system {

    struct context {
      static std::string local_ip;

      static std::atomic<bool> system_quitting;

      // use atomic to avoid multi-thread problem
      static std::atomic<int> outside_node_count;
      static std::atomic<int> inside_node_count;

      // ip list
      static std::set<std::string> outside_ip_list;
      static std::set<std::string> inside_ip_list;

      // mutex for common usage for all context variables
      static std::mutex mutex;
    };

  } // system
} // pioneer

#endif /* PIONEER_SYSTEM_CONTEXT_H_ */
