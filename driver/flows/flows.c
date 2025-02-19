#include "flows/flows.h"

api_error_type TD_setup() {
    uint64_t target_tdr_pa;
    hkid_api_input_t hkid_info;
    api_error_type error = UNINITIALIZE_ERROR;

    __CPROVER_havoc_object(&target_tdr_pa);
    __CPROVER_havoc_object(&hkid_info);
}