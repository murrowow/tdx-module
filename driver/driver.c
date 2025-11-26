#include "driver/driver.h"
#include "driver/flows/flows.h"
#include "stdlib.h"
#include "stdint.h"

#include "../src/common/helpers/virt_msr_helpers.h"
#include "../include/auto_gen/cpuid_configurations.h"

#ifdef SOURCE
#else 
void setup() {
    __CPROVER_havoc_object(&global_data); // .private_hkid_min and .private_hkid_max
    __CPROVER_assume((global_data.private_hkid_min == 0x00000000));  //&& (global_data.private_hkid_min < (0xFFFFFFFF - (HKID_SIZE)))); 
    __CPROVER_assume((global_data.private_hkid_max == global_data.private_hkid_min + (HKID_SIZE))); //&& (global_data.private_hkid_max < 0xFFFFFFFF)); 
    __CPROVER_assume(global_data.hkid_start_bit == (32 - (HKID_SIZE))); 
    __CPROVER_assume(global_data.hkid_mask == HKID_MASK);
    __CPROVER_havoc_object(&tables); 
    __CPROVER_havoc_object(&vmcs);

    __CPROVER_havoc_object(&tables);
    __CPROVER_assume(global_data.kot.lock.raw == SHAREX_FREE);
    //init the kot table
    for (int i = 0; i < HKID_SIZE; i++) {
       __CPROVER_assume(global_data.kot.entries[i].state == KOT_STATE_HKID_FREE);
       __CPROVER_assume(tables[i].tdr_table.management_fields.fatal == false); 
       __CPROVER_assume(tables[i].pamt_entry.pt == PT_NDA); 
       __CPROVER_assume(tables[i].tdr_table.key_management_fields.pkg_config_bitmap == 0); 
       __CPROVER_assume(tables[i].tdr_table.management_fields.num_tdcx == 0); 
       __CPROVER_assume(tables[i].tdcx_pamt_entry.pt == PT_NDA);
       // SOPHIA: highkey have no idea what this does for add_cx
       __CPROVER_assume(tables[i].tdcx_table.management_fields.op_state == 0); // TDH_MNG_ADDCX_LEAF == 1, [1][0] == 1
    }
}

void key_config_setup(uint16_t index) {
    __CPROVER_havoc_object(&local_data.lp_info.pkg);
    __CPROVER_assume(local_data.lp_info.pkg >= 0 && local_data.lp_info.pkg < HKID_SIZE);
    __CPROVER_havoc_object(&global_data.pkg_config_bitmap);
    __CPROVER_assume(global_data.pkg_config_bitmap & BIT(local_data.lp_info.pkg) != 0);

    
    // Ensure at least one element has pamt_entry.pt set to PT_TDR
    bool_t found = false;
    for (int i = 0; i < HKID_SIZE; i++) {
        if ((tables[i].pamt_entry.pt == PT_TDR) &&
            !tables[i].tdr_table.management_fields.fatal &&
            (tables[i].tdr_table.management_fields.lifecycle_state == TD_HKID_ASSIGNED) &&
            !(tables[i].tdr_table.key_management_fields.pkg_config_bitmap & (BIT(local_data.lp_info.pkg)))) {
            found = true;
            index = i;
            break;
        }
    }
    __CPROVER_assume(found);
}

void add_cx_setup(uint16_t index) {
    for (int i = 0; i < HKID_SIZE; i++) {
        __CPROVER_havoc_object(&tables[i].pamt_entry);
        __CPROVER_havoc_object(&tables[i].tdr_table.management_fields.lifecycle_state);
        __CPROVER_havoc_object(&tables[i].tdcx_table.management_fields.op_state);
    }

    for(int i = 0; i < HKID_SIZE; i++)
        __CPROVER_assume(tables[i].tdcx_table.management_fields.op_state >= 0 && tables[i].tdcx_table.management_fields.op_state <= 10);
    
    // Ensure at least one element is as we need it
    bool_t found = false;
    for (int i = 0; i < HKID_SIZE; i++) {
        if (tables[i].pamt_entry.pt == PT_TDR && 
            tables[i].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED && 
            tables[i].tdcx_table.management_fields.op_state == OP_STATE_UNINITIALIZED && 
            !tables[i].tdr_table.management_fields.fatal &&
            tables[i].tdr_table.management_fields.num_tdcx < MAX_NUM_TDCS_PAGES && 
            tables[i].tdcx_pamt_entry.pt == PT_NDA && 
            seamcall_state_lookup[TDH_MNG_ADDCX_LEAF][tables[i].tdcx_table.management_fields.op_state]) {
            found = true;
            index = i; 
            break;
        }
    }
    __CPROVER_assume(found);

}

