/*
 * ip.h
 *
 *  Created on: Apr 23, 2012
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

#ifndef NET_IP_H_
#define NET_IP_H_

#include <arpa/inet.h>

#include <cstring>

namespace pioneer {

  class ip {
  public:

    static const unsigned MAX_TCP_PORT = 65535;

  public:

    static std::string get_ip_part(const std::string& ip_port) {
      return ip_port.substr(0, ip_port.find(":"));
    }

    static std::string get_port_part(const std::string& ip_port) {
      return ip_port.substr(ip_port.find(":") + 1);
    }

    static std::string get_host_name() {
      char hostname[32];

      if (gethostname(hostname, sizeof(hostname))) {
        return "";
      }

      return hostname;
    }

    static std::string get_ip_port(const sockaddr_in& addr) {
      char host[INET_ADDRSTRLEN] = "INVALID";
      ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof host);
      uint16_t port = be16toh(addr.sin_port);

      static const int ip_port_size = 128;
      char buf[ip_port_size];
      std::memset(buf, '\0', sizeof(buf));
      snprintf(buf, ip_port_size, "%s:%u", host, port);

      return buf;
    }
  };

} // pioneer

#endif /* NET_IP_H_ */
