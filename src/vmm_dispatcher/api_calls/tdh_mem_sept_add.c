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
 * @file tdh_mem_sept_add
 * @brief TDHMEMSEPTADD API handler
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

static void init_new_sept_page(tdr_t* tdr_ptr, pa_t tdr_pa, pa_t sept_page_pa,
                               pamt_entry_t* sept_page_pamt_entry_ptr, uint64_t sept_page_init_val)
{
    void* sept_page_ptr;

    // Map the new SEPT EPT page
    sept_page_ptr = map_pa_with_hkid(sept_page_pa.raw_void, tdr_ptr->key_management_fields.hkid, TDX_RANGE_RW);

    // Initialize the new Secure EPT page using the TD’s ephemeral private HKID and direct writes(MOVDIR64B)
    fill_area_cacheline(sept_page_ptr, TDX_PAGE_SIZE_IN_BYTES, sept_page_init_val);

    // Update the new Secure EPT page’s PAMT entry
    sept_page_pamt_entry_ptr->pt = PT_EPT;
    set_pamt_entry_owner(sept_page_pamt_entry_ptr, tdr_pa);
    sept_page_pamt_entry_ptr->bepoch.raw = 0;

    // Increment TDR child count, use an atomic operation since we have SHARED lock on TDR
    (void)_lock_xadd_64b(&(tdr_ptr->management_fields.chldcnt), 1);

    free_la(sept_page_ptr);
}

static api_error_type process_l1_page(tdx_module_local_t* local_data_ptr, uint64_t version,
                                      pa_t sept_page_pa[MAX_VMS], pa_t flagged_sept_page_pa[MAX_VMS],
                                      ept_level_t page_level_entry, ia32e_sept_t page_sept_entry_copy,
                                      pamt_block_t sept_page_pamt_block[MAX_VMS], pamt_entry_t* sept_page_pamt_entry_ptr[MAX_VMS],
                                      bool_t sept_page_locked_flag[MAX_VMS], bool_t allow_existing)
{
    api_error_type return_val;

    if (sept_page_pa[0].raw == NULL_PA)
    {
        /* No new L1 SEPT page was provided.  This is only allowed for version 1 or higher, and an L1 SEPT page
           must already exist.  The existing L1 SEPT entry should be non-leaf, mapped. */
        #ifdef SOURCE
        if (version == 0)
        {
            return api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_R8);
        }
        #endif // SOURCE

        #ifdef MODULAR_PROOF
            __CPROVER_assume(version > 0); //"Version is greater than 0 when no new L1 SEPT page is provided"
        #endif // MODULAR_PROOF

        #ifdef FLOW_PROOF
            if (version == 0) {
                __CPROVER_assert(version > 0, "Version is greater than 0 when no new L1 SEPT page is provided");
                return api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_R8); 
            }
        #endif // FLOW_PROOF

        #ifdef SOURCE
        if (!is_sept_nl_mapped(&page_sept_entry_copy))
        {
            set_arch_septe_details_in_vmm_regs(page_sept_entry_copy, page_level_entry, local_data_ptr);
            TDX_ERROR("Parent entry is not non-leaf and mapped - 0x%llx\n", page_sept_entry_copy.raw);
            return api_error_with_operand_id(TDX_EPT_ENTRY_STATE_INCORRECT, OPERAND_ID_RCX);
        }
        #endif // SOURCE 

        #ifdef MODULAR_PROOF
            __CPROVER_assume(is_sept_nl_mapped(&page_sept_entry_copy)); //"The parent SEPT entry is non-leaf and mapped"
        #endif // MODULAR_PROOF
        #ifdef FLOW_PROOF
            if (!is_sept_nl_mapped(&page_sept_entry_copy)) {
                __CPROVER_assert(is_sept_nl_mapped(&page_sept_entry_copy), "The parent SEPT entry is non-leaf and mapped");
                return api_error_with_operand_id(TDX_EPT_ENTRY_STATE_INCORRECT, OPERAND_ID_RCX); 
            }
        #endif // FLOW_PROOF    
    }
    else
    {
        // A new L1 SEPT page was provided
        // Verify the parent entry located for new SEPT page is FREE

        if (is_sept_free(&page_sept_entry_copy))
        {
            #ifdef SOURCE
            // Prepare the new L1 SEPT page. The page will be added later after all checks are done.
            // Check and lock the new SEPT page in PAMT
            return_val = check_and_lock_explicit_4k_private_hpa(sept_page_pa[0],
                                                                OPERAND_ID_R8,
                                                                TDX_LOCK_EXCLUSIVE,
                                                                PT_NDA,
                                                                &sept_page_pamt_block[0],
                                                                &sept_page_pamt_entry_ptr[0],
                                                                &sept_page_locked_flag[0]);

            if (return_val != TDX_SUCCESS)
            {
                TDX_ERROR("Failed to check/lock the new SEPT EPT page 0x%llx - error = %llx\n",
                           sept_page_pa[0].raw, return_val);
                return return_val;
            }
            #endif // SOURCE 

            #ifdef MODULAR_PROOF
                __CPROVER_assume(is_sept_free(&page_sept_entry_copy));  //"The parent SEPT entry is free" 
            #endif // MODULAR_PROOF

            #ifdef FLOW_PROOF
            __CPROVER_assert(is_sept_free(&page_sept_entry_copy), "The parent SEPT entry is free");
            #endif // FLOW_PROOF
        }
        else
        {
            // An SEPT page already exists
            if (allow_existing)
            {
                #ifdef FLOW_PROOF
                #else 
                flagged_sept_page_pa[0].raw |= BIT(63); // Set bit 63 to indicate that the page has not been used
                #endif // FLOW_PROOF
            }
            #ifdef SOURCE
            else
            {
                set_arch_septe_details_in_vmm_regs(page_sept_entry_copy, page_level_entry, local_data_ptr);
                TDX_ERROR("SEPT page already exists - 0x%llx, but existing pages are not allowed\n", page_sept_entry_copy.raw);
                return api_error_with_operand_id(TDX_EPT_ENTRY_STATE_INCORRECT, OPERAND_ID_RCX);
            }
            #endif // SOURCE
        }
    }   // A new L1 SEPT page was provided

    return TDX_SUCCESS;
}

