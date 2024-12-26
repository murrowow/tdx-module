#include "helpers/cbmc_helper.h"
#include "src/common/helpers/helpers.h"

pamt_entry_t * get_pamt_entry(uint64_t target_tdr_pa, hkid_api_input_t hkid_info)
{
    tdx_module_global_t * global_data = get_global_data();

    // TDR related variables
    pa_t                  tdr_pa;                   // TDR physical address
    tdr_t               * tdr_ptr;                  // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;           // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;       // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;  // Indicate TDR is locked

    api_error_type        return_val = UNINITIALIZE_ERROR;

    tdr_pa.raw = target_tdr_pa;
    check_lock_and_map_explicit_tdr(tdr_pa, //input target_tdr_pa
                                OPERAND_ID_RCX, //constant
                                TDX_RANGE_RW, //constant
                                TDX_LOCK_EXCLUSIVE, //constant
                                PT_NDA, //constant 
                                &tdr_pamt_block,
                                &tdr_pamt_entry_ptr,
                                &tdr_locked_flag,
                                &tdr_ptr);

    page_size_t leaf_size = PT_4KB;
    pamt_get_block(tdr_pa, &tdr_pamt_block); 

    api_error_code_e errc = pamt_walk(tdr_pa, tdr_pamt_block, TDX_LOCK_EXCLUSIVE, &leaf_size,
                                      true, false, &tdr_pamt_entry_ptr);
    return tdr_pamt_entry_ptr;
}