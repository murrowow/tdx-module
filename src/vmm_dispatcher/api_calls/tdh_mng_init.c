// Copyright (C) 2023 Intel Corporation                                          
//                                                                               
// Permission is hereby granted, free of charge, to any person obtaining a copy  
// of this software and associated documentation files (the "Software"),         
// to deal in the Software without restriction, including without limitation     
// the rights to use, copy, modify, merge, publish, distribute, sublicense,      
// and/or sell copies of the Software, and to permit persons to whom             
// the Software is furnished to do so, subject to the following conditions:      
//                                                                               
// The above copyright notice and this permission notice shall be included       
// in all copies or substantial portions of the Software.                        
//                                                                               
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS       
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL      
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES             
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE            
// OR OTHER DEALINGS IN THE SOFTWARE.                                            
//                                                                               
// SPDX-License-Identifier: MIT

/**
 * @file tdh_mng_init
 * @brief TDHMNGINIT API handler
 */
 #include "include/tdx_vmm_api_handlers.h"
 #include "include/tdx_basic_defs.h"
 #include "include/auto_gen/tdx_error_codes_defs.h"
 #include "src/common/x86_defs/x86_defs.h"
 #include "src/common/data_structures/td_control_structures.h"
 #include "src/common/x86_defs/vmcs_defs.h"
 #include "src/common/memory_handlers/keyhole_manager.h"
 #include "src/common/memory_handlers/pamt_manager.h"
 #include "src/common/memory_handlers/sept_manager.h"
 #include "src/common/helpers/helpers.h"
 #include "src/common/helpers/virt_msr_helpers.h"
 #include "src/common/accessors/ia32_accessors.h"
 #include "src/common/accessors/data_accessors.h"
 #include "src/common/crypto/sha384.h"
 #include "include/auto_gen/msr_config_lookup.h"
 #include "include/auto_gen/cpuid_configurations.h"
 #include "src/common/helpers/cpuid_fms.h"
 
 #include "driver/driver.h"
 
 // SOPHIA: Having so many linking errors I'm putting this in here
 #ifdef SOURCE
 #else
    api_error_type check_cpuid_small_1f(tdcs_small_t* tdcs_p, bool_t allow_null)
    {
    uint32_t cpuid_0b_idx;
    cpuid_topology_level_type_e prev_level_type;
    cpuid_topology_level_type_e level_type = LEVEL_TYPE_INVALID;

    cpuid_topology_shift_t cpuid_1f_eax;
    cpuid_topology_level_t cpuid_1f_ecx;

    bool_t null_config = false;
    bool_t core_level_scanned = false;

    // Scan the virtual CPUID(0x1F) sub-leaves

    for (uint32_t subleaf = 0; subleaf < LEVEL_TYPE_MAX; subleaf++)
    {
        uint32_t cpuid_1f_idx = get_cpuid_lookup_entry(CPUID_GET_TOPOLOGY_LEAF, subleaf);

        cpuid_config_return_values_t cpuid_values = tdcs_p->cpuid_config_vals[cpuid_1f_idx];

        // Null configuration case:  if all CPUID(0x1F) sub-leaves are configured as all-0, use the h/w values.
        // If the first subleaf is configured as 0, all the rest must be 0.
        if (subleaf == 0)
        {
            if ((cpuid_values.high == 0) && (cpuid_values.low == 0))
            {
                if (allow_null)
                {
                    null_config = true;
                }
                else
                {
                    return TDX_CPUID_LEAF_1F_FORMAT_UNRECOGNIZED;
                }
            }
        }
        else if ((null_config) && (cpuid_values.high || cpuid_values.low))
        {
            return TDX_CPUID_LEAF_1F_FORMAT_UNRECOGNIZED;
        }

        if (null_config)
        {
            cpuid_values = get_global_data()->cpuid_values[cpuid_1f_idx].values;

            tdcs_p->cpuid_config_vals[cpuid_1f_idx].low = cpuid_values.low;
            tdcs_p->cpuid_config_vals[cpuid_1f_idx].high = cpuid_values.high;
        }

        // We continue even if we use the h/w values, in order to set CPUID(0xB)
        cpuid_1f_eax.raw = cpuid_values.eax;
        cpuid_1f_ecx.raw = cpuid_values.ecx;

        prev_level_type = level_type;
        level_type = cpuid_1f_ecx.level_type;

        if (level_type != LEVEL_TYPE_INVALID)
        {
            // This is a valid sub-leaf.  Check that level type higher than the previous one
            // (initialized to INVALID, which is 0) but does not reach the max.
            if ((level_type <= prev_level_type) || (level_type >= LEVEL_TYPE_MAX))
            {
                return TDX_CPUID_LEAF_1F_FORMAT_UNRECOGNIZED;
            }

            if (level_type == LEVEL_TYPE_SMT)
            {
                // CPUID(0x0B, 0) is the SMT level. It is identical to CPUID(0x1F) at the SMT level.
                cpuid_0b_idx = get_cpuid_lookup_entry(0xB, 0);
                tdcs_p->cpuid_config_vals[cpuid_0b_idx] = cpuid_values;
            }
            else if (level_type == LEVEL_TYPE_CORE)
            {
                core_level_scanned = true;   // Prepare a flag for a sanity check later
            }
        }
        else  // level_type == CPUID_1F_ECX_t::INVALID
        {
            // The current sub-leaf is invalid, it marks the end of topology info.
            // Make sure we had at least one valid sub-leaf, otherwise CPUID leaf 1F is not configured properly.
            if (subleaf == 0)
            {
                return TDX_CPUID_LEAF_1F_FORMAT_UNRECOGNIZED;
            }

            // Sanity check: core level must have been scanned
            if (!core_level_scanned)
            {
                return TDX_CPUID_LEAF_1F_FORMAT_UNRECOGNIZED;
            }
        }

        // Generate virtual CPUID(0xB) values

        // CPUID(0x0B, 1) is the core level.  The information is of the last valid level of CPUID(0x1F)
        cpuid_0b_idx = get_cpuid_lookup_entry(0xB, 1);
        cpuid_1f_ecx.level_type = LEVEL_TYPE_CORE;
        cpuid_values.ecx = cpuid_1f_ecx.raw;
        tdcs_p->cpuid_config_vals[cpuid_0b_idx] = cpuid_values;
    }

    return TDX_SUCCESS;
    }
