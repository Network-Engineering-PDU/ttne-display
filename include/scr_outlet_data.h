#ifndef SCR_OUTLET_DATA_H
#define SCR_OUTLET_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the outlet data screen.
 *
 * @param[in] menu      Pointer to menu.
 */
//TODO: return value: page
lv_obj_t* scr_outlet_data_create(lv_obj_t* menu);

/**
 */
//TODO: doc
void scr_outlet_data_set_out(int outlet);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_OUTLET_DATA_H */