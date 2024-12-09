#include "helpers/cbmc_helper.h"
#include "src/common/helpers/helpers.h"

pamt_entry_t * get_pamt_entry(uint64_t target_tdr_pa, hkid_api_input_t hkid_info)
{
    __CPROVER_assume(target_tdr_pa != 0);
    __CPROVER_assume(hkid_info.hkid != 0);
    __CPROVER_assume(hkid_info.reserved == 0);

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

    kot_locked_flag = return_true(); 
    return_val = check_lock_and_map_explicit_tdr(tdr_pa,
                                                 OPERAND_ID_RCX,
                                                 TDX_RANGE_RW,
                                                 TDX_LOCK_EXCLUSIVE,
                                                 PT_NDA,
                                                 &tdr_pamt_block,
                                                 &tdr_pamt_entry_ptr,
                                                 &tdr_locked_flag,
                                                 &tdr_ptr);

    return tdr_pamt_entry_ptr;
}
