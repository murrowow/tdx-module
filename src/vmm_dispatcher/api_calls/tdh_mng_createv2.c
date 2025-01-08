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
 * @file tdh_mng_create
 * @brief TDHMNGCREATE API handler
 */
#include "include/tdx_vmm_api_handlers.h"
#include "include/tdx_basic_defs.h"
#include "include/auto_gen/tdx_error_codes_defs.h"
#include "src/common/x86_defs/x86_defs.h"
#include "src/common/data_structures/td_control_structures.h"
#include "src/common/memory_handlers/keyhole_manager.h"
#include "src/common/memory_handlers/pamt_manager.h"
#include "src/common/helpers/helpers.h"
#include "src/common/accessors/data_accessors.h"
#include "src/common/accessors/ia32_accessors.h"

// SOPHIA: prove helper files
// SOPHIA: THis is the first step in the TD setup process

#include "helpers/cbmc_helper.h"

api_error_type tdh_mng_create(uint64_t target_tdr_pa, hkid_api_input_t hkid_info)
{
    tdx_module_global_t * global_data = get_global_data();

    // TDR related variables
    pa_t                  tdr_pa;                   // TDR physical address
    tdr_t               * tdr_ptr;                  // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;           // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;       // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;  // Indicate TDR is locked

    uint16_t              td_hkid;
    bool_t                kot_locked_flag = false;  // Indicate KOT is locked

    api_error_type        return_val = UNINITIALIZE_ERROR;

    tdr_pa.raw = target_tdr_pa;
    td_hkid = hkid_info.hkid;

    //__CPROVER_assert(false, "this should fail and it does"); 

    // SOPHIA: not sure if this is correct or not then
    //uint64_t seamrr_base = ia32_rdmsr(IA32_SEAMRR_BASE_MSR_ADDR) & IA32_SEAMRR_BASE_AND_MASK_MASK; 
    //uint64_t seamrr_size = ia32_rdmsr(IA32_SEAMRR_MASK_MSR_ADDR) & IA32_SEAMRR_BASE_AND_MASK_MASK;
    //seamrr_size = mask_to_size(seamrr_size);
    //__CPROVER_assert(false, "this should fail and it doesn't");
    //__CPROVER_assert((td_hkid >= ia32_rdmsr(IA32_SEAMRR_BASE_MSR_ADDR) & IA32_SEAMRR_BASE_AND_MASK_MASK) && (td_hkid < ia32_rdmsr(IA32_SEAMRR_BASE_MSR_ADDR) & IA32_SEAMRR_BASE_AND_MASK_MASK + mask_to_size(ia32_rdmsr(IA32_SEAMRR_MASK_MSR_ADDR) & IA32_SEAMRR_BASE_AND_MASK_MASK)), "value of HKID must be in the range configured for TDX");
    //__CPROVER_assert(get_pamt_entry(target_tdr_pa, hkid_info)->pt == PT_NDA, "TDR page metadata in PAMT is correct (PT must be PT_NDA)");
    //__CPROVER_assert(global_data->kot.entries[td_hkid].state == KOT_STATE_HKID_FREE, "KOT of the specified HKID must be marked as HKID_FREE");

    // Verify HKID
    if ((hkid_info.reserved != 0) || !is_private_hkid(td_hkid))
    {
        TDX_ERROR("HKID %d is not a private HKID\n", td_hkid);
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
        goto EXIT;
    }


    /**
     * Check TDR (explicit access, opaque semantics, exclusive lock).
     */

    return_val = check_lock_and_map_explicit_tdr(tdr_pa,
                                                 OPERAND_ID_RCX,
                                                 TDX_RANGE_RW,
                                                 TDX_LOCK_EXCLUSIVE,
                                                 PT_NDA,
                                                 &tdr_pamt_block,
                                                 &tdr_pamt_entry_ptr,
                                                 &tdr_locked_flag,
                                                 &tdr_ptr);

    if (return_val != TDX_SUCCESS)
    {
        TDX_ERROR("Failed to check/lock/map a TDR - error = %llx\n", return_val);
        goto EXIT;
    }

    // Acquire exclusive access to KOT
    if(acquire_sharex_lock_ex(&global_data->kot.lock) != LOCK_RET_SUCCESS)
    {
        TDX_ERROR("Failed to acquire lock on KOT\n");
        return_val = api_error_with_operand_id(TDX_OPERAND_BUSY, OPERAND_ID_KOT);
        goto EXIT;
    }
    kot_locked_flag = true;

    // Protection against speculation attacks with out-of-bound td_hkid user input value
    lfence();

    // Check the provided HKID entry in KOT
    if (global_data->kot.entries[td_hkid].state != KOT_STATE_HKID_FREE)
    {
        TDX_ERROR("Given HKID %d is not free in KOT\n", td_hkid);
        return_val = TDX_HKID_NOT_FREE;
        goto EXIT;
    }

    // Clear the content of the TDR page using direct writes
    zero_area_cacheline(tdr_ptr, TDX_PAGE_SIZE_IN_BYTES);

    /**
     * Initialize the TD Management and Key Management Fields.
     * Fields which are initialized to zero are implicitly zero'd in the
     * previous state.
     */
    // Generate a random 256-bit TD_UUID
    if (!generate_256bit_random(&tdr_ptr->management_fields.td_uuid))
    {
        TDX_ERROR("Failed to generate random 256-bit UUID number\n");
        return_val = TDX_RND_NO_ENTROPY;
        goto EXIT;
    }

    // ALL_CHECKS_PASSED:  The function is guaranteed to succeed

    // Mark the HKID entry in the KOT as assigned
    global_data->kot.entries[td_hkid].state = (uint8_t)KOT_STATE_HKID_ASSIGNED;

    // Set HKID in the TKT entry
    tdr_ptr->key_management_fields.hkid = td_hkid;
    tdr_ptr->management_fields.lifecycle_state = TD_HKID_ASSIGNED;

    tdr_ptr->td_preserving_fields.seamdb_index = global_data->seamdb_index;

    for (uint32_t i = 0; i < 4; i++)
    {
        tdr_ptr->td_preserving_fields.seamdb_nonce.qwords[i] = global_data->seamdb_nonce.qwords[i];
    }
    tdr_ptr->td_preserving_fields.handoff_version = global_data->module_hv;

    // Set the new TDR page PAMT fields
    tdr_pamt_entry_ptr->pt = PT_TDR;
    tdr_pamt_entry_ptr->owner = 0;

EXIT:
    // Release all acquired locks and free keyhole mappings
    if (kot_locked_flag)
    {
        release_sharex_lock_ex(&global_data->kot.lock);
    }

    if (tdr_locked_flag)
    {
        pamt_unwalk(tdr_pa, tdr_pamt_block, tdr_pamt_entry_ptr, TDX_LOCK_EXCLUSIVE, PT_4KB);
        free_la(tdr_ptr);
    }
    return return_val;
    
    //__CPROVER_postcondition(tdr_ptr->zero out the TDR page contents using direct write);
    //__CPROVER_postcondition(Initialize the key management fields);
    //__CPROVER_postcondition(Initialize the state variables);
    //__CPROVER_postcondition(Initialize the TD management fields);
    //__CPROVER_postcondition(initialize the TD preserving fields handoff version and current SEAMDB entry index/nonce pair );
    //__CPROVER_postcondition(MARK KOT entry for the specified HKID as HKID_ASSIGNED);
    //__CPROVER_postcondition(Intialize the TDR page metadata in PAMT); 
}
