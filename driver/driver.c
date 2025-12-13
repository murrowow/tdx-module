#include "driver/driver.h"
#include "driver/flows/flows.h"
#include "stdlib.h"
#include "stdint.h"

#include "../src/common/helpers/virt_msr_helpers.h"
#include "../include/auto_gen/cpuid_configurations.h"

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

void bootup_setup() {
        __CPROVER_havoc_object(&sysinfo); 
        __CPROVER_havoc_object(&global_data); 
        __CPROVER_havoc_object(&local_data); 
        __CPROVER_havoc_object(&tables); 
        __CPROVER_havoc_object(&msr_values_ptr_model); 
        __CPROVER_havoc_object(&seamop_cap_model); 

        __CPROVER_assume(sysinfo.num_handoff_pages >= TDX_MIN_HANDOFF_PAGES); 

        __CPROVER_assume(global_data.global_state.sys_state == SYSINIT_PENDING); 
        __CPROVER_assume(global_data.kot.lock.raw == SHAREX_FREE); 
        __CPROVER_assume(global_data.global_lock.raw == SHAREX_FREE);
        __CPROVER_assume(global_data.hkid_start_bit == (64 - n - 1));

        for (uint64_t i = 0; i < MAX_CMR; i++)
        {
            uint64_t cmr_area_start = sysinfo.cmr_data[i].cmr_base;
            uint64_t cmr_area_start_plus_size = sysinfo.cmr_data[i].cmr_base + sysinfo.cmr_data[i].cmr_size;
            __CPROVER_assume(global_data.tdmr_info_copy[i].pamt_1g_base >= cmr_area_start);
            __CPROVER_assume((global_data.tdmr_info_copy[i].pamt_1g_base + global_data.tdmr_info_copy[i].pamt_1g_size) <= cmr_area_start_plus_size);  
            __CPROVER_assume(global_data.tdmr_info_copy[i].pamt_2m_base >= cmr_area_start);
            __CPROVER_assume((global_data.tdmr_info_copy[i].pamt_2m_base + global_data.tdmr_info_copy[i].pamt_2m_size) <= cmr_area_start_plus_size);  
            __CPROVER_assume(global_data.tdmr_info_copy[i].pamt_4k_base >= cmr_area_start);
            __CPROVER_assume((global_data.tdmr_info_copy[i].pamt_4k_base + global_data.tdmr_info_copy[i].pamt_4k_size) <= cmr_area_start_plus_size);  
            
        }
        
        for (int i = 0; i < MAX_TDMRS; i++) { // MAX_TDMRS = 64
            __CPROVER_assume(global_data.tdmr_info_copy[i].tdmr_base == 0);
            __CPROVER_assume(global_data.tdmr_info_copy[i].tdmr_size > 0);
            __CPROVER_assume(global_data.tdmr_info_copy[i].tdmr_base <= (MAX_UINT64 - global_data.tdmr_info_copy[i].tdmr_size));
            uint64_t tdmr_end = global_data.tdmr_info_copy[i].tdmr_base + global_data.tdmr_info_copy[i].tdmr_size - 1;
            __CPROVER_assume(((tdmr_end & global_data.hkid_mask) >> global_data.hkid_start_bit) == 0);
            __CPROVER_assume(is_addr_aligned_pwr_of_2(global_data.tdmr_info_copy[i].tdmr_size, _1GB)); 
            for (uint32_t j = 0; j < MAX_RESERVED_AREAS; j++) {
                uint64_t area_offset = global_data.tdmr_info_copy[i].rsvd_areas[j].offset;
                uint64_t area_size = global_data.tdmr_info_copy[i].rsvd_areas[j].size;
                __CPROVER_assume(area_offset <= (MAX_UINT64 - area_size)); 
                uint64_t prev_area_offset, prev_area_size;
                if (j < MAX_RESERVED_AREAS-1 ) {
                    __CPROVER_assume(global_data.tdmr_info_copy[i].rsvd_areas[j+1].size == 0); 
                    __CPROVER_assume(global_data.tdmr_info_copy[i].rsvd_areas[j+1].size == 0); 
                }

                if (j > 0) {
                    prev_area_offset = global_data.tdmr_info_copy[i].rsvd_areas[j-1].offset;
                    prev_area_size = global_data.tdmr_info_copy[i].rsvd_areas[j-1].size;

                    __CPROVER_assume(area_offset >= prev_area_offset);
                    __CPROVER_assume((area_offset >= prev_area_offset + prev_area_size));
                }
                 __CPROVER_assume(is_addr_aligned_pwr_of_2(area_offset, _4KB) &&
                                  is_addr_aligned_pwr_of_2(area_size, _4KB)); 

                uint64_t tdmr_start =  global_data.tdmr_info_copy[i].tdmr_base;
                uint64_t tdmr_end = global_data.tdmr_info_copy[i].tdmr_base + global_data.tdmr_info_copy[i].tdmr_size;
                uint64_t rsvd_start = tdmr_start + area_offset;
                __CPROVER_assume(is_valid_integer_range(rsvd_start, area_size));
            
                uint64_t rsvd_end = rsvd_start + area_size;
                __CPROVER_assume((rsvd_start >= tdmr_start) && (rsvd_end <= tdmr_end));
            }
        }

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
}

