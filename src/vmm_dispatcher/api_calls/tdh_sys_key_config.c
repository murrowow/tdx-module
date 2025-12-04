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
 * @file tdh_sys_key_config.c
 * @brief TDHSYSKEYCONFIG API handler
 */
#include "include/tdx_api_defs.h"
#include "include/tdx_basic_defs.h"
#include "include/tdx_basic_types.h"
#include "include/tdx_vmm_api_handlers.h"
#include "include/auto_gen/tdx_error_codes_defs.h"
#include "src/common/x86_defs/mktme.h"
#include "src/common/accessors/data_accessors.h"
#include "src/common/helpers/helpers.h"

#include "driver/driver.h"

api_error_type tdh_sys_key_config(void)
{
    bool_t tmp_global_lock_acquired = false;
    #ifdef SOURCE
    tdx_module_global_t* tdx_global_data_ptr = get_global_data();
    tdx_module_local_t* tdx_local_data_ptr = get_local_data();
    #else 
    tdx_module_global_t* tdx_global_data_ptr = &global_data;
    tdx_module_local_t* tdx_local_data_ptr = &local_data;
    #endif // SOURCE
    api_error_type retval = TDX_SYS_BUSY;

    #ifdef SOURCE
    if (acquire_sharex_lock_ex(&tdx_global_data_ptr->global_lock) != LOCK_RET_SUCCESS)
    {
        TDX_ERROR("Failed to acquire global lock for LP\n");
        retval = TDX_SYS_BUSY;
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(tdx_global_data_ptr->global_lock.raw == SHAREX_FREE); 
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
    if (tdx_global_data_ptr->global_lock.raw != SHAREX_FREE)
    {
        __CPROVER_assert(tdx_global_data_ptr->global_lock.raw == SHAREX_FREE, "global lock haas not been obtained"); 
        TDX_ERROR("Failed to acquire global lock for LP\n");
        retval = TDX_SYS_BUSY;
        goto EXIT;
    }
    #endif // FLOW_PROOF
    tmp_global_lock_acquired = true;

    // Verify that TDHSYSCONFIG has completed successfully (PL.SYS_STATE is SYSCONFIG_DONE)
    #ifdef SOURCE
    if (tdx_global_data_ptr->global_state.sys_state != SYSCONFIG_DONE)
    {
        TDX_ERROR("Wrong sys_init state: %d\n", tdx_global_data_ptr->global_state.sys_state);
        {
            retval = TDX_SYS_KEY_CONFIG_NOT_PENDING;
            goto EXIT;
        }
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(tdx_global_data_ptr->global_state.sys_state == SYSCONFIG_DONE); 
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
        if (tdx_global_data_ptr->global_state.sys_state != SYSCONFIG_DONE)
        {
        TDX_ERROR("Wrong sys_init state: %d\n", tdx_global_data_ptr->global_state.sys_state);
        {
            __CPROVER_assert(tdx_global_data_ptr->global_state.sys_state == SYSCONFIG_DONE, "global state is incorrect"); 
            retval = TDX_SYS_KEY_CONFIG_NOT_PENDING;
            goto EXIT;
        }
        }
    #endif // FLOW_PROOF

    #ifdef SOURCE
    // Use an atomic operation (e.g., LOCK BTS) on PL.PKG_CONFIG_BITMAP to verify
    // the package has not been configured and mark it as configured.
    if (_lock_bts_32b(&tdx_global_data_ptr->pkg_config_bitmap, tdx_local_data_ptr->lp_info.pkg))
    {
        TDX_ERROR("Package %d already configured its key\n", tdx_local_data_ptr->lp_info.pkg);
        retval = TDX_KEY_CONFIGURED;
        goto EXIT;
    }
    #endif // SOURCE
    
    #ifdef MODULAR_PROOF
        uint32_t mask = 1u << tdx_local_data_ptr->lp_info.pkg;
        __CPROVER_assume((tdx_global_data_ptr->pkg_config_bitmap & mask) == 0);
        tdx_global_data_ptr->pkg_config_bitmap &= ~mask;
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
        uint32_t mask = 1u << tdx_local_data_ptr->lp_info.pkg;
        tdx_global_data_ptr->pkg_config_bitmap &= ~mask;
        __CPROVER_printf("SOPHIA: %d", tdx_global_data_ptr->pkg_config_bitmap);
        __CPROVER_assert((tdx_global_data_ptr->pkg_config_bitmap & mask) == 0, "Package has already configured its key");
    #endif // FLOW_PROOF

    // SOPHIA: for now assume that mktme configuring a key always succeeds since it generates a key for the package
    #ifdef SOURCE
    // Execute PCONFIG to configure the TDX-SEAM global private HKID on the package, with a CPU-generated random key.
    // PCONFIG may fail due to and entropy error or a device busy error.
    // In this case, the VMM should retry TDHSYSKEYCONFIG.
    retval = program_mktme_keys(tdx_global_data_ptr->hkid);
    if (retval != TDX_SUCCESS)
    {
        TDX_ERROR("Failed to program MKTME keys for this package\n");
        // Clear the package configured bit
        _lock_btr_32b(&tdx_global_data_ptr->pkg_config_bitmap, tdx_local_data_ptr->lp_info.pkg);
        goto EXIT;
    }
    #endif // SOURCE

    // Update the number of initialized packages. If this is the last one, update the system state.
    #ifdef SOURCE
    #else 
        // SOPHIA: need this temp variable for assertions
        uint32_t old_num_pkgs = tdx_global_data_ptr->num_of_init_pkgs;
    #endif // SOURCE

    #ifdef FLOW_PROOF
    #else 
    tdx_global_data_ptr->num_of_init_pkgs++;

    if (tdx_global_data_ptr->num_of_init_pkgs == tdx_global_data_ptr->num_of_pkgs)
    {
        tdx_global_data_ptr->global_state.sys_state = SYS_READY;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        if (tdx_global_data_ptr->num_of_init_pkgs == tdx_global_data_ptr->num_of_pkgs)
        {
            __CPROVER_assert(tdx_global_data_ptr->global_state.sys_state == SYS_READY, "sys_ready");
        }
        __CPROVER_assert(tdx_global_data_ptr->num_of_init_pkgs == (old_num_pkgs + 1), "number of configured packages incremented by 1");
    #endif // MODULAR_PROOF
    retval = TDX_SUCCESS;

EXIT:
    #ifdef SOURCE
    if (tmp_global_lock_acquired)
    {
        release_sharex_lock_ex(&tdx_global_data_ptr->global_lock);
    }
    #endif // SOURCE

    #ifdef FLOW_PROOF
    if (tdx_global_data_ptr->num_of_init_pkgs == tdx_global_data_ptr->num_of_pkgs)
    {
        __CPROVER_assume(tdx_global_data_ptr->global_state.sys_state == SYS_READY);
    }
    #endif // FLOW_PROOF
    return retval;
}

