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
 * @file tdh_mng_key_config
 * @brief TDHKEYCONFIG API handler
 */
#include "include/tdx_vmm_api_handlers.h"
#include "include/tdx_basic_defs.h"
#include "include/auto_gen/tdx_error_codes_defs.h"
#include "src/common/x86_defs/x86_defs.h"
#include "src/common/x86_defs/mktme.h"
#include "src/common/data_structures/td_control_structures.h"
#include "src/common/memory_handlers/keyhole_manager.h"
#include "src/common/memory_handlers/pamt_manager.h"
#include "src/common/helpers/helpers.h"
#include "src/common/accessors/data_accessors.h"
#include "src/common/accessors/ia32_accessors.h"

#include "driver/driver.h"

api_error_type tdh_mng_key_config(uint64_t target_tdr_pa)
{
    // // global data
    // tdx_module_global_t * global_data = get_global_data();

    // // Local data
    // tdx_module_local_t  * local_data = get_local_data();

    // TDR related variables
    pa_t                  tdr_pa;                    // TDR physical address
    tdr_t               * tdr_ptr;                   // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;            // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;        // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;   // Indicate TDR is locked

    api_error_type        return_val = UNINITIALIZE_ERROR;

    driver_main(1);

    __CPROVER_assume(tables[target_tdr_pa & HKID_MASK].pamt_entry.pt == PT_TDR);
    tdr_pa.raw = target_tdr_pa;
   
    // tdr_pa.raw = target_tdr_pa;

    // Check,lock and map the TDR page
    // return_val = check_lock_and_map_explicit_tdr(tdr_pa,
    //                                              OPERAND_ID_RCX,
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

    // //Verify TDR is not in fatal state
    // if (tdr_ptr->management_fields.fatal)
    // {
    //     TDX_ERROR("TDR is in fatal state.\n");
    //     return_val = TDX_TD_FATAL;
    //     goto EXIT;
    // }
    __CPROVER_assume(!tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.fatal);

    // // Verify LIFECYCLE_STATE
    // if (tdr_ptr->management_fields.lifecycle_state != TD_HKID_ASSIGNED)
    // {
    //     TDX_ERROR("TDR HKID state is not assigned. lifecycle_state = %d\n", tdr_ptr->management_fields.lifecycle_state);
    //     return_val = TDX_LIFECYCLE_STATE_INCORRECT;
    //     goto EXIT;
    // }
    __CPROVER_assume(tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.lifecycle_state == TD_HKID_ASSIGNED);

    // // Check if the key is already configured
    // if (tdr_ptr->key_management_fields.pkg_config_bitmap & (BIT(local_data->lp_info.pkg)))
    // {
    //     TDX_ERROR("Key is already configured for this package\n");
    //     return_val = TDX_KEY_CONFIGURED;
    //     goto EXIT;
    // }
    __CPROVER_assume(!(tables[tdr_pa.raw & HKID_MASK].tdr_table.key_management_fields.pkg_config_bitmap & (BIT(local_data.lp_info.pkg))));

    /** Try to configure the key on the package using a CPU-generated key.
     * This operation acquires an exclusive lock on KET (encryption engine tables)
     * and may fail if a concurrent TDHKEYCONFIG or PCONFIG is in progress.
     * Key generation may fail.
     */
    // return_val = program_mktme_keys(tdr_ptr->key_management_fields.hkid);
    // if (return_val != TDX_SUCCESS)
    // {
    //     TDX_ERROR("Failed to program MKTME keys for this TD\n");
    //     goto EXIT;
    // }


    // ALL_CHECKS_PASSED:  The instruction is guaranteed to succeed

    // Set the configuration indication for the current package
    // tdr_ptr->key_management_fields.pkg_config_bitmap |= BIT(local_data->lp_info.pkg);
    tables[tdr_pa.raw & HKID_MASK].tdr_table.key_management_fields.pkg_config_bitmap |= (BIT(local_data.lp_info.pkg));

    // // Check whether the TD keys have been configured on all packages
    // if (tdr_ptr->key_management_fields.pkg_config_bitmap == (uint64_t)global_data->pkg_config_bitmap)
    // {
    //     tdr_ptr->management_fields.lifecycle_state = (uint8_t)TD_KEYS_CONFIGURED;
    // }

    if (tables[tdr_pa.raw & HKID_MASK].tdr_table.key_management_fields.pkg_config_bitmap == (uint64_t)global_data.pkg_config_bitmap)
    {
        tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.lifecycle_state = (uint8_t)TD_KEYS_CONFIGURED;
    }

    __CPROVER_assert(false, "false");
    __CPROVER_assert(tables[tdr_pa.raw & HKID_MASK].tdr_table.key_management_fields.pkg_config_bitmap & (BIT(local_data.lp_info.pkg)), "current package configured");
    __CPROVER_assert(((tables[tdr_pa.raw & HKID_MASK].tdr_table.key_management_fields.pkg_config_bitmap == global_data.pkg_config_bitmap) && (tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.lifecycle_state == (uint8_t)TD_KEYS_CONFIGURED))
                    || (!(tables[tdr_pa.raw & HKID_MASK].tdr_table.key_management_fields.pkg_config_bitmap == global_data.pkg_config_bitmap) && !(tables[tdr_pa.raw & HKID_MASK].tdr_table.management_fields.lifecycle_state == (uint8_t)TD_KEYS_CONFIGURED)),
                    "correctly adjusts lifecycle state if keys are configured on all packages");

EXIT:
    // // Release all acquired locks and free keyhole mappings
    // if (tdr_locked_flag)
    // {
    //     pamt_unwalk(tdr_pa, tdr_pamt_block, tdr_pamt_entry_ptr, TDX_LOCK_EXCLUSIVE, PT_4KB);
    //     free_la(tdr_ptr);
    // }

    return return_val;
}
