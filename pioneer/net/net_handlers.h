/*
 * net_handlers.h
 *
 *  Created on: Jul 22, 2013
 *      Author: vincent
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

#ifndef PIONEER_NET_HANDLERS_H_
#define PIONEER_NET_HANDLERS_H_

#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <atlas/io/iomanip.h> // put_time
#include <atlas/rpc.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>

#include <pioneer/net/ip.h>
#include <pioneer/net/request.h>
#include <pioneer/system/status.h>
#include <pioneer/system/context.h>
#include <pioneer/system/thread_pool.h>

namespace pioneer {
  namespace net {

    namespace mn = muduo::net;

    class connection_handler {
    public:

      enum connection_type { outward_server_connection, inward_server_connection, inward_client_connection };

    public:

      // connections actively connected by an app engine
      static void on_outward_server_connection(const mn::TcpConnectionPtr& conn) {
        handle_connection(outward_server_connection, conn);
      }

      // connections accepted by data node server
      static void on_inward_server_connection(const mn::TcpConnectionPtr& conn) {
        handle_connection(inward_server_connection, conn);
      }

      // connections actively connected by this data node server
      static void on_inward_client_connection(const mn::TcpConnectionPtr& conn) {
        handle_connection(inward_client_connection, conn);
      }

      template<typename pool_tag>
      static void on_write_complete(const mn::TcpConnectionPtr& conn) {
        connection_pool<pool_tag>::ref().put(conn);
      }

    private:

      static void handle_connection(connection_type type, const mn::TcpConnectionPtr& conn) {
        std::string peer_ip_port = conn->peerAddress().toIpPort();
        std::string local_ip_port = conn->localAddress().toIpPort();

        LOG(INFO) << peer_ip_port << " -> " << local_ip_port << " is " << (conn->connected() ? "UP" : "DOWN");

        try_set_local_ip(ip::get_ip_part(local_ip_port));

        if (type == inward_client_connection) {
          handle_inner_client_connection(conn);
          stat_inward_connection(conn);
        }
        else if (type == inward_server_connection) {
          handle_inner_server_connection(conn);
          stat_inward_connection(conn);
        }
        else if (type == outward_server_connection) {
          handle_outward_server_connection(conn);
          stat_outward_connection(conn);
        }
      }

      static void handle_outward_server_connection(const mn::TcpConnectionPtr& conn) {
        std::string peer_ip_port = conn->peerAddress().toIpPort();
        std::string peer_ip = ip::get_ip_part(peer_ip_port);

        bool connected = conn->connected();
        if (connected) {
          outward_connection_pool::ref().put(conn);
        }
        else {
          outward_connection_pool::ref().erase(peer_ip_port);

          // server side half-close : close the connection channel
          conn->shutdown();
          // and close the socket file descriptor
          // TODO : we should check this api for sure
          conn->connectDestroyed();
        }
      }

      static void handle_inner_server_connection(const mn::TcpConnectionPtr& conn) {
        std::string peer_ip_port = conn->peerAddress().toIpPort();
        std::string peer_ip = ip::get_ip_part(peer_ip_port);

        bool connected = conn->connected();
        if (connected) {
          inward_connection_pool::ref().put(conn);
        }
        else {
          inward_connection_pool::ref().erase(peer_ip_port);

          // server side half-close : close the connection channel
          conn->shutdown();
          // and close the socket file descriptor
          // TODO : we should check this api for sure
          conn->connectDestroyed();
        }
      }

      static void handle_inner_client_connection(const mn::TcpConnectionPtr& conn) {
        std::string peer_ip_port = conn->peerAddress().toIpPort();
        std::string peer_ip = ip::get_ip_part(peer_ip_port);

        bool connected = conn->connected();
        if (connected) {
          inward_connection_pool::ref().put(conn);
        }
        else {
          inward_connection_pool::ref().erase(peer_ip_port);
          inward_client_pool::ref().erase(peer_ip_port);

          if (inward_client_pool::ref().empty()) {
            // since all connections raised by client are disconnected,
            // the tcp client pool can stop safely
            inward_client_pool::ref().destroy();
          }
        }
      }

      static void stat_outward_connection(const mn::TcpConnectionPtr& conn) {
        std::string peer_ip_port = conn->peerAddress().toIpPort();
        std::string peer_ip = ip::get_ip_part(peer_ip_port);

        bool connected = conn->connected();

        std::lock_guard<std::mutex> guard(system::context::mutex);
        if (connected) {
          system::context::outside_ip_list.insert(peer_ip);
        }
        else {
          system::context::outside_ip_list.erase(peer_ip);
        }

        system::context::outside_node_count = system::context::outside_ip_list.size();

        LOG(INFO) << "outside ip list : " << system::context::outside_ip_list;
      }

      static void stat_inward_connection(const mn::TcpConnectionPtr& conn) {
        std::string peer_ip_port = conn->peerAddress().toIpPort();
        std::string peer_ip = ip::get_ip_part(peer_ip_port);

        bool connected = conn->connected();

        std::lock_guard<std::mutex> guard(system::context::mutex);
        if (connected) {
          system::context::inside_ip_list.insert(peer_ip);
        }
        else {
          system::context::inside_ip_list.erase(peer_ip);
        }

        system::context::inner_node_count = system::context::inside_ip_list.size();

        LOG(INFO) << "inside ip list : " << system::context::inside_ip_list;
      }

      static void try_set_local_ip(const std::string& local_ip) {
        std::lock_guard<std::mutex> guard(system::context::mutex);
        if (system::context::local_ip.empty()) {
          system::context::local_ip = local_ip;
        }
      }
    };

    class message_handler {
    public:

      enum message_type { outer_message, inner_message, reporter_message };

    public:

      static void on_outward_server_message(const mn::TcpConnectionPtr& conn, mn::Buffer* buf, muduo::Timestamp t) {
        handle_tcp_message(outer_message, conn, buf, t);
      }

      static void on_inward_client_message(const mn::TcpConnectionPtr& conn, mn::Buffer* buf, muduo::Timestamp t) {
        handle_tcp_message(inner_message, conn, buf, t);
      }

      static void on_inward_server_message(const mn::TcpConnectionPtr& conn, mn::Buffer* buf, muduo::Timestamp t) {
        handle_tcp_message(inner_message, conn, buf, t);
      }

      static void on_mcast_message(const std::string& source_ip_port, const char* message, size_t len) {
        // for multicast, the source port must not be used to send back the respond
        run_task(ip::get_ip_part(source_ip_port), message, len);
      }

      static void on_report_server_message(const mn::HttpRequest& request, mn::HttpResponse* response) {
        handle_http_message(request, response);
      }

    private:

      static void handle_tcp_message(message_type type, const mn::TcpConnectionPtr& conn, mn::Buffer* buf, muduo::Timestamp t) {
        const char* message = buf->peek();
        size_t len = buf->readableBytes();

        DLOG(INFO) << "message: " << len << " bytes, "
            << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort();

        const atlas::rpc::request_header* header = reinterpret_cast<const atlas::rpc::request_header*>(buf->peek());

        // TODO : handle TCP stick package problem

        if (len < sizeof(header->length) || (int32_t)len < header->length) {
          LOG(INFO) << "i will read more data. read " << len
              << " bytes while " << header->length << " bytes expected.";

          return;
        }

        try {
          run_task(conn->peerAddress().toIpPort(), message, len);
        }
        catch (const net_error& e) {
          LOG(ERROR) << e.what();
        }
        catch (...) {
          LOG(ERROR) << "unexpected exception";
        }

        buf->retrieveAll();
      }

      static void handle_http_message(const mn::HttpRequest& request, mn::HttpResponse* response) {
        if (request.path() == "/") {
          response->setStatusCode(mn::HttpResponse::k200Ok);
          response->setStatusMessage("OK");
          response->setContentType("text/html");

          time_t then = system::status::last_check_time;
          time_t now = std::time(0);
          time_t elapsed = now - then;

          std::stringstream ss;
          ss << "<ol>"
              << "<li>" << "last check time:" << atlas::put_time(std::localtime(&then), "%F %T") << "</li>"
              << "<li>" << "now:" << atlas::put_time(std::localtime(&now), "%F %T") << "</li>"
              << "<li>" << "elapsed:" << elapsed << "s</li>"
              << "</ol>";

          std::stringstream ss2;
          ss2 << "<ol>";
          for (size_t i = 0; i < system::status::test_rounds; ++i) {
            unsigned long long sent = system::status::udp_test_sent[i];
            double failure_rate = (sent == 0) ? 0 : (1 - 1.0 * system::status::good_ack[i] / sent);

            ss2 << "<li>"
                << "<span style='padding:10px'>" << "mcast sent:" << system::status::udp_test_sent[i] << "</span>"
                << "<span style='padding:10px'>" << "mcast received:" << system::status::udp_test_received[i] << "</span>"
                << "<span style='padding:10px'>" << "interval:" << (system::status::udp_test_interval[i] / 1000.0) << "ms</span>"
                << "<span style='padding:10px'>" << "good ack:" << system::status::good_ack[i] << "</span>"
                << "<span style='padding:10px'>" << "failure rate:" << 100 * failure_rate << "%</span>"
                << "</li>";
          }
          ss2 << "</ol>";

          std::stringstream ss3;
          ss3 << "<html><head><title>pioneer server status report</title></head>"
              << "<body><h1>pioneer server status report</h1>"
              << ss.str()
              << ss2.str()
              << "</body></html>";

          system::status::last_check_time = now;

          std::string str = ss3.str();
          muduo::string result(str.data(), str.size());
          response->setBody(result);
        }
        else {
          response->setStatusCode(mn::HttpResponse::k404NotFound);
          response->setStatusMessage("Not Found");
          response->setCloseConnection(true);
        }
      }

      // build a executable task and put the task into the worker thread pool
      static void run_task(const std::string& source_ip_port, const char* message, size_t len) {
        auto request = session_manager::ref().build_request(source_ip_port, message, len);
        system::worker_pool::ref().schedule(std::bind(&request::execute, request));
      }

    };

  } // net
} // pioneer

#endif /* NET_HANDLERS_H_ */
