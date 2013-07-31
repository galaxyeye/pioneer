/*
 * client.h
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

#ifndef PIONEER_RFC_CLIENTS_H_
#define PIONEER_RFC_CLIENTS_H_

#include <pioneer/net/multicast.h>
#include <pioneer/rfc/rfc.h>
#include <pioneer/net/net.h>

namespace pioneer {
  namespace rfc {

    using boost::uuids::nil_uuid;
    namespace mn = muduo::net;

    class bcast_client : public rfc::remote_caller {
    public:

      bcast_client(rfc::client_type ct = rfc::client_type::any_client) : remote_caller(ct)
      {}

      virtual ~bcast_client() {}

    protected:

      virtual void send(const char* message, size_t sz) {
        // TODO : employ a broadcast client
      }
    };

    class mcast_client : public rfc::remote_caller {
    public:

      mcast_client(rfc::client_type client = rfc::any_client, int resp_expect = 1)
        : remote_caller(client, resp_expect) {}

      virtual ~mcast_client() {}

    public:

      virtual void send(const char* message, size_t sz) {
        net::mcast_client::ref().send(message, sz);
      }
    };

    class p2p_client : public rfc::remote_caller {
    public:

      p2p_client(rfc::client_type client, const std::string& ip) : rfc::remote_caller(client), _client(client), _ip(ip) {}

      virtual ~p2p_client() {}

    public:

      virtual void send(const char* message, size_t size) {
        mn::TcpConnectionPtr conn;

        if (rfc::client_type::inward_client & _client) {
          conn = net::inward_connection_pool::ref().take(_ip);
        }

        if (!conn && (rfc::client_type::outward_client & _client)) {
          conn = net::outward_connection_pool::ref().take(_ip);
        }

        if (!conn) {
          LOG(ERROR) << "no connection for " << _ip;
          return;
        }

        conn->send(message, size);
      }

    private:

      rfc::client_type _client;
      std::string _ip;
    };

    class random_client : public rfc::remote_caller {
    public:

      random_client(rfc::client_type client) : rfc::remote_caller(client), _client(client) {}

      virtual ~random_client() {}

    public:

      virtual void send(const char* message, size_t size) {
        mn::TcpConnectionPtr conn;

        if (rfc::client_type::inward_client & _client) {
          conn = net::inward_connection_pool::ref().random_take();
        }

        if (!conn && (rfc::client_type::outward_client & _client)) {
          conn = net::outward_connection_pool::ref().random_take();
        }

        if (!conn) {
          LOG(ERROR) << "no connection";
          return;
        }

        conn->send(message, size);
      }

    private:

      rfc::client_type _client;
    };

  } // net
} // pioneer

#endif /* PIONEER_RFC_CLIENTS_H_ */
