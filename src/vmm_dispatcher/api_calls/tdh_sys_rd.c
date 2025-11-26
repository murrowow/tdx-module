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
 * @file tdh_sys_rd
 * @brief TDH_SYS_RD API handler
 */
#include "include/tdx_vmm_api_handlers.h"
#include "include/tdx_basic_defs.h"
#include "include/auto_gen/tdx_error_codes_defs.h"
#include "src/common/x86_defs/x86_defs.h"
#include "src/common/x86_defs/vmcs_defs.h"
#include "src/common/data_structures/tdx_local_data.h"
#include "src/common/memory_handlers/keyhole_manager.h"
#include "src/common/helpers/helpers.h"
#include "src/common/accessors/data_accessors.h"
#include "include/auto_gen/global_sys_fields_lookup.h"
#include "src/common/metadata_handlers/metadata_generic.h"

#include "driver/driver.h"
api_error_type tdh_sys_rd(md_field_id_t field_id)
{
    #ifdef SOURCE
    tdx_module_local_t*     local_data_ptr = get_local_data();
    #else 
    tdx_module_local_t*     local_data_ptr = &local_data;
    #endif //SOURCE
    uint64_t                rd_value = 0;           // Data read from field

    md_access_qualifier_t   access_qual = { .raw = 0 };
    md_context_ptrs_t       md_ctx;
    api_error_type          retval = TDX_SUCCESS;
    // Default output register operands
    local_data_ptr->vmm_regs.rdx = MD_FIELD_ID_NA;
    local_data_ptr->vmm_regs.r8  = 0;

    // Check that LP-scope initialization has been done.
    // This also implies that TDH_SYS_INIT has been done.
    #ifdef SOURCE
    if (!local_data_ptr->lp_is_init)
    {
        retval = TDX_SYS_LP_INIT_NOT_DONE;
        TDX_ERROR("TDSYSINITLP not done!\n");
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(local_data_ptr->lp_is_init);
    #endif //MODULAR_PROOF

    #ifdef FLOW_PROOF
    if (!local_data_ptr->lp_is_init)
    {
        __CPROVER_assert(local_data_ptr->lp_is_init, "TDSYSINITLP not done");
        retval = TDX_SYS_LP_INIT_NOT_DONE;
        TDX_ERROR("TDSYSINITLP not done!\n");
        goto EXIT;
    }
    #endif // FLOW_PROOF
    // Set the proper context code
    field_id.context_code = MD_CTX_SYS;

    md_ctx.tdr_ptr = NULL;
    md_ctx.tdcs_ptr = NULL;
    md_ctx.tdvps_ptr = NULL;

    #ifdef SOURCE
    // For read, a null field ID means return the first field ID in context
    if (is_null_field_id(field_id))
    {
        local_data_ptr->vmm_regs.rdx =
                (md_get_next_element_in_context(MD_CTX_SYS, field_id, md_ctx, MD_HOST_RD, access_qual)).raw;

        retval = TDX_METADATA_FIRST_FIELD_ID_IN_CONTEXT;
        goto EXIT;
    }
    #endif //SOURCE

    // SOPHIA: For now assume abstract away since nothing is being read
    #ifdef MODULAR_PROOF
        __CPROVER_assume(!is_null_field_id(field_id));
    #endif

    #ifdef FLOW_PROOF
        if (is_null_field_id(field_id))
        {
            retval = TDX_METADATA_FIRST_FIELD_ID_IN_CONTEXT;
            goto EXIT;
        }
    #endif //FLOW_PROOF

    #ifdef SOURCE
    retval = md_check_as_single_element_id(field_id);
    if (retval != TDX_SUCCESS)
    {
        TDX_ERROR("Request field id doesn't match single element = %llx\n", field_id.raw);
        goto EXIT;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(md_check_as_single_element_id(field_id) == TDX_SUCCESS);
    #endif //MODULAR_PROOF

    #ifdef FLOW_PROOF
        retval = md_check_as_single_element_id(field_id);
        if (retval != TDX_SUCCESS)
        {
            __CPROVER_assert(retval == TDX_SUCCESS, "field id doesn't match single element");
        TDX_ERROR("Request field id doesn't match single element = %llx\n", field_id.raw);
        goto EXIT;
        }
    #endif //FLOW_PROOF

    retval = md_read_element(MD_CTX_SYS, field_id, MD_HOST_RD, access_qual, md_ctx, &rd_value);

    local_data_ptr->vmm_regs.r8 = rd_value;

    #ifdef SOURCE
    if (retval == TDX_SUCCESS)
    {
        local_data_ptr->vmm_regs.rdx =
                (md_get_next_element_in_context(MD_CTX_SYS, field_id, md_ctx, MD_HOST_RD, access_qual)).raw;
    }
    #endif // SOURCE

    #ifdef FLOW_PROOF
    if (retval == TDX_SUCCESS)
    {
        uint8_t temp;
        __CPROVER_havoc_slice(&(local_data_ptr->vmm_regs.rdx), sizeof(uint64_t));
    }
    #endif // FLOW_PROOF

EXIT:

    return retval;
}
