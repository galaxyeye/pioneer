/*
 * system_context.ipp
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

#ifndef PIONEER_SYSTEM_CONTEXT_IPP_
#define PIONEER_SYSTEM_CONTEXT_IPP_

// NOTICE : this file must be included after all .h files in main.cpp

namespace pioneer {
  namespace system {

    std::string context::local_ip;

    std::atomic<bool> context::system_quitting = ATOMIC_VAR_INIT(false);

    // TODO : check whether we need this
    std::atomic<int> context::outside_connection_count = ATOMIC_VAR_INIT(0);
    std::atomic<int> context::inside_connection_count = ATOMIC_VAR_INIT(0);

    std::set<std::string> context::outside_ip_list;
    std::set<std::string> context::inside_ip_list;

    std::atomic<bool> context::udp_test_enabled = ATOMIC_VAR_INIT(true);

    std::mutex context::mutex;

  } // system
} // pioneer

#endif // PIONEER_SYSTEM_CONTEXT_IPP_