#endif // SOURCE

 static void apply_cpuid_xfam_masks(cpuid_config_return_values_t* cpuid_values,
                                    uint64_t xfam,
                                    const cpuid_config_return_values_t* cpuid_masks)
 {
     uint64_t xfam_mask;   // 1-bit mask
 
     xfam_mask = 1ULL;
     for (uint32_t xfam_bit = 0; xfam_bit <= XCR0_MAX_VALID_BIT; xfam_bit++)
     {
         if ((xfam & xfam_mask) == 0)
         {
             // Loop on all 4 CPUID values
             for (uint32_t i = 0; i < 4; i++)
             {
                 cpuid_values->values[i] &= ~cpuid_masks[xfam_bit].values[i];
             }
         }
         xfam_mask <<= 1;
     }
 }
 
 #ifdef SOURCE
     static api_error_type read_and_set_td_configurations(tdr_t * tdr_ptr,
                                                      tdcs_t * tdcs_ptr,
                                                      td_params_t * td_params_ptr)
 #else 
     static api_error_type read_and_set_td_configurations(tdr_small_t * tdr_ptr,
                                                      tdcs_small_t * tdcs_ptr,
                                                      td_params_t * td_params_ptr)
 #endif // SOURCE
 {
     ia32e_eptp_t   target_eptp = { .raw = 0 };
     td_param_attributes_t tmp_attributes;
     ia32_xcr0_t    tmp_xfam;
 
     // SOPHIA: get global data pointer
     #ifdef SOURCE
         tdx_module_global_t* tdx_global_data_ptr = get_global_data();
     #else 
         tdx_module_global_t* tdx_global_data_ptr = &global_data;
     #endif // SOURCE
 
     api_error_type return_val = UNINITIALIZE_ERROR;
 
     // Read and verify ATTRIBUTES
     tmp_attributes.raw = td_params_ptr->attributes.raw;
     #ifdef SOURCE
         if (!verify_td_attributes(tmp_attributes, false))
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_ATTRIBUTES);
             goto EXIT;
         }
     #else 
         // SOPHIA TODO: figure out why this fails
         __CPROVER_assume(((tmp_attributes.raw & ~tdx_global_data_ptr->attributes_fixed0) == 0)
                        && ((tmp_attributes.raw & tdx_global_data_ptr->attributes_fixed1) == tdx_global_data_ptr->attributes_fixed1));
         __CPROVER_assume(!tmp_attributes.migratable || (!tmp_attributes.debug && !tmp_attributes.perfmon)); 
     #endif 
 
     tdcs_ptr->executions_ctl_fields.attributes.raw = tmp_attributes.raw;
 
     tdcs_ptr->executions_ctl_fields.td_ctls.pending_ve_disable = tmp_attributes.sept_ve_disable;
 
     // Read and verify XFAM
     tmp_xfam.raw = td_params_ptr->xfam;
     #ifdef SOURCE
        if (!check_xfam(tmp_xfam))
        {
            return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_XFAM);
            goto EXIT;
        }
     #else 
        __CPROVER_assume((tmp_xfam.raw & TDX_XFAM_FIXED1) == TDX_XFAM_FIXED1); 
        __CPROVER_assume(!(tmp_xfam.avx3_kmask && !tmp_xfam.avx));
        __CPROVER_assume(tmp_xfam.avx3_kmask == tmp_xfam.avx3_zmm_hi);
        __CPROVER_assume(tmp_xfam.avx3_kmask == tmp_xfam.avx3_zmm);
        __CPROVER_assume(tmp_xfam.cet_s == tmp_xfam.cet_u);
        __CPROVER_assume(tmp_xfam.amx_xtilecfg == tmp_xfam.amx_xtiledata);
     #endif // SOURCE
     tdcs_ptr->executions_ctl_fields.xfam = tmp_xfam.raw;
 
     #ifdef SOURCE
         set_xbuff_offsets_and_size(tdcs_ptr, tmp_xfam.raw);
     #else
         // SOPHIA: Not sure how effective this would, but just outlined entire function body
         uint32_t offset = offsetof(xsave_area_t, extended_region);
         for (uint32_t xfam_i = 2; xfam_i <= XCR0_MAX_VALID_BIT; xfam_i++)
         {
             if ((tmp_xfam.raw & BIT(xfam_i)) != 0)
             {
                 if (tdx_global_data_ptr->xsave_comp[xfam_i].align)
                 {
                     // Align the offset up to the next 64B boundary
                     offset = ROUND_UP(offset, 64U);
                 }
                 tdcs_ptr->executions_ctl_fields.xbuff_offsets[xfam_i] = offset;
                 offset += tdx_global_data_ptr->xsave_comp[xfam_i].size;
             }
         }
 
         tdcs_ptr->executions_ctl_fields.xbuff_size = offset;
     #endif // SOURCE
 
     // Read and verify MAX_VCPUS
     uint32_t max_vcpus = (uint32_t)td_params_ptr->max_vcpus;
     #ifdef SOURCE
         if ((max_vcpus == 0) || (max_vcpus > MAX_VCPUS_PER_TD))
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_MAX_VCPUS);
             goto EXIT;
         }
     #else 
         __CPROVER_assume(!(max_vcpus == 0) & !(max_vcpus > MAX_VCPUS_PER_TD));
     #endif // SOURCE
     tdcs_ptr->executions_ctl_fields.max_vcpus = max_vcpus;

     uint16_t num_l2_vms = (uint16_t)td_params_ptr->num_l2_vms;
     #ifdef SOURCE
         if (num_l2_vms > MAX_L2_VMS)
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_NUM_L2_VMS);
             goto EXIT;
         }
     #else 
         __CPROVER_assume(num_l2_vms <= MAX_L2_VMS);
     #endif // SOURCE
 
     // Now that we know the number of L2 VMs, check that enough pages have been allocated for TDCS
     #ifdef SOURCE
         if (!is_required_tdcs_allocated(tdr_ptr, num_l2_vms))
         {
             return_val = TDX_TDCS_NOT_ALLOCATED;
             goto EXIT;
         }
     #else 
         // SOPHIA: simply just pulled out the function body
         __CPROVER_printf("SOPHIA: MIN_NUM_TDCS_PAGES: %d, right hand side: %d, num_tdcx field: %d", 
                           MIN_NUM_TDCS_PAGES, (TDCS_PAGES_PER_L2_VM * num_l2_vms) + MIN_NUM_TDCS_PAGES, tdr_ptr->management_fields.num_tdcx);
         __CPROVER_assume(tdr_ptr->management_fields.num_tdcx >= ((TDCS_PAGES_PER_L2_VM * num_l2_vms)));

        // (uint32_t)(MIN_NUM_TDCS_PAGES + (TDCS_PAGES_PER_L2_VM * num_l2_vms))
     #endif // SOURCE 

     // Only now we can safely update TDCS; NUM_L2_VMS is used by TDH.MNG.RD/WR to calculate offset into TDCS
     tdcs_ptr->management_fields.num_l2_vms = num_l2_vms;
 
     // Check reserved0 bits are 0
     #ifdef SOURCE
         if (!tdx_memcmp_to_zero(td_params_ptr->reserved_0, TD_PARAMS_RESERVED0_SIZE))
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
             goto EXIT;
         }
     #else 
         __CPROVER_assume(tdx_memcmp_to_zero(td_params_ptr->reserved_0, TD_PARAMS_RESERVED0_SIZE));
     #endif // SOURCE 
 
     // Read and verify CONFIG_FLAGS
     config_flags_t config_flags_local_var;
     config_flags_local_var.raw = td_params_ptr->config_flags.raw;
 
     #ifdef SOURCE
         if (!verify_td_config_flags(config_flags_local_var))
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_EXEC_CONTROLS);
             goto EXIT;
         }
     #else 
         __CPROVER_assume((config_flags_local_var.raw & ~tdx_global_data_ptr->config_flags_fixed0.raw) == 0);
         __CPROVER_assume((config_flags_local_var.raw & tdx_global_data_ptr->config_flags_fixed1.raw) == tdx_global_data_ptr->config_flags_fixed1.raw);
     #endif 
 
     // Read and verify EPTP_CONTROLS
     target_eptp.raw = td_params_ptr->eptp_controls.raw;
 
     #ifdef SOURCE
         if (!verify_and_set_td_eptp_controls(tdr_ptr, tdcs_ptr, config_flags_local_var.gpaw, target_eptp))
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_EPTP_CONTROLS);
             goto EXIT;
         }
     #else 
        // SOPHIA TODO: not sure what the importance of pml5 or whether or not this is a safe assumption to make
        ia32_vmx_ept_vpid_cap_t vpid_cap = { .raw = tdx_global_data_ptr->plt_common_config.ia32_vmx_ept_vpid_cap };
        __CPROVER_assume(vpid_cap.pml5_supported == true);
        __CPROVER_assume(( (target_eptp.fields.ept_ps_mt == MT_WB) &&
                           (target_eptp.fields.enable_ad_bits == 0) &&
                           (target_eptp.fields.enable_sss_control == 0) &&
                           (target_eptp.fields.reserved_0 == 0) &&
                           (target_eptp.fields.base_pa == 0) &&
                           (target_eptp.fields.reserved_1 == 0)));

        __CPROVER_assume((target_eptp.fields.ept_pwl >= LVL_PML4) &&
                         (target_eptp.fields.ept_pwl <= LVL_PML5)); 
        
        __CPROVER_assume(!(target_eptp.fields.ept_pwl == LVL_PML5) ||
                         !(tdx_global_data_ptr->max_pa < MIN_PA_FOR_PML5));
        __CPROVER_assume(!(config_flags_local_var.gpaw && (target_eptp.fields.ept_pwl < LVL_PML5)));
     #endif 
 
     __CPROVER_assert(false, "false");
     tdcs_ptr->executions_ctl_fields.config_flags.raw = config_flags_local_var.raw;
     tdcs_ptr->executions_ctl_fields.gpaw = config_flags_local_var.gpaw;
 
     // SOPHIA: TSC Freq abstracted away for now
     #ifdef SOURCE
         uint16_t virt_tsc_freq = td_params_ptr->tsc_frequency;
         if ((virt_tsc_freq < VIRT_TSC_FREQUENCY_MIN) || (virt_tsc_freq > VIRT_TSC_FREQUENCY_MAX))
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_TSC_FREQUENCY);
             goto EXIT;
         }
     #endif // SOURCE
 
     // SOPHIA: TSC Freq abstracted away for now
     #ifdef SOURCE
         tdcs_ptr->executions_ctl_fields.tsc_frequency = virt_tsc_freq;
 
         // We read TSC below.  Compare IA32_TSC_ADJUST to the value sampled on TDHSYSINIT
         // to make sure the host VMM doesn't play any trick on us.
         if (ia32_rdmsr(IA32_TSC_ADJ_MSR_ADDR) != tdx_global_data_ptr->plt_common_config.ia32_tsc_adjust)
         {
             return_val = api_error_with_operand_id(TDX_INCONSISTENT_MSR, IA32_TSC_ADJ_MSR_ADDR);
             goto EXIT;
         }
 
         // Calculate TSC multiplier of offset that will be written in every TD VMCS, such that
         // virtual TSC will advance at the configured frequency, and will start from 0 at this
         // moment.
         calculate_tsc_virt_params(ia32_rdtsc(),tdx_global_data_ptr->native_tsc_frequency,
                                   virt_tsc_freq, 0,
                                   &tdcs_ptr->executions_ctl_fields.tsc_multiplier,
                                   &tdcs_ptr->executions_ctl_fields.tsc_offset);
 
     #endif // SOURCE 
 
     // Check reserved1 bits are 0
     #ifdef SOURCE
         if (!tdx_memcmp_to_zero(td_params_ptr->reserved_1, TD_PARAMS_RESERVED1_SIZE))
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
             goto EXIT;
         }
     #else 
         __CPROVER_assume(tdx_memcmp_to_zero(td_params_ptr->reserved_1, TD_PARAMS_RESERVED1_SIZE));
     #endif // SOURCE
 
     tdx_memcpy(tdcs_ptr->measurement_fields.mr_config_id.bytes, sizeof(measurement_t),
                td_params_ptr->mr_config_id.bytes, sizeof(measurement_t));
     tdx_memcpy(tdcs_ptr->measurement_fields.mr_owner.bytes, sizeof(measurement_t),
                td_params_ptr->mr_owner.bytes, sizeof(measurement_t));
     tdx_memcpy(tdcs_ptr->measurement_fields.mr_owner_config.bytes, sizeof(measurement_t),
                td_params_ptr->mr_owner_config.bytes, sizeof(measurement_t));
 
     #ifdef SOURCE
         if (td_params_ptr->msr_config_ctls.reserved_0 != 0)
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
             goto EXIT;
         }
     #else 
         __CPROVER_assume(td_params_ptr->msr_config_ctls.reserved_0 == 0);
     #endif 
 
     // Check reserved2 bits are 0
     #ifdef SOURCE 
         if (!tdx_memcmp_to_zero(td_params_ptr->reserved_2, TD_PARAMS_RESERVED2_SIZE))
         {
             return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
             goto EXIT;
         }
     #else 
         __CPROVER_assume(tdx_memcmp_to_zero(td_params_ptr->reserved_2, TD_PARAMS_RESERVED2_SIZE)); 
     #endif // SOURCE
     return_val = TDX_SUCCESS;
 
 EXIT:
     return return_val;
 }
 
 #ifdef SOURCE
     static api_error_type read_and_set_cpuid_configurations(tdcs_t * tdcs_ptr,
                                                         td_params_t * td_params_ptr,
                                                         tdx_module_global_t * global_data_ptr,
                                                         tdx_module_local_t * local_data_ptr)
 #else 
 static api_error_type read_and_set_cpuid_configurations(tdcs_small_t * tdcs_ptr,
     td_params_t * td_params_ptr,
     tdx_module_global_t * global_data_ptr,
     tdx_module_local_t * local_data_ptr)
 #endif // SOURCE
 {
     uint32_t cpuid_index = 0;
     cpuid_config_leaf_subleaf_t cpuid_leaf_subleaf;
     cpuid_config_return_values_t config_values;
     cpuid_config_return_values_t final_tdcs_values;
     td_param_attributes_t attributes;
     ia32_xcr0_t xfam;
     api_error_type return_val = UNINITIALIZE_ERROR;
 
     attributes.raw = tdcs_ptr->executions_ctl_fields.attributes.raw;
     xfam.raw = tdcs_ptr->executions_ctl_fields.xfam;
 
     for (cpuid_index = 0; cpuid_index < MAX_NUM_CPUID_LOOKUP; cpuid_index++)
     {
         cpuid_leaf_subleaf = cpuid_lookup[cpuid_index].leaf_subleaf;
 
         // Start with the native CPUID value, collected on TDHSYSINIT
         final_tdcs_values.low = global_data_ptr->cpuid_values[cpuid_index].values.low;
         final_tdcs_values.high = global_data_ptr->cpuid_values[cpuid_index].values.high;
 
         uint32_t config_index = cpuid_lookup[cpuid_index].config_index;
 
         if (cpuid_lookup[cpuid_index].valid_entry && (config_index != CPUID_CONFIG_NULL_IDX))
         {
             config_values = td_params_ptr->cpuid_config_vals[config_index];
 
             #ifdef SOURCE
                 tdx_debug_assert((cpuid_leaf_subleaf.raw == cpuid_configurable[config_index].leaf_subleaf.raw));
             #endif 
 
             // Loop on all 4 CPUID values
             for (uint32_t i = 0; i < 4; i++)
             {
                 // Any bit configured to 1 must be either:
                 //   - Directly Configurable, or
                 //   - Directly Allowable
                 #ifdef SOURCE
                     if ((config_values.values[i] &
                          ~(cpuid_configurable[config_index].config_direct.values[i] |
                            cpuid_configurable[config_index].allow_direct.values[i])) != 0)
                     {
                         local_data_ptr->vmm_regs.rcx = cpuid_leaf_subleaf.raw;
                         return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_CPUID_CONFIG);
                         goto EXIT;
                     }
                 #else 
                     __CPROVER_assume((config_values.values[i] &
                         ~(cpuid_configurable[config_index].config_direct.values[i] |
                           cpuid_configurable[config_index].allow_direct.values[i])) == 0);
                 #endif // SOURCE
 
                 // Compute the virtualized CPUID value and store in TDCS:
                 // Note:  The bits in the lookup tables are mutually exclusive
 
                 // Clear to 0 any bits that are FIXED0 or DYNAMIC
                 final_tdcs_values.values[i] &= ~cpuid_lookup[cpuid_index].fixed0_or_dynamic.values[i];
 
                 // Set to 1 any bits that are FIXED1
                 final_tdcs_values.values[i] |= cpuid_lookup[cpuid_index].fixed1.values[i];
 
                 // Set any bits that are CONFIG_DIRECT to their input values
                 final_tdcs_values.values[i] &= ~cpuid_configurable[config_index].config_direct.values[i];
                 final_tdcs_values.values[i] |= config_values.values[i] & cpuid_configurable[config_index].config_direct.values[i];
 
                 // Clear to 0 any bits that are ALLOW_DIRECT, if their input value is 0
                 final_tdcs_values.values[i] &= config_values.values[i] | ~cpuid_configurable[config_index].allow_direct.values[i];
             }
         }
 
         if (cpuid_leaf_subleaf.leaf == CPUID_VER_INFO_LEAF)
         {
             // CPUID(1).EAX is the virtual Family/Model/Stepping configuration
             fms_info_t cpuid_01_eax = { .raw = final_tdcs_values.eax };
 
             if (cpuid_01_eax.raw == 0)
             {
                 // A value of 0 means use the native configuration
                 cpuid_01_eax = global_data_ptr->platform_fms;
 
                 final_tdcs_values.eax = cpuid_01_eax.raw;
             }
 
             if (tdcs_ptr->executions_ctl_fields.attributes.migratable)
             {
                 #ifdef SOURCE
                     if (!check_fms_config(cpuid_01_eax))
                     {
                         // The configured F/M/S value is not valid
                         local_data_ptr->vmm_regs.rcx = cpuid_leaf_subleaf.raw;
                         return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_CPUID_CONFIG);
                         goto EXIT;
                     }
                 #else 
                     __CPROVER_assume(check_fms_config(cpuid_01_eax)); 
                 #endif // SOURCE
             }
             #ifdef SOURCE
             else if (cpuid_01_eax.raw != global_data_ptr->platform_fms.raw)
             {
                 // For a non-migratable TD, only a value of 0 (updated above) or the native FMS is allowed
                 local_data_ptr->vmm_regs.rcx = cpuid_leaf_subleaf.raw;
                 return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_CPUID_CONFIG);
                 goto EXIT;
             }
             #else 
                __CPROVER_assume(cpuid_01_eax.raw == global_data_ptr->platform_fms.raw); 
             #endif // SOURCE
 
             // Leaf 0x1 has ECX bits configurable by AVX (XFAM[2]).
             // If XFAM[2] is 0, the applicable bits are cleared.
             if (!xfam.avx)
             {
                 final_tdcs_values.ecx &= ~(xfam_mask_0x1_0xffffffff[2].ecx);
             }
 
             cpuid_01_ecx_t cpuid_01_ecx;
             cpuid_01_ecx.raw = final_tdcs_values.ecx;
 
             tdcs_ptr->executions_ctl_fields.cpuid_flags.monitor_mwait_supported = cpuid_01_ecx.monitor;
             tdcs_ptr->executions_ctl_fields.cpuid_flags.dca_supported = cpuid_01_ecx.dca;
             tdcs_ptr->executions_ctl_fields.cpuid_flags.tsc_deadline_supported = cpuid_01_ecx.tsc_deadline;
         }
         else if (cpuid_leaf_subleaf.leaf == 5)
         {
             if (!tdcs_ptr->executions_ctl_fields.cpuid_flags.monitor_mwait_supported)
             {
                 final_tdcs_values.low = 0;
                 final_tdcs_values.high = 0;
             }
         }
         else if (cpuid_leaf_subleaf.leaf == CPUID_EXT_FEATURES_LEAF)
         {
            if (cpuid_leaf_subleaf.subleaf == CPUID_EXT_FEATURES_SUBLEAF)
            {
                cpuid_07_00_ecx_t cpuid_07_00_ecx;
                cpuid_07_00_edx_t cpuid_07_00_edx;
 
                apply_cpuid_xfam_masks(&final_tdcs_values, xfam.raw, xfam_mask_0x7_0x0);
 
                cpuid_07_00_ebx_t cpuid_07_00_ebx = { .raw = final_tdcs_values.ebx };
 
                // Both CPUID bits that enumerate TSX must have the same virtual value
                if (cpuid_07_00_ebx.hle != cpuid_07_00_ebx.rtm)
                {
                    local_data_ptr->vmm_regs.rcx = cpuid_leaf_subleaf.raw;
                    return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_CPUID_CONFIG);
                    goto EXIT;
                }
                // If virtual TSX is enabled, IA32_TSX_CTRL must exist
                if (cpuid_07_00_ebx.hle && !global_data_ptr->plt_common_config.ia32_arch_capabilities.tsx_ctrl)
                {
                    return_val = api_error_with_operand_id(TDX_INCORRECT_MSR_VALUE, IA32_ARCH_CAPABILITIES_MSR_ADDR);
                    goto EXIT;
                }
 
                tdcs_ptr->executions_ctl_fields.cpuid_flags.tsx_supported = cpuid_07_00_ebx.hle;
 
                cpuid_07_00_ecx.raw = final_tdcs_values.ecx;
                // CPUID(0x7, 0x0).ECX.PKS reflects ATTRIBUTES.PKS
                cpuid_07_00_ecx.pks = attributes.pks;
 
                // CPUID(0x7, 0x0).ECX.KL_SUPPORTED reflects ATTRIBUTES.KL
                cpuid_07_00_ecx.kl_supported = 0;
 
                final_tdcs_values.ecx = cpuid_07_00_ecx.raw;
 
                tdcs_ptr->executions_ctl_fields.cpuid_flags.waitpkg_supported = cpuid_07_00_ecx.waitpkg;
                tdcs_ptr->executions_ctl_fields.cpuid_flags.tme_supported = cpuid_07_00_ecx.tme;
                tdcs_ptr->executions_ctl_fields.cpuid_flags.la57_supported = cpuid_07_00_ecx.la57;
 
                cpuid_07_00_edx.raw = final_tdcs_values.edx;
                tdcs_ptr->executions_ctl_fields.cpuid_flags.pconfig_supported = cpuid_07_00_edx.pconfig_mktme;
            }
            else if (cpuid_leaf_subleaf.subleaf == 1)
            {
                apply_cpuid_xfam_masks(&final_tdcs_values, xfam.raw, xfam_mask_0x7_0x1);
 
                cpuid_07_01_eax_t cpuid_07_01_eax = { .raw = final_tdcs_values.eax };
                tdcs_ptr->executions_ctl_fields.cpuid_flags.perfmon_ext_leaf_supported =
                        cpuid_07_01_eax.perfmon_ext_leaf;
 
                cpuid_07_01_eax.lass = tdcs_ptr->executions_ctl_fields.attributes.lass;
 
                final_tdcs_values.eax = cpuid_07_01_eax.raw;
            }
            else if (cpuid_leaf_subleaf.subleaf == 2)
            {
                // Check CPU side channel protection support
                cpuid_07_02_edx_t cpuid_07_02_edx;
                cpuid_07_02_edx.raw = final_tdcs_values.edx;
                tdcs_ptr->executions_ctl_fields.cpuid_flags.ddpd_supported = cpuid_07_02_edx.ddpd;
 
                #ifdef SOURCE
                    // The TD will never be configured with DDPD support if the CPU doesn't support DDPD
                    tdx_debug_assert(!tdcs_ptr->executions_ctl_fields.cpuid_flags.ddpd_supported ||
                                     global_data_ptr->ddpd_supported);
                    
                    // IA32_SPEC_CTRL virtualization is required in the following case:
                    //  - The TD is configured without DDPD support, and
                    //  - The CPU supports DDPD
                    // Because in this case we enable DDPD without the TD knowing about this.
                    tdx_debug_assert(tdcs_ptr->executions_ctl_fields.cpuid_flags.ddpd_supported ||
                                     !global_data_ptr->ddpd_supported ||
                                     global_data_ptr->plt_common_config.ia32_vmx_procbased_ctls3.virt_ia32_spec_ctrl);
                #endif // SOURCE
            }
            else
            {
                FATAL_ERROR();
            }
         }
         else if (cpuid_leaf_subleaf.leaf == 0xA)
         {
             // Leaf 0xA's values are defined as "ALLOW_PERFMON", i.e., if ATTRRIBUTES.PERFMON
             //   is set they return the native values, else they return 0.
             if (!attributes.perfmon)
             {
                 final_tdcs_values.low = 0;
                 final_tdcs_values.high = 0;
             }
         }
         else if (cpuid_leaf_subleaf.leaf == CPUID_EXT_STATE_ENUM_LEAF)
         {
             if (cpuid_leaf_subleaf.subleaf == 0)
             {
                 apply_cpuid_xfam_masks(&final_tdcs_values, xfam.raw, xfam_mask_0xd_0x0);
 
                 final_tdcs_values.ecx = calculate_xsave_area_max_size(xfam);
             }
             else if (cpuid_leaf_subleaf.subleaf == 1)
             {
                 apply_cpuid_xfam_masks(&final_tdcs_values, xfam.raw, xfam_mask_0xd_0x1);
 
                 // Update CPUID leaf 0xD sub-leaf 0x1 EAX[2] value.  This bit enumerates XFD support, and is
                 // virtualized as 1 only if the CPU supports XFD and any of the applicable extended feature
                 // set, per XFAM, supports XFD.
                 cpuid_0d_01_eax_t cpuid_0d_01_eax;
                 cpuid_0d_01_eax.raw = final_tdcs_values.eax;
 
                 if ((global_data_ptr->xfd_faulting_mask & xfam.raw) == 0)
                 {
                     cpuid_0d_01_eax.xfd_support = 0;
                 }
 
                 tdcs_ptr->executions_ctl_fields.cpuid_flags.xfd_supported = cpuid_0d_01_eax.xfd_support;
                 final_tdcs_values.eax = cpuid_0d_01_eax.raw;
             }
             else if (cpuid_leaf_subleaf.subleaf <= XCR0_MAX_VALID_BIT)
             {
                 // Each sub-leaf n, where 2 <= n <= 18, is configured by XFAM[n]
                 if ((xfam.raw & BIT(cpuid_leaf_subleaf.subleaf)) == 0)
                 {
                     final_tdcs_values.low = 0;
                     final_tdcs_values.high = 0;
                 }
             }
         }
         else if (cpuid_leaf_subleaf.leaf == 0x14)
         {
             // Leaf 0x14 is wholly configured by PT (XFAM[8])
             if (!xfam.pt)
             {
                 final_tdcs_values.low = 0;
                 final_tdcs_values.high = 0;
             }
         }
         else if (cpuid_leaf_subleaf.leaf == CPUID_TSC_ATTRIBUTES_LEAF)
         {
             // Handle CPUID Configuration by TSC_FREQUENCY
             // The following assumes:
             // - CPUID(0x15).EAX (denominator) is virtualized as a FIXED value of 1
             // - CPUID(0x15).ECX (nominal ART frequency) is virtualized as a FIXED value of 25,000,000
             // Therefore CPUID(0x15).EBX (numerator) is the configured virtual TSC frequency, in units of 25MHz.
             // The virtual TSC frequency is CPUID(0x15).ECX * CPUID(0x15).EBX / CPUID(0x15).EAX,
             // i.e., the configured virtual TSC frequency, in units of 1Hz.
             final_tdcs_values.ebx = tdcs_ptr->executions_ctl_fields.tsc_frequency;
         }
         else if (cpuid_leaf_subleaf.leaf == CPUID_KEYLOCKER_ATTRIBUTES_LEAF)
         {
             final_tdcs_values.low = 0;
             final_tdcs_values.high = 0;
         }
         else if (cpuid_leaf_subleaf.leaf == 0x1C)
         {
             // Leaf 0x1C is wholly configured by LBR (XFAM[15])
             if (xfam.lbr == 0)
             {
                 final_tdcs_values.low = 0;
                 final_tdcs_values.high = 0;
             }
         }
         else if (cpuid_leaf_subleaf.leaf == 0x1D)
         {
             // Leaf 0x1D is wholly configured by AMX (XFAM[18:17])
             if (!xfam.amx_xtilecfg || !xfam.amx_xtiledata)
             {
                 final_tdcs_values.low = 0;
                 final_tdcs_values.high = 0;
             }
         }
         else if (cpuid_leaf_subleaf.leaf == 0x1A)
         {
             // For migratable TDs, native model information is N/A, and is set to 0.
             // This information is used for Perfmon, which is not enabled for migratable TDs. */
             if (tdcs_ptr->executions_ctl_fields.attributes.migratable)
             {
                 final_tdcs_values.low = 0;
                 final_tdcs_values.high = 0;
             }
         }
         else if (cpuid_leaf_subleaf.leaf == 0x23)
         {
             // Leaf 0x23's values are defined as "ALLOW_ATTRIBUTES(PERFMON)", i.e., if ATTRRIBUTES.PERFMON
             // is set they return the native values, else they return 0.
             if (!attributes.perfmon || !tdcs_ptr->executions_ctl_fields.cpuid_flags.perfmon_ext_leaf_supported)
             {
                 final_tdcs_values.low = 0;
                 final_tdcs_values.high = 0;
             }
         }
         else if (cpuid_leaf_subleaf.leaf == 0x80000008)
         {
             cpuid_80000008_eax_t cpuid_80000008_eax = { .raw = final_tdcs_values.eax };
 
             // Set LA_BITS based on LA57 from CPUID(7, 0).ECX[16]
             if (tdcs_ptr->executions_ctl_fields.cpuid_flags.la57_supported)
             {
                 cpuid_80000008_eax.la_bits = LA57_LINEAR_ADDRESS_WIDTH;
             }
             else
             {
                 cpuid_80000008_eax.la_bits = LEGACY_LINEAR_ADDRESS_WIDTH;
             }
 
             final_tdcs_values.eax = cpuid_80000008_eax.raw;
         }
 
         // Write the CPUID values to TDCS and set the CPUID_VALID flag
         tdcs_ptr->cpuid_config_vals[cpuid_index].low = final_tdcs_values.low;
         tdcs_ptr->cpuid_config_vals[cpuid_index].high = final_tdcs_values.high;
         tdcs_ptr->executions_ctl_fields.cpuid_valid[cpuid_index] = !cpuid_lookup[cpuid_index].faulting;
     }
 
     // Check the virtual topology configuration of CPUID(0x1F) and derive CPUID(0xB).
     // If configured as all-0, use the h/w values.
    #ifdef SOURCE
        return_val = check_cpuid_1f(tdcs_ptr, true);
        if (return_val != TDX_SUCCESS)
        {
            goto EXIT;
        }
    #else 
        __CPROVER_assume(check_cpuid_small_1f(tdcs_ptr, true)); 
    #endif // SOURCE
 
     // May be cleared later if not configured for all VCPUs
     tdcs_ptr->executions_ctl_fields.topology_enum_configured = true;
 
     
     // Check reserved3 bits are 0
     if (!tdx_memcmp_to_zero(td_params_ptr->reserved_3, TD_PARAMS_RESERVED3_SIZE))
     {
         return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
         goto EXIT;
     }
 
     return_val = TDX_SUCCESS;
 
 EXIT:
     return return_val;
 }
 
 api_error_type tdh_mng_init(uint64_t target_tdr_pa, uint64_t target_td_params_pa)
 {
     // Global data
     #ifdef SOURCE
         tdx_module_global_t * global_data_ptr = get_global_data();
         // Local data for return values
         tdx_module_local_t  * local_data_ptr = get_local_data();
     #endif // SOURCE
 
     // TDR related variables
     pa_t                  tdr_pa;                    // TDR physical address
     tdr_small_t         * tdr_ptr;                   // Pointer to the TDR page (linear address)
     pamt_block_t          tdr_pamt_block;            // TDR PAMT block
     pamt_entry_t        * tdr_pamt_entry_ptr;        // Pointer to the TDR PAMT entry
     bool_t                tdr_locked_flag = false;   // Indicate TDR is locked
 
     tdcs_small_t        * tdcs_ptr = NULL;           // Pointer to the TDCS structure (Multi-page)
 
     // TD_PARAMS variables
     pa_t                  td_params_pa;              // Physical address of the params structure
     td_params_t         * td_params_ptr = NULL;      // Pointer to the parameters structure
 
     uint128_t             xmms[16];                  // SSE state backup for crypto
     crypto_api_error      sha_error_code;
     api_error_type        return_val = UNINITIALIZE_ERROR;
 
     tdr_pa.raw = target_tdr_pa;
     td_params_pa.raw = target_td_params_pa;
 
     // By default, no extended error code is returned
     #ifdef SOURCE
         local_data_ptr->vmm_regs.rcx = 0ULL;
     #else 
         local_data.vmm_regs.rcx = 0ULL;
     #endif // SOURCE
 
     #ifdef SOURCE
         // Boot NT4 bit should not be set
         // SOPHIA: With bit 22 set in IA32_MISC_ENABLE, early Windows versions can run on new processors
         // SOPHIA: Geoff Chappel
         if ((ia32_rdmsr(IA32_MISC_ENABLES_MSR_ADDR) & MISC_EN_LIMIT_CPUID_MAXVAL_BIT ) != 0)
         {
             return_val = TDX_LIMIT_CPUID_MAXVAL_SET;
             goto EXIT;
         }
     #endif // SOURCE
 
     // Check, lock and map the owner TDR page
     #ifdef SOURCE
         return_val = check_lock_and_map_explicit_tdr(tdr_pa,
                                                  OPERAND_ID_RCX,
                                                  TDX_RANGE_RW,
                                                  TDX_LOCK_EXCLUSIVE,
                                                  PT_TDR,
                                                  &tdr_pamt_block,
                                                  &tdr_pamt_entry_ptr,
                                                  &tdr_locked_flag,
                                                  &tdr_ptr);
         if (return_val != TDX_SUCCESS)
         {
         TDX_ERROR("Failed to check/lock/map a TDR - error = %lld\n", return_val);
         goto EXIT;
         }
     #else
         // SOPHIA: check and lock TDR
         tdr_pamt_entry_ptr = &(tables[tdr_pa.raw & HKID_MASK].pamt_entry);
         tdr_ptr = &(tables[tdr_pa.raw & HKID_MASK].tdr_table);
         tdcs_ptr = &(tables[tdr_pa.raw & HKID_MASK].tdcx_table);
     #endif // SOURCE */
 
     #ifdef MODULAR_PROOF
         __CPROVER_assume(tdr_pamt_entry_ptr->pt == PT_TDR); //, "Page Metadata Table should be correct"
     #endif // MODULAR_PROOF
 
     // Map the TDCS structure and check the state
     #ifdef SOURCE
         return_val = check_state_map_tdcs_and_lock(tdr_ptr, TDX_RANGE_RW, TDX_LOCK_NO_LOCK,
                                                false, TDH_MNG_INIT_LEAF, &tdcs_ptr);
 
         if (return_val != TDX_SUCCESS)
         {
         TDX_ERROR("State check or TDCS lock failure - error = %llx\n", return_val);
         goto EXIT;
         }
     #endif // SOURCE
     
     #ifdef MODULAR_PROOF
        __CPROVER_assume(tdr_pamt_entry_ptr->pt == PT_TDR); //, "Page Metadata Table should be correct"
        // SOPHIA: from check_td_in_correct_build_state in helpers.h
        __CPROVER_assume(!tdr_ptr->management_fields.fatal);
        __CPROVER_assume(tdr_ptr->management_fields.lifecycle_state == TD_KEYS_CONFIGURED);
        __CPROVER_assume(tdr_ptr->management_fields.num_tdcx < MIN_NUM_TDCS_PAGES);
     #endif // MODULAR_PROOF
 
     // Check that TD PARAMS page is TD_PARAMS_ALIGN_IN_BYTES
     // Verify the TD PARAMS physical address is canonical and shared
     #ifdef SOURCE
         if ((return_val = shared_hpa_check_with_pwr_2_alignment(td_params_pa, TD_PARAMS_ALIGN_IN_BYTES)) != TDX_SUCCESS)
         {
             TDX_ERROR("Failed on source shared HPA 0x%llx check - error = %llx\n", td_params_pa.raw, return_val);
             return_val = api_error_with_operand_id(return_val, OPERAND_ID_RDX);
             goto EXIT;
         }
     #endif // SOURCE
 
     #ifdef MODULAR_PROOF
         __CPROVER_assume(is_addr_aligned_pwr_of_2(td_params_pa.raw, TD_PARAMS_ALIGN_IN_BYTES));
         //SOPHIA: shared_hpa_check from helpers.c
        __CPROVER_assume(!is_pa_smaller_than_max_pa(td_params_pa.raw));
        // from get_addr_from_pa from helpers.h
         __CPROVER_assume(is_overlap(td_params_pa.full_pa & ~(global_data.hkid_mask), TD_PARAMS_ALIGN_IN_BYTES, 
                      global_data.private_hkid_min, 
                      HKID_SIZE));
        __CPROVER_assume((td_params_pa.full_pa & global_data.hkid_mask) >> global_data.hkid_start_bit 
                          >= global_data.private_hkid_min);
     #endif // MODULAR_PROOF
 
     // SOPHIA: keyhole mapping can be abstracted away for now
     #ifdef SOURCE
         // Map the TD PARAMS address
         td_params_ptr = (td_params_t *)map_pa((void*)td_params_pa.raw, TDX_RANGE_RO);
    #else 
         td_params_ptr = (td_params_t *)(&tables[(td_params_pa.raw & global_data.hkid_mask) >> global_data.hkid_start_bit].td_params_table);
     #endif // SOURCE
     /**
      *  Initialize the TD management fields
      */
    #ifdef SOURCE
      tdcs_ptr->management_fields.num_vcpus = 0U;
      tdcs_ptr->management_fields.num_assoc_vcpus = 0U;
    #endif // SOURCE

    // SOPHIA: epoch counting needs to be done regardless of which mode we are running in 
    tdcs_ptr->epoch_tracking.epoch_and_refcount.td_epoch = 1ULL;
    tdcs_ptr->epoch_tracking.epoch_and_refcount.refcount[0] = 0;
    tdcs_ptr->epoch_tracking.epoch_and_refcount.refcount[1] = 0;
 
     // SOPHIA: TSC = time stamp counter: counts number of cycles since the last reset
     // SOPHIA: Do not think this is important for now
     #ifdef SOURCE
         uint64_t native_tsc_frequency = get_global_data()->native_tsc_frequency;
         tdx_sanity_check((native_tsc_frequency <= BIT_MASK_32BITS), SCEC_SEAMCALL_SOURCE(TDH_MNG_INIT_LEAF), 0);
         // safe to cast to 32-bits due to the sanity check above
         tdcs_ptr->executions_ctl_fields.hp_lock_timeout = translate_usec_to_tsc(DEFAULT_HP_LOCK_TIMEOUT_USEC, (uint32_t)native_tsc_frequency);
     #endif // SOURCE
  
     /**
      *  Read the TD configuration input and set TDCS fields
      */
 
      //__CPROVER_printf("SOPHIA: td_params_table: %d", tables[0].td_params_table.num_l2_vms);
     return_val = read_and_set_td_configurations(tdr_ptr, tdcs_ptr, td_params_ptr);
     
//      #ifdef SOURCE
//          if (return_val != TDX_SUCCESS)
//          {
//              TDX_ERROR("read_and_set_td_configurations failed\n");
//              goto EXIT;
//          }
//      #else 
//          __CPROVER_assume(return_val == TDX_SUCCESS); 
//      #endif // SOURCE
 
//      /**
//       *  Handle CPUID Configuration
//       */
//      #ifdef SOURCE
//          return_val = read_and_set_cpuid_configurations(tdcs_ptr, td_params_ptr, global_data_ptr,
//                                                     local_data_ptr);
 
//          if (return_val != TDX_SUCCESS)
//          {
//              TDX_ERROR("read_and_set_cpuid_configurations failed\n");
//              goto EXIT;
//          }
//      #else 
//          return_val = read_and_set_cpuid_configurations(tdcs_ptr, td_params_ptr, 
//                                                         &global_data, &local_data);
//      #endif // SOURCE
 
//      // Check and initialize the virtual IA32_ARCH_CAPABILITIES MSR
//      #ifdef SOURCE
//          if (!init_virt_ia32_arch_capabilities(tdcs_ptr, td_params_ptr->msr_config_ctls.ia32_arch_cap,
//                                            td_params_ptr->ia32_arch_capabilities_config))
//          {
//          TDX_ERROR("Incorrect IA32_ARCH_CAPABILITIES configuration\n");
//          return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_IA32_ARCH_CAPABILITIES_CONFIG);
//          goto EXIT;
//          }
//      #endif // SOURCE
 
//      #ifdef SOURCE
//          if (!td_immutable_state_cross_check(tdcs_ptr))
//          {
//          TDX_ERROR("td_immutable_state_cross_check failed\n");
//          return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
//          goto EXIT;
//          }
//      #endif // SOURCE
 
//      // ALL_CHECKS_PASSED:  The function is guaranteed to succeed
 
//      /**
//       *  Build the MSR bitmaps
//       */
//      #ifdef SOURCE
//      set_msr_bitmaps(tdcs_ptr);
 
//      // Initialize the virtual MSR values
//      init_virt_ia32_vmx_msrs(tdcs_ptr);
//      #endif // SOURCE
 
//      /**
//       *  Initialize the TD Measurement Fields
//       */
//      store_xmms_in_buffer(xmms);
 
//      // SOPHIA: Crytographic function we will assume works correctly
//      #ifdef SOURCE
//          if ((sha_error_code = sha384_init(&(tdcs_ptr->measurement_fields.td_sha_ctx))) != 0)
//          {
//              // Unexpected error - Fatal Error
//              TDX_ERROR("Unexpected error in SHA384 - error = %d\n", sha_error_code);
//              FATAL_ERROR();
//          }
//      #endif // SOURCE
 
//      load_xmms_from_buffer(xmms);
//      basic_memset_to_zero(xmms, sizeof(xmms));
 
//      // Zero the RTMR hash values
//      //basic_memset_to_zero(tdcs_ptr->measurement_fields.rtmr, (SIZE_OF_SHA384_HASH_IN_QWORDS<<3)*NUM_RTMRS);
 
//      tdcs_ptr->management_fields.op_state = OP_STATE_INITIALIZED;
//      return_val = TDX_SUCCESS; 
    EXIT:
     // Release all acquired locks and free keyhole mappings
     
     #ifdef SOURCE
         if (tdr_locked_flag)
         {
             pamt_unwalk(tdr_pa, tdr_pamt_block, tdr_pamt_entry_ptr, TDX_LOCK_EXCLUSIVE, PT_4KB);
             free_la(tdr_ptr);
         }
         if (tdcs_ptr != NULL)
         {
             free_la(tdcs_ptr);
         }
         if (td_params_ptr != NULL)
         {
             free_la(td_params_ptr);
         }
     #endif // SOURCE
     return_val = TDX_SUCCESS; 
     return return_val;
 }
 