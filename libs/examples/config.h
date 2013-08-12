/*
 * config.h
 *
 *  Created on: Aug 10, 2013
 *      Author: vincent
 */

#ifndef PIONEER_CONFIG_H_
#define PIONEER_CONFIG_H_

#define ATLAS_DEBUG_RPC 1

const char* PIONEER_MULTIGROUP = "234.1.1.18";

const int PIONEER_OUTWARD_SERVER_PORT = 9100;
const int PIONEER_INWARD_SERVER_PORT = 9102;
const int PIONEER_REPORT_SERVER_PORT = 9080;

const int OUTWARD_SERVER_THREADS = 2;
const int INWARD_SERVER_THREADS = 2;
const int INWARD_CLIENT_POOL_THREADS = 2;

#endif /* CONFIG_H_ */
