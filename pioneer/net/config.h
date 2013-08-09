/*
 * config.h
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

#ifndef PIONEER_NET_CONFIG_H_
#define PIONEER_NET_CONFIG_H_

namespace pioneer {

  static const unsigned short FOREIGN_SERVER_PORT = 9100;
  static const unsigned short INNER_SERVER_PORT = 9101;
  static const unsigned short CATALOG_SERVER_PORT = 9102;
  static const unsigned short CONNPOOL_SERVER_PORT = 9103;
  static const unsigned short OUTWARD_CLIENT_PORT = 9104;
  static const unsigned short REPORT_SERVER_PORT = 9190;

  static const int FOREIGN_SERVER_THREADS = 2;
  static const int INNER_SERVER_THREADS = 2;
  static const int inward_client_POOL_THREADS = 2;
  static const int CATALOG_CLIENT_POOL_THREADS = 2;

  // TODO : move the static variables to config file
  static const char PIONEER_MULTIGROUP[] = "234.1.1.18";
  static const size_t MULTICAST_PORT = 1234;
  static const size_t RECV_BUFFER_SIZE = 220 * 1024;
  static const int MAX_WAIT_TIME = 2;

}

#endif /* pioneer_NET_CONFIG_H_ */
