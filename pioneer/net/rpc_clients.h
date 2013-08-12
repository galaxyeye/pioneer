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

#ifndef PIONEER_RPC_CLIENTS_H_
#define PIONEER_RPC_CLIENTS_H_

#include <atlas/rpc/rpc.h>

#include <pioneer/net/multicast.h>
#include <pioneer/net/net.h>

namespace pioneer {
  namespace rpc {

    using boost::uuids::nil_uuid;
    namespace mn = muduo::net;

    enum client_type { outward_client, inward_client, any_client };

    class bcast_client : public atlas::rpc::remote_caller {
    public:

      bcast_client(client_type client = client_type::any_client, int response_expected = 1)
        : atlas::rpc::remote_caller(client, response_expected) {}

      virtual ~bcast_client() {}

    protected:

      virtual void send(const char* message, size_t sz) {
        // TODO : employ a broadcast client
      }
    };

    class mcast_client : public atlas::rpc::remote_caller {
    public:

      mcast_client(client_type client = client_type::any_client, int response_expected = 1)
        : atlas::rpc::remote_caller(client, response_expected) {}

      virtual ~mcast_client() {}

    public:

      virtual void send(const char* message, size_t sz) {
        net::mcast_client::ref().send(message, sz);
      }
    };

    class p2p_client : public atlas::rpc::remote_caller {
    public:

      p2p_client(client_type client, const std::string& ip) : atlas::rpc::remote_caller(client), _client(client), _ip(ip) {}

      virtual ~p2p_client() {}

    public:

      virtual void send(const char* message, size_t size) {
        mn::TcpConnectionPtr conn;

        if (client_type::inward_client & _client) {
          conn = net::inward_connection_pool::ref().take(_ip);
        }

        if (client_type::outward_client & _client) {
          conn = net::outward_connection_pool::ref().take(_ip);
        }

        if (!conn) {
          LOG(ERROR) << "no connection for " << _ip;
          return;
        }

        conn->send(message, size);
      }

    private:

      int _client;
      std::string _ip;
    };

    class random_client : public atlas::rpc::remote_caller {

      random_client(client_type client) : atlas::rpc::remote_caller(client), _client_id(client) {}

      virtual ~random_client() {}

    public:

      virtual void send(const char* message, size_t size) {
        mn::TcpConnectionPtr conn;

        if (client_type::inward_client & _client_id) {
          conn = net::inward_connection_pool::ref().random_take();
        }

        if (!conn && (client_type::outward_client & _client_id)) {
          conn = net::outward_connection_pool::ref().random_take();
        }

        if (!conn) {
          LOG(ERROR) << "no connection";
          return;
        }

        conn->send(message, size);
      }

    private:

      int _client_id;
    };

  } // net
} // pioneer

#endif /* PIONEER_RFC_CLIENTS_H_ */
