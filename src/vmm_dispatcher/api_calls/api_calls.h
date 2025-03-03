#ifndef VMMAPICALLS_H
#define VMMAPICALLS_H

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
#include "driver/driver.h"
#include "driver/api_calls.h"
#include "driver/flows/flows.h"
#include "src/vmm_dispatcher/api_calls/tdh_mng_create.c"

api_error_type tdh_mng_create(uint64_t, hkid_api_input_t);

#endif