#include "driver/driver.h"

void driver_main() {
    // init the secure memory region to be something random
    __CPROVER_havoc_object(&seamrr_base); 
    __CPROVER_havoc_object(&seamrr_top); 
    __CPROVER_assume((seamrr_base > 0x00000000) && (seamrr_base < 0xFFFFFFFF)); //0xFFFFFFFF
    __CPROVER_assume((seamrr_top > seamrr_base) && (seamrr_top < 0xFFFFFFFF)); //&& (seamrr_top > 0x0) &&

    // init the kot table
    for (int i = 0; i < tabe_size; i++) {
        kot_table[i].state = KOT_STATE_HKID_FREE;
        pamt[i] = PT_NDA; 
    }

    __CPROVER_printf(("seamrr_base: %llx seamrr_top: %llx", seamrr_base, seamrr_top));
}