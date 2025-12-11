#ifndef FLOWSCALLS_H
#define FLOWSCALLS_H

#include "include/tdx_vmm_api_handlers.h"
#include "include/tdx_td_api_handlers.h"
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
#include "src/td_transitions/td_exit.h"

#include "driver/driver.h"

#include "stdio.h"
#include "stdint.h"

void TD_setup(uint16_t);
void TDX_bootup(); 
void TD_mem_setup(page_info_api_input_t, page_info_api_input_t); 
void TD_enter();
void TD_exit(); 

#endif 