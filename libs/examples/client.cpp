/*
 * client.cpp
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

#include "config.h"

#include <functional>

#include <boost/bind.hpp>
#include <glog/logging.h>
#include <atlas/console.h>
#include <atlas/rpc.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoopThread.h>

#include "commander.h"

using namespace pioneer;
using namespace muduo;
using namespace muduo::net;

class pioneer_client {
public:

  pioneer_client(EventLoop* loop, const InetAddress& listenAddr) :
      _loop(loop), _client(loop, listenAddr, "pioneer_client")
  {
    _client.setConnectionCallback(boost::bind(&pioneer_client::on_connection, this, _1));
    _client.setMessageCallback(boost::bind(&pioneer_client::on_message, this, _1, _2, _3));
    _client.enableRetry();
  }

  void connect() { _client.connect(); }

  void disconnect() { _client.disconnect(); }

  void send(const std::string& message) { _connection->send(message.data(), message.size()); }

  void send(const char* message, size_t size) { _connection->send(message, size); }

private:

  void on_connection(const TcpConnectionPtr& connection) {
    MutexLockGuard lock(_mutex);
    _connection = connection;
  }

  /*
   * |------------|----------------------|------------|------|-----|---------|----------------
   *   total size         cson-header         type      ecat  ecode  [count]      body
   * */
  void on_message(const TcpConnectionPtr& conn, Buffer* buf, muduo::Timestamp) {
    size_t bytes_received = buf->readableBytes();
    auto header = reinterpret_cast<const atlas::rpc::request_header*>(buf->peek());

    if (bytes_received < sizeof(header->length) || (int32_t)bytes_received < header->length) {
      LOG(INFO) << "I will read more data. Read " << bytes_received
          << " bytes while " << header->length << " bytes expected.";
      return;
    }

    buf->retrieve(sizeof(atlas::rpc::request_header));
    std::string rpc_str(buf->peek(), buf->readableBytes());

    atlas::rpc::message message(buf->peek());

    if (!rpc_str.empty()) {
      // invoke built-in dispatchers
      atlas::rpc::dispatcher_manager::ref().dispatch(header->fn_id, rpc_str, nullptr);
    }
    else {
      LOG(ERROR) << "bad rpc message!";
    }

    buf->retrieveAll();
  }

  EventLoop* _loop;
  TcpClient _client;
  MutexLock _mutex;
  TcpConnectionPtr _connection;
};

int main(int argc, char* argv[]) {
  if (argc > 2) {
    EventLoopThread loopThread;
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));

    pioneer_client client(loopThread.startLoop(), server_addr);
    client.connect();
    rpc::commander<pioneer_client> commander(client);

    const char* line;
    atlas::console console("pioneer >", "/tmp/pioneer_console_history");
    while ((line = console.getline()) != NULL) {
      if (line[0] != '\0') {
        if (!strcmp(line, "quit")) {
          printf("Exit!\n\r");
          return 0;
        }

        commander.order(line);

        std::cout << std::endl;
        console.add_history(line);
      }
    }
  }
  else {
    std::cerr << "usage : client ip port";
    std::cerr << "try : client 127.0.0.1 9100";
  }
}
