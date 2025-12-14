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
 * @file tdh_mem_track
 * @brief TDHMEMTRACK API handler
 */
#include "include/tdx_vmm_api_handlers.h"
#include "include/tdx_basic_defs.h"
#include "include/auto_gen/tdx_error_codes_defs.h"
#include "src/common/x86_defs/x86_defs.h"
#include "src/common/data_structures/td_control_structures.h"
#include "src/common/memory_handlers/keyhole_manager.h"
#include "src/common/memory_handlers/pamt_manager.h"
#include "src/common/memory_handlers/sept_manager.h"
#include "src/common/helpers/helpers.h"
#include "src/common/accessors/ia32_accessors.h"
#include "src/common/accessors/data_accessors.h"

#include "driver/driver.h"

api_error_type tdh_mem_track(uint64_t target_tdr_pa)
{
    // TDR related variables
    pa_t                  tdr_pa;                    // TDR physical address
    tdr_t               * tdr_ptr;                   // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;            // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;        // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;   // Indicate TDR is locked

    tdcs_t              * tdcs_ptr = NULL;           // Pointer to the TDCS structure (Multi-page)

    bool_t                epoch_locked_flag = false;

    api_error_type        return_val = UNINITIALIZE_ERROR;


    tdr_pa.raw = target_tdr_pa;

    #ifdef SOURCE
    // Check, lock and map the owner TDR page
    return_val = check_lock_and_map_explicit_tdr(tdr_pa,
                                                 OPERAND_ID_RCX,
                                                 TDX_RANGE_RO,
                                                 TDX_LOCK_SHARED,
                                                 PT_TDR,
                                                 &tdr_pamt_block,
                                                 &tdr_pamt_entry_ptr,
                                                 &tdr_locked_flag,
                                                 &tdr_ptr);
    if (return_val != TDX_SUCCESS)
    {
        TDX_ERROR("Failed to check/lock/map a TDR - error = %llx\n", return_val);
        goto EXIT;
    }
    #else 
    tdr_pamt_entry_ptr = &tables[tdr_pa.raw & HKID_MASK].pamt_entry;
    tdr_ptr = &tables[tdr_pa.raw & HKID_MASK].tdr;
    #endif // SOURCE

    // Map the TDCS structure and check the state
    #ifdef SOURCE
    return_val = check_state_map_tdcs_and_lock(tdr_ptr, TDX_RANGE_RW, TDX_LOCK_SHARED,
                                               false, TDH_MEM_TRACK_LEAF, &tdcs_ptr);

    if (return_val != TDX_SUCCESS)
    {
        TDX_ERROR("State check or TDCS lock failure - error = %llx\n", return_val);
        goto EXIT;
    }
    #else 
    tdcs_ptr = &tables[tdr_pa.raw & HKID_MASK].tdcs_table;
    #endif 

    #ifdef SOURCE
    // Lock the TD epoch
    if (acquire_sharex_lock_ex(&tdcs_ptr->epoch_tracking.epoch_lock)
                                != LOCK_RET_SUCCESS)
    {
        TDX_ERROR("Could not lock the TD epoch\n");
        return_val = api_error_with_operand_id(TDX_OPERAND_BUSY, OPERAND_ID_TD_EPOCH);
        goto EXIT;
    }
    #endif // SOURCE
    epoch_locked_flag = true;

    #ifdef SOURCE
    // Verify that no VCPUs are associated with the previous epoch
    uint64_t td_epoch = tdcs_ptr->epoch_tracking.epoch_and_refcount.td_epoch;
    uint16_t* refcount = tdcs_ptr->epoch_tracking.epoch_and_refcount.refcount;
    #else 
    uint64_t td_epoch = tables[tdr_pa.raw & HKID_MASK].tdcs_table.epoch_tracking.epoch_and_refcount.td_epoch;
    uint16_t* refcount = tables[tdr_pa.raw & HKID_MASK].tdcs_table.epoch_tracking.epoch_and_refcount.refcount;
    #endif // SOURCE

    #ifdef SOURCE
    if (refcount[1 - (td_epoch  & 1)] != 0)
    {
        TDX_ERROR("VCPU associated with the previous epoch\n");
        return_val = TDX_PREVIOUS_TLB_EPOCH_BUSY;
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(refcount[1 - (td_epoch  & 1)] == 0);
    #endif // MODULAR_PROOF

    // #ifdef FLOW_PROOF
    //     if (refcount[1 - (td_epoch  & 1)] != 0)
    //     {
    //         __CPROVER_assert(refcount[1 - (td_epoch  & 1)] == 0, "Previous epoch refcount check failed");
    //         TDX_ERROR("VCPU associated with the previous epoch\n");
    //         return_val = TDX_PREVIOUS_TLB_EPOCH_BUSY;
    //         goto EXIT;
    //     }
    // #endif // FLOW_PROOF

    // ALL_CHECKS_PASSED:  The function is guaranteed to succeed

    // Switch to the next TD epoch.  Note that since we only have 2 REFCOUNTs,
    // the previous epoch's REFCOUNT, verified above to be 0, is now the
    // current epoch's REFCOUNT.
    // TD_EPOCH's bit 63 must be 0 since this is a special range used for migration.
    // This can't happen in practice (it would take thousands of years)
    #ifdef SOURCE
    if (tdcs_ptr->epoch_tracking.epoch_and_refcount.td_epoch >= BITS(62, 0))
    {
        FATAL_ERROR();
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(tdcs_ptr->epoch_tracking.epoch_and_refcount.td_epoch < BITS(62, 0));
    #endif // MODULAR_PROOF
    return_val = TDX_SUCCESS;
EXIT:

#ifdef SOURCE
    // Release all acquired locks
    if (epoch_locked_flag)
    {
        release_sharex_lock_ex(&tdcs_ptr->epoch_tracking.epoch_lock);
    }

    if(tdcs_ptr != NULL)
    {
        release_sharex_lock_hp_sh(&tdcs_ptr->management_fields.op_state_lock);
        free_la(tdcs_ptr);
    }

    if (tdr_locked_flag)
    {
        pamt_unwalk(tdr_pa, tdr_pamt_block, tdr_pamt_entry_ptr, TDX_LOCK_SHARED, PT_4KB);
        free_la(tdr_ptr);
    }
    #endif // SOURCE
    return return_val;
}
