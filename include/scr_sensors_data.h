#ifndef SCR_SENSORS_DATA_H
#define SCR_SENSORS_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the sensor data screen.
 *
 * @param[in] menu      Pointer to menu.
 */
//TODO: return value: page
lv_obj_t* scr_sensors_data_create(lv_obj_t* menu);

/**
 */
//TODO: doc
void scr_sensors_data_set_sensor(int sensor);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_SENSORS_DATA_H */