#include "driver/driver.h"
#include "stdlib.h"

void write(void *address, void *val, int structure) {
    *(int*)address = (int)val;
}

void driver_main(driver_flag flag) {
    if (flag == BOOTUP) {
        // init the secure memory region to be something random
        // __CPROVER_havoc_object(&seamrr_base); 
        // __CPROVER_havoc_object(&seamrr_top); 
        // __CPROVER_assume((seamrr_base > 0x00000000) && (seamrr_base < 0xFFFFFFFF)); 
        // __CPROVER_assume((seamrr_top > seamrr_base) && (seamrr_top < 0xFFFFFFFF)); 
    
        // init global data
        __CPROVER_havoc_object(&global_data.private_hkid_min);
        __CPROVER_havoc_object(&global_data.private_hkid_max); 
        __CPROVER_assume((global_data.private_hkid_min > 0x00000000) && (global_data.private_hkid_min < 0xFFFFFFFF)); 
        __CPROVER_assume((global_data.private_hkid_max > global_data.private_hkid_min) && (global_data.private_hkid_max < 0xFFFFFFFF)); 
        //init the kot table
        for (int i = 0; i < hkid_size; i++) {
            global_data.kot.entries[i].state = KOT_STATE_HKID_FREE;
            tables[i].pamt_entry.pt = PT_NDA; 
        }
        kot_lock = false; 

        __CPROVER_printf(("seamrr_base: %llx seamrr_top: %llx", global_data.private_hkid_min, global_data.private_hkid_max));
    }
}