static api_error_type process_l2_pages(tdr_t* tdr_ptr, tdcs_t* tdcs_ptr, pa_t sept_page_pa[MAX_VMS],
                                       pa_t flagged_sept_page_pa[MAX_VMS],
                                       pa_t page_gpa, ept_level_t page_level_entry,
                                       pamt_block_t sept_page_pamt_block[MAX_VMS], pamt_entry_t* sept_page_pamt_entry_ptr[MAX_VMS],
                                       bool_t sept_page_locked_flag[MAX_VMS], ia32e_sept_t* page_sept_entry_ptr[MAX_VMS],
                                       bool_t allow_existing)
{
    api_error_type return_val = UNINITIALIZE_ERROR;

    for (uint16_t vm_id = 1; vm_id < MAX_VMS; vm_id++)
    {
        if (sept_page_pa[vm_id].raw == NULL_PA)
        {
            continue;
        }

        // Check that the request is for an existing L2 VM
        if (vm_id > tdcs_ptr->management_fields.num_l2_vms)
        {
            TDX_ERROR("Requested VM (%d) doesn't exist\n", vm_id);
            return api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_R8 + vm_id);
        }

        // Walk the L2 SEPT tree to locate the parent entry for the new SEPT page
        return_val = l2_sept_walk(tdr_ptr, tdcs_ptr, vm_id, page_gpa, &page_level_entry, &page_sept_entry_ptr[vm_id]);
        if (return_val != TDX_SUCCESS)
        {
            TDX_ERROR("L2 SEPT walk failed on VM(%d), level %d\n", vm_id, page_level_entry)
            set_arch_l2_septe_details_in_vmm_regs(*page_sept_entry_ptr[vm_id], vm_id,
                    tdcs_ptr->executions_ctl_fields.attributes.debug, page_level_entry, get_local_data());
            return api_error_with_l2_details(TDX_L2_SEPT_WALK_FAILED, vm_id, (uint16_t)page_level_entry);
        }

        if (is_l2_sept_free(page_sept_entry_ptr[vm_id]))
        {
            // Prepare the new L2 SEPT page.  The page will be added later after all checks are done.
            return_val = check_and_lock_explicit_4k_private_hpa(sept_page_pa[vm_id],
                                                                OPERAND_ID_R8 + vm_id,
                                                                TDX_LOCK_EXCLUSIVE,
                                                                PT_NDA,
                                                                &sept_page_pamt_block[vm_id],
                                                                &sept_page_pamt_entry_ptr[vm_id],
                                                                &sept_page_locked_flag[vm_id]);

            if (return_val != TDX_SUCCESS)
            {
                TDX_ERROR("Failed to check/lock the new SEPT EPT page 0x%llx for VM (%d) - error = %llx\n",
                           sept_page_pa[vm_id].raw, vm_id, return_val);
                return return_val;
            }
        }
        else
        {
            // An SEPT page already exists
            if (allow_existing)
            {
                flagged_sept_page_pa[vm_id].raw |= BIT(63); // Set bit 63 to indicate that the page has not been used
            }
            else
            {
                TDX_ERROR("L2 SEPT 0x%llx is not free, and existing not allowed!\n", page_sept_entry_ptr[vm_id]->raw);
                set_arch_l2_septe_details_in_vmm_regs(*page_sept_entry_ptr[vm_id], vm_id,
                        tdcs_ptr->executions_ctl_fields.attributes.debug, page_level_entry, get_local_data());
                return api_error_with_l2_details(TDX_L2_SEPT_ENTRY_NOT_FREE, vm_id, (uint16_t)page_level_entry);
            }
        }
    }

    return TDX_SUCCESS;
}

