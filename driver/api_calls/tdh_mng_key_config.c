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
 
     // TDR related variables
     pa_t                  tdr_pa;                    // TDR physical address
     tdr_t               * tdr_ptr;                   // Pointer to the TDR page (linear address)
     pamt_block_t          tdr_pamt_block;            // TDR PAMT block
     pamt_entry_t        * tdr_pamt_entry_ptr;        // Pointer to the TDR PAMT entry
     bool_t                tdr_locked_flag = false;   // Indicate TDR is locked
 
     api_error_type        return_val = UNINITIALIZE_ERROR;
 
     __CPROVER_assert(tables[target_tdr_pa & hkid_mask].pamt_entry.pt == PT_TDR);
     tdr_pa.raw = target_tdr_pa;
     tdr_pamt_entry_ptr = &(tables[tdr_pa.raw & hkid_mask].pamt_entry);
     __CPROVER_assert(tdr_pamt_entry_ptr->pt == PT_TDR);
     __CPROVER_assert(!tables[tdr_pa.raw & hkid_mask].tdr_table.management_fields.fatal);
     __CPROVER_assert(tables[tdr_pa.raw & hkid_mask].tdr_table.management_fields.lifecycle_state == TD_HKID_ASSIGNED);
     __CPROVER_assert(!(tables[tdr_pa.raw & hkid_mask].tdr_table.key_management_fields.pkg_config_bitmap & (BIT(local_data.lp_info.pkg))));
 
    // tables[tdr_pa.raw & hkid_mask].tdr_table.key_management_fields.pkg_config_bitmap |= (BIT(local_data.lp_info.pkg));
 
    //  if (tables[tdr_pa.raw & hkid_mask].tdr_table.key_management_fields.pkg_config_bitmap == (uint64_t)global_data.pkg_config_bitmap)
    //  {
    //      tables[tdr_pa.raw & hkid_mask].tdr_table.management_fields.lifecycle_state = (uint8_t)TD_KEYS_CONFIGURED;
    //  }
 

     // "current package configured"
     __CPROVER_assume(tables[tdr_pa.raw & hkid_mask].tdr_table.key_management_fields.pkg_config_bitmap & (BIT(local_data.lp_info.pkg)));

     // "correctly adjusts lifecycle state if keys are configured on all packages"
     __CPROVER_assume(((tables[tdr_pa.raw & hkid_mask].tdr_table.key_management_fields.pkg_config_bitmap == global_data.pkg_config_bitmap) && (tables[tdr_pa.raw & hkid_mask].tdr_table.management_fields.lifecycle_state == (uint8_t)TD_KEYS_CONFIGURED))
                     || (!(tables[tdr_pa.raw & hkid_mask].tdr_table.key_management_fields.pkg_config_bitmap == global_data.pkg_config_bitmap) && !(tables[tdr_pa.raw & hkid_mask].tdr_table.management_fields.lifecycle_state == (uint8_t)TD_KEYS_CONFIGURED)));
 
 EXIT:
 
     return return_val;
 }
 