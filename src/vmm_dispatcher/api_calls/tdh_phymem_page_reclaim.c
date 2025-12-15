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
 * @file tdh_phymem_page_reclaim
 * @brief TDHPHYMEMPAGERECLAIM API handler
 */
#include "include/tdx_vmm_api_handlers.h"
#include "include/tdx_basic_defs.h"
#include "include/auto_gen/tdx_error_codes_defs.h"
#include "src/common/x86_defs/x86_defs.h"
#include "src/common/data_structures/tdx_local_data.h"
#include "src/common/data_structures/td_control_structures.h"
#include "src/common/memory_handlers/keyhole_manager.h"
#include "src/common/memory_handlers/pamt_manager.h"
#include "src/common/helpers/helpers.h"
#include "src/common/accessors/data_accessors.h"

#include "driver/driver.h"

api_error_type tdh_phymem_page_reclaim(uint64_t page_pa)
{
    // TDX Local data
    #ifdef SOURCE
    tdx_module_local_t  * local_data_ptr = get_local_data();
    #else 
    tdx_module_local_t  * local_data_ptr = &local_data;
    #endif //SOURCE

    // Reclaimed page related variables
    pa_t                  reclaimed_page_pa = {.raw = page_pa}; // Reclaimed page physical address
    pamt_block_t          reclaimed_page_pamt_block;            // Reclaimed page PAMT block
    pamt_entry_t        * reclaimed_page_pamt_entry_ptr;        // Pointer to the reclaimed page PAMT entry
    bool_t                reclaimed_page_pamt_locked_flag = false; // Indicate pamt is locked for this page
    page_size_t           reclaimed_page_leaf_size;
    page_size_api_input_t reclaimed_page_level = {.raw = 0};    // Output - reclaimed page level

    // TDR related variables
    pa_t                  page_owner_pa = {.raw = 0};           // Owner of this page (points to TDR if not TDR itself)
    tdr_t               * tdr_ptr = NULL;                       // Pointer to the TDR page (linear address)
    pamt_entry_t        * tdr_pamt_entry_ptr;                   // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;              // Indicate TDR is locked

    api_error_type        return_val = UNINITIALIZE_ERROR;

    // Initialize output registers to default values
    #ifdef SOURCE
    local_data_ptr->vmm_regs.rcx = 0ULL; // Reclaimed page type (from PAMT entry)
    local_data_ptr->vmm_regs.rdx = 0ULL; // Reclaimed page owner (from PAMT entry)
    local_data_ptr->vmm_regs.r8  = 0ULL; // Reclaimed page size (from PAMT walk)
    local_data_ptr->vmm_regs.r9  = 0ULL; // Reserved
    local_data_ptr->vmm_regs.r10 = 0ULL; // Reserved
    local_data_ptr->vmm_regs.r11 = 0ULL; // Reserved
    #else 
    local_data.vmm_regs.rcx = 0ULL; // Reclaimed page type (from PAMT entry)
    local_data.vmm_regs.rdx = 0ULL; // Reclaimed page owner (from PAMT entry)
    local_data.vmm_regs.r8  = 0ULL; //  Reclaimed page size (from PAMT walk)
    local_data.vmm_regs.r9  = 0ULL; // Reserved
    local_data.vmm_regs.r10 = 0ULL; // Reserved
    local_data.vmm_regs.r11 = 0ULL; // Reserved
    #endif //SOURCE

    #ifdef SOURCE
    // Check that page address is page-aligned and that its HKID is zero
    if (!is_addr_aligned_pwr_of_2(reclaimed_page_pa.raw, TDX_PAGE_SIZE_IN_BYTES) ||
        !is_pa_smaller_than_max_pa(reclaimed_page_pa.raw) ||
        (get_hkid_from_pa(reclaimed_page_pa) != 0))
    {
        TDX_ERROR("Page is not aligned or does not have zero-ed HKID bits\n");
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
    __CPROVER_assume(is_addr_aligned_pwr_of_2(reclaimed_page_pa.raw, TDX_PAGE_SIZE_IN_BYTES));
    __CPROVER_assume(is_pa_smaller_than_max_pa(reclaimed_page_pa.raw)); 
    __CPROVER_assume(page_pa & HKID_MASK == 0); 
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
    if (!is_addr_aligned_pwr_of_2(reclaimed_page_pa.raw, TDX_PAGE_SIZE_IN_BYTES) ||
        !is_pa_smaller_than_max_pa(reclaimed_page_pa.raw) ||
        (reclaimed_page_pa.raw & HKID_MASK != 0))
    {
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
        goto EXIT;
    }
    #endif // FLOW_PROOF

    #ifdef SOURCE
    // Get reclaimed page PAMT block
    if (!pamt_get_block(reclaimed_page_pa, &reclaimed_page_pamt_block))
    {
        TDX_ERROR("Page PA does not comply with PAMT range rules\n");
        return_val = api_error_with_operand_id(TDX_OPERAND_ADDR_RANGE_ERROR, OPERAND_ID_RCX);
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef SOURCE
    // Walk and locate the leaf PAMT entry
    if ((return_val = pamt_walk(reclaimed_page_pa, reclaimed_page_pamt_block, TDX_LOCK_EXCLUSIVE,
                                &reclaimed_page_leaf_size, false, false, &reclaimed_page_pamt_entry_ptr)) != TDX_SUCCESS)
    {
        TDX_ERROR("Failed to PAMT walk to entry - PAMT is locked\n");
        return_val = api_error_with_operand_id(return_val, OPERAND_ID_RCX);
        goto EXIT;
    }
    page_owner_pa = get_pamt_entry_owner(reclaimed_page_pamt_entry_ptr);
    reclaimed_page_pamt_locked_flag = true;
    #else 
        // SOPHIA assume 4KB
        reclaimed_page_pamt_entry_ptr = &(tables[reclaimed_page_pa.raw & HKID_MASK].pamt_entry);
        reclaimed_page_leaf_size = PT_4KB; // assume 4KB for now
        page_owner_pa.raw = reclaimed_page_pa.raw; // assume page is owned by itself for now
    #endif // SOURCE
 
    #ifdef SOURCE
    // Verify that the target page type is not NDA or reserved
    if ((reclaimed_page_pamt_entry_ptr->pt == PT_NDA) ||
        (reclaimed_page_pamt_entry_ptr->pt == PT_RSVD))
    {
        TDX_WARN("Page to reclaim is NDA or reserved\n");
        return_val = api_error_with_operand_id(TDX_PAGE_METADATA_INCORRECT, OPERAND_ID_RCX);
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(tables[reclaimed_page_pa.raw & HKID_MASK].pamt_entry.pt != PT_NDA);
        __CPROVER_assume(tables[reclaimed_page_pa.raw & HKID_MASK].pamt_entry.pt != PT_RSVD);
    #endif // MODULAR_PROOF

    // #ifdef FLOW_PROOF
    //     if ((tables[page_pa & HKID_MASK].pamt_entry.pt == PT_NDA) ||
    //         (tables[page_pa & HKID_MASK].pamt_entry.pt == PT_RSVD))
    //     {
    //         // __CPROVER_assert((tables[reclaimed_page_pa.raw & HKID_MASK].pamt_entry.pt != PT_NDA) &&
    //         //                  (tables[reclaimed_page_pa.raw & HKID_MASK].pamt_entry.pt != PT_RSVD), "Reclaimed page is not NDA or RSVD");
    //         TDX_WARN("Page to reclaim is NDA or reserved\n");
    //         return_val = api_error_with_operand_id(TDX_PAGE_METADATA_INCORRECT, OPERAND_ID_RCX);
    //         goto EXIT;
    //     }
    // #endif // FLOW_PROOF

    #ifdef SOURCE
    reclaimed_page_level.level = reclaimed_page_leaf_size;
    local_data_ptr->vmm_regs.rcx = reclaimed_page_pamt_entry_ptr->pt;
    local_data_ptr->vmm_regs.rdx = page_owner_pa.raw;
    local_data_ptr->vmm_regs.r8 = reclaimed_page_level.raw;
    #endif // SOURCE

    #ifdef MODULAR_PROOF
    reclaimed_page_level.level = reclaimed_page_leaf_size;
    local_data.vmm_regs.rcx = tables[reclaimed_page_pa.raw & HKID_MASK].pamt_entry.pt;
    local_data.vmm_regs.rdx = page_owner_pa.raw;
    local_data.vmm_regs.r8 = reclaimed_page_level.raw;  
    #endif // MODULAR_PROOF

    // Re-check page alignment if PAMT entry is larger than 4KB
    // 2MB = 4KB*(2^9) 1GB = 4KB*(2^18)
    #ifdef SOURCE
    if ((reclaimed_page_leaf_size > PT_4KB) &&
        (!is_addr_aligned_pwr_of_2(reclaimed_page_pa.raw, TDX_PAGE_SIZE_IN_BYTES << (9 * reclaimed_page_leaf_size))))
    {
        TDX_ERROR("Page is not aligned according to its PAMT level = (%d)\n", reclaimed_page_leaf_size);
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
        goto EXIT;
    }
    #endif // SOURCE

    // SOPHIA: assume only PT_4KB for now so can abstract away

    // Reclaimed page is not a TDR, lock and map the TDR to update params
    #ifdef SOURCE
    if (reclaimed_page_pamt_entry_ptr->pt != PT_TDR)
    {
        // Lock and map the TDR page
        return_val = lock_and_map_implicit_tdr(page_owner_pa,
                                               OPERAND_ID_TDR,
                                               TDX_RANGE_RW,
                                               TDX_LOCK_SHARED,
                                               &tdr_pamt_entry_ptr,
                                               &tdr_locked_flag,
                                               &tdr_ptr);
        if (return_val != TDX_SUCCESS)
        {
            TDX_ERROR("Failed to lock/map a TDR page - error = %llx\n", return_val);
            goto EXIT;
        }

        // Verify that the TD is in teardown state
        if (tdr_ptr->management_fields.lifecycle_state != TD_TEARDOWN)
        {
            TDX_ERROR("TD lifecycle state is not in teardown\n");
            return_val = TDX_LIFECYCLE_STATE_INCORRECT;
            goto EXIT;
        }

        // Atomically decrement TDR child count by the amount of reclaimed 4KB pages
        (void)_lock_xadd_64b(&tdr_ptr->management_fields.chldcnt, -(1 << (9 * reclaimed_page_leaf_size)));
    }
    else // Reclaimed page is TDR
    {
        // Map the TDR page to read the TD state variables
        // no need to walk n' lock as we already did that for this page
        tdr_ptr = map_pa_with_global_hkid((void*)reclaimed_page_pa.raw, TDX_RANGE_RW);

        // Verify that the TD is in teardown state
        if (tdr_ptr->management_fields.lifecycle_state != TD_TEARDOWN)
        {
            TDX_ERROR("TD lifecycle state is not in teardown\n");
            return_val = TDX_LIFECYCLE_STATE_INCORRECT;
            goto EXIT;
        }

        // Verify that TDR.CHLDCNT is 0
        if (tdr_ptr->management_fields.chldcnt != 0)
        {
            TDX_ERROR("Reclaiming TDR but child count is not zero = (%llu)\n",
                       tdr_ptr->management_fields.chldcnt);
            return_val = TDX_TD_ASSOCIATED_PAGES_EXIST;
            goto EXIT;
        }
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(tables[page_owner_pa.raw & HKID_MASK].tdr.management_fields.lifecycle_state == TD_TEARDOWN);
        if (reclaimed_page_pamt_entry_ptr->pt != PT_TDR) {
            // Decrement child count
            __CPROVER_assume(tables[page_owner_pa.raw & HKID_MASK].tdr.management_fields.chldcnt >= (1 << (9 * reclaimed_page_leaf_size)));
        } else {
            __CPROVER_assume(tables[reclaimed_page_pa.raw & HKID_MASK].tdr.management_fields.chldcnt == 0);
        }
    #endif // MODULAR_PROOF

    // ALL_CHECKS_PASSED:  The function is guaranteed to succeed

    // Update the PAMT entry of the reclaimed page to PT_FREE
    #ifdef FLOW_PROOF
    #else
    reclaimed_page_pamt_entry_ptr->pt = PT_NDA;
    #endif // FLOW_PROOF

    return_val= TDX_SUCCESS;

EXIT:
    #ifdef SOURCE
    // Release all acquired locks and free keyhole mappings
    if (tdr_locked_flag)
    {
        pamt_implicit_release_lock(tdr_pamt_entry_ptr, TDX_LOCK_SHARED);
    }
    if (tdr_ptr != NULL)
    {
        free_la(tdr_ptr);
    }
    if (reclaimed_page_pamt_locked_flag)
    {
        pamt_unwalk(reclaimed_page_pa,
                    reclaimed_page_pamt_block,
                    reclaimed_page_pamt_entry_ptr,
                    TDX_LOCK_EXCLUSIVE,
                    reclaimed_page_leaf_size);
    }
    #endif // SOURCE

    return return_val;
}
