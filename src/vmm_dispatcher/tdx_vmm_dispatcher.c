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
     * SOPHIA: tdx_seamcall_entry_point.S
     * SOPHIA: TDX instruction setup
     * SOPHIA: TDX instruction valid instruction check 
     * SOPHIA: TDX instruction execution
     * SOPHIA: TDX instruction close
     * SOPHIA: tdx_seamret_to_vmm.S
*/

/**
 * @file tdx_vmm_dispatcher.c
 * @brief VMM dispatcher and return sequence
 */
#include "vmm_dispatcher/tdx_vmm_dispatcher.h"
#include "accessors/ia32_accessors.h"
#include "accessors/vt_accessors.h"
#include "accessors/data_accessors.h"
#include "x86_defs/vmcs_defs.h"
#include "auto_gen/tdx_error_codes_defs.h"
#include "data_structures/tdx_global_data.h"
#include "data_structures/tdx_local_data.h"
#include "tdx_vmm_api_handlers.h"
#include "debug/tdx_debug.h"
#include "helpers/helpers.h"
#include "metadata_handlers/metadata_generic.h"

_STATIC_INLINE_ void mark_lp_as_busy(void)
{
    get_local_data()->lp_is_busy = true;
}

_STATIC_INLINE_ void mark_lp_as_free(void)
{
    get_local_data()->lp_is_busy = false;
}

