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

uint64_t seamrr_base = 0x0; 
uint64_t seamrr_top = 0xFFFFFFFF;

int table_size = 100; 

//KOT entry table
kot_entry_t kot_table[100];

//PAMT TABLE
pamt_entry_t pamt[100];

void driver_main(void); 

#endif 