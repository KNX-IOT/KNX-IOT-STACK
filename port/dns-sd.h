#ifndef DNS_SD_H
#define DNS_SD_H

/**
 * @brief Publish the KNX mDNS service in order to enable DNS-SD discovery.
 *
 * @param serial_no KNX serial number. The advertised service will be
 * "${serial_no}._knx._udp"
 * @param iid KNX Installation ID. Set to 0 if the device has not been
 * commissioned yet (in programming mode)
 * @param ia KNX Individual Address. Set to 0 if the device has not been
 * commissioned yet (in programming mode)
 * @return int 0 on success, -1 on error.
 */
int knx_publish_service(char *serial_no, uint32_t iid, uint32_t ia);

#endif // DNS_SD_H