void tdx_vmm_dispatcher(void)
{
    // SOPHIA: TDX instruction setup START
    // AHMAD: PRECONDITION - No TDX state (local or global data) has been retrieved.
    // AHMAD: local data refers data tied to a specfic LP excecution context and it lives in a secure section of memory only accessed by the LP
    // AHMAD: global data refers to data that is shared across all LPs and lives in a secure memory region only accessible by the excecuting LPs  

    // Must be first thing to do before accessing local/global data or sysinfo table
    /** 
     * SOPHIA: local_data->local_data_fast_ref_ptr = NULL
     * SOPHIA: local_data->sysinfo_table = NULL
     * SOPHIA: local_data->global_data_fast_ref_ptr = NULL 
    */

    tdx_module_local_t* local_data = init_data_fast_ref_ptrs();

    /**
     * SOPHIA: local_data->local_data_fast_ref_ptr = address of local_data
     * SOPHIA: local_data->sysinfo_table = address of sysinfo_table
     * SOPHIA: local_data->global_data_fast_ref_ptr = address of global_data after region of local_data 
    */

    TDX_LOG("Module entry start\n");

    // SOPHIA: prints out statement that TDX Module has been entered
    vm_vmexit_exit_reason_t exit_reason;
    ia32_vmread(VMX_VM_EXIT_REASON_ENCODE, &exit_reason.raw);

    // AHMAD: exit_reason.raw = VMCS[VMX_VM_EXIT_REASON_ENCODE]
    // SOPHIA: exit_reason reads from VMCS data on why the VMM exited like basic_reason, bus_lock_preempted, enclave_interruption, etc.
    
    tdx_sanity_check(exit_reason.basic_reason == VMEXIT_REASON_SEAMCALL, SCEC_VMM_DISPATCHER_SOURCE, 2);
    // SOPHIA: reads exit_reason to make sure it was basic_reason of the VMM made a SEAMCALL

    tdx_module_global_t * global_data = get_global_data();
    // SOPHIA: global_data = address of global_data after region of local_data
    // Get leaf code from RAX in local data (saved on entry)
    tdx_leaf_and_version_t leaf_opcode;
    leaf_opcode.raw = local_data->vmm_regs.rax;

    // AHMAD: leaf_opcode.raw = RAX
    // SOPHIA: gets leaf opcode which is stored in the rax register

    ia32_core_capabilities_t core_capabilities;

    TDX_LOG("leaf_opcode = 0x%llx\n", leaf_opcode);

    bhb_drain_sequence(global_data);
    // SOPHIA: performs a series of jumps to clear out the branch predictor to be all taken

    mark_lp_as_busy();

    // AHMAD: get_local_data()->lp_is_busy = true;
    // SOPHIA: local memory of the LP is marked as being busy

    // Save IA32_SPEC_CTRL and set speculative execution variant 4 defense
    // using Speculative Store Bypass Disable (SSBD), which delays speculative
    // execution of a load until the addresses for all older stores are known.
    local_data->vmm_non_extended_state.ia32_spec_ctrl = ia32_rdmsr(IA32_SPEC_CTRL_MSR_ADDR);
    wrmsr_opt(IA32_SPEC_CTRL_MSR_ADDR, TDX_MODULE_IA32_SPEC_CTRL, local_data->vmm_non_extended_state.ia32_spec_ctrl);

    // AHMAD: MSR = TDX_MODULE_IA32_SPEC_CTRL if needed

    // All IA32_DEBGCTL bits have been cleared by SEAMCALL.
    // Set IA32_DEBUGCTL.ENABLE_UNCORE_PMI to the VMM's value, all other bits remain 0.
    ia32_debugctl_t debugctl;
    ia32_vmread(VMX_GUEST_IA32_DEBUGCTLMSR_FULL_ENCODE, &debugctl.raw);
    local_data->ia32_debugctl_value.raw = 0;
    local_data->ia32_debugctl_value.en_uncore_pmi = debugctl.en_uncore_pmi;
    wrmsr_opt(IA32_DEBUGCTL_MSR_ADDR, local_data->ia32_debugctl_value.raw, debugctl.raw);

    // AHMAD: MSR = {0, VMCS[VMX_GUEST_IA32_DEBUGCTLMSR_FULL_ENCODE].en_uncore_pmi} if needed
    //SOPHIA: write to Debug MSR the IA32_DEBGCTL bits

    // SOPHIA: LAM is Linear Address Mapping?
    // If simplified LAM is supported, save & disable its state
    if (local_data->lp_is_init)
    {
        core_capabilities.raw = global_data->plt_common_config.ia32_core_capabilities.raw;
    }
    else
    {
        core_capabilities.raw = ia32_rdmsr(IA32_CORE_CAPABILITIES);
    }

    // SOPHIA: restore lp data if it has already been initialized, otherwise read the MSR (model specific register)

    local_data->vmm_non_extended_state.ia32_lam_enable = 0;
    if (core_capabilities.lam_supported != 0)
    {
        local_data->vmm_non_extended_state.ia32_lam_enable = ia32_rdmsr(IA32_LAM_ENABLE_MSR_ADDR);
        if (local_data->vmm_non_extended_state.ia32_lam_enable != 0)
        {
            ia32_wrmsr(IA32_LAM_ENABLE_MSR_ADDR, 0);
        }
    }
    // SOPHIA: TDX instruction setup END
    // AHMAD: POSTCONDITION - relevant local and global data is retrieved (explained in precondition)
    // AHMAD: POSTCONDITION - Branch predictor is cleared. Branch predictor is part of the hardware and not secure so flushing occurs to mitigate speculative execution attacks 
    // AHMAD: POSTCONDITION - IA32_SPEC_CTRL register is saved to local data and speculation defenses are applied. This is a hardware register not specific to TDX
    // AHMAD: POSTCONDITION - IA32_DEBUGCTL MSR is restored based on VMM data. This is a hardware register not specific to the TDX

    // AHMAD: if LAM is supported, ia32_lam_enable  = ia32_rdmsr(IA32_LAM_ENABLE_MSR_ADDR), else ia32_lam_enable = 0
    // AHMAD: if LAM enable, MSR = 0
    // SOPHIA: if LAM is supported, write to the MSR the current state

    // SOPHIA: TDX instruction valid instruction check START
    // AHMAD: PRECONDITION - local data and global data is retrieved

    if ((leaf_opcode.reserved0 != 0) || (leaf_opcode.reserved1 != 0))
    {
        TDX_ERROR("Leaf and version not supported 0x%llx\n", leaf_opcode.raw);
        // update RAX in local data with error code
        local_data->vmm_regs.rax = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RAX);
        goto EXIT;
    }

    // SOPHIA: error check to make sure reserved bits in the opcode are not written to something else incorrectly

    // Only a few functions have multiple versions
    if (leaf_opcode.version > 0)
    {
        switch (leaf_opcode.leaf)
        {
            case TDH_MEM_PAGE_PROMOTE_LEAF:
            case TDH_MEM_SEPT_ADD_LEAF:
            case TDH_MEM_SEPT_REMOVE_LEAF:
            case TDH_MNG_RD_LEAF:
            case TDH_VP_RD_LEAF:
            case TDH_VP_INIT_LEAF:
                break;
            default:
                TDX_ERROR("Version greater than zero not supported for current leaf 0x%llx\n", leaf_opcode.raw);
                // update RAX in local data with error code
                local_data->vmm_regs.rax = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RAX);
                goto EXIT;
        }
    }
    // SOPHIA: if there is a version, check to make sure that it is a one of a specific subset of operations

    if (SYS_SHUTDOWN == global_data->global_state.sys_state)
    {
        if (leaf_opcode.leaf != TDH_SYS_LP_SHUTDOWN_LEAF)
        {
            TDX_ERROR("Module in shutdown state\n");
            // update RAX in local data with error code
            local_data->vmm_regs.rax = TDX_SYS_SHUTDOWN;
            goto EXIT;
        }
    }
    // SOPHIA: check for errors associated with shutting down and not having the corresponding TDX shutdown opcode

    // Check if module is in ready state, if not
    // only some leaf functions are allowed to run
    if ((global_data->global_state.sys_state != SYS_READY)     &&
        (leaf_opcode.leaf != TDH_SYS_INFO_LEAF)                &&
        (leaf_opcode.leaf != TDH_SYS_RD_LEAF)                  &&
        (leaf_opcode.leaf != TDH_SYS_RDALL_LEAF)               &&
        (leaf_opcode.leaf != TDH_SYS_INIT_LEAF)                &&
        (leaf_opcode.leaf != TDH_SYS_LP_INIT_LEAF)             &&
        (leaf_opcode.leaf != TDH_SYS_CONFIG_LEAF)              &&
        (leaf_opcode.leaf != TDH_SYS_KEY_CONFIG_LEAF)          &&
        (leaf_opcode.leaf != TDH_SYS_LP_SHUTDOWN_LEAF)         &&
        (leaf_opcode.leaf != TDH_SYS_UPDATE_LEAF)
#ifdef DEBUGFEATURE_TDX_DBG_TRACE
        && (leaf_opcode.leaf != TDDEBUGCONFIG_LEAF)
#endif
        )
    {
        TDX_ERROR("Module system not ready, can't execute leaf 0x%llx\n", leaf_opcode.leaf);
        // update RAX in local data with error code
        local_data->vmm_regs.rax = TDX_SYS_NOT_READY;
        goto EXIT;
    }
    // SOPHIA: checks on module ready state, if not fully ready, only some leaf operations will be allowed so need to error out for the other cases
    // SOPHIA: TDX instruction valid instruction check END
    // AHMAD: POSTCONDITION - All data remains unchanged through this section of code assuming no error (only reach here if no error)

    // SOPHIA: TDX instruction execution START
    // AHMAD: PRECONDITION - The SEAMCALL instruction has passed validation, and the system (hardware and firmware) is in a valid and secure state.

    // SOPHIA: big switch statement to cover all the possible leaf statements
    // switch over leaf opcodes
    switch (leaf_opcode.leaf)
    {
#ifdef DEBUGFEATURE_TDX_DBG_TRACE
    case TDDEBUGCONFIG_LEAF:
    {
        local_data->vmm_regs.rax = td_debug_config(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx,
                                                   local_data->vmm_regs.r8);
        break;
    }
#endif
    case TDH_MNG_ADDCX_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mng_add_cx(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_MEM_PAGE_ADD_LEAF:
    {
        page_info_api_input_t gpa_page_info;
        gpa_page_info.raw = local_data->vmm_regs.rcx;
        local_data->vmm_regs.rax = tdh_mem_page_add(gpa_page_info,
                                               local_data->vmm_regs.rdx,
                                               local_data->vmm_regs.r8,
                                               local_data->vmm_regs.r9);
        break;
    }
    case TDH_MEM_SEPT_ADD_LEAF:
    {
        page_info_api_input_t sept_level_and_gpa;
        sept_level_and_gpa.raw = local_data->vmm_regs.rcx;

        td_handle_and_flags_t target_tdr_and_flags = { .raw = local_data->vmm_regs.rdx };

        local_data->vmm_regs.rax = tdh_mem_sept_add(sept_level_and_gpa,
                                                    target_tdr_and_flags,
                                                    local_data->vmm_regs.r8,
                                                    leaf_opcode.version);
        break;
    }
    case TDH_VP_ADDCX_LEAF:
    {
        local_data->vmm_regs.rax = tdh_vp_addcx(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_MEM_PAGE_RELOCATE:
    {
        local_data->vmm_regs.rax = tdh_mem_page_relocate(local_data->vmm_regs.rcx,
                local_data->vmm_regs.rdx, local_data->vmm_regs.r8);

        break;
    }
    case TDH_MEM_PAGE_AUG_LEAF:
    {
        page_info_api_input_t gpa_page_info;
        gpa_page_info.raw = local_data->vmm_regs.rcx;
        local_data->vmm_regs.rax = tdh_mem_page_aug(gpa_page_info,
                                               local_data->vmm_regs.rdx,
                                               local_data->vmm_regs.r8);
        break;
    }
    case TDH_MEM_RANGE_BLOCK_LEAF:
    {
        page_info_api_input_t page_info;
        page_info.raw = local_data->vmm_regs.rcx;

        local_data->vmm_regs.rax = tdh_mem_range_block(page_info, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_MNG_KEY_CONFIG_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mng_key_config(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_MNG_CREATE_LEAF:
    {
        hkid_api_input_t hkid_info;
        hkid_info.raw = local_data->vmm_regs.rdx;

        local_data->vmm_regs.rax = tdh_mng_create(local_data->vmm_regs.rcx, hkid_info);
        break;
    }
    case TDH_VP_CREATE_LEAF:
    {
        local_data->vmm_regs.rax = tdh_vp_create(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_MNG_INIT_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mng_init(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_VP_INIT_LEAF:
    {
        local_data->vmm_regs.rax = tdh_vp_init(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_MNG_RD_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mng_rd(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx,
                                              leaf_opcode.version);
        break;
    }
    case TDH_MEM_RD_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mem_rd(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_MNG_WR_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mng_wr(local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.rdx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9);
        break;
    }
    case TDH_MEM_WR_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mem_wr(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx,
                                                    local_data->vmm_regs.r8);
        break;
    }
    case TDH_MEM_PAGE_DEMOTE_LEAF:
    {
        page_info_api_input_t page_info;
        page_info.raw = local_data->vmm_regs.rcx;

        td_handle_and_flags_t target_tdr_and_flags = { .raw = local_data->vmm_regs.rdx };

        local_data->vmm_regs.rax = tdh_mem_page_demote(page_info, target_tdr_and_flags);
        break;
    }
    case TDH_VP_ENTER_LEAF:
    {
        local_data->vmm_regs.rax = tdh_vp_enter(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_MR_EXTEND_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mr_extend(local_data->vmm_regs.rcx, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_MR_FINALIZE_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mr_finalize(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_VP_FLUSH_LEAF:
    {
        local_data->vmm_regs.rax = tdh_vp_flush(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_MNG_VPFLUSHDONE_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mng_vpflushdone(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_MNG_KEY_FREEID_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mng_key_freeid(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_MEM_PAGE_PROMOTE_LEAF:
    {
        page_info_api_input_t page_info;
        page_info.raw = local_data->vmm_regs.rcx;

        local_data->vmm_regs.rax = tdh_mem_page_promote(page_info, local_data->vmm_regs.rdx, leaf_opcode.version);
        break;
    }
    case TDH_PHYMEM_PAGE_RDMD_LEAF:
    {
        local_data->vmm_regs.rax = tdh_phymem_page_rdmd(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_MEM_SEPT_RD_LEAF:
    {
        page_info_api_input_t sept_page_info;
        sept_page_info.raw = local_data->vmm_regs.rcx;

        local_data->vmm_regs.rax = tdh_mem_sept_rd(sept_page_info, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_VP_RD_LEAF:
    {
        md_field_id_t field_code = {.raw = local_data->vmm_regs.rdx};
        local_data->vmm_regs.rax = tdh_vp_rd(local_data->vmm_regs.rcx, field_code, leaf_opcode.version);
        break;
    }
    case TDH_MNG_KEY_RECLAIMID_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mng_key_reclaimid(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_PHYMEM_PAGE_RECLAIM_LEAF:
    {
        local_data->vmm_regs.rax = tdh_phymem_page_reclaim(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_MEM_PAGE_REMOVE_LEAF:
    {
        page_info_api_input_t page_info;
        page_info.raw = local_data->vmm_regs.rcx;

        local_data->vmm_regs.rax = tdh_mem_page_remove(page_info, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_MEM_SEPT_REMOVE_LEAF:
    {
        page_info_api_input_t sept_page_info;
        sept_page_info.raw = local_data->vmm_regs.rcx;

        local_data->vmm_regs.rax = tdh_mem_sept_remove(sept_page_info, local_data->vmm_regs.rdx,
                                                       leaf_opcode.version);
        break;
    }
    case TDH_SYS_CONFIG_LEAF:
    {
        hkid_api_input_t global_private_hkid;
        global_private_hkid.raw = local_data->vmm_regs.r8;

        local_data->vmm_regs.rax = tdh_sys_config(local_data->vmm_regs.rcx,
                                                 local_data->vmm_regs.rdx,
                                                 global_private_hkid);
        break;
    }
    case TDH_SYS_KEY_CONFIG_LEAF:
    {
        local_data->vmm_regs.rax = tdh_sys_key_config();
        break;
    }
    case TDH_SYS_INFO_LEAF:
    {
        local_data->vmm_regs.rax = tdh_sys_info(local_data->vmm_regs.rcx,
                                               local_data->vmm_regs.rdx,
                                               local_data->vmm_regs.r8,
                                               local_data->vmm_regs.r9);
        break;
    }
    case TDH_SYS_INIT_LEAF:
    {
        local_data->vmm_regs.rax = tdh_sys_init();
        break;
    }
    case TDH_SYS_RD_LEAF:
    {
        md_field_id_t field_code = {.raw = local_data->vmm_regs.rdx};
        local_data->vmm_regs.rax = tdh_sys_rd(field_code);
        break;
    }
    case TDH_SYS_LP_INIT_LEAF:
    {
        local_data->vmm_regs.rax = tdh_sys_lp_init();
        break;
    }
    case TDH_SYS_TDMR_INIT_LEAF:
    {
        local_data->vmm_regs.rax = tdh_sys_tdmr_init(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_SYS_RDALL_LEAF:
    {
        md_field_id_t field_code = {.raw = local_data->vmm_regs.r8};
        local_data->vmm_regs.rax = tdh_sys_rdall(local_data->vmm_regs.rdx, field_code);
        break;
    }
    case TDH_SYS_LP_SHUTDOWN_LEAF:
    {
        local_data->vmm_regs.rax = tdh_sys_lp_shutdown();
        break;
    }
    case TDH_SYS_SHUTDOWN_LEAF:
    {
        local_data->vmm_regs.rax = tdh_sys_shutdown(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_SYS_UPDATE_LEAF:
    {
        local_data->vmm_regs.rax = tdh_sys_update();
        break;
    }
    case TDH_MEM_TRACK_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mem_track(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_MEM_RANGE_UNBLOCK_LEAF:
    {
        page_info_api_input_t page_info;
        page_info.raw = local_data->vmm_regs.rcx;

        local_data->vmm_regs.rax = tdh_mem_range_unblock(page_info, local_data->vmm_regs.rdx);
        break;
    }
    case TDH_PHYMEM_CACHE_WB_LEAF:
    {
        local_data->vmm_regs.rax = tdh_phymem_cache_wb(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_PHYMEM_PAGE_WBINVD_LEAF:
    {
        local_data->vmm_regs.rax = tdh_phymem_page_wbinvd(local_data->vmm_regs.rcx);
        break;
    }
    case TDH_VP_WR_LEAF:
    {
        md_field_id_t field_code = {.raw = local_data->vmm_regs.rdx};
        local_data->vmm_regs.rax = tdh_vp_wr(local_data->vmm_regs.rcx,
                                             field_code,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9);
        break;
    }
    case TDH_SERVTD_BIND_LEAF:
    {
        servtd_attributes_t servtd_attr = {.raw = local_data->vmm_regs.r10};
        local_data->vmm_regs.rax = tdh_servtd_bind(local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.rdx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9,
                                             servtd_attr);
        break;
    }
    case TDH_SERVTD_PREBIND_LEAF:
    {
        servtd_attributes_t servtd_attr = {.raw = local_data->vmm_regs.r10};
        local_data->vmm_regs.rax = tdh_servtd_prebind(local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.rdx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9,
                                             servtd_attr);
        break;
    }
    case TDH_EXPORT_ABORT_LEAF:
    {
        local_data->vmm_regs.rax = tdh_export_abort(
                                         local_data->vmm_regs.rcx,
                                         local_data->vmm_regs.r8,
                                         local_data->vmm_regs.r10);

        break;
    }
    case TDH_EXPORT_BLOCKW_LEAF:
    {
        local_data->vmm_regs.rax = tdh_export_blockw((gpa_list_info_t)local_data->vmm_regs.rcx,
                                                     local_data->vmm_regs.rdx);

        break;
    }
    case TDH_EXPORT_RESTORE_LEAF:
    {
        local_data->vmm_regs.rax = tdh_export_restore((gpa_list_info_t)local_data->vmm_regs.rcx,
                                                      local_data->vmm_regs.rdx);

        break;
    }
    case TDH_EXPORT_PAUSE_LEAF:
    {
        local_data->vmm_regs.rax = tdh_export_pause(local_data->vmm_regs.rcx);

        break;
    }
    case TDH_EXPORT_TRACK_LEAF:
    {
        local_data->vmm_regs.rax = tdh_export_track(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r10);

        break;
    }
    case TDH_EXPORT_STATE_IMMUTABLE_LEAF:
    {
        local_data->vmm_regs.rax = tdh_export_state_immutable(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9,
                                             local_data->vmm_regs.r10);

        break;
    }
    case TDH_EXPORT_STATE_TD_LEAF:
    {
        local_data->vmm_regs.rax = tdh_export_state_td(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9,
                                             local_data->vmm_regs.r10);

        break;
    }
    case TDH_EXPORT_STATE_VP_LEAF:
    {
        local_data->vmm_regs.rax = tdh_export_state_vp(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9,
                                             local_data->vmm_regs.r10);

        break;
    }
    case TDH_EXPORT_UNBLOCKW_LEAF:
    {
        local_data->vmm_regs.rax = tdh_export_unblockw(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.rdx);

        break;
    }
    case TDH_IMPORT_TRACK_LEAF:
    {
        local_data->vmm_regs.rax = tdh_import_track(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r10);

        break;
    }
    case TDH_IMPORT_STATE_IMMUTABLE_LEAF:
    {
        local_data->vmm_regs.rax = tdh_import_state_immutable(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9,
                                             local_data->vmm_regs.r10);

        break;
    }
    case TDH_IMPORT_STATE_TD_LEAF:
    {
        local_data->vmm_regs.rax = tdh_import_state_td(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9,
                                             local_data->vmm_regs.r10);

        break;
    }
    case TDH_IMPORT_STATE_VP_LEAF:
    {
        local_data->vmm_regs.rax = tdh_import_state_vp(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.r8,
                                             local_data->vmm_regs.r9,
                                             local_data->vmm_regs.r10);

        break;
    }
    case TDH_MIG_STREAM_CREATE_LEAF:
    {
        local_data->vmm_regs.rax = tdh_mig_stream_create(
                                             local_data->vmm_regs.rcx,
                                             local_data->vmm_regs.rdx);
        break;
    }
    case TDH_IMPORT_ABORT_LEAF:
    {
        local_data->vmm_regs.rax = tdh_import_abort(local_data->vmm_regs.rcx,
                                            local_data->vmm_regs.r8,
                                            local_data->vmm_regs.r10);

        break;
    }
    case TDH_IMPORT_COMMIT_LEAF:
    {
        local_data->vmm_regs.rax = tdh_import_commit(local_data->vmm_regs.rcx);

        break;
    }
    case TDH_IMPORT_END_LEAF:
    {
        local_data->vmm_regs.rax = tdh_import_end(local_data->vmm_regs.rcx);

        break;
    }
    case TDH_EXPORT_MEM_LEAF:
    {
        gpa_list_info_t gpa_list_info = { .raw = local_data->vmm_regs.rcx};
        local_data->vmm_regs.rax = tdh_export_mem(
                                            gpa_list_info,
                                            local_data->vmm_regs.rdx,
                                            local_data->vmm_regs.r8,
                                            local_data->vmm_regs.r9,
                                            local_data->vmm_regs.r10,
                                            local_data->vmm_regs.r11,
                                            local_data->vmm_regs.r12);
        break;
    }
    case TDH_IMPORT_MEM_LEAF:
    {
        gpa_list_info_t gpa_list_info = { .raw = local_data->vmm_regs.rcx};
        local_data->vmm_regs.rax = tdh_import_mem(
                                            gpa_list_info,
                                            local_data->vmm_regs.rdx,
                                            local_data->vmm_regs.r8,
                                            local_data->vmm_regs.r9,
                                            local_data->vmm_regs.r10,
                                            local_data->vmm_regs.r11,
                                            local_data->vmm_regs.r12,
                                            local_data->vmm_regs.r13);
        break;
    }
    default:
    {
        TDX_ERROR("tdx_vmm_dispatcher - TDX_OPERAND_INVALID - invalid leaf = %d\n", leaf_opcode);
        local_data->vmm_regs.rax = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RAX);
        break;
    }
}


    // SOPHIA: TDX instruction execution END
    // AHMAD: POSTCONDITION - The SEAMCALL leaf operation is fully excecuted.
    // AHMAD: POSTCONDITION - The result of the operation (success or failure) is stored in the RAX register (hardware register) for further handling.

    // SOPHIA: TDX instruction close START
    // AHMAD: PRECONDITION - A valid result or error code has been stored in RAX (hardware register)

    tdx_sanity_check(local_data->vmm_regs.rax != UNINITIALIZE_ERROR, SCEC_VMM_DISPATCHER_SOURCE, 1);
    // SOPHIA: check to make sure the result is valid (not an uninitialized error)
    IF_RARE (local_data->reset_avx_state)
    {
        // Current IPP crypto lib uses SSE state only (XMM's), so we only clear them
        clear_xmms();
        local_data->reset_avx_state = false;
    }


    // SOPHIA: TDX instruction close END
    // AHMAD: POSTCONDITION - A valid result or error code has been stored in RAX (hardware register)
EXIT:
    // No return after calling the post dispatching operations
    // Eventually call SEAMRET
    // SOPHIA: Perform cleanup after the SEAMCALL instruction has been complete
    tdx_vmm_post_dispatching();
}


void tdx_vmm_post_dispatching(void)
{
    // AHMAD: PRECONDITION - TDX instruction has fully completed and a valid result or error code has been stored in RAX (hardware register)
    advance_guest_rip();
    // AHMAD: RIP_new = RIP_current + VM_EXIT_INSTRUCTION_LENGTH
    // SOPHIA: add VM-Exit Instruction Length VMCS field offset to Guest RIP of current active VMCS

    tdx_module_local_t* local_data_ptr = get_local_data();

    // SOPHIA: write to IA32_SPEC_CTRL_MSR_ADDR with new ia32_spec_ctrl from the local_data_ptr if its different from TDX_MODULE_IA32_SPEC_CTRL
    // Restore IA32_SPEC_CTRL
    wrmsr_opt(IA32_SPEC_CTRL_MSR_ADDR, local_data_ptr->vmm_non_extended_state.ia32_spec_ctrl,
                                       TDX_MODULE_IA32_SPEC_CTRL);
    // AHMAD: SPEC_CTRL_MSR = local_data_ptr->vmm_non_extended_state.ia32_spec_ctrl if needed

    // SOPHIA: restore the LAM state if it was saved and disabled
    // If simplified LAM was saved & disabled, restore its state
    if (local_data_ptr->vmm_non_extended_state.ia32_lam_enable != 0)
    {
        ia32_wrmsr(IA32_LAM_ENABLE_MSR_ADDR, local_data_ptr->vmm_non_extended_state.ia32_lam_enable);
    }
    // AHMAD: LAM_ENABLE_MSR = local_data_ptr->vmm_non_extended_state.ia32_lam_enable if enabled

    // SOPHIA: mark the current LP as being free
    mark_lp_as_free();
    // AHMAD: get_local_data()->lp_is_busy = false

    // Check that we have no mapped keyholes left
    tdx_sanity_check(local_data_ptr->keyhole_state.total_ref_count == 0, SCEC_KEYHOLE_MANAGER_SOURCE, 20);

    TDX_LOG("tdx_vmm_post_dispatching - preparing to do SEAMRET\n");

    // SOPHIA: Restore GPRs and SEAMRET back to the VMM
    tdx_seamret_to_vmm(); // Restore GPRs and SEAMRET
    // AHMAD: VMM GPRs = LP local data

    // AHMAD: POSTCONDITION - RIP (hardware register) is advanced to continue execution
    // AHMAD: POSTCONDITION - original saved hardware state is restored
    // AHMAD: POSTCONDITION - keyholes in local data are cleaned up
    // AHMAD: POSTCONDITION - control is handed back to VMM and GPRs are restored from the LP local data

    // Shouldn't reach here:
    tdx_sanity_check(0, SCEC_VMM_DISPATCHER_SOURCE, 0);
}
