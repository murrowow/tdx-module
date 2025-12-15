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
 * @file tdh_mng_vpflushdone
 * @brief TDHMNGVPFLUSHDONE API handler
 */
#include "include/tdx_vmm_api_handlers.h"
#include "include/tdx_basic_defs.h"
#include "include/auto_gen/tdx_error_codes_defs.h"
#include "src/common/x86_defs/x86_defs.h"
#include "src/common/data_structures/tdx_global_data.h"
#include "src/common/data_structures/td_control_structures.h"
#include "src/common/memory_handlers/keyhole_manager.h"
#include "src/common/memory_handlers/pamt_manager.h"
#include "src/common/helpers/helpers.h"
#include "src/common/accessors/data_accessors.h"

#include "driver/driver.h"

api_error_type tdh_mng_vpflushdone(uint64_t target_tdr_pa)
{
    // TDX Global data
    #ifdef SOURCE
    tdx_module_global_t * global_data_ptr = get_global_data();
    #else 
    tdx_module_global_t * global_data_ptr = &global_data;
    #endif // SOURCE

    // TDR related variables
    pa_t                  tdr_pa = {.raw = target_tdr_pa}; // TDR physical address
    tdr_t               * tdr_ptr;                         // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;                  // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;              // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;         // Indicate TDR is locked

    tdcs_t              * tdcs_ptr = NULL;                 // Pointer to the TDCS structure (Multi-page)

    uint16_t              curr_hkid;
    bool_t                kot_locked_flag = false;         // Indicates whether KOT is locked

    api_error_type        return_val = UNINITIALIZE_ERROR;

    /**
     * Check TDR (explicit access, opaque semantics, exclusive lock).
     */
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
        tdr_pamt_entry_ptr = &(tables[target_tdr_pa & HKID_MASK].pamt_entry);
        tdr_ptr = &tables[target_tdr_pa& HKID_MASK].tdr;
    #endif // SOURCE

    #ifdef SOURCE
    // Acquire exclusive access to KOT
    if (acquire_sharex_lock_ex(&global_data_ptr->kot.lock) != LOCK_RET_SUCCESS)
    {
        TDX_ERROR("Failed to acquire lock on KOT\n");
        return_val = api_error_with_operand_id(TDX_OPERAND_BUSY, OPERAND_ID_KOT);
        goto EXIT;
    }
    kot_locked_flag = true;
    #endif // SOURCE

    // Get the TD's ephemeral HKID
    #ifdef SOURCE
    curr_hkid = tdr_ptr->key_management_fields.hkid;
    #else 
    curr_hkid = target_tdr_pa & HKID_MASK; 
    #endif // SOURCE

    #ifdef SOURCE
    // Verify TD life cycle state
    if ((tdr_ptr->management_fields.lifecycle_state != TD_KEYS_CONFIGURED) &&
        (tdr_ptr->management_fields.lifecycle_state != TD_HKID_ASSIGNED))
    {
        TDX_ERROR("Incorrect TD life cycle %d\n", tdr_ptr->management_fields.lifecycle_state);
        return_val = TDX_LIFECYCLE_STATE_INCORRECT;
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume((tdr_ptr->management_fields.lifecycle_state == TD_KEYS_CONFIGURED) ||
                         (tdr_ptr->management_fields.lifecycle_state == TD_HKID_ASSIGNED)); // TD in correct life cycle state
    #endif // MODULAR_PROOF
    return TDX_SUCCESS; 
    #ifdef FLOW_PROOF
        if ((tables[target_tdr_pa & HKID_MASK].tdr.management_fields.lifecycle_state != TD_KEYS_CONFIGURED) &&
            (tables[target_tdr_pa & HKID_MASK].tdr.management_fields.lifecycle_state != TD_HKID_ASSIGNED))
        {
            //__CPROVER_assert((tables[target_tdr_pa & HKID_MASK].tdr.management_fields.lifecycle_state == TD_KEYS_CONFIGURED) ||
            //                 (tables[target_tdr_pa & HKID_MASK].tdr.management_fields.lifecycle_state == TD_HKID_ASSIGNED), "TD in correct life cycle state");
            TDX_ERROR("Incorrect TD life cycle %d\n", tables[target_tdr_pa & HKID_MASK].tdr.management_fields.lifecycle_state);
            return_val = TDX_LIFECYCLE_STATE_INCORRECT;
            goto EXIT;
        }
    #endif // FLOW_PROOF

    #ifdef SOURCE
    // Verify KOT entry state
    tdx_debug_assert(global_data_ptr->kot.entries[curr_hkid].state == KOT_STATE_HKID_ASSIGNED);
    #endif // SOURCE
    /**
     * At this point no new concurrent VCPU association can be done.
     * Verify that the number of associated VCPUs is 0.
     */
     #ifdef SOURCE
    if (tdr_ptr->management_fields.num_tdcx >= MIN_NUM_TDCS_PAGES)
    {
        // Map the TDCS structure and check the state. No need to lock
        tdcs_ptr = map_implicit_tdcs(tdr_ptr, TDX_RANGE_RO, false);
        if (op_state_is_any_initialized(tdcs_ptr->management_fields.op_state) &&
            (tdcs_ptr->management_fields.num_assoc_vcpus != 0))
        {
            TDX_ERROR("TD associated vcpus is (%d) and not zero\n",
                      tdcs_ptr->management_fields.num_assoc_vcpus);
            return_val = TDX_FLUSHVP_NOT_DONE;
            goto EXIT;
        }
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume((tdr_ptr->management_fields.num_tdcx < MIN_NUM_TDCS_PAGES) ||
                         ((!state_flags_lookup[tdcs_ptr->management_fields.op_state].any_initialized) ||
                         (tdcs_ptr->management_fields.num_assoc_vcpus == 0))); // No associated VCPUs
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
        if (tables[target_tdr_pa & HKID_MASK].tdr_table.management_fields.num_tdcx >= MIN_NUM_TDCS_PAGES)
        {
            // Map the TDCS structure and check the state. No need to lock
            tdcs_ptr = &tables[target_tdr_pa & HKID_MASK].tdcs_table;
            if (state_flags_lookup[tables[target_tdr_pa & HKID_MASK].tdcs_table.management_fields.op_state].any_initialized &&
                (tables[target_tdr_pa & HKID_MASK].tdcs_table.management_fields.num_assoc_vcpus != 0))
            {
                //__CPROVER_assert((tdr_ptr->management_fields.num_tdcx < MIN_NUM_TDCS_PAGES) ||
                //                 (!op_state_is_any_initialized(tdcs_ptr->management_fields.op_state)) ||
                //                 (tdcs_ptr->management_fields.num_assoc_vcpus == 0), "No associated VCPUs");
                TDX_ERROR("TD associated vcpus is (%d) and not zero\n",
                          tdcs_ptr->management_fields.num_assoc_vcpus);
                return_val = TDX_FLUSHVP_NOT_DONE;
                goto EXIT;
            }
        }
    #endif  // FLOW_PROOF
    // 1m 38s
    // ALL_CHECKS_PASSED:  The function is guaranteed to succeed

    /**
     * Create the WBINVD_BITMAP per-package.
     * Set to 1 num_of_pkgs bits from the LSB
     */
    #ifdef SOURCE
    global_data_ptr->kot.entries[curr_hkid].wbinvd_bitmap = global_data_ptr->pkg_config_bitmap;

    // Set new TD life cycle state
    tdr_ptr->management_fields.lifecycle_state = TD_BLOCKED;

    // Set the proper new KOT entry state
    global_data_ptr->kot.entries[curr_hkid].state = (uint8_t)KOT_STATE_HKID_FLUSHED;
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        // Mark the HKID entry in the KOT as assigned
        global_data.kot.entries[curr_hkid & HKID_MASK].wbinvd_bitmap = global_data.pkg_config_bitmap;
        global_data.kot.entries[curr_hkid & HKID_MASK].state = (uint8_t)KOT_STATE_HKID_FLUSHED;
        tables[curr_hkid & HKID_MASK].tdr_table.management_fields.lifecycle_state = TD_BLOCKED;

    #endif // MODULAR_PROOF
    
    #ifdef SOURCE
    #else 
    return_val = TDX_SUCCESS;
    #endif // SOURCE
EXIT:
    #ifdef SOURCE
    // Release all acquired locks and free keyhole mappings
    if (kot_locked_flag)
    {
        release_sharex_lock_ex(&global_data_ptr->kot.lock);
    }
    if (tdr_locked_flag)
    {
        pamt_unwalk(tdr_pa, tdr_pamt_block, tdr_pamt_entry_ptr, TDX_LOCK_EXCLUSIVE, PT_4KB);
        free_la(tdr_ptr);
    }
    if (tdcs_ptr != NULL)
    {
        free_la(tdcs_ptr);
    }
    #endif // SOURCE

    return return_val;
}
