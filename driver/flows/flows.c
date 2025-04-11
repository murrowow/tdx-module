#include "driver/flows/flows.h"

#ifdef SOURCE
#else
void TD_setup(uint16_t index) {
    uint64_t target_tdr_pa;
    uint64_t target_tdcx_pa; 
    uint64_t target_td_params_pa;
    hkid_api_input_t hkid_info;
    api_error_type error = UNINITIALIZE_ERROR;

    __CPROVER_havoc_object(&target_tdr_pa);
    __CPROVER_havoc_object(&target_tdcx_pa);
    __CPROVER_havoc_object(&hkid_info); 
    __CPROVER_havoc_object(&target_td_params_pa); 

    // SOPHIA: make sure the target td params pa is well formed
    __CPROVER_assume(is_addr_aligned_pwr_of_2(target_td_params_pa, TD_PARAMS_ALIGN_IN_BYTES));
    __CPROVER_assume(is_pa_smaller_than_max_pa(target_td_params_pa)); 
    
    // SOPHIA: For now assume HKID is upper most bits of the PA
    #ifdef SETUP
        __CPROVER_assume(hkid_info.hkid == target_tdr_pa >> (64-16)); 
        __CPROVER_assume(hkid_info.hkid >= global_data.private_hkid_min & hkid_info.hkid <= global_data.private_hkid_max); 
    #else 
        __CPROVER_assume(target_tdr_pa >> (64-16) == index); 
        __CPROVER_assume(hkid_info.hkid == index);
    #endif 

    #ifdef CREATE 
        error = tdh_mng_create(target_tdr_pa, hkid_info); 
        __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    #endif // CREATE

    #ifdef KEY_CONFIG 
        error = tdh_mng_key_config(target_tdr_pa); 
        __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    #endif // KEY_CONFIG

    #ifdef ADD_CX 
        error = tdh_mng_add_cx(target_tdcx_pa, target_tdr_pa);
        __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
    #endif // ADDCX

    #ifdef INIT
        error = tdh_mng_init(target_tdr_pa, target_td_params_pa);
        __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
    #endif // INIT

    #ifdef WHOLE_FLOW
        error = tdh_mng_create(target_tdr_pa, hkid_info); 
        __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
        error = tdh_mng_key_config(target_tdr_pa); 
        __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
        // SOPHIA: min_num is 6 max_num is 9
        for (int i = 0; i < MIN_NUM_TDCS_PAGES; i++) {
            error = tdh_mng_add_cx(target_tdcx_pa, target_tdr_pa);
            __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
        }
        error = tdh_mng_init(target_tdr_pa, target_td_params_pa);
        __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
    #endif // WHOLE_FLOW
}

void TDX_bootup() {
    api_error_type error = UNINITIALIZE_ERROR;
}
#endif // not SOURCE