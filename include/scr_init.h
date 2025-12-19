#ifndef SCR_INIT_H
#define SCR_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the init screen.
 * 
 * @return Init screen object
 */
lv_obj_t* scr_init_create();

/**
 * @brief Show the init screen.
 */
void scr_init_show();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCR_INIT_H */