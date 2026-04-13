#include "lvgl/lvgl.h"
#include "scr_settings_nw_modbus.h"
#include "tt_obj.h"

/* Modbus configuration variables */
static lv_obj_t* modbus_tcp_combo;
static lv_obj_t* modbus_eth1_combo;
static lv_obj_t* modbus_eth2_combo;
static lv_obj_t* modbus_baud_txt;
static lv_obj_t* modbus_parity_txt;
static lv_obj_t* modbus_stopbits_txt;
static lv_obj_t* modbus_rt485_acc1_combo;
static lv_obj_t* modbus_rt485_acc2_combo;
static lv_obj_t* modbus_rt485_baud_txt;
static lv_obj_t* modbus_rt485_parity_txt;
static lv_obj_t* modbus_rt485_stopbits_txt;

void scr_settings_nw_modbus_create(lv_obj_t* menu, lv_obj_t* btn)
{
    lv_obj_t* modbus_page = tt_obj_menu_page_create(menu, btn, NULL, "Modbus");
    lv_obj_t* cont = tt_obj_cont_create(modbus_page);

    /* TCP/Ethernet Section */
    tt_obj_label_create(cont, "TCP / Ethernet 1");
    modbus_eth1_combo = tt_obj_dropdown_create(cont, "Enabled\nDisabled", NULL);
    lv_dropdown_set_selected(modbus_eth1_combo, 0);

    tt_obj_label_create(cont, "Ethernet 2");
    modbus_eth2_combo = tt_obj_dropdown_create(cont, "Enabled\nDisabled", NULL);
    lv_dropdown_set_selected(modbus_eth2_combo, 0);

    /* Baud Rate Configuration */
    tt_obj_label_create(cont, "Baud rate");
    modbus_baud_txt = tt_obj_txt_create(cont, "Baud rate", NULL);

    tt_obj_label_create(cont, "Parity");
    modbus_parity_txt = tt_obj_txt_create(cont, "Parity", NULL);

    tt_obj_label_create(cont, "Stop bits");
    modbus_stopbits_txt = tt_obj_txt_create(cont, "Stop bits", NULL);

    /* RT485 Section */
    tt_obj_label_create(cont, "RT 485");
    
    tt_obj_label_create(cont, "ACC-1");
    modbus_rt485_acc1_combo = tt_obj_dropdown_create(cont, "Enabled\nDisabled", NULL);
    lv_dropdown_set_selected(modbus_rt485_acc1_combo, 0);

    tt_obj_label_create(cont, "ACC-2");
    modbus_rt485_acc2_combo = tt_obj_dropdown_create(cont, "Enabled\nDisabled", NULL);
    lv_dropdown_set_selected(modbus_rt485_acc2_combo, 0);

    tt_obj_label_create(cont, "Baud rate");
    modbus_rt485_baud_txt = tt_obj_txt_create(cont, "Baud rate", NULL);

    tt_obj_label_create(cont, "Parity");
    modbus_rt485_parity_txt = tt_obj_txt_create(cont, "Parity", NULL);

    tt_obj_label_create(cont, "Stop bits");
    modbus_rt485_stopbits_txt = tt_obj_txt_create(cont, "Stop bits", NULL);

    /* OK / Cancel buttons */
    lv_obj_t* btn_row = lv_obj_create(cont);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    tt_obj_btn_std_create(btn_row, NULL, "OK");
    tt_obj_btn_std_create(btn_row, NULL, "Cancel");
}
