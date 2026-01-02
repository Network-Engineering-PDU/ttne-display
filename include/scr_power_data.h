#ifndef SCR_POWER_DATA_H
#define SCR_POWER_DATA_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct power_data_t {
	float voltage;
	float current;
	float active_power;
	float reactive_power;
	float apparent_power;
	float power_factor;
	float frequency;
	float phase_vi;
	float energy;
} power_data_t;

/**
 * @brief Creates the power data screen.
 *
 * @param[in] menu      Pointer to menu.
 * @param[in] btn       Main menu page button.
 */
void scr_power_data_create(lv_obj_t* menu, lv_obj_t* btn);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_POWER_DATA_H */