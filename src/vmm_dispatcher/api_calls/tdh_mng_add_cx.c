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

    uint16_t td_hkid = tdr_pa.raw >> 48;

    // Check, lock and map the owner TDR page
    #ifdef SOURCE
        return_val = check_lock_and_map_explicit_tdr(tdr_pa,
                                                    OPERAND_ID_RDX,
                                                    TDX_RANGE_RW,
                                                    TDX_LOCK_EXCLUSIVE,
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
        tdr_pamt_entry_ptr = &(tables[td_hkid & HKID_MASK].pamt_entry);
    #endif //SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(tdr_pamt_entry_ptr->pt == PT_TDR);
    #endif //MODULAR_PROOF
    #ifdef FLOW_PROOF
        __CPROVER_assert(tdr_pamt_entry_ptr->pt == PT_TDR, "PAMT is labeled correctly");
    #endif //FLOW_PROOF

    // Check the TD state
    #ifdef SOURCE
        if (tdr_ptr->management_fields.fatal)
        {
            TDX_ERROR("TDR state is fatal\n");
            return_val = TDX_TD_FATAL;
            goto EXIT;
        }
    #endif //SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(!tables[td_hkid & HKID_MASK].tdr_table.management_fields.fatal);
    #endif //MODULAR_PROOF
    #ifdef FLOW_PROOF
        __CPROVER_assert(!tables[td_hkid & HKID_MASK].tdr_table.management_fields.fatal, "Make sure the TD is not in a fatal state");
    #endif //FLOW_PROOF

    #ifdef SOURCE
        if (tdr_ptr->management_fields.lifecycle_state != TD_KEYS_CONFIGURED)
        {
            TDX_ERROR("TDR key state not configured\n");
            return_val = TDX_TD_KEYS_NOT_CONFIGURED;
            goto EXIT;
        }
    #endif //SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(tables[td_hkid & HKID_MASK].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED);
    #endif //MODULAR_PROOF
    #ifdef FLOW_PROOF
        __CPROVER_assert(tables[td_hkid & HKID_MASK].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED, "Lifecycle is in the correct state");
    #endif //FLOW_PROOF

    // Get the current number of TDCS pages and verify
    #ifdef SOURCE
        tdcx_index_num = tdr_ptr->management_fields.num_tdcx;
    #else
        tdcx_index_num = tables[td_hkid & HKID_MASK].tdr_table.management_fields.num_tdcx;
    #endif //SOURCE

    #ifdef SOURCE
        if (tdcx_index_num > (MAX_NUM_TDCS_PAGES-1))
        {
            TDX_ERROR("Number of TDCS pages (%lu) exceeds the allowed count (%d)\n", tdcx_index_num, MAX_NUM_TDCS_PAGES-1);
            return_val = TDX_TDCX_NUM_INCORRECT;
            goto EXIT;
        }
    #endif //SOURCE
    
    #ifdef MODULAR_PROOF
        __CPROVER_assume(tdcx_index_num < MAX_NUM_TDCS_PAGES);
    #endif //MODULAR_PROOF
    #ifdef FLOW_PROOF
        __CPROVER_assert(tdcx_index_num < MAX_NUM_TDCS_PAGES, "ensure valid number of TDCS pages added");
    #endif //FLOW_PROOF

    // Check, lock and map the new TDCX page
    #ifdef SOURCE
        return_val = check_lock_and_map_explicit_private_4k_hpa(tdcx_pa,
                                                                OPERAND_ID_RCX,
                                                                tdr_ptr,
                                                                TDX_RANGE_RW,
                                                                TDX_LOCK_EXCLUSIVE,
                                                                PT_NDA,
                                                                &tdcx_pamt_block,
                                                                &tdcx_pamt_entry_ptr,
                                                                &tdcx_locked_flag,
                                                                (void**)&tdcx_ptr);
                                                                
        if (return_val != TDX_SUCCESS)
        {
            TDX_ERROR("Failed to check/lock/map a TDCS - error = %llx\n", return_val);
            goto EXIT;
        }
    #else
        tdcx_pamt_entry_ptr = &(tables[td_hkid & HKID_MASK].tdcx_pamt_entry);
    #endif //SOURCE

    // AHMAD: Abstract away mappings for now
    #ifdef SOURCE
        pa_t hpa_with_hkid = assign_hkid_to_hpa(tdr_ptr, tdcx_pa);

        tdcx_ptr = map_pa((void*)hpa_with_hkid.full_pa, TDX_RANGE_RW);
    #endif //SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(tdcx_pamt_entry_ptr->pt == PT_NDA);
    #endif //MODULAR_PROOF
    #ifdef FLOW_PROOF
        __CPROVER_assert(tdcx_pamt_entry_ptr->pt == PT_NDA, "Make sure TDCX entry is correct");
    #endif //FLOW_PROOF

    // ALL_CHECKS_PASSED:  The function is guaranteed to succeed

    /**
     *  Fill the content of the TDCX page using direct writes.
     *  To save later work during TDHMNGINIT, the MSR bitmaps page is filled with
     *  all 1's, which is the default case for most MSRs.
     *  Other pages are filled with 0's.
     */

    #ifdef SOURCE
        if (tdcx_index_num == MSR_BITMAPS_PAGE_INDEX)
        {
            fill_area_cacheline(tdcx_ptr, TDX_PAGE_SIZE_IN_BYTES, (~(uint64_t)0));
        }
        else if (tdcx_index_num == SEPT_ROOT_PAGE_INDEX)
        {
            fill_area_cacheline(tdcx_ptr, TDX_PAGE_SIZE_IN_BYTES, SEPTE_INIT_VALUE);
        }
        else
        {
            fill_area_cacheline(tdcx_ptr, TDX_PAGE_SIZE_IN_BYTES, SEPTE_L2_INIT_VALUE);
        }
    #endif //SOURCE

    #ifdef MODULAR_PROOF
        if (tdcx_index_num == MSR_BITMAPS_PAGE_INDEX)
        {
            tables[td_hkid & HKID_MASK].tdcx_mem = ~(uint64_t)0; 
        }
        else if (tdcx_index_num == SEPT_ROOT_PAGE_INDEX)
        {
            tables[td_hkid & HKID_MASK].tdcx_mem = SEPTE_INIT_VALUE; 
        }
        else
        {
            tables[td_hkid & HKID_MASK].tdcx_mem = SEPTE_L2_INIT_VALUE; 
        }
    #endif //MODULAR_PROOF

    #ifdef FLOW_PROOF
        __CPROVER_havoc_slice(&tables[td_hkid & HKID_MASK].tdcx_mem, sizeof(uint8_t));
        if (tdcx_index_num == MSR_BITMAPS_PAGE_INDEX)
        {
            __CPROVER_assume(tables[td_hkid & HKID_MASK].tdcx_mem == ~(uint64_t)0); 
        }
        else if (tdcx_index_num == SEPT_ROOT_PAGE_INDEX)
        {
           __CPROVER_assume(tables[td_hkid & HKID_MASK].tdcx_mem == SEPTE_INIT_VALUE); 
        }
        else
        {
            __CPROVER_assume(tables[td_hkid & HKID_MASK].tdcx_mem == SEPTE_L2_INIT_VALUE); 
        }
    #endif //FLOW_PROOF

    #ifdef SOURCE
        if ((tdcx_index_num + 1) >= MIN_NUM_TDCS_PAGES)
        {
            // With the new page, we have enough TDCS pages to do some initializations and checks.

            // Map the TDCS structure and check the state.
            tdcs_p = map_implicit_tdcs(tdr_ptr, TDX_RANGE_RW, false);

            if ((tdcx_index_num + 1) == MIN_NUM_TDCS_PAGES)
            {
                // Generate a 256-bit encryption key for the next migration session
                if (!generate_256bit_random(&tdcs_p->migration_fields.mig_enc_key))
                {
                    TDX_ERROR("migration encryption key generation failed\n");
                    return_val = TDX_RND_NO_ENTROPY;
                    goto EXIT;
                }
            }
            else
            {
                // We have more than the minimum number of TDCS pages.
                // OP_STATE is now available; check it.
                if (!op_state_is_seamcall_allowed(TDH_MNG_ADDCX_LEAF, tdcs_p->management_fields.op_state, false))
                {
                    TDX_ERROR("Current OP state is incorrect %d\n", tdcs_p->management_fields.op_state);
                    return_val = TDX_OP_STATE_INCORRECT;
                    goto EXIT;
                }
            }
        }
    #else 
        if ((tdcx_index_num + 1) >= MIN_NUM_TDCS_PAGES)
        {
            // With the new page, we have enough TDCS pages to do some initializations and checks.

            // Map the TDCS structure and check the state.
            // AHMAD: Abstract away mapping the tdcs pages to keyholes
            if ((tdcx_index_num + 1) != MIN_NUM_TDCS_PAGES)
            {
                #ifdef MODULAR_PROOF
                    __CPROVER_assume(seamcall_state_lookup[TDH_MNG_ADDCX_LEAF][tables[(td_hkid & HKID_MASK)].tdcx_table.management_fields.op_state]);
                #endif //MODULAR_PROOF
                #ifdef FLOW_PROOF
                    __CPROVER_assert(seamcall_state_lookup[TDH_MNG_ADDCX_LEAF][tables[(td_hkid & HKID_MASK)].tdcx_table.management_fields.op_state], "Correct op state");
                #endif //FLOW_PROOF
            }
        }
    #endif //SOURCE

    // Register the new TDCS page in its parent TDR
    #ifdef SOURCE
        tdr_ptr->management_fields.tdcx_pa[tdcx_index_num] = assign_hkid_to_hpa(tdr_ptr, tdcx_pa).raw;
        tdr_ptr->management_fields.num_tdcx = (tdcx_index_num + 1);

        // Complete new TDCX page registration in its parent TDR
        tdr_ptr->management_fields.chldcnt++;

        // Set the new TDCS page PAMT fields
        tdcx_pamt_entry_ptr->pt = PT_TDCX;
        set_pamt_entry_owner(tdcx_pamt_entry_ptr, tdr_pa);  
    #else
        uint16_t hkid;
        if (&tables[td_hkid & HKID_MASK].tdr_table == NULL) {
            hkid = global_data.hkid;
        } else {
            hkid = tables[td_hkid & HKID_MASK].tdr_table.key_management_fields.hkid;
        }
        tdcx_pa.full_pa &= ~(HKID_MASK);
        tdcx_pa.full_pa |= ((uint64_t)hkid << global_data.hkid_start_bit);
    #endif //SOURCE

    #ifdef MODULAR_PROOF
        tables[td_hkid & HKID_MASK].tdr_table.management_fields.tdcx_pa[tdcx_index_num].val = tdcx_pa.raw & ((HKID_SIZE << 1) - 1);
        tables[td_hkid & HKID_MASK].tdr_table.management_fields.num_tdcx = (tdcx_index_num + 1);

        // Complete new TDCX page registration in its parent TDR
        tables[td_hkid & HKID_MASK].tdr_table.management_fields.chldcnt++;

        // Set the new TDCS page PAMT fields
        tdcx_pamt_entry_ptr->pt = PT_TDCX;
        set_pamt_entry_owner(tdcx_pamt_entry_ptr, tdr_pa); 
    #endif //MODULAR_PROOF


    #ifdef FLOW_PROOF
        uint64_t currChildCount = tables[td_hkid & HKID_MASK].tdr_table.management_fields.chldcnt;
        __CPROVER_havoc_slice(&tables[td_hkid & HKID_MASK].tdr_table.management_fields, sizeof(tables[td_hkid & HKID_MASK].tdr_table.management_fields));
        struct {
            unsigned val : 2;
        } tdcx_pa_two_bit;
        tdcx_pa_two_bit.val = tdcx_pa.raw & ((HKID_SIZE << 1) - 1);
        __CPROVER_assume(tables[td_hkid & HKID_MASK].tdr_table.management_fields.fatal == false);
        // SOPHIA: need to preserve state after the previous havoc
        __CPROVER_assume(tables[td_hkid & HKID_MASK].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED);
        __CPROVER_assume(tables[td_hkid & HKID_MASK].tdr_table.management_fields.tdcx_pa[tdcx_index_num].val == tdcx_pa_two_bit.val);

        __CPROVER_assume(tables[td_hkid & HKID_MASK].tdr_table.management_fields.num_tdcx == tdcx_index_num + 1);
        __CPROVER_assume(tables[td_hkid & HKID_MASK].tdr_table.management_fields.chldcnt == currChildCount + 1); 

        __CPROVER_havoc_slice(tdcx_pamt_entry_ptr, sizeof(pamt_entry_t)); 
        __CPROVER_assume(tdcx_pamt_entry_ptr->pt == PT_TDCX);
        __CPROVER_assume(tdcx_pamt_entry_ptr->owner == tdr_pa.page_4k_num);
    #endif //FLOW_PROOF

EXIT:
    #ifdef SOURCE
        if (tdcs_p)
        {
            free_la(tdcs_p);
        }
        // Release all acquired locks and free keyhole mappings
        if (tdr_locked_flag)
        {
            pamt_unwalk(tdr_pa, tdr_pamt_block, tdr_pamt_entry_ptr, TDX_LOCK_EXCLUSIVE, PT_4KB);
            free_la(tdr_ptr);
        }
        if (tdcx_locked_flag)
        {
            pamt_unwalk(tdcx_pa, tdcx_pamt_block, tdcx_pamt_entry_ptr, TDX_LOCK_EXCLUSIVE, PT_4KB);
            free_la(tdcx_ptr);
        }
    #endif //SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assert(tables[td_hkid & HKID_MASK].tdr_table.management_fields.num_tdcx == (tdcx_index_num + 1) & HKID_MASK, "Increment TDR.NUM_TDCX");
        __CPROVER_assert((tdcx_index_num == MSR_BITMAPS_PAGE_INDEX && tables[td_hkid & HKID_MASK].tdcx_mem == (uint8_t)(~0)) || 
        (tdcx_index_num == SEPT_ROOT_PAGE_INDEX && tables[td_hkid & HKID_MASK].tdcx_mem == (uint8_t)SEPTE_INIT_VALUE) || 
        (tables[td_hkid & HKID_MASK].tdcx_mem == (uint8_t)SEPTE_L2_INIT_VALUE), "Initialize the TDCX page contents using direct writes");
        __CPROVER_assert(tables[td_hkid & HKID_MASK].tdr_table.management_fields.tdcx_pa[tdcx_index_num].val == (tdcx_pa.raw & ((HKID_SIZE << 1) - 1)), "Set the TDCX pointer entry in the TDR.TDCX_PA array");
    #endif //MODULAR_PROOF

    __CPROVER_printf("SOPHIA: index: %d, tables.fatal: %d, lifecycle: %d", td_hkid & HKID_MASK, tables[td_hkid & HKID_MASK].tdr_table.management_fields.fatal, tables[td_hkid & HKID_MASK].tdr_table.management_fields.lifecycle_state);
    return_val = TDX_SUCCESS;
    return return_val;
}