void sys_lp_init_setup() {
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
}

void sys_rd_setup() {
    __CPROVER_havoc_object(&local_data); 
    __CPROVER_assume(local_data.lp_is_init);
}

void sys_config_setup() {
        __CPROVER_havoc_object(&global_data);
        __CPROVER_havoc_object(&tables); 
        __CPROVER_havoc_object(&sysinfo); 

        __CPROVER_assume(global_data.global_state.sys_state == SYSINIT_DONE); 
        __CPROVER_assume(global_data.num_of_init_lps == global_data.num_of_lps);
        __CPROVER_assume(global_data.global_lock.raw == SHAREX_FREE);
        __CPROVER_assume(global_data.hkid_mask == HKID_MASK); 
        __CPROVER_assume(global_data.hkid_start_bit == (sizeof(signed long int) - n - 1));

        // SOPHIA TODO: add to BOOTUP
        for (uint64_t i = 0; i < MAX_CMR; i++)
        {
            uint64_t cmr_area_start = sysinfo.cmr_data[i].cmr_base;
            uint64_t cmr_area_start_plus_size = sysinfo.cmr_data[i].cmr_base + sysinfo.cmr_data[i].cmr_size;
            __CPROVER_assume(global_data.tdmr_info_copy[i].pamt_1g_base >= cmr_area_start);
            __CPROVER_assume((global_data.tdmr_info_copy[i].pamt_1g_base + global_data.tdmr_info_copy[i].pamt_1g_size) <= cmr_area_start_plus_size);  
            __CPROVER_assume(global_data.tdmr_info_copy[i].pamt_2m_base >= cmr_area_start);
            __CPROVER_assume((global_data.tdmr_info_copy[i].pamt_2m_base + global_data.tdmr_info_copy[i].pamt_2m_size) <= cmr_area_start_plus_size);  
            __CPROVER_assume(global_data.tdmr_info_copy[i].pamt_4k_base >= cmr_area_start);
            __CPROVER_assume((global_data.tdmr_info_copy[i].pamt_4k_base + global_data.tdmr_info_copy[i].pamt_4k_size) <= cmr_area_start_plus_size);  
            
        }
        //__CPROVER_assume((tables[0].tdmr_info_table.tdmr_base + tables[0].tdmr_info_table.tdmr_size - 1) < MAX_PA );
        //__CPROVER_assume((tables[1].tdmr_info_table.tdmr_base + tables[1].tdmr_info_table.tdmr_size - 1) < MAX_PA );
        //__CPROVER_assume(tables[0].tdmr_table.pa.raw & (TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT -  1) == 0); 
        //__CPROVER_assume(tables[1].tdmr_table.pa.raw & (TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT -  1) == 0); 

}

void sys_key_config_setup(){
    __CPROVER_havoc_object(&global_data);
    __CPROVER_havoc_object(&tables); 

    __CPROVER_assume(global_data.global_state.sys_state == SYSCONFIG_DONE);
    __CPROVER_assume((global_data.private_hkid_min == 0x00000000)); 
    __CPROVER_assume((global_data.private_hkid_max == global_data.private_hkid_min + (HKID_SIZE)));
}

