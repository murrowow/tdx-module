#include "driver/driver.h"
#include "driver/flows/flows.h"
#include "stdlib.h"

#ifdef SOURCE
#else 
void driver_main() {
    #ifdef SETUP
        // init global data
        __CPROVER_havoc_object(&global_data); // .private_hkid_min and .private_hkid_max
        __CPROVER_assume((global_data.private_hkid_min > 0x00000000) && (global_data.private_hkid_min < 0xFFFFFFFF)); 
        __CPROVER_assume((global_data.private_hkid_max > global_data.private_hkid_min) && (global_data.private_hkid_max < 0xFFFFFFFF)); 
        

        __CPROVER_havoc_object(&tables);
        //init the kot table
        for (int i = 0; i < HKID_SIZE; i++) {
            __CPROVER_assume(global_data.kot.entries[i].state == KOT_STATE_HKID_FREE);
            __CPROVER_assume(tables[i].tdr_table.management_fields.fatal == false); 
            __CPROVER_assume(tables[i].pamt_entry.pt == PT_NDA); 
            __CPROVER_assume(tables[i].tdr_table.key_management_fields.pkg_config_bitmap == 0); 
        }
        __CPROVER_printf("SOPHIA in driver fatal: %d", tables[0].tdr_table.management_fields.fatal);
        //__CPROVER_printf(("seamrr_base: %llx seamrr_top: %llx", global_data.private_hkid_min, global_data.private_hkid_max));
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
            __CPROVER_assume(tables[i].tdcx_table.management_fields.op_state >= 0 && tables[i].tdcx_table.management_fields.op_state <= 10);
            __CPROVER_printf("INDEX: %d      OPSTATE VERY: %d\n", i, tables[(i)].tdcx_table.management_fields.op_state);
        }
        // Ensure at least one element has pamt_entry.pt set to PT_TDR
        bool_t found = false;
        for (int i = 0; i < HKID_SIZE; i++) {
            if (tables[i].pamt_entry.pt == PT_TDR && tables[i].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED && tables[i].tdcx_table.management_fields.op_state == OP_STATE_UNINITIALIZED) {
                found = true;
                break;
            }
        }
        __CPROVER_assume(found);

    #endif // ADD_CX_SETUP

    TD_setup();
}
#endif // not SOURCE