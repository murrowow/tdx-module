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

#include "stdio.h"
#include "stdint.h"

// from tdx_global_data.h the max number of hkids is 2048 
#define n 1
#define hkid_size 0x1 << n
#define hkid_mask (0x1U << n) - 1

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

struct hardware_states
{
   pamt_entry_t pamt_entry; 
   tdr_t tdr_table; 
   uint8_t tdr_mem; 
};


// some kind of flag to give to the driver
driver_flag flag; 

// global_data pointer 
// private_hkid_min and private_hkid_max
tdx_module_global_t global_data;

// local_data pointer
tdx_module_local_t  local_data;

//pamt entry table
struct hardware_states tables[hkid_size];

char kot_lock;

//PAMT TABLE

void driver_main(driver_flag); 

#endif 