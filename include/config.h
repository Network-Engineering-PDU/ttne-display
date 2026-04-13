#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize config module.
 */
void config_init();

/**
 * @brief Set the rotation of the display.
 *
 * @param[in] rotation  Rotation value [0->0; 1->90; 2->180; 3->270].
 */
void config_set_rotation(int rotation);

/**
 * @brief Get the rotation of the display.
 *
 * @return Rotation value [0->0; 1->90; 2->180; 3->270].
 */
int config_get_rotation();

/**
 * @brief Set the inactivity time for the screensaver.
 *
 * @param[in] inactivity_time  inactivity time value in minutes.
 */
void config_set_inactivity_time(int inactivity_time);

/**
 * @brief Get the inactivity time for the screensaver.
 *
 * @return inactivity time value in minutes.
 */
int config_get_inactivity_time();

/**
 * @brief Set PDU company name.
 *
 * @param[in] value  Company name string.
 */
void config_set_pdu_company(const char* value);

/**
 * @brief Get PDU company name.
 *
 * @return Company name string.
 */
const char* config_get_pdu_company();

/**
 * @brief Set PDU rack name.
 *
 * @param[in] value  Rack name string.
 */
void config_set_pdu_rack(const char* value);

/**
 * @brief Get PDU rack name.
 *
 * @return Rack name string.
 */
const char* config_get_pdu_rack();

/**
 * @brief Set PDU system name.
 *
 * @param[in] value  System name string.
 */
void config_set_pdu_system(const char* value);

/**
 * @brief Get PDU system name.
 *
 * @return System name string.
 */
const char* config_get_pdu_system();

/**
 * @brief Set PDU UPS name.
 *
 * @param[in] value  UPS name string.
 */
void config_set_pdu_ups(const char* value);

/**
 * @brief Get PDU UPS name.
 *
 * @return UPS name string.
 */
const char* config_get_pdu_ups();

/**
 * @brief Set PDU electrical board name.
 *
 * @param[in] value  Electrical board name string.
 */
void config_set_pdu_elec_board(const char* value);

/**
 * @brief Get PDU electrical board name.
 *
 * @return Electrical board name string.
 */
const char* config_get_pdu_elec_board();

/**
 * @brief Set PDU breaker name.
 *
 * @param[in] value  Breaker name string.
 */
void config_set_pdu_breaker(const char* value);

/**
 * @brief Get PDU breaker name.
 *
 * @return Breaker name string.
 */
const char* config_get_pdu_breaker();

/**
 * @brief Set PDU service name.
 *
 * @param[in] value  Service name string.
 */
void config_set_pdu_service(const char* value);

/**
 * @brief Get PDU service name.
 *
 * @return Service name string.
 */
const char* config_get_pdu_service();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CONFIG_H */
