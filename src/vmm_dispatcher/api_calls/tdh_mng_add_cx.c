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
 * @file tdh_mng_add_cx.c
 * @brief TDHMNGADDCX API handler
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
#include "src/common/accessors/data_accessors.h"
#include "src/common/accessors/ia32_accessors.h"

#include "driver/driver.h"


api_error_type tdh_mng_add_cx(uint64_t target_tdcx_pa, uint64_t target_tdr_pa)
{
    // TDCX related variables
    pa_t                  tdcx_pa;                   // TDCX physical address
    void                * tdcx_ptr;                  // Pointer to the TDCX page (linear address)
    pamt_block_t          tdcx_pamt_block;           // TDCX PAMT block
    pamt_entry_t        * tdcx_pamt_entry_ptr;       // Pointer to the TDCX PAMT entry
    bool_t                tdcx_locked_flag = false;  // Indicate TDCX is locked

    // TDR related variables
    pa_t                  tdr_pa;                    // TDR physical address
    tdr_t               * tdr_ptr;                   // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;            // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;        // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;   // Indicate TDR is locked
    tdcs_t              * tdcs_p = NULL;

    uint32_t              tdcx_index_num;

    api_error_type        return_val = UNINITIALIZE_ERROR;

    tdcx_pa.raw = target_tdcx_pa;
    tdr_pa.raw = target_tdr_pa;

    // Check, lock and map the owner TDR page
    // return_val = check_lock_and_map_explicit_tdr(tdr_pa,
    //                                              OPERAND_ID_RDX,
    //                                              TDX_RANGE_RW,
    //                                              TDX_LOCK_EXCLUSIVE,
    //                                              PT_TDR,
    //                                              &tdr_pamt_block,
    //                                              &tdr_pamt_entry_ptr,
    //                                              &tdr_locked_flag,
    //                                              &tdr_ptr);
    // if (return_val != TDX_SUCCESS)
    // {
    //     TDX_ERROR("Failed to check/lock/map a TDR - error = %llx\n", return_val);
    //     goto EXIT;
    // }
    tdr_pamt_entry_ptr = &(tables[tdr_pa.raw & HKID_MASK].pamt_entry);
    __CPROVER_assume(tdr_pamt_entry_ptr->pt == PT_TDR);

    // Check the TD state
    // if (tdr_ptr->management_fields.fatal)
    // {
    //     TDX_ERROR("TDR state is fatal\n");
    //     return_val = TDX_TD_FATAL;
    //     goto EXIT;
    // }
    __CPROVER_assume(!tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.fatal);

    // if (tdr_ptr->management_fields.lifecycle_state != TD_KEYS_CONFIGURED)
    // {
    //     TDX_ERROR("TDR key state not configured\n");
    //     return_val = TDX_TD_KEYS_NOT_CONFIGURED;
    //     goto EXIT;
    // }
    __CPROVER_assume(tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED);

    // Get the current number of TDCS pages and verify
    tdcx_index_num = tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.num_tdcx;
    // if (tdcx_index_num > (MAX_NUM_TDCS_PAGES-1))
    // {
    //     TDX_ERROR("Number of TDCS pages (%lu) exceeds the allowed count (%d)\n", tdcx_index_num, MAX_NUM_TDCS_PAGES-1);
    //     return_val = TDX_TDCX_NUM_INCORRECT;
    //     goto EXIT;
    // }
    __CPROVER_assume(tdcx_index_num < MAX_NUM_TDCS_PAGES);

    // Check, lock and map the new TDCX page
    // return_val = check_lock_and_map_explicit_private_4k_hpa(tdcx_pa,
    //                                                         OPERAND_ID_RCX,
    //                                                         tdr_ptr,
    //                                                         TDX_RANGE_RW,
    //                                                         TDX_LOCK_EXCLUSIVE,
    //                                                         PT_NDA,
    //                                                         &tdcx_pamt_block,
    //                                                         &tdcx_pamt_entry_ptr,
    //                                                         &tdcx_locked_flag,
    //                                                         (void**)&tdcx_ptr);
                                                            
    // if (return_val != TDX_SUCCESS)
    // {
    //     TDX_ERROR("Failed to check/lock/map a TDCS - error = %llx\n", return_val);
    //     goto EXIT;
    // }

    // pa_t hpa_with_hkid = assign_hkid_to_hpa(tdr_ptr, tdcx_pa);

    // tdcx_ptr = map_pa((void*)hpa_with_hkid.full_pa, TDX_RANGE_RW);

    tdcx_pamt_entry_ptr = &(tables[tdr_pa.raw & HKID_MASK].tdcx_pamt_entry);
    __CPROVER_assume(tdcx_pamt_entry_ptr->pt == PT_NDA);

    // ALL_CHECKS_PASSED:  The function is guaranteed to succeed

    /**
     *  Fill the content of the TDCX page using direct writes.
     *  To save later work during TDHMNGINIT, the MSR bitmaps page is filled with
     *  all 1's, which is the default case for most MSRs.
     *  Other pages are filled with 0's.
     */

    if (tdcx_index_num == MSR_BITMAPS_PAGE_INDEX)
    {
        // fill_area_cacheline(tdcx_ptr, TDX_PAGE_SIZE_IN_BYTES, (~(uint64_t)0));
        tables[tdr_pa.raw & HKID_MASK].tdcx_mem = ~(uint64_t)0; 
    }
    else if (tdcx_index_num == SEPT_ROOT_PAGE_INDEX)
    {
        // fill_area_cacheline(tdcx_ptr, TDX_PAGE_SIZE_IN_BYTES, SEPTE_INIT_VALUE);
        tables[tdr_pa.raw & HKID_MASK].tdcx_mem = SEPTE_INIT_VALUE; 
    }
    else
    {
        // fill_area_cacheline(tdcx_ptr, TDX_PAGE_SIZE_IN_BYTES, SEPTE_L2_INIT_VALUE);
        tables[tdr_pa.raw & HKID_MASK].tdcx_mem = SEPTE_L2_INIT_VALUE; 
    }

    /* OP_STATE is assumed to reside in the first TDCS page, and its value is 0 so there's no need
           to initialize it separately. */
    // tdx_sanity_check(offsetof(tdcs_t, management_fields) + offsetof(tdcs_management_fields_t, op_state) <=
    //                  _4KB - sizeof(tdcs_p->management_fields.op_state),
    //                  SCEC_SEAMCALL_SOURCE(TDH_MNG_ADDCX_LEAF), 0);  // Ensure it fits in the first page
    // tdx_sanity_check(0 == OP_STATE_UNINITIALIZED, SCEC_SEAMCALL_SOURCE(TDH_MNG_ADDCX_LEAF), 0);

    // if ((tdcx_index_num + 1) >= MIN_NUM_TDCS_PAGES)
    // {
        // With the new page, we have enough TDCS pages to do some initializations and checks.

        // Map the TDCS structure and check the state.
        // AHMAD: Stubbed this function since it maps the tdcs pages to keyholes
        // tdcs_p = map_implicit_tdcs(tdr_ptr, TDX_RANGE_RW, false);
        // tables[tdr_pa.raw & HKID_MASK].tdcx_table = map_implicit_tdcs(tdr_ptr, TDX_RANGE_RW, false);

    //     if ((tdcx_index_num + 1) == MIN_NUM_TDCS_PAGES)
    //     {
    //         Generate a 256-bit encryption key for the next migration session
    //         if (!generate_256bit_random(&tdcs_p->migration_fields.mig_enc_key))
    //         {
    //             TDX_ERROR("migration encryption key generation failed\n");
    //             return_val = TDX_RND_NO_ENTROPY;
    //             goto EXIT;
    //         }
    //     }
    //     else
    //     {
    //         We have more than the minimum number of TDCS pages.
    //         OP_STATE is now available; check it.
    //         if (!op_state_is_seamcall_allowed(TDH_MNG_ADDCX_LEAF, tdcs_p->management_fields.op_state, false))
    //         {
    //             TDX_ERROR("Current OP state is incorrect %d\n", tdcs_p->management_fields.op_state);
    //             return_val = TDX_OP_STATE_INCORRECT;
    //             goto EXIT;
    //         }

    //         __CPROVER_assume(seamcall_state_lookup[TDH_MNG_ADDCX_LEAF][tables[tdr_pa.raw & HKID_MASK].tdcx_table.management_fields.op_state]);
    //     }
    // }

    // Register the new TDCS page in its parent TDR
    // tdr_ptr->management_fields.tdcx_pa[tdcx_index_num] = assign_hkid_to_hpa(tdr_ptr, tdcx_pa).raw;
    uint16_t hkid;
    if (&tables[tdr_pa.raw & HKID_MASK].tdr_table == NULL) {
        hkid = global_data.hkid;
    } else {
        hkid = tables[tdr_pa.raw & HKID_MASK].tdr_table.key_management_fields.hkid;
    }
    tdcx_pa.full_pa &= ~(HKID_MASK);
    tdcx_pa.full_pa |= ((uint64_t)hkid << global_data.hkid_start_bit);
    tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.tdcx_pa[tdcx_index_num] = tdcx_pa.full_pa;

    // tdr_ptr->management_fields.num_tdcx = (tdcx_index_num + 1);
    tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.num_tdcx = (tdcx_index_num + 1);

    // Complete new TDCX page registration in its parent TDR
    // tdr_ptr->management_fields.chldcnt++;
    tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.chldcnt++;

    // Set the new TDCS page PAMT fields
    tdcx_pamt_entry_ptr->pt = PT_TDCX;
    set_pamt_entry_owner(tdcx_pamt_entry_ptr, tdr_pa); 

EXIT:
    // if (tdcs_p)
    // {
    //     free_la(tdcs_p);
    // }
    // // Release all acquired locks and free keyhole mappings
    // if (tdr_locked_flag)
    // {
    //     pamt_unwalk(tdr_pa, tdr_pamt_block, tdr_pamt_entry_ptr, TDX_LOCK_EXCLUSIVE, PT_4KB);
    //     free_la(tdr_ptr);
    // }
    // if (tdcx_locked_flag)
    // {
    //     pamt_unwalk(tdcx_pa, tdcx_pamt_block, tdcx_pamt_entry_ptr, TDX_LOCK_EXCLUSIVE, PT_4KB);
    //     free_la(tdcx_ptr);
    // }

    __CPROVER_assert(tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.num_tdcx == (tdcx_index_num + 1), "Increment TDR.NUM_TDCX");
    __CPROVER_assert((tdcx_index_num == MSR_BITMAPS_PAGE_INDEX && tables[tdr_pa.raw & HKID_MASK].tdcx_mem == ~(uint64_t)0) || 
    (tdcx_index_num == SEPT_ROOT_PAGE_INDEX && tables[tdr_pa.raw & HKID_MASK].tdcx_mem == SEPTE_INIT_VALUE) || 
    (tables[tdr_pa.raw & HKID_MASK].tdcx_mem == SEPTE_L2_INIT_VALUE), "Initialize the TDCX page contents using direct writes");
    __CPROVER_assert(tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.tdcx_pa[tdcx_index_num], "Set the TDCX pointer entry in the TDR.TDCX_PA array");


    return return_val;
}
