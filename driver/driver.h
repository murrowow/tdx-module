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

// SOPHIA: define SOURCE, MODULAR_PROOF, FLOW_PROOF on command line for compile tine defines

#ifdef SOURCE
#else 
// from tdx_global_data.h the max number of hkids is 2048 
#define n 1
#define HKID_SIZE 0x1 << n
#define HKID_MASK (0x1U << n) - 1
#define NUM_TDCX_BIT_WIDTH ((MAX_NUM_TDCS_PAGES) == 0 ? 1 : (32 - __builtin_clz(MAX_NUM_TDCS_PAGES)))


typedef struct tdr_small_s
{
    struct {
        bool_t fatal;

        unsigned num_tdcx : NUM_TDCX_BIT_WIDTH;
        unsigned chldcnt : NUM_TDCX_BIT_WIDTH;
        td_lifecycle_state_t  lifecycle_state;
        struct {
            unsigned val : HKID_SIZE;
        }  tdcx_pa[MAX_NUM_TDCS_PAGES];
    } management_fields;

    struct {
        unsigned hkid : n; 
        unsigned pkg_config_bitmap : HKID_SIZE;
    } key_management_fields;

    struct {
        unsigned handoff_version : HKID_SIZE;
        unsigned seamdb_index : HKID_SIZE;
        uint256_t seamdb_nonce;

        struct {
          unsigned val : HKID_SIZE;  
        } reserved[16];
    } td_preserving_fields;

} tdr_small_t;

typedef struct tdcs_small_s
{
    struct {
        op_state_e op_state;
        uint16_t num_l2_vms;
        uint32_t num_vcpus;
        uint32_t num_assoc_vcpus;
    } management_fields;
    tdcs_execution_control_fields_t        executions_ctl_fields;
    tdcs_epoch_tracking_fields_t           epoch_tracking;
    tdcs_measurement_fields_t              measurement_fields;
    cpuid_config_return_values_t           cpuid_config_vals[MAX_POSSIBLE_CPUID_LOOKUP];

} tdcs_small_t;

typedef struct tdvps_small_s
{
    struct {
        unsigned state : HKID_SIZE;
        unsigned num_tdvps_pages : HKID_SIZE;
        unsigned assoc_lpid : HKID_SIZE;
        struct {
            unsigned val : HKID_SIZE;
        } tdvps_pa[MAX_TDVPS_PAGES];
    } management;

} tdvps_small_t;

struct hardware_states
{
   pamt_entry_t pamt_entry; 
   tdr_small_t  tdr_table; 
   uint8_t tdr_mem; 
   bool_t tdr_lock;
   
   pamt_entry_t tdcx_pamt_entry;
   tdcs_small_t tdcx_table;
   uint8_t tdcx_mem;
   bool_t tdcx_lock;

   td_params_t td_params_table;

//    pamt_entry_t tdvpr_pamt_entry;
//    tdvps_small_t tdvpr_table;
//    uint8_t tdvpr_mem;
//    bool_t tdvpr_lock;
};

// global_data pointer 
// private_hkid_min and private_hkid_max
tdx_module_global_t global_data;

// local_data pointer
tdx_module_local_t  local_data;

//pamt entry table
struct hardware_states tables[HKID_SIZE];

char kot_lock;

//PAMT TABLE

void driver_main(); 

_STATIC_INLINE_ pa_t set_hkid_to_pa_local(pa_t pa, uint16_t hkid)
{
    pa.full_pa &= ~(global_data.hkid_mask);
    pa.full_pa |= ((uint64_t)hkid << global_data.hkid_start_bit);

    return pa;
}

#endif // not SOURCE
#endif