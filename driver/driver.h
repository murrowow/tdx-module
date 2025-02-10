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

#define n 2
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
    // KOT_STATE_HKID_FREE 
    // KOT_STATE_HKID_ASSIGNED
    // KOT_STATE_HKID_FLUSHED 
    // KOT_STATE_HKID_RESERVED  
   unsigned kot_state : 2;   

    // PT_NDA
    // PT_RSVD
    // PT_REG
    // PT_TDR
    // PT_TDCX 
    // PT_TDVPR
    // reserved = 7
    // PT_EPT
   //unsigned pamt_state : 4;
   pamt_entry_t pamt_entry; 
};

// some kind of flag to give to the driver
driver_flag flag; 

uint64_t seamrr_base = 0x0; 
uint64_t seamrr_top = 0xFFFFFFFF;

pamt_entry_t pamt; 

//KOT entry table
struct hardware_states tables[hkid_size];
//struct pamt_entry_t pamt_table[hkid_size]; 

char kot_lock;

//PAMT TABLE

void driver_main(driver_flag); 
void write(void *, void *, int);

#endif 