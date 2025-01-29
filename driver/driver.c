#include "driver/driver.h"

void driver_main() {
    __CPROVER_havoc_object(&seamrr_base); 
    __CPROVER_havoc_object(&seamrr_top); 
    __CPROVER_assume((seamrr_base > 0x00000000) && (seamrr_base < 0xFFFFFFFF)); //0xFFFFFFFF
    __CPROVER_assume((seamrr_top > seamrr_base) && (seamrr_top < 0xFFFFFFFF)); //&& (seamrr_top > 0x0) &&

    __CPROVER_printf(("seamrr_base: %llx seamrr_top: %llx", seamrr_base, seamrr_top));
    
    uint64_t target_tdr_pa; 
    api_error_type ret_val = UNINITIALIZE_ERROR;
}