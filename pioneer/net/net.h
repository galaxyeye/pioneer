/*
 * servers.h
 *
 *  Created on: Sep 12, 2011
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

#ifndef PIONEER_SERVERS_H_
#define PIONEER_SERVERS_H_

#include <muduo/net/TcpServer.h>

#include <pioneer/net/net_pools.h>

namespace pioneer {
  namespace net {

    // tag the services that serves for requests from outside the cluster
    struct outward_tag {};
    // tag the services that serves for requests from inside the cluster
    struct inward_tag {};

    // the TCP client pool used to establish TCP connections to host nodes inside the cluster
    typedef tcp_client_pool<inward_tag> inward_client_pool;

    // holds the TCP connections from outside clients
    typedef connection_pool<outward_tag> outward_connection_pool;
    // holds the TCP connections from inside clients
    typedef connection_pool<inward_tag> inward_connection_pool;

    namespace mn = muduo::net;

    // TCP server serves for outside clients
    typedef mn::TcpServer outward_server;
    // TCP server serves for inside clients
    typedef mn::TcpServer inward_server;
    // HTTP server used to report the system status
    typedef mn::HttpServer report_server;

  } // net
} // pioneer

#endif /* PIONEER_SERVERS_H_ */
