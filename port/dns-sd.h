#ifndef DNS_SD_H
#define DNS_SD_H

/**
 * @brief Publish the KNX mDNS service in order to enable DNS-SD discovery.
 * 
 * @param serial_no KNX serial number. The advertised service will be "${serial_no}._knx._udp"
 * @return int 0 on success, -1 on error.
 */
int knx_publish_service(char *serial_no);

#endif // DNS_SD_H