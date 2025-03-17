#include "driver/flows/flows.h"

#ifdef SOURCE
#else
void TD_setup() {
    uint64_t target_tdr_pa;
    uint64_t target_tdcx_pa; 
    uint64_t target_td_params_pa;
    hkid_api_input_t hkid_info;
    api_error_type error = UNINITIALIZE_ERROR;

    __CPROVER_havoc_object(&target_tdr_pa);
    __CPROVER_havoc_object(&target_tdcx_pa);
    __CPROVER_havoc_object(&hkid_info); 
    __CPROVER_havoc_object(&target_td_params_pa); 

    // SOPHIA: For now assume HKID is upper most bits of the PA
    __CPROVER_assume(hkid_info.hkid == target_tdr_pa >> (64-16)); 
    __CPROVER_assume(hkid_info.hkid >= global_data.private_hkid_min & hkid_info.hkid <= global_data.private_hkid_max); 

    error = tdh_mng_create(target_tdr_pa, hkid_info); 
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    error = tdh_mng_key_config(target_tdr_pa); 
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
    error = tdh_mng_add_cx(target_tdcx_pa, target_tdr_pa);
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
    // tdh_mng_init(target_tdr_pa, target_td_params_pa);
    // __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
}

void TDX_bootup() {
    api_error_type error = UNINITIALIZE_ERROR;
}
#endif // not SOURCE