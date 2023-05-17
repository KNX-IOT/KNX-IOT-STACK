/*
// Copyright (c) 2022 Cascoda Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
/**
  @brief DNS SD platform abstraction
  @file
*/
#ifndef DNS_SD_H
#define DNS_SD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Publish the KNX mDNS service in order to enable DNS-SD discovery.
 *
 * @param serial_no KNX serial number. The advertised service will be
 * "${serial_no}._knx._udp"
 * @param iid KNX Installation ID. Set to 0 if the device has not been
 * commissioned yet
 * @param ia KNX Individual Address. Set to 0 if the device has not been
 * commissioned yet
 * @param pm True if the device is in Programming Mode, false otherwise.
 * @return int 0 on success, -1 on error.
 */
int knx_publish_service(char *serial_no, uint64_t iid, uint32_t ia, bool pm);

/**
 * @brief Set the advertised sleep period within the mDNS service.
 *
 * @param sp The period, in milliseconds. A value of 0 removes the
 * advertisement, signalling that the device is wakeful.
 */
void knx_service_sleep_period(uint32_t sp);

#ifdef __cplusplus
}
#endif

#endif // DNS_SD_H