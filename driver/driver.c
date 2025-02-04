#include "driver/driver.h"

void driver_main(driver_flag flag) {
    if (flag == BOOTUP) {
        // init the secure memory region to be something random
        __CPROVER_havoc_object(&seamrr_base); 
        __CPROVER_havoc_object(&seamrr_top); 
        __CPROVER_assume((seamrr_base > 0x00000000) && (seamrr_base < 0xFFFFFFFF)); //0xFFFFFFFF
        __CPROVER_assume((seamrr_top > seamrr_base) && (seamrr_top < 0xFFFFFFFF)); 

        get_global_data()->private_hkid_min = seamrr_base; 
        get_global_data()->private_hkid_max = seamrr_top;

        __CPROVER_assume(get_global_data()->private_hkid_min == seamrr_base); 
        __CPROVER_assume(get_global_data()->private_hkid_max == seamrr_top); 


        _Bool var = __CPROVER_same_object(get_global_data()->private_hkid_min, seamrr_base); //, "why does this fail"
        __CPROVER_printf("SOPHIA var:%d, hkid_min: %llu, hkid_max: %llu, seamrr_base: %llu, seamrr_top: %llu", var, get_global_data()->private_hkid_min, get_global_data()->private_hkid_max, seamrr_base, seamrr_top);

        __CPROVER_assert(get_global_data()->private_hkid_max == seamrr_top, "why does this fail");
    
        // init the kot table
        // for (int i = 0; i < table_size; i++) {
        //     kot_table[i].state = KOT_STATE_HKID_FREE;
        //     pamt[i].pt = PT_NDA; 
        // }
        // kot_lock = false; 

        __CPROVER_printf(("seamrr_base: %llx seamrr_top: %llx", seamrr_base, seamrr_top));
    }
}