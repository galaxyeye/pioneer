/*
 * mcast.h
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

#ifndef NET_MULTICAST_H_
#define NET_MULTICAST_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <array>

#include <glog/logging.h>

#include <pioneer/net/ip.h>
#include <pioneer/net/config.h>
#include <pioneer/system/status.h>

namespace pioneer {
  namespace net {

    // ip, data buffer, data size
    typedef std::function<void(const std::string&, const char*, size_t)> mcast_message_callback;

    class mcast_server {

      // TODO : move to config file
      const static int MESSAGE_BUFFER_SIZE = 3.5 * 1024;
      const static int MULTIPORT = 1234;
      const static int BUFLEN = 256;
      const static size_t RECVBUFFER_SIZE = 220 * 1024;
      const static size_t BUF_SIZE = 3.5 * 1024;

    public:

      mcast_server(const char* multi_group) : _running(false), _recv_sockfd(0), _from_addr_len(sizeof(sockaddr_in)) {
        int recv_buf_size = RECVBUFFER_SIZE;
        struct sockaddr_in mcast_addr;
        struct ip_mreq recv_mcast_req;

        _recv_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_recv_sockfd  == -1) {
          LOG(ERROR) << strerror(errno);
          return;
        }

        bzero(&mcast_addr, sizeof(struct sockaddr_in));
        mcast_addr.sin_family = AF_INET;
        mcast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        mcast_addr.sin_port = htons(MULTIPORT);

        if (bind(_recv_sockfd, (struct sockaddr *) &mcast_addr, sizeof(struct sockaddr_in)) == -1) {
          LOG(ERROR) << strerror(errno);
          return;
        }

        int reuseaddr = 1;
        if (setsockopt(_recv_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
          LOG(ERROR) << strerror(errno);
          return;
        }

        if (setsockopt(_recv_sockfd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(int)) == -1) {
          LOG(ERROR) << strerror(errno);
          return;
        }

        // timeout
        struct timeval tv{MAX_WAIT_TIME, 0};
        if (setsockopt(_recv_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) == -1) {
          LOG(ERROR) << strerror(errno);
          return;
        }

        inet_pton(AF_INET, multi_group, &(recv_mcast_req.imr_multiaddr));
        recv_mcast_req.imr_interface.s_addr = htonl(INADDR_ANY);

        if (setsockopt(_recv_sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&recv_mcast_req,
            sizeof(struct ip_mreq)) == -1)
        {
          LOG(ERROR) << strerror(errno);
          return;
        }
      }

      ~mcast_server() {
        if (_recv_sockfd) {
          close(_recv_sockfd);
          _recv_sockfd = 0;
        }
      }

    public:

      void start() {
        _running = true;

        while (_running) {
          std::lock_guard<std::mutex> guard(_mutex);

          _buffer.fill('\0');
          int num_bytes = recvfrom(_recv_sockfd, _buffer.data(), _buffer.size(), 0,
              (struct sockaddr*)&_from_addr, &_from_addr_len);

          if (num_bytes == -1) {
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
              LOG(ERROR) << strerror(errno);
            }

            continue;
          }

          ++status::mcast_received;

          // DLOG(INFO) << num_bytes << " bytes received from " << ip::get_ip_port(_from_addr);

          std::string ip_port = ip::get_ip_port(_from_addr);
          _on_message(ip_port, _buffer.data(), num_bytes);
        } // while
      }

      void set_message_callback(const mcast_message_callback& cb) { _on_message = cb; }

      void stop() {
        _running = false;

        if (_recv_sockfd) {
          close(_recv_sockfd);
          _recv_sockfd = 0;
        }
      }

    private:

      bool _running;
      int _recv_sockfd;
      sockaddr_in _from_addr;
      unsigned int _from_addr_len;

      mcast_message_callback _on_message;

      std::mutex _mutex;
      std::array<char, MESSAGE_BUFFER_SIZE> _buffer;
    };

    class mcast_client : public atlas::singleton<mcast_client> {
    private:

      friend class atlas::singleton<mcast_client>;
      mcast_client() : _stopped(false), _send_buf_size(0), _mcast_addr_len(0), _send_sockfd(0) {
        init();
      }

    public:

      ~mcast_client() {
        if (_send_sockfd) close(_send_sockfd);
      }

    public:

      void stop() {
        _stopped = true;

        if (_send_sockfd) {
          close(_send_sockfd);
          _send_sockfd = 0;
        }
      }

      int send(const std::string& message) {
        return send(message.data(), message.size());
      }

      int send(const char* message, const int len) {
        if (_stopped) {
          LOG(INFO) << "sorry, have a rest";
          return -1;
        }

        if (!message || !len) return 0;

        std::lock_guard<std::mutex> guard(_send_mutex);
        int num_bytes = sendto(_send_sockfd, message, len, 0, (struct sockaddr*) &_mcast_addr, sizeof(struct sockaddr));
        if (num_bytes == -1) {
          LOG(ERROR) << strerror(errno);

          return -1;
        }

        ++status::mcast_sent;
        // DLOG(INFO) << "multicast " << num_bytes << " bytes";

        return num_bytes;
      }

    private:

      void init() {
        _send_buf_size = RECV_BUFFER_SIZE;

        _send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_send_sockfd == -1) {
          LOG(ERROR) << strerror(errno);
          return;
        }

        bzero(&_mcast_addr, sizeof(struct sockaddr_in));
        _mcast_addr.sin_family = AF_INET;

        inet_pton(AF_INET, PIONEER_MULTIGROUP, &_mcast_addr.sin_addr);
        _mcast_addr.sin_port = htons(MULTICAST_PORT);
        _mcast_addr_len = sizeof(_mcast_addr);

        if (-1 == setsockopt(_send_sockfd, SOL_SOCKET, SO_SNDBUF, &_send_buf_size, sizeof(int))) {
          LOG(ERROR) << strerror(errno);
          return;
        }

        int reuseaddr = 1;
        if (-1 == setsockopt(_send_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int))) {
          LOG(ERROR) << strerror(errno);
          return;
        }
      }

    private:

      std::atomic<bool> _stopped;
      int _send_buf_size;
      int _mcast_addr_len;
      int _send_sockfd;
      sockaddr_in _mcast_addr;

      std::mutex _send_mutex;
    };

  } // net
} // pioneer

#endif /* NET_MULTICAST_H_ */