void init_setup(uint16_t index) {
    for (int i = 0; i < HKID_SIZE; i++) {
        __CPROVER_havoc_object(&tables[i].pamt_entry);
        __CPROVER_havoc_object(&tables[i].tdr_table.management_fields.lifecycle_state);
        __CPROVER_havoc_object(&tables[i].tdcx_table.management_fields.op_state);
    }

    for(int i = 0; i < HKID_SIZE; i++)
        __CPROVER_assume(tables[i].tdcx_table.management_fields.op_state >= 0 && tables[i].tdcx_table.management_fields.op_state <= 10);

    // Ensure at least one element is as we need it
    bool_t found = false;
    for (int i = 0; i < HKID_SIZE; i++) {
        if (tables[i].pamt_entry.pt == PT_TDR && 
            tables[i].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED &&
            tables[i].tdr_table.management_fields.fatal == false && 
            tables[i].tdr_table.management_fields.num_tdcx >= MIN_NUM_TDCS_PAGES && 
            tables[i].td_params_table.ia32_arch_capabilities_config == 0 
        )
            {
                found = true;
                index = i; 
                break;
        }
    }

    __CPROVER_assume(found);
}

void driver_main() {

    #ifdef BOOTUP_SETUP
        __CPROVER_havoc_object(&sysinfo); 
        __CPROVER_havoc_object(&global_data); 
        __CPROVER_havoc_object(&local_data); 
        __CPROVER_havoc_object(&msr_values_ptr_model); 
        __CPROVER_havoc_object(&seamop_cap_model); 

        __CPROVER_assume(sysinfo.num_handoff_pages >= TDX_MIN_HANDOFF_PAGES); 

        __CPROVER_assume(global_data.global_state.sys_state == SYSINIT_PENDING); 
        __CPROVER_assume(global_data.kot.lock.raw == SHAREX_FREE); 

        // SYSINFO assumptions
        __CPROVER_assume((sysinfo.module_hv == 0)); 
        __CPROVER_assume((sysinfo.no_downgrade == 0)); 
        __CPROVER_assume((sysinfo.min_update_hv == 0)); 
        __CPROVER_assume((sysinfo.num_handoff_pages + 1) >= TDX_MIN_HANDOFF_PAGES);
        __CPROVER_assume(sysinfo.data_rgn_base > 0); 
        __CPROVER_assume(TDX_PAGE_SIZE_IN_BYTES * (sysinfo.num_tls_pages + 1) != 0);
        __CPROVER_assume(local_data.lp_info.pkg < MAX_PKGS); 

        // tdx_local_data assumptions
        __CPROVER_assume(!local_data.lp_is_init);
        __CPROVER_assume(local_data.vmm_regs.rcx == 0);

        // MSR for Bootup
        platform_common_config_t msr_values_ptr = global_data.plt_common_config;
        __CPROVER_assume(msr_values_ptr.ia32_vmx_basic.vmcs_region_size <= TD_VMCS_SIZE);
        __CPROVER_assume(msr_values_ptr.ia32_vmx_basic.ia32_vmx_true_available == 1U);
        __CPROVER_assume(msr_values_ptr.ia32_vmx_basic.vmexit_info_on_ios == 1U);

        __CPROVER_assume((msr_values_ptr.ia32_vmx_true_procbased_ctls.not_allowed0 & ~(PROCBASED_CTLS_INIT | PROCBASED_CTLS_UNKNOWN)) == 0);
        __CPROVER_assume(((~msr_values_ptr.ia32_vmx_true_procbased_ctls.allowed1) & PROCBASED_CTLS_INIT) == 0); 
        __CPROVER_assume(((msr_values_ptr.ia32_vmx_true_procbased_ctls.not_allowed0 | ~msr_values_ptr.ia32_vmx_true_procbased_ctls.allowed1) & PROCBASED_CTLS_VARIABLE) == 0);
        __CPROVER_assume((msr_values_ptr.ia32_vmx_true_pinbased_ctls.not_allowed0 & ~(PINBASED_CTLS_INIT | PINBASED_CTLS_UNKNOWN)) == 0);
        __CPROVER_assume(((~msr_values_ptr.ia32_vmx_true_pinbased_ctls.allowed1) & PINBASED_CTLS_INIT) == 0); 
        __CPROVER_assume(((msr_values_ptr.ia32_vmx_true_pinbased_ctls.not_allowed0 | ~msr_values_ptr.ia32_vmx_true_pinbased_ctls.allowed1) & PINBASED_CTLS_VARIABLE) == 0);

        __CPROVER_assume(check_native_ia32_arch_capabilities(msr_values_ptr_model.ia32_arch_capabilities));
        __CPROVER_assume(msr_values_ptr_model.ia32_misc_package_ctls.energy_filtering_enable);
        __CPROVER_assume((msr_values_ptr_model.ia32_perf_capabilities.freeze_while_smm_supported == 1) &&
                         (msr_values_ptr_model.ia32_perf_capabilities.full_write == 1));
        __CPROVER_assume(msr_values_ptr_model.ia32_mtrrcap.smrr != 0);
        __CPROVER_assume(msr_values_ptr_model.ia32_mtrrcap.smrr_lock != 0);

        // MSR SYSINIT LP
        __CPROVER_assume(global_data.plt_common_config.ia32_core_capabilities.raw == msr_values_ptr_model.ia32_core_capabilities.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_arch_capabilities.raw == msr_values_ptr_model.ia32_arch_capabilities.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_misc_package_ctls.raw == msr_values_ptr_model.ia32_misc_package_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_xapic_disable_status.raw == msr_values_ptr_model.ia32_xapic_disable_status.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_perf_capabilities.raw == msr_values_ptr_model.ia32_perf_capabilities.raw);
        __CPROVER_assume(global_data.plt_common_config.ia32_tsc_adjust == msr_values_ptr_model.ia32_tsc_adjust);
        
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_basic.raw == msr_values_ptr_model.ia32_vmx_basic.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_true_pinbased_ctls.raw == msr_values_ptr_model.ia32_vmx_true_pinbased_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_true_procbased_ctls.raw == msr_values_ptr_model.ia32_vmx_true_procbased_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_procbased_ctls2.raw== msr_values_ptr_model.ia32_vmx_procbased_ctls2.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_procbased_ctls3.raw == msr_values_ptr_model.ia32_vmx_procbased_ctls3.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_true_exit_ctls.raw== msr_values_ptr_model.ia32_vmx_true_exit_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_true_entry_ctls.raw == msr_values_ptr_model.ia32_vmx_true_entry_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_misc.raw == msr_values_ptr_model.ia32_vmx_misc.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_ept_vpid_cap == msr_values_ptr_model.ia32_vmx_ept_vpid_cap); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_cr0_fixed0.raw == msr_values_ptr_model.ia32_vmx_cr0_fixed0.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_cr0_fixed1.raw == msr_values_ptr_model.ia32_vmx_cr0_fixed1.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_cr4_fixed0.raw == msr_values_ptr_model.ia32_vmx_cr4_fixed0.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_cr4_fixed1.raw == msr_values_ptr_model.ia32_vmx_cr4_fixed1.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_mtrrcap.raw == msr_values_ptr_model.ia32_mtrrcap.raw);
        __CPROVER_assume(global_data.plt_common_config.smrr[0].smrr_base.raw == msr_values_ptr_model.smrr[0].smrr_base.raw);
        __CPROVER_assume(global_data.plt_common_config.smrr[0].smrr_mask.raw == msr_values_ptr_model.smrr[0].smrr_mask.raw);
        __CPROVER_assume(global_data.plt_common_config.smrr[1].smrr_base.raw == msr_values_ptr_model.smrr[1].smrr_base.raw);
        __CPROVER_assume(global_data.plt_common_config.smrr[1].smrr_mask.raw == msr_values_ptr_model.smrr[1].smrr_mask.raw);
    #endif //BOOTUP_SETUP

    #ifdef SYS_LP_INIT_SETUP
        __CPROVER_havoc_object(&sysinfo); 
        __CPROVER_havoc_object(&global_data); 
        __CPROVER_havoc_object(&local_data); 
        __CPROVER_havoc_object(&msr_values_ptr_model); 
        __CPROVER_havoc_object(&seamop_cap_model); 

        // SEAMOP_CAP_MODEL
        __CPROVER_assume((seamop_cap_model.raw & TD_PRESERVING_CAPABILITIES) == TD_PRESERVING_CAPABILITIES);
        // correct initiliazation state
        __CPROVER_assume(global_data.global_state.sys_state == SYSINIT_DONE); 
        __CPROVER_assume(!local_data.lp_is_init);

        // make sure that we do not ruin through shifting
        __CPROVER_assume(global_data.x2apic_core_id_shift_count < 32);
        __CPROVER_assume(global_data.x2apic_pkg_id_shift_count< 32);

        // MSRs are correct 
        __CPROVER_assume(global_data.plt_common_config.ia32_core_capabilities.raw == msr_values_ptr_model.ia32_core_capabilities.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_arch_capabilities.raw == msr_values_ptr_model.ia32_arch_capabilities.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_misc_package_ctls.raw == msr_values_ptr_model.ia32_misc_package_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_xapic_disable_status.raw == msr_values_ptr_model.ia32_xapic_disable_status.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_perf_capabilities.raw == msr_values_ptr_model.ia32_perf_capabilities.raw);
        __CPROVER_assume(global_data.plt_common_config.ia32_tsc_adjust == msr_values_ptr_model.ia32_tsc_adjust);
        __CPROVER_assume(global_data.seam_capabilities.raw == seamop_cap_model.raw); 

        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_basic.raw == msr_values_ptr_model.ia32_vmx_basic.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_true_pinbased_ctls.raw == msr_values_ptr_model.ia32_vmx_true_pinbased_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_true_procbased_ctls.raw == msr_values_ptr_model.ia32_vmx_true_procbased_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_procbased_ctls2.raw== msr_values_ptr_model.ia32_vmx_procbased_ctls2.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_procbased_ctls3.raw == msr_values_ptr_model.ia32_vmx_procbased_ctls3.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_true_exit_ctls.raw== msr_values_ptr_model.ia32_vmx_true_exit_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_true_entry_ctls.raw == msr_values_ptr_model.ia32_vmx_true_entry_ctls.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_misc.raw == msr_values_ptr_model.ia32_vmx_misc.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_ept_vpid_cap == msr_values_ptr_model.ia32_vmx_ept_vpid_cap); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_cr0_fixed0.raw == msr_values_ptr_model.ia32_vmx_cr0_fixed0.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_cr0_fixed1.raw == msr_values_ptr_model.ia32_vmx_cr0_fixed1.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_cr4_fixed0.raw == msr_values_ptr_model.ia32_vmx_cr4_fixed0.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_cr4_fixed1.raw == msr_values_ptr_model.ia32_vmx_cr4_fixed1.raw); 
        __CPROVER_assume(global_data.plt_common_config.ia32_mtrrcap.raw == msr_values_ptr_model.ia32_mtrrcap.raw);
        __CPROVER_assume(global_data.plt_common_config.smrr[0].smrr_base.raw == msr_values_ptr_model.smrr[0].smrr_base.raw);
        __CPROVER_assume(global_data.plt_common_config.smrr[0].smrr_mask.raw == msr_values_ptr_model.smrr[0].smrr_mask.raw);
        __CPROVER_assume(global_data.plt_common_config.smrr[1].smrr_base.raw == msr_values_ptr_model.smrr[1].smrr_base.raw);
        __CPROVER_assume(global_data.plt_common_config.smrr[1].smrr_mask.raw == msr_values_ptr_model.smrr[1].smrr_mask.raw);

        __CPROVER_assume(sysinfo.mcheck_fields.smrr2_not_supported != 0 || msr_values_ptr_model.ia32_mtrrcap.smrr2 == 0); 
        __CPROVER_assume(sysinfo.data_rgn_base > 0); 
        __CPROVER_assume(TDX_PAGE_SIZE_IN_BYTES * (sysinfo.num_tls_pages + 1) != 0);
        __CPROVER_assume(local_data.lp_info.pkg < MAX_PKGS); 

        __CPROVER_havoc_object(&num_cached_sub_blocks_model); 
    #endif //SYS_LP_INIT_SETUP

    uint16_t index = 0; 
    #ifdef SETUP
        setup(); 
    #endif // SETUP
    
    #ifdef KEY_CONFIG_SETUP
        key_config_setup(index); 
    #endif //KEY_CONFIG_SETUP

    #ifdef ADD_CX_SETUP
        add_cx_setup(index);
    #endif // ADD_CX_SETUP

    #ifdef INIT_SETUP
        init_setup(index); 
    #endif // INIT_SETUP

    TDX_bootup(); 
    //TD_setup(index); 
}
#endif // not SOURCE