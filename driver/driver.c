#include "driver/driver.h"
#include "driver/flows/flows.h"
#include "stdlib.h"

void driver_main(driver_flag flag) {
    if (flag == BOOTUP) {
        // init global data
        __CPROVER_havoc_object(&global_data.private_hkid_min);
        __CPROVER_havoc_object(&global_data.private_hkid_max); 
        __CPROVER_assume((global_data.private_hkid_min > 0x00000000) && (global_data.private_hkid_min < 0xFFFFFFFF)); 
        __CPROVER_assume((global_data.private_hkid_max > global_data.private_hkid_min) && (global_data.private_hkid_max < 0xFFFFFFFF)); 
        
        //init the kot table
        for (int i = 0; i < HKID_SIZE; i++) {
            global_data.kot.entries[i].state = KOT_STATE_HKID_FREE;
            tables[i].pamt_entry.pt = PT_NDA; 
            tables[i].tdr_lock = false; 
        }
        //global_data.kot.lock.raw = SHAREX_FREE; 

        __CPROVER_printf(("seamrr_base: %llx seamrr_top: %llx", global_data.private_hkid_min, global_data.private_hkid_max));
    } else if (flag == MID_SETUP) {
        __CPROVER_havoc_object(&local_data.lp_info.pkg);
        __CPROVER_assume(local_data.lp_info.pkg >= 0 && local_data.lp_info.pkg < 32);
        __CPROVER_havoc_object(&global_data.pkg_config_bitmap);
        __CPROVER_assume(global_data.pkg_config_bitmap & BIT(local_data.lp_info.pkg) != 0);
    }
    TD_setup();
}