static api_error_type add_l1_and_l2_pages(uint64_t version, tdr_t* tdr_ptr, pa_t tdr_pa, pa_t sept_page_pa[MAX_VMS],
                                          pa_t flagged_sept_page_pa[MAX_VMS],
                                          pamt_entry_t* sept_page_pamt_entry_ptr[MAX_VMS],
                                          ia32e_sept_t* page_sept_entry_ptr[MAX_VMS],
                                          uint64_t original_rcx,
                                          uint64_t original_rdx)
{
    // Local data for return values
    #ifdef SOURCE
    tdx_module_local_t* local_data_ptr = get_local_data();
    #else
    tdx_module_local_t* local_data_ptr = &local_data;
    #endif // SOURCE

    bool_t sept_page_added_flag = false;

    #ifdef FLOW_PROOF
    #else 
    if (!(flagged_sept_page_pa[0].raw & BIT(63)))
    {
        // There's a new SEPT page (non-NULL and not pre-existing)
        #ifdef SOURCE
        init_new_sept_page(tdr_ptr, tdr_pa, sept_page_pa[0], sept_page_pamt_entry_ptr[0], SEPTE_INIT_VALUE);
        #endif // SOURCE

        #ifdef MODULAR_PROOF
            sept_page_pamt_entry_ptr[0]->pt = PT_EPT;
            sept_page_pamt_entry_ptr[0]->owner = tdr_pa;
            sept_page_pamt_entry_ptr[0]->bepoch.raw = 0;
        #endif // MODULAR_PROOF

        // Update the L1 SEPT entry in memory with the new Secure EPT page HPA and NL_MAPPED state.
        // Keep the L1 SEPT entry locked.
        #ifdef SOURCE
        sept_set_mapped_non_leaf(page_sept_entry_ptr[0], sept_page_pa[0], true);
        #endif // SOURCE

        #ifdef SOURCE
        // Nullify the page HPA to indicate it no longer needs to be allocated
        flagged_sept_page_pa[0].raw = NULL_PA;
        sept_page_added_flag = true;
        #endif // SOURCE
    }
    #endif // FLOW_PROOF

    // SOPHIA: not consider this case for now
    #ifdef SOURCE
    if (version > 0)
    {
        for (uint16_t vm_id = 1; vm_id < MAX_VMS; vm_id++)
        {
            if (!(flagged_sept_page_pa[vm_id].raw & BIT(63)))
            {
                // There's a new SEPT page (non-NULL and not pre-existing)
                // Check for a pending interrupt only if at least one SEPT page has been added
                if ((true == sept_page_added_flag) && is_interrupt_pending_host_side())
                {
                    // Restore the original RCX and RDX values and terminate the flow
                    local_data_ptr->vmm_regs.rcx = original_rcx;
                    local_data_ptr->vmm_regs.rdx = original_rdx;
                    TDX_ERROR("Pending interrupt\n");
                    return TDX_INTERRUPTED_RESUMABLE;
                }

                init_new_sept_page(tdr_ptr, tdr_pa, sept_page_pa[vm_id], sept_page_pamt_entry_ptr[vm_id], SEPTE_L2_INIT_VALUE);

                // Set the alias indication in the L1 SEPT entry
                sept_set_aliased(page_sept_entry_ptr[0], vm_id);

                // Map the new page in the parent table
                sept_l2_set_mapped_non_leaf(page_sept_entry_ptr[vm_id], sept_page_pa[vm_id]);

                // Nullify the page HPA to indicate it no longer needs to be allocated
                flagged_sept_page_pa[vm_id].raw = NULL_PA;

                sept_page_added_flag = true;
            }
        }
    }
    #endif // SOURCE

    return TDX_SUCCESS;
}

