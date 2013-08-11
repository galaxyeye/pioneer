/*
 * status.h
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

#ifndef PIONEER_SYSTEM_STATUS_H_
#define PIONEER_SYSTEM_STATUS_H_

#include <atomic>
#include <array>
#include <ctime>

namespace pioneer {
  namespace system {

    const static int max_udp_test_rounds = 50;

    struct status {
      static std::atomic<long> last_check_time;

      // mcast
      static std::atomic<unsigned long long> mcast_sent;
      static std::atomic<unsigned long long> mcast_received;

      // connections
      static std::atomic<unsigned long long> active_outer_connections;
      static std::atomic<unsigned long long> failed_outer_connections;

      static std::atomic<unsigned long long> active_inner_connections;
      static std::atomic<unsigned long long> failed_inner_connections;

      // udp_test
      static std::atomic<unsigned long long> test_rounds;
      static std::array<std::atomic<unsigned long long>, max_udp_test_rounds> udp_test_interval;
      static std::array<std::atomic<unsigned long long>, max_udp_test_rounds> udp_test_sent;
      static std::array<std::atomic<unsigned long long>, max_udp_test_rounds> udp_test_received;
      static std::array<std::atomic<unsigned long long>, max_udp_test_rounds> good_ack;

    };

    std::atomic<long> status::last_check_time = ATOMIC_VAR_INIT(::time(0));

    // mcast
    std::atomic<unsigned long long> status::mcast_sent = ATOMIC_VAR_INIT(0);
    std::atomic<unsigned long long> status::mcast_received = ATOMIC_VAR_INIT(0);

    // connections
    std::atomic<unsigned long long> status::active_outer_connections = ATOMIC_VAR_INIT(0);
    std::atomic<unsigned long long> status::failed_outer_connections = ATOMIC_VAR_INIT(0);

    std::atomic<unsigned long long> status::active_inner_connections = ATOMIC_VAR_INIT(0);
    std::atomic<unsigned long long> status::failed_inner_connections = ATOMIC_VAR_INIT(0);

    // udp_test
    std::atomic<unsigned long long> status::test_rounds = ATOMIC_VAR_INIT(0);
    std::array<std::atomic<unsigned long long>, max_udp_test_rounds> status::udp_test_interval;
    std::array<std::atomic<unsigned long long>, max_udp_test_rounds> status::udp_test_sent;
    std::array<std::atomic<unsigned long long>, max_udp_test_rounds> status::udp_test_received;
    std::array<std::atomic<unsigned long long>, max_udp_test_rounds> status::good_ack;

  } // system
} // pioneer

#endif /* PIONEER_SYSTEM_STATUS_H_ */
