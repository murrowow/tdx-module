#ifndef DRIVERHEADER_H
#define DRIVERHEADER_H

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
#include "src/vmm_dispatcher/tdx_vmm_dispatcher.h"
#include "driver/api_calls/api_calls.h"
#include "driver/flows/flows.h"

#include "stdio.h"
#include "stdint.h"

// from tdx_global_data.h the max number of hkids is 2048 
#define n 1
#define HKID_SIZE 0x1 << n
#define HKID_MASK (0x1U << n) - 1

// flags that control what state the hardware will be set to
typedef enum{
    BOOTUP = 0,
    MID_SETUP = 1
} driver_flag;

// flag that controls what hardware is being written to
typedef enum{
    KOT = 0,
    PAMT = 1
}hardware_flag; 

typedef struct tdr_small_s
{
    struct {
        bool_t fatal;

        unsigned num_tdcx : 2;
        unsigned chldcnt : 2;
        td_lifecycle_state_t  lifecycle_state;
        struct {
            unsigned val : 2;
        }  tdcx_pa[MAX_NUM_TDCS_PAGES];
    } management_fields;

    struct {
        unsigned hkid : 1; 
        unsigned pkg_config_bitmap : 2;
    } key_management_fields;

    struct {
        unsigned handoff_version : 2;
        unsigned seamdb_index : 2;
        uint256_t seamdb_nonce;

        struct {
          unsigned val : 2;  
        } reserved[16];
    } td_preserving_fields;

} tdr_small_t;

struct hardware_states
{
   pamt_entry_t pamt_entry; 
   tdr_small_t  tdr_table; 
   uint8_t tdr_mem; 
   bool_t tdr_lock;
   
//    pamt_entry_t tdcx_pamt_entry;
//    tdcs_t tdcx_table;
//    uint8_t tdcx_mem;
//    bool_t tdcx_lock;
};

// some kind of flag to give to the driver
driver_flag flag; 

// global_data pointer 
// private_hkid_min and private_hkid_max
tdx_module_global_t global_data;

// local_data pointer
tdx_module_local_t  local_data;

//pamt entry table
struct hardware_states tables[HKID_SIZE];

char kot_lock;

//PAMT TABLE

void driver_main(driver_flag); 

#endif 