api_error_type tdh_mem_sept_add(page_info_api_input_t sept_level_and_gpa,
                                td_handle_and_flags_t target_tdr_and_flags,
                                uint64_t target_sept_page_pa,
                                uint64_t version)
{
    // Local data for return values
    #ifdef SOURCE 
    tdx_module_local_t  * local_data_ptr = get_local_data();
    #else 
    tdx_module_local_t  * local_data_ptr = &local_data;
    #endif // SOURCE 
    // TDR related variables
    pa_t                  tdr_pa = { .raw = 0 };     // TDR physical address
    tdr_t               * tdr_ptr;                   // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;            // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;        // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;   // Indicate TDR is locked

    tdcs_t              * tdcs_ptr = NULL;           // Pointer to the TDCS structure (Multi-page)

    // GPA and SEPT related variables
    pa_t                  page_gpa;                             // Target page GPA
    page_info_api_input_t gpa_mappings = sept_level_and_gpa;    // GPA and SEPT level
    ia32e_sept_t        * page_sept_entry_ptr[MAX_VMS] = { 0 }; // SEPT entry of the page
    ia32e_sept_t          page_sept_entry_copy;                 // Cached SEPT entry of the page
    ept_level_t           page_level_entry = sept_level_and_gpa.level;  // SEPT entry level of the page parent
    bool_t                sept_locked_flag = false;  // Indicate SEPT tree is locked
    bool_t                septe_locked_flag = false; // Indicate SEPT entry is locked

    // New SEPT EPT page variables
    pa_t                  flagged_sept_page_pa[MAX_VMS] = { 0 };     // Physical address of the new Secure-EPT page
    pa_t                  sept_page_pa[MAX_VMS] = { 0 };             // Physical address of the new Secure-EPT page - can be modified
    pamt_block_t          sept_page_pamt_block[MAX_VMS] = { 0 };     // New Secure-EPT page PAMT block
    pamt_entry_t        * sept_page_pamt_entry_ptr[MAX_VMS] = { 0 }; // Pointer to the Secure-EPT PAMT entry
    bool_t                sept_page_locked_flag[MAX_VMS] = { 0 };    // Indicate SEPT EPT page PAMT entry is locked

    api_error_type        return_val = UNINITIALIZE_ERROR;

    uint64_t original_rcx = local_data_ptr->vmm_regs.rcx;  // Original value of RCX, to be restored in case on an interrupt
    uint64_t original_rdx = local_data_ptr->vmm_regs.rdx;  // Original value of RDX, to be restored in case on an interrupt

    sept_page_pa[0].raw = target_sept_page_pa;

    if (version > 0)
    {
        sept_page_pa[1].raw = local_data_ptr->vmm_regs.r9;
        sept_page_pa[2].raw = local_data_ptr->vmm_regs.r10;
        sept_page_pa[3].raw = local_data_ptr->vmm_regs.r11;
    }

    // By default, no extended error code is returned
    local_data_ptr->vmm_regs.rcx = 0;
    local_data_ptr->vmm_regs.rdx = 0;

    // Only versions 0 and 1 are supported
    #ifdef SOURCE
    if (version > 1)
    {
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RAX);
        goto EXIT_NO_GPR_CHANGE;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(version <= 1);
        __CPROVER_assume(sept_level_and_gpa.level >= 0 && sept_level_and_gpa.level <= 3); // Constrain SEPT level to valid range
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
    if (version > 1)
        {
            __CPROVER_assert(version <= 1, "Version check failed");
            return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RAX);
            goto EXIT_NO_GPR_CHANGE;
        }
    #endif // FLOW_PROOF

    #ifdef FLOW_PROOF
    #else 
    // If the input new SEPT page pa is not NULL_PA, then we ignore bit 63
    for (uint16_t vm_id = 0; vm_id < MAX_VMS; vm_id++)
    {
        // If the input new SEPT page pa is not NULL_PA, then we ignore bit 63
        if (sept_page_pa[vm_id].raw != NULL_PA)
        {
            sept_page_pa[vm_id].raw &= ~BIT(63);
        }
        flagged_sept_page_pa[vm_id].raw = sept_page_pa[vm_id].raw;
    }
    #endif // FLOW_PROOF


    #ifdef SOURCE
    // Check the TD handle in RDX
    if (target_tdr_and_flags.reserved_0 || target_tdr_and_flags.reserved_1)
    {
        TDX_ERROR("Input TD handle (0x%llx) is not valid\n", target_tdr_and_flags.raw);
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
        goto EXIT;
    }
    #endif // SOURCE
    tdr_pa.page_4k_num  = target_tdr_and_flags.tdr_hpa_51_12;

    #ifdef MODULAR_PROOF
        __CPROVER_assume(!target_tdr_and_flags.reserved_0); 
        __CPROVER_assume(!target_tdr_and_flags.reserved_1); 
    #endif // MODULAR_PROOF 

    #ifdef FLOW_PROOF
        if (target_tdr_and_flags.reserved_0 || target_tdr_and_flags.reserved_1)
        {
            __CPROVER_assert(!target_tdr_and_flags.reserved_0, "TD handle reserved_0 check failed");
            __CPROVER_assert(!target_tdr_and_flags.reserved_1, "TD handle reserved_1 check failed");
            TDX_ERROR("Input TD handle (0x%llx) is not valid\n", target_tdr_and_flags.raw);
            return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
            goto EXIT;
        }
    #endif // FLOW_PROOF

    // Check, lock and map the owner TDR page (Shared lock!)
    #ifdef SOURCE
    return_val = check_lock_and_map_explicit_tdr(tdr_pa,
                                                 OPERAND_ID_RDX,
                                                 TDX_RANGE_RW,
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
        // SOPHIA: assume that this passes for now
        tdr_pamt_entry_ptr = &(tables[tdr_pa.raw & HKID_MASK].pamt_entry);
        tdr_ptr = &tables[tdr_pa.raw & HKID_MASK].tdr;
    #endif // SOURCE

    #ifdef SOURCE
    // Map the TDCS structure and check the state
    return_val = check_state_map_tdcs_and_lock(tdr_ptr, TDX_RANGE_RW, TDX_LOCK_SHARED,
                                               false, TDH_MEM_SEPT_ADD_LEAF, &tdcs_ptr);

    if (return_val != TDX_SUCCESS)
    {
        TDX_ERROR("State check or TDCS lock failure - error = %llx\n", return_val);
        goto EXIT;
    }
    #else 
        // SOPHIA: hardware model stub
        tdcs_ptr = &tables[tdr_pa.raw & HKID_MASK].tdcs_table;
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(!tdr_ptr->management_fields.fatal); // TD is not in fatal state
        __CPROVER_assume(tdr_ptr->management_fields.lifecycle_state == TD_KEYS_CONFIGURED); // TD keys are configured 
        __CPROVER_assume(tdr_ptr->management_fields.num_tdcx >= MIN_NUM_TDCS_PAGES); // Minimal num of TDCS pages allocated
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
        if (tdr_ptr->management_fields.fatal)
            {
                __CPROVER_assert(!tdr_ptr->management_fields.fatal, "TD is not in fatal state");
                TDX_ERROR("TD is in fatal state\n");
                return api_error_fatal(TDX_TD_FATAL);
            }
        
            if (tdr_ptr->management_fields.lifecycle_state != TD_KEYS_CONFIGURED)
            {
                __CPROVER_assert(tdr_ptr->management_fields.lifecycle_state == TD_KEYS_CONFIGURED, "TD keys are configured");
                TDX_ERROR("TD key are not configured\n");
                return TDX_TD_KEYS_NOT_CONFIGURED;
            }
        
            if (tdr_ptr->management_fields.num_tdcx < MIN_NUM_TDCS_PAGES)
            {
                __CPROVER_assert(tdr_ptr->management_fields.num_tdcx >= MIN_NUM_TDCS_PAGES, "Minimal num of TDCS pages allocated");
                TDX_ERROR("TDCS minimal num of pages %d is not allocated\n", MIN_NUM_TDCS_PAGES);
                return TDX_TDCS_NOT_ALLOCATED;
            }
    #endif // FLOW_PROOF

    #ifdef SOURCE 
    if (!verify_page_info_input(gpa_mappings, LVL_PD, tdcs_ptr->executions_ctl_fields.eptp.fields.ept_pwl))
    {
        TDX_ERROR("Input GPA page info (0x%llx) is not valid\n", gpa_mappings.raw);
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
        goto EXIT;
    }
    #endif // SOURCE 

    #ifdef MODULAR_PROOF
        __CPROVER_assume(verify_page_info_input(gpa_mappings, LVL_PD, tdcs_ptr->executions_ctl_fields.eptp.fields.ept_pwl));
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
        if (!verify_page_info_input(gpa_mappings, LVL_PD, tdcs_ptr->executions_ctl_fields.eptp.fields.ept_pwl))
        {
            __CPROVER_assert(verify_page_info_input(gpa_mappings, LVL_PD, tdcs_ptr->executions_ctl_fields.eptp.fields.ept_pwl), "Input GPA page info is valid");
            TDX_ERROR("Input GPA page info (0x%llx) is not valid\n", gpa_mappings.raw);
            return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
            goto EXIT;
        }
    #endif // FLOW_PROOF

    #ifdef SOURCE
    page_gpa = page_info_to_pa(gpa_mappings);
    #endif // SOURCE

    // // Step #1:
    // // L1 SEPT tree walk and state checks

    // Check GPA, lock SEPT and walk to find entry
    #ifdef SOURCE
    return_val = lock_sept_check_and_walk_private_gpa(tdcs_ptr,
                                                      OPERAND_ID_RCX,
                                                      page_gpa,
                                                      tdr_ptr->key_management_fields.hkid,
                                                      TDX_LOCK_SHARED,
                                                      &page_sept_entry_ptr[0],
                                                      &page_level_entry,
                                                      &page_sept_entry_copy,
                                                      &sept_locked_flag);
    if (return_val != TDX_SUCCESS)
    {
        if (return_val == api_error_with_operand_id(TDX_EPT_WALK_FAILED, OPERAND_ID_RCX))
        {
            // Update output register operands
            set_arch_septe_details_in_vmm_regs(page_sept_entry_copy, page_level_entry, local_data_ptr);
        }

        TDX_ERROR("Failed on GPA check, SEPT lock or walk - error = %llx\n", return_val);
        goto EXIT;
    }
    #else 
        // SOPHIA: assume that this passes for now
        page_sept_entry_ptr[0] = &tables[(page_gpa.raw & HKID_MASK)].sept_entries[0];
        page_level_entry = gpa_mappings.level;
        page_sept_entry_copy = *page_sept_entry_ptr[0];
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(!gpa_mappings.reserved_0);
        __CPROVER_assume(!gpa_mappings.reserved_1);
        __CPROVER_assume(gpa_mappings.level >= LVL_PD && gpa_mappings.level <= tdcs_ptr->executions_ctl_fields.eptp.fields.ept_pwl);
        __CPROVER_assume((gpa_mappings.gpa << 12) % (1ULL << (12 + (gpa_mappings.level * 9))) == 0);  // GPA is aligned
    #endif // MODULAR_PROOF 

    #ifdef FLOW_PROOF
        if (gpa_mappings.reserved_0 || gpa_mappings.reserved_1)
        {
            __CPROVER_assert(!gpa_mappings.reserved_0, "GPA mappings reserved_0 check failed");
            __CPROVER_assert(!gpa_mappings.reserved_1, "GPA mappings reserved_1 check failed");
            TDX_ERROR("Input GPA page info (0x%llx) is not valid\n", gpa_mappings.raw);
            return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
            goto EXIT;
        }

        if (gpa_mappings.level < LVL_PD || gpa_mappings.level > tdcs_ptr->executions_ctl_fields.eptp.fields.ept_pwl)
        {
            __CPROVER_assert(gpa_mappings.level >= LVL_PD && gpa_mappings.level <= tdcs_ptr->executions_ctl_fields.eptp.fields.ept_pwl,
                             "GPA mappings level check failed");
            TDX_ERROR("Input GPA page info (0x%llx) has invalid level %d\n", gpa_mappings.raw, gpa_mappings.level);
            return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
            goto EXIT;
        }

        if ((gpa_mappings.gpa << 12) % (1ULL << (12 + (gpa_mappings.level * 9))) != 0)
        {
            __CPROVER_assert((gpa_mappings.gpa << 12) % (1ULL << (12 + (gpa_mappings.level * 9))) == 0,
                             "GPA mappings alignment check failed");
            TDX_ERROR("Input GPA page info (0x%llx) has unaligned GPA\n", gpa_mappings.raw);
            return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
            goto EXIT;
        }
    #endif // FLOW_PROOF

    // Lock the SEPT entry in memory
    #ifdef SOURCE
    return_val = sept_lock_acquire_host(page_sept_entry_ptr[0]);
    if (TDX_SUCCESS != return_val)
    {
        return_val = api_error_with_operand_id(return_val, OPERAND_ID_RCX);
        set_arch_septe_details_in_vmm_regs(page_sept_entry_copy, page_level_entry, local_data_ptr);
        TDX_ERROR("Failed on SEPT host-side lock attempt\n");
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(!tables[(page_gpa.raw & HKID_MASK)].sept_page_lock);
        return_val = TDX_SUCCESS; 
        return return_val; 
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
        // Lock the SEPT entry in memory
        if (tables[(page_gpa.raw & HKID_MASK)].sept_page_lock)
        {
            __CPROVER_assert(tables[(page_gpa.raw & HKID_MASK)].sept_page_lock, "Failed on SEPT host-side lock attempt");
            return_val = api_error_with_operand_id(return_val, OPERAND_ID_RCX);
            set_arch_septe_details_in_vmm_regs(page_sept_entry_copy, page_level_entry, local_data_ptr);
            TDX_ERROR("Failed on SEPT host-side lock attempt\n");
            goto EXIT;
        }
    #endif // FLOW_PROOF
    septe_locked_flag = true;

    // // Read the SEPT entry (again after locking)
    // page_sept_entry_copy = *page_sept_entry_ptr[0];

    #ifdef SOURCE
    // Check if the L1 SEPT entry state is allowed.  Refined checks are done below.
    if (!sept_state_is_seamcall_leaf_allowed(TDH_MEM_SEPT_ADD_LEAF, page_sept_entry_copy))
    {
        TDX_ERROR("L1 SEPT sate (0x%llx) is not allowed for this SEAMCALL\n", page_sept_entry_copy.raw);
        set_arch_septe_details_in_vmm_regs(page_sept_entry_copy, page_level_entry, local_data_ptr);
        return_val = api_error_with_operand_id(TDX_EPT_ENTRY_STATE_INCORRECT, OPERAND_ID_RCX);
        goto EXIT;
    }
    #endif // SOURCE

    //SOPHIA: assume the check is allowed
    #ifdef FLOW_PROOF
        // Constrain the encoding and index to avoid out-of-bounds in the auto-generated lookup
        uint64_t septe_state_enc = SEPT_CONVERT_TO_ENCODING(page_sept_entry_copy);
        __CPROVER_assume(septe_state_enc < MAX_SEPT_STATE_ENC);
        __CPROVER_assume(sept_special_flags_lookup[septe_state_enc].index < NUM_SEPT_STATES);

        // Now it's safe to assume the seamcall lookup allows the operation
        __CPROVER_assume(seamcall_sept_state_lookup[TDH_MEM_SEPT_ADD_LEAF]
                          [sept_special_flags_lookup[septe_state_enc].index]);
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
    #endif // FLOW_PROOF

    
    // Step #2:
    // Check and lock the new L1 and L2 SEPT physical pages.

    #ifdef SOURCE
    // Process the L1 SEPT page. Either add a new page or make sure it exists.
    return_val = process_l1_page(local_data_ptr, version, sept_page_pa, flagged_sept_page_pa,
                                 page_level_entry, page_sept_entry_copy,
                                 sept_page_pamt_block, sept_page_pamt_entry_ptr, sept_page_locked_flag,
                                 target_tdr_and_flags.allow_existing);
    #endif // SOURCE

    #ifdef SOURCE 
    if (return_val != TDX_SUCCESS)
    {
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef SOURCE
    // Process the L2 SEPT pages
    if (version > 0)
    {
        return_val = process_l2_pages(tdr_ptr, tdcs_ptr, sept_page_pa, flagged_sept_page_pa, page_gpa, page_level_entry,
                                      sept_page_pamt_block, sept_page_pamt_entry_ptr, sept_page_locked_flag,
                                      page_sept_entry_ptr, target_tdr_and_flags.allow_existing);

        if (return_val != TDX_SUCCESS)
        {
            goto EXIT;
        }
    }
    #endif // SOURCE

    // Step #3:
    // Add the new L1 and L2 SEPT pages
    #ifdef SOURCE
    return_val = add_l1_and_l2_pages(version, tdr_ptr, tdr_pa, sept_page_pa, flagged_sept_page_pa,
                                     sept_page_pamt_entry_ptr, page_sept_entry_ptr, original_rcx, original_rdx);

    if (return_val != TDX_SUCCESS)
    {
        goto EXIT;
    }
    #endif // SOURCE

    return_val = TDX_SUCCESS;
    __CPROVER_assert(false, "false");  
EXIT:

    #ifdef SOURCE
    if (version > 0)
    {
        local_data_ptr->vmm_regs.r8  = flagged_sept_page_pa[0].raw;
        local_data_ptr->vmm_regs.r9  = flagged_sept_page_pa[1].raw;
        local_data_ptr->vmm_regs.r10 = flagged_sept_page_pa[2].raw;
        local_data_ptr->vmm_regs.r11 = flagged_sept_page_pa[3].raw;
    }
    #endif // SOURCE
EXIT_NO_GPR_CHANGE:

    // Release all acquired locks and free keyhole mappings

    #ifdef SOURCE
    if (septe_locked_flag)
    {
        sept_lock_release(page_sept_entry_ptr[0]);
    }

    for (uint16_t vm_id = 0; vm_id < MAX_VMS; vm_id++)
    {
        if (sept_page_locked_flag[vm_id])
        {
            pamt_unwalk(sept_page_pa[vm_id], sept_page_pamt_block[vm_id], sept_page_pamt_entry_ptr[vm_id],
                        TDX_LOCK_EXCLUSIVE, PT_4KB);
        }

        if (page_sept_entry_ptr[vm_id] != NULL)
        {
            free_la(page_sept_entry_ptr[vm_id]);
        }
    }

    if (sept_locked_flag)
    {
        release_sharex_lock_sh(&tdcs_ptr->executions_ctl_fields.secure_ept_lock);
    }

    if (tdcs_ptr != NULL)
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
