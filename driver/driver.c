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
    __CPROVER_havoc_object(&vmcs);
    
    uint16_t index = 0; 
    #ifdef SETUP
        __CPROVER_havoc_object(&tables);
        __CPROVER_assume(global_data.kot.lock.raw == SHAREX_FREE);
        //init the kot table
        for (int i = 0; i < HKID_SIZE; i++) {
            __CPROVER_assume(global_data.kot.entries[i].state == KOT_STATE_HKID_FREE);
            __CPROVER_assume(tables[i].tdr_table.management_fields.fatal == false); 
            __CPROVER_assume(tables[i].pamt_entry.pt == PT_NDA); 
            __CPROVER_assume(tables[i].tdr_table.key_management_fields.pkg_config_bitmap == 0); 
            __CPROVER_assume(tables[i].tdr_table.management_fields.num_tdcx == 0); 
            __CPROVER_assume(tables[i].tdcx_pamt_entry.pt == PT_NDA);

            // SOPHIA: highkey have no idea what this does for add_cx
            __CPROVER_assume(tables[i].tdcx_table.management_fields.op_state == 0); // TDH_MNG_ADDCX_LEAF == 1, [1][0] == 1
        }

    #endif // SETUP
    
    #ifdef KEY_CONFIG_SETUP
        __CPROVER_havoc_object(&local_data.lp_info.pkg);
        __CPROVER_assume(local_data.lp_info.pkg >= 0 && local_data.lp_info.pkg < HKID_SIZE);
        __CPROVER_havoc_object(&global_data.pkg_config_bitmap);
        __CPROVER_assume(global_data.pkg_config_bitmap & BIT(local_data.lp_info.pkg) != 0);

        
        // Ensure at least one element has pamt_entry.pt set to PT_TDR
        bool_t found = false;
        for (int i = 0; i < HKID_SIZE; i++) {
            if ((tables[i].pamt_entry.pt == PT_TDR) &&
                !tables[i].tdr_table.management_fields.fatal &&
                (tables[i].tdr_table.management_fields.lifecycle_state == TD_HKID_ASSIGNED) &&
                !(tables[i].tdr_table.key_management_fields.pkg_config_bitmap & (BIT(local_data.lp_info.pkg)))) {
                found = true;
                index = i;
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
            if (tables[i].pamt_entry.pt == PT_TDR && 
                tables[i].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED && 
                tables[i].tdcx_table.management_fields.op_state == OP_STATE_UNINITIALIZED && 
                !tables[i].tdr_table.management_fields.fatal &&
                tables[i].tdr_table.management_fields.num_tdcx < MAX_NUM_TDCS_PAGES && 
                tables[i].tdcx_pamt_entry.pt == PT_NDA && 
                seamcall_state_lookup[TDH_MNG_ADDCX_LEAF][tables[i].tdcx_table.management_fields.op_state]) {
                found = true;
                index = i; 
                break;
            }
        }
        __CPROVER_assume(found);

    #endif // ADD_CX_SETUP

    #ifdef INIT_SETUP
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
            if (tables[i].pamt_entry.pt == PT_TDR && 
                tables[i].tdr_table.management_fields.lifecycle_state == TD_KEYS_CONFIGURED &&
                tables[i].tdr_table.management_fields.fatal == false && 
                tables[i].tdr_table.management_fields.num_tdcx >= MIN_NUM_TDCS_PAGES && 
                tables[i].td_params_table.ia32_arch_capabilities_config == 0 
            )
                {
                    found = true;
                    index = i; 
                    break;
            }
        }

        __CPROVER_assume(found);
    #endif // INIT_SETUP

    //TDX_bootup(); 
    TD_setup(index); 
}
#endif // not SOURCE