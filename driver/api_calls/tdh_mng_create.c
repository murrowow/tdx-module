/**
 * @file tdh_mng_create
 * @brief TDHMNGCREATE API handler
 */
#include "include/tdx_vmm_api_handlers.h"
#include "include/tdx_basic_defs.h"
#include "include/auto_gen/tdx_error_codes_defs.h"
#include "src/common/x86_defs/x86_defs.h"
#include "src/common/data_structures/td_control_structures.h"
#include "src/common/memory_handlers/keyhole_manager.h"
#include "src/common/memory_handlers/pamt_manager.h"
#include "src/common/helpers/helpers.h"
#include "src/common/accessors/data_accessors.h"
#include "src/common/accessors/ia32_accessors.h"

#include "driver/driver.h"

// Not sure if this is fully correct
api_error_type tdh_mng_create(uint64_t target_tdr_pa, hkid_api_input_t hkid_info)
{
    // TDR related variables
    pa_t                  tdr_pa;                   // TDR physical address
    tdr_t               * tdr_ptr;                  // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;           // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;       // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;  // Indicate TDR is locked

    uint16_t              td_hkid;

    api_error_type        return_val = UNINITIALIZE_ERROR;

    tdr_pa.raw = target_tdr_pa;
    td_hkid = hkid_info.hkid;

    tdr_pamt_entry_ptr = &(tables[td_hkid & hkid_mask].pamt_entry); 
    __CPROVER_assert((td_hkid >= global_data.private_hkid_min) && (td_hkid <= global_data.private_hkid_max), "hkid within valid bounds");
    __CPROVER_assert(tdr_pamt_entry_ptr->pt == PT_NDA, "the pamt table is PT_NDA"); 
    __CPROVER_assert((global_data.kot.lock.raw == SHAREX_FREE), "exclusive access to the lock");
    __CPROVER_assert(global_data.kot.entries[td_hkid & hkid_mask].state == KOT_STATE_HKID_FREE, "HKID in KOT has the correct value in the table"); 

EXIT:

    __CPROVER_assume(tables[td_hkid & hkid_mask].pamt_entry.pt == PT_TDR, "hardware pamt was set correctly");
    __CPROVER_assume(global_data.kot.entries[td_hkid & hkid_mask].state ==  KOT_STATE_HKID_ASSIGNED, "hardware pamt was set correctly");
    __CPROVER_assume(tables[td_hkid & hkid_mask].tdr_mem == 0, "memory at tdr correctly zeroed out");
    
    return_val = TDX_SUCESS; 
    return return_val;
    
}
