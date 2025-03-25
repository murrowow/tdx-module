#include "driver/driver.h"
#include "driver/flows/flows.h"
#include "stdlib.h"

#ifdef SOURCE
#else 
void driver_main() {
    __CPROVER_havoc_object(&global_data); // .private_hkid_min and .private_hkid_max
    __CPROVER_assume((global_data.private_hkid_min == 0x00000000));  //&& (global_data.private_hkid_min < (0xFFFFFFFF - (HKID_SIZE)))); 
    __CPROVER_assume((global_data.private_hkid_max == global_data.private_hkid_min + (HKID_SIZE))); //&& (global_data.private_hkid_max < 0xFFFFFFFF)); 
    __CPROVER_assume(global_data.hkid_start_bit == (32 - (HKID_SIZE))); 
    __CPROVER_assume(global_data.hkid_mask == HKID_MASK);
    __CPROVER_havoc_object(&tables); 
    
    // __CPROVER_printf("SOPHIA: size of pa: %d hkid_start_bit %d hkid_mask: %X hkid_min: %X hkid_max: %X", 
    //     52, global_data.hkid_start_bit, global_data.hkid_mask, global_data.private_hkid_min, global_data.private_hkid_max);
    #ifdef SETUP
        __CPROVER_havoc_object(&tables);
        //init the kot table
        for (int i = 0; i < HKID_SIZE; i++) {
            __CPROVER_assume(global_data.kot.entries[i].state == KOT_STATE_HKID_FREE);
            __CPROVER_assume(tables[i].tdr_table.management_fields.fatal == false); 
            __CPROVER_assume(tables[i].pamt_entry.pt == PT_NDA); 
            __CPROVER_assume(tables[i].tdr_table.key_management_fields.pkg_config_bitmap == 0); 
            __CPROVER_assume(tables[i].tdr_table.management_fields.num_tdcx < MAX_NUM_TDCS_PAGES); 
            __CPROVER_assume(tables[i].tdcx_pamt_entry.pt == PT_NDA);
            __CPROVER_assume((tables[i].tdcx_table.management_fields.op_state >= 0) && (tables[i].tdcx_table.management_fields.op_state <= 10));

            // SOPHIA: highkey have no idea what this does for add_cx
            __CPROVER_assume(seamcall_state_lookup[TDH_MNG_ADDCX_LEAF][tables[i].tdcx_table.management_fields.op_state]); 
        }

    #endif // SETUP
    
    #ifdef KEY_CONFIG_SETUP
        __CPROVER_havoc_object(&local_data.lp_info.pkg);
        __CPROVER_assume(local_data.lp_info.pkg >= 0 && local_data.lp_info.pkg < HKID_SIZE);
        __CPROVER_havoc_object(&global_data.pkg_config_bitmap);
        __CPROVER_assume(global_data.pkg_config_bitmap & BIT(local_data.lp_info.pkg) != 0);

        for (int i = 0; i < HKID_SIZE; i++) {
            __CPROVER_havoc_object(&tables[i]);
        }
        // Ensure at least one element has pamt_entry.pt set to PT_TDR
        bool_t found = false;
        for (int i = 0; i < HKID_SIZE; i++) {
            if (tables[i].pamt_entry.pt == PT_TDR) {
                found = true;
                break;
            }
        }
        __CPROVER_assume(found);

    #endif // KEY_CONFIG_SETUP

    #ifdef ADD_CX_SETUP
        for (int i = 0; i < HKID_SIZE; i++) {
            __CPROVER_havoc_object(&tables[i].pamt_entry);
            __CPROVER_havoc_object(&tables[i].tdr_table.management_fields.lifecycle_state);
            __CPROVER_havoc_object(&tables[i].tdcx_table.management_fields.op_state);
        }

        for(int i = 0; i < HKID_SIZE; i++)
            __CPROVER_assume(tables[i].tdcx_table.management_fields.op_state >= 0 && tables[i].tdcx_table.management_fields.op_state <= 10);
        
        // Ensure at least one element is as we need it
        bool_t found = false;
        for (int i = 0; i < HKID_SIZE; i++) {
            if (tables[i].pamt_entry.pt == PT_TDR && tables[i].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED && tables[i].tdcx_table.management_fields.op_state == OP_STATE_UNINITIALIZED) {
                found = true;
                break;
            }
        }
        __CPROVER_assume(found);

    #endif // ADD_CX_SETUP

    #ifdef INIT_SETUP

    // Ensure at least one element is as we need it
    bool_t found = false;
    for (int i = 0; i < HKID_SIZE; i++) {
        if (tables[i].pamt_entry.pt == PT_TDR 
            && tables[i].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED 
            && tables[i].tdr_table.management_fields.fatal == false);  {
            found = true;
            break;
        }
    }
    // TDR page metadata in the PAMT must be correct (PT must be PT_TDR)
    // TD is not in a fatal state
    // TD keys are configured on the hardware
    __CPROVER_assume(found);
    #endif // INIT_SETUP
    TD_setup();
}
#endif // not SOURCE