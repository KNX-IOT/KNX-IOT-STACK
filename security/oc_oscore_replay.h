#ifndef OC_OSCORE_REPLAY_H
#define OC_OSCORE_REPLAY_H

#ifdef OC_OSCORE
#define OC_MAX_RX_SEQUENCE_NUMBERS 30

/**
 * @brief Add an endpoint to the table of sequence numbers.
 * 
 * The newly created endpoint will be stored with sequence number 0.
 * This function saves a copy of endpoint - the pointer need not be valid
 * after this function returns.
 * 
 * @param endpoint The endpoint to be saved
 * @return int 0 on success, 1 if the table of endpoints is full.
 */
int oc_oscore_replay_add_endpoint(const oc_endpoint_t *endpoint);

/**
 * @brief Delete an endpoint from the table of sequence numbers.
 * 
 * @param endpoint The endpoint to be deleted. Uses oc_endpoint_compare_address
 * for the comparison
 * @return int 0 upon successful deletion, 1 if the endpoint was not found.
 */
int oc_oscore_replay_delete_endpoint(const oc_endpoint_t *endpoint);

/**
 * @brief Get the sequence number for an endpoint.
 * 
 * @param endpoint Endpoint to search for. Uses oc_endpoint_compare_address.
 * @param sequence_number Pointer to sequence number variable to be filled in.
 * @return int 0 on success, 1 if the endpoint was not found.
 */
int oc_oscore_replay_get_sequence_number(const oc_endpoint_t *endpoint, uint16_t *sequence_number);

/**
 * @brief Update an existing endpoint with a new sequence number.
 * 
 * @param endpoint Endpoint to search for. Uses oc_endpoint_compare_address
 * @param sequence_number The new value of the sequence number
 * @return int 0 on success, 1 if the endpoint was not found.
 */
int oc_oscore_replay_update_sequence_number(const oc_endpoint_t *endpoint, uint16_t sequence_number);
#endif
#endif