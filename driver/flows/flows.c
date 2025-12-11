#include "driver/flows/flows.h"

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
    md_field_id_t field_id; 
    uint64_t tdmr_info_array_pa;
    uint64_t num_of_tdmr_entries;
    hkid_api_input_t global_private_hkid;

    __CPROVER_havoc_object(&field_id);
    __CPROVER_assume((field_id.reserved_0 == 0) && (field_id.reserved_1 == 0) &&
                         (field_id.reserved_2 == 0) && (field_id.reserved_3 == 0) &&
                         (field_id.last_element_in_field == 0) && (field_id.last_field_in_sequence == 0));
    __CPROVER_havoc_object(&tdmr_info_array_pa);
    __CPROVER_assume(is_addr_aligned_pwr_of_2(tdmr_info_array_pa, TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT));
    __CPROVER_havoc_object(&num_of_tdmr_entries);
    __CPROVER_assume(num_of_tdmr_entries == 1); 
    __CPROVER_havoc_object(&global_private_hkid);
    __CPROVER_assume((global_private_hkid.hkid >= global_data.private_hkid_min) && 
                     (global_private_hkid.hkid <= global_data.private_hkid_max));
    __CPROVER_assume(global_private_hkid.reserved == 0); 

    api_error_type error = UNINITIALIZE_ERROR;

    #ifdef TDXBOOTUP
    error = tdh_sys_init(); 
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    #endif // BOOTUP_SETUP

    #ifdef SYS_LP_INIT
    error = tdh_sys_lp_init(); 
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    #endif //SYS_LP_INIT_SETUP

    #ifdef SYS_RD
    error = tdh_sys_rd(field_id); 
    __CPROVER_assert(error == TDX_SUCCESS,  "seamcall success"); 
    #endif // SYS_RD_SETUP

    #ifdef SYS_CONFIG
    error = tdh_sys_config(tdmr_info_array_pa, num_of_tdmr_entries, global_private_hkid); 
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
    #endif // SYS_CONFIG_SETUP

    #ifdef SYS_KEY_CONFIG
    error = tdh_sys_key_config();
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    #endif // SYS_KEY_CONFIG_SETUP

    #ifdef WHOLE_FLOW
    error = tdh_sys_init(); 
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    error = tdh_sys_lp_init(); 
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
    error = tdh_sys_rd(field_id); 
    __CPROVER_assert(error == TDX_SUCCESS,  "seamcall success"); 
    error = tdh_sys_config(tdmr_info_array_pa, num_of_tdmr_entries, global_private_hkid); 
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
    error = tdh_sys_key_config();
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    //__CPROVER_assert(false, "false"); 
    #endif // WHOLE_FLOW

}

void TD_mem_setup( page_info_api_input_t sept_level_and_gpa, page_info_api_input_t gpa_page_info) {
    td_handle_and_flags_t target_tdr_and_flags;
    uint64_t target_sept_page_pa;
    uint64_t version;

    uint64_t target_tdr_pa;
    uint64_t target_page_pa;
    uint64_t source_page_pa;

    __CPROVER_havoc_object(&target_tdr_and_flags);
    __CPROVER_havoc_object(&target_sept_page_pa);
    __CPROVER_havoc_object(&version);
    __CPROVER_havoc_object(&target_tdr_pa);
    __CPROVER_havoc_object(&target_page_pa);
    __CPROVER_havoc_object(&source_page_pa);

    api_error_type error = UNINITIALIZE_ERROR;

    __CPROVER_assume(version == 0); // SOPHIA: limit to version 0 for now
    __CPROVER_assume(!target_tdr_and_flags.reserved_0); 
    __CPROVER_assume(!target_tdr_and_flags.reserved_1); 
    __CPROVER_assume(is_addr_aligned_pwr_of_2(target_page_pa, 256));

    error = tdh_mem_sept_add(sept_level_and_gpa,target_tdr_and_flags, target_sept_page_pa, version);
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    error = tdh_mem_page_add(gpa_page_info, target_tdr_pa, target_page_pa, source_page_pa);
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    error = tdh_mr_extend(target_page_pa, target_tdr_pa);
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
}

void TD_enter() {
    uint64_t vcpu_handle_and_flags;
    __CPROVER_havoc_object(&vcpu_handle_and_flags);
    api_error_type error = UNINITIALIZE_ERROR;
    vcpu_and_flags_t      vcpu_and_flags = { .raw = vcpu_handle_and_flags };
    __CPROVER_assume(vcpu_and_flags.reserved_0 == 0);
    __CPROVER_assume(vcpu_and_flags.reserved_1 == 0);
    __CPROVER_assume(vcpu_and_flags.resume_l1 == 0);

    error = tdh_vp_enter(vcpu_handle_and_flags); 
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
}

void TD_exit(uint64_t controller_value) {
    tdg_vp_vmcall(controller_value);
}

void TD_add_page(uint64_t controller_value, page_info_api_input_t sept_level_and_gpa, page_info_api_input_t gpa_page_info) {
    td_handle_and_flags_t target_tdr_and_flags;
    uint64_t target_sept_page_pa;
    uint64_t version;

    uint64_t target_tdr_pa;
    uint64_t target_page_pa;
    uint64_t source_page_pa;

    __CPROVER_havoc_object(&target_tdr_and_flags);
    __CPROVER_havoc_object(&target_sept_page_pa);
    __CPROVER_havoc_object(&version);
    __CPROVER_havoc_object(&target_tdr_pa);
    __CPROVER_havoc_object(&target_page_pa);
    __CPROVER_havoc_object(&source_page_pa);

    api_error_type error = UNINITIALIZE_ERROR;

    __CPROVER_assume(version == 0); // SOPHIA: limit to version 0 for now
    __CPROVER_assume(!target_tdr_and_flags.reserved_0); 
    __CPROVER_assume(!target_tdr_and_flags.reserved_1); 
    __CPROVER_assume(is_addr_aligned_pwr_of_2(target_page_pa, 256));

    // error = tdg_vp_vmcall(controller_value); 
    // __CPROVER_assert(error == TDX_SUCCESS, "seamcall success"); 
    // error = tdh_mem_sept_add(sept_level_and_gpa,target_tdr_and_flags, target_sept_page_pa, version);
    // __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    error = tdh_mem_page_aug(gpa_page_info, target_tdr_pa, target_page_pa);
    __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    // error = tdh_vp_enter(target_page_pa, target_tdr_pa);
    // __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
    // error = tdg_mem_page_accept(); 
    // __CPROVER_assert(error == TDX_SUCCESS, "seamcall success");
}