void driver_main() {

    // TDX Setup 
    #ifdef BOOTUP_SETUP 
        bootup_setup(); 
    #endif // BOOTUP_SETUP

    #ifdef SYS_LP_INIT_SETUP 
        sys_lp_init_setup(); 
    #endif // SYS_LP_INIT_SETUP]

    #ifdef SYS_RD_SETUP
        sys_rd_setup(); 
    #endif // SYS_RD_SETUP

    #ifdef SYS_CONFIG_SETUP
        sys_config_setup(); 
    #endif // SYS_CONFIG_SETUP

    #ifdef SYS_KEY_CONFIG_SETUP
        sys_key_config_setup(); 
    #endif // SYS_KEY_CONFIG_SETUP
    
    // TD Setup
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

    page_info_api_input_t sept_level_and_gpa;
    __CPROVER_havoc_object(&sept_level_and_gpa);
    __CPROVER_assume(sept_level_and_gpa.level >= 0 && sept_level_and_gpa.level <= 3); 
    page_info_api_input_t gpa_page_info; 
    __CPROVER_havoc_object(&gpa_page_info);
    __CPROVER_assume(gpa_page_info.level >= 0 && gpa_page_info.level <= 3);
    #ifdef TD_MEM_SETUP
        __CPROVER_havoc_object(&local_data);
        __CPROVER_havoc_object(&global_data);
        __CPROVER_havoc_object(&tables);

        for (int i = 0; i < HKID_SIZE; i++) {
        __CPROVER_assume(!tables[i].tdr.management_fields.fatal); // TD is not in fatal state
        __CPROVER_assume(tables[i].tdr.management_fields.lifecycle_state == TD_KEYS_CONFIGURED); // TD keys are configured 
        __CPROVER_assume(tables[i].tdr.management_fields.num_tdcx >= MIN_NUM_TDCS_PAGES); // Minimal num of TDCS pages allocated
        __CPROVER_assume(tables[i].tdcs_table.management_fields.num_l2_vms < MAX_VMS);
        __CPROVER_assume(tables[i].tdr.management_fields.num_tdcx < MAX_NUM_TDCS_PAGES);
        __CPROVER_assume(!tables[i].sept_page_lock);
        __CPROVER_assume(verify_page_info_input(sept_level_and_gpa, LVL_PD, tables[i].tdcs_table.executions_ctl_fields.eptp.fields.ept_pwl));
        __CPROVER_assume(verify_page_info_input(gpa_page_info, LVL_PT, LVL_PT));
    }

        /* Bounds for global data fields used by helpers to avoid undefined shifts and NULL-pointer style warnings */
        __CPROVER_assume(global_data.hkid_mask != 0);
        __CPROVER_assume(global_data.hkid_start_bit >= 0 && global_data.hkid_start_bit <= 52);

        /* Make a few platform fields concrete enough for helpers that read plt_common_config */
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_basic.vmcs_region_size <= TD_VMCS_SIZE);
        __CPROVER_assume(global_data.plt_common_config.ia32_vmx_basic.ia32_vmx_true_available == 1U);
        __CPROVER_assume(global_data.plt_common_config.ia32_mtrrcap.smrr != 0);

        /* Constrain generic sizes used by index calculations */
        __CPROVER_assume(MAX_VMS > 0 && MAX_VMS <= 16);
    #endif // TD_MEM_SETUP

    #ifdef TD_ENTER_SETUP
        __CPROVER_havoc_object(&local_data);
        __CPROVER_havoc_object(&global_data);
        __CPROVER_havoc_object(&tables);

        for (int i = 0; i < HKID_SIZE; i++) {
            __CPROVER_assume(!tables[i].tdr.management_fields.fatal); // TD is not in fatal state
            __CPROVER_assume(tables[i].tdvps_table.management.state == VCPU_READY);
            __CPROVER_assume(tables[i].tdvps_table.management.curr_vm == 0);
            __CPROVER_assume(!tables[i].tdcs_table.executions_ctl_fields.cpuid_flags.monitor_mwait_supported); // TD memory is configured
        }
    #endif // TD_ENTER_SETUP

    #ifdef TD_EXIT_SETUP
        __CPROVER_havoc_object(&local_data);
        __CPROVER_havoc_object(&global_data);
        __CPROVER_havoc_object(&tables);

        for (int i = 0; i < HKID_SIZE; i++) {
            __CPROVER_assume(!tables[i].tdr.management_fields.fatal); // TD is not in fatal state
            __CPROVER_assume(tables[i].tdvps_table.management.state == VCPU_READY);
            __CPROVER_assume(tables[i].tdvps_table.management.curr_vm == 0);
            __CPROVER_assume(!tables[i].tdcs_table.executions_ctl_fields.cpuid_flags.monitor_mwait_supported); // TD memory is configured
        }
        uint64_t controller_value;
        __CPROVER_havoc_object(&controller_value);
        uint16_t gpr_check_mask = (uint16_t)(BIT(0) | BIT(1) | BIT(4));
        tdvmcall_control_t control = { .raw = controller_value };
        __CPROVER_assume((control.gpr_select & gpr_check_mask) == 0);
        __CPROVER_assume(control.reserved == 0);
    #endif // TD_EXIT_SETUP

    #ifdef TD_ADD_PAGE_SETUP
        __CPROVER_havoc_object(&local_data);
        __CPROVER_havoc_object(&global_data);
        __CPROVER_havoc_object(&tables);

        for (int i = 0; i < HKID_SIZE; i++) {
            __CPROVER_assume(!tables[i].tdr.management_fields.fatal); // TD is not in fatal state
            __CPROVER_assume(tables[i].tdvps_table.management.state == VCPU_READY);
            __CPROVER_assume(tables[i].tdvps_table.management.curr_vm == 0);
            __CPROVER_assume(!tables[i].tdcs_table.executions_ctl_fields.cpuid_flags.monitor_mwait_supported); // TD memory is configured
            __CPROVER_assume(tables[i].tdr.management_fields.lifecycle_state == TD_KEYS_CONFIGURED); // TD keys are configured 
            __CPROVER_assume(tables[i].tdr.management_fields.num_tdcx >= MIN_NUM_TDCS_PAGES); // Minimal num of TDCS pages allocated
            __CPROVER_assume(tables[i].tdcs_table.management_fields.num_l2_vms < MAX_VMS);
            __CPROVER_assume(tables[i].tdr.management_fields.num_tdcx < MAX_NUM_TDCS_PAGES);
            __CPROVER_assume(!tables[i].sept_page_lock);
            __CPROVER_assume(verify_page_info_input(sept_level_and_gpa, LVL_PD, tables[i].tdcs_table.executions_ctl_fields.eptp.fields.ept_pwl));
            __CPROVER_assume(verify_page_info_input(gpa_page_info, LVL_PT, LVL_PT));
        }
        uint64_t controller_value;
        __CPROVER_havoc_object(&controller_value);
        uint16_t gpr_check_mask = (uint16_t)(BIT(0) | BIT(1) | BIT(4));
        tdvmcall_control_t control = { .raw = controller_value };
        __CPROVER_assume((control.gpr_select & gpr_check_mask) == 0);
        __CPROVER_assume(control.reserved == 0);
    #endif // TD_ADD_PAGE_SETUP

    #ifdef TD_REMOVE_PAGE_SETUP
        __CPROVER_havoc_object(&local_data);
        __CPROVER_havoc_object(&global_data);
        __CPROVER_havoc_object(&tables);

        uint64_t controller_value;
        __CPROVER_havoc_object(&controller_value);
    #endif // TD_REMOVE_PAGE_SETUP
    
    // Call the flows
    // TDX_bootup(); 
    // TD_setup(index); 
    // TD_mem_setup(sept_level_and_gpa, gpa_page_info); 
    // TD_enter();
    // TD_exit(controller_value);
    //TD_add_page(controller_value, sept_level_and_gpa, gpa_page_info); 
    TD_remove_page(controller_value); 
}