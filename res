CBMC version 6.4.0 (cbmc-6.4.0) 64-bit arm64 macos
Type-checking tdh_mng_create
Generating GOTO Program
Adding CPROVER library (arm64)
Removal of function pointers and virtual functions
Generic Property Instrumentation
Starting Bounded Model Checking
Passing problem to propositional reduction
converting SSA
Running propositional reduction
SAT checker: instance is SATISFIABLE
Running propositional reduction
SAT checker: instance is SATISFIABLE
Running propositional reduction
SAT checker: instance is SATISFIABLE

** Results:
/Users/sz7155/Documents/TDX/tdx-module/src/common/helpers/helpers.h function fill_area_cacheline
[fill_area_cacheline.array_bounds.1] line 356 array 'chunk' upper bound in chunk[(signed long int)i]: SUCCESS

/Users/sz7155/Documents/TDX/tdx-module/src/common/helpers/helpers.h function is_private_hkid
[is_private_hkid.pointer_dereference.1] line 176 dereference failure: pointer NULL in return_value_get_global_data->private_hkid_min: FAILURE
[is_private_hkid.pointer_dereference.2] line 176 dereference failure: pointer invalid in return_value_get_global_data->private_hkid_min: FAILURE
[is_private_hkid.pointer_dereference.3] line 176 dereference failure: deallocated dynamic object in return_value_get_global_data->private_hkid_min: FAILURE
[is_private_hkid.pointer_dereference.4] line 176 dereference failure: dead object in return_value_get_global_data->private_hkid_min: FAILURE
[is_private_hkid.pointer_dereference.5] line 176 dereference failure: pointer outside object bounds in return_value_get_global_data->private_hkid_min: FAILURE
[is_private_hkid.pointer_dereference.6] line 176 dereference failure: invalid integer address in return_value_get_global_data->private_hkid_min: FAILURE
[is_private_hkid.pointer_dereference.7] line 177 dereference failure: pointer NULL in return_value_get_global_data$0->private_hkid_max: FAILURE
[is_private_hkid.pointer_dereference.8] line 177 dereference failure: pointer invalid in return_value_get_global_data$0->private_hkid_max: FAILURE
[is_private_hkid.pointer_dereference.9] line 177 dereference failure: deallocated dynamic object in return_value_get_global_data$0->private_hkid_max: FAILURE
[is_private_hkid.pointer_dereference.10] line 177 dereference failure: dead object in return_value_get_global_data$0->private_hkid_max: FAILURE
[is_private_hkid.pointer_dereference.11] line 177 dereference failure: pointer outside object bounds in return_value_get_global_data$0->private_hkid_max: FAILURE
[is_private_hkid.pointer_dereference.12] line 177 dereference failure: invalid integer address in return_value_get_global_data$0->private_hkid_max: FAILURE

src/vmm_dispatcher/api_calls/tdh_mng_create.c function tdh_mng_create
[tdh_mng_create.no-body.check_lock_and_map_explicit_tdr] line 72 no body for callee check_lock_and_map_explicit_tdr: FAILURE
[tdh_mng_create.array_bounds.1] line 100 array.kot.entries dynamic object upper bound in global_data->kot.entries[(signed long int)td_hkid]: SUCCESS
[tdh_mng_create.pointer_dereference.1] line 100 dereference failure: pointer NULL in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.2] line 100 dereference failure: pointer invalid in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.3] line 100 dereference failure: deallocated dynamic object in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.4] line 100 dereference failure: dead object in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.5] line 100 dereference failure: pointer outside object bounds in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.6] line 100 dereference failure: invalid integer address in global_data->kot: SUCCESS
[tdh_mng_create.array_bounds.2] line 126 array.kot.entries dynamic object upper bound in global_data->kot.entries[(signed long int)td_hkid]: SUCCESS
[tdh_mng_create.pointer_dereference.7] line 126 dereference failure: pointer NULL in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.8] line 126 dereference failure: pointer invalid in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.9] line 126 dereference failure: deallocated dynamic object in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.10] line 126 dereference failure: dead object in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.11] line 126 dereference failure: pointer outside object bounds in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.12] line 126 dereference failure: invalid integer address in global_data->kot: SUCCESS
[tdh_mng_create.pointer_dereference.13] line 129 dereference failure: pointer NULL in tdr_ptr->key_management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.14] line 129 dereference failure: pointer invalid in tdr_ptr->key_management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.15] line 129 dereference failure: deallocated dynamic object in tdr_ptr->key_management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.16] line 129 dereference failure: dead object in tdr_ptr->key_management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.17] line 129 dereference failure: pointer outside object bounds in tdr_ptr->key_management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.18] line 129 dereference failure: invalid integer address in tdr_ptr->key_management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.19] line 130 dereference failure: pointer NULL in tdr_ptr->management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.20] line 130 dereference failure: pointer invalid in tdr_ptr->management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.21] line 130 dereference failure: deallocated dynamic object in tdr_ptr->management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.22] line 130 dereference failure: dead object in tdr_ptr->management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.23] line 130 dereference failure: pointer outside object bounds in tdr_ptr->management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.24] line 130 dereference failure: invalid integer address in tdr_ptr->management_fields: SUCCESS
[tdh_mng_create.pointer_dereference.25] line 132 dereference failure: pointer NULL in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.26] line 132 dereference failure: pointer invalid in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.27] line 132 dereference failure: deallocated dynamic object in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.28] line 132 dereference failure: dead object in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.29] line 132 dereference failure: pointer outside object bounds in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.30] line 132 dereference failure: invalid integer address in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.31] line 132 dereference failure: pointer NULL in global_data->seamdb_index: SUCCESS
[tdh_mng_create.pointer_dereference.32] line 132 dereference failure: pointer invalid in global_data->seamdb_index: SUCCESS
[tdh_mng_create.pointer_dereference.33] line 132 dereference failure: deallocated dynamic object in global_data->seamdb_index: SUCCESS
[tdh_mng_create.pointer_dereference.34] line 132 dereference failure: dead object in global_data->seamdb_index: SUCCESS
[tdh_mng_create.pointer_dereference.35] line 132 dereference failure: pointer outside object bounds in global_data->seamdb_index: SUCCESS
[tdh_mng_create.pointer_dereference.36] line 132 dereference failure: invalid integer address in global_data->seamdb_index: SUCCESS
[tdh_mng_create.array_bounds.3] line 136 array dynamic object upper bound in byte_extract_little_endian(tdr_ptr->td_preserving_fields.seamdb_nonce, 0l, uint64_t [4l])[(signed long int)i]: SUCCESS
[tdh_mng_create.array_bounds.4] line 136 array dynamic object upper bound in byte_extract_little_endian(global_data->seamdb_nonce, 0l, uint64_t [4l])[(signed long int)i]: SUCCESS
[tdh_mng_create.pointer_dereference.37] line 136 dereference failure: pointer NULL in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.38] line 136 dereference failure: pointer invalid in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.39] line 136 dereference failure: deallocated dynamic object in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.40] line 136 dereference failure: dead object in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.41] line 136 dereference failure: pointer outside object bounds in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.42] line 136 dereference failure: invalid integer address in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.43] line 136 dereference failure: pointer NULL in global_data->seamdb_nonce: SUCCESS
[tdh_mng_create.pointer_dereference.44] line 136 dereference failure: pointer invalid in global_data->seamdb_nonce: SUCCESS
[tdh_mng_create.pointer_dereference.45] line 136 dereference failure: deallocated dynamic object in global_data->seamdb_nonce: SUCCESS
[tdh_mng_create.pointer_dereference.46] line 136 dereference failure: dead object in global_data->seamdb_nonce: SUCCESS
[tdh_mng_create.pointer_dereference.47] line 136 dereference failure: pointer outside object bounds in global_data->seamdb_nonce: SUCCESS
[tdh_mng_create.pointer_dereference.48] line 136 dereference failure: invalid integer address in global_data->seamdb_nonce: SUCCESS
[tdh_mng_create.pointer_dereference.49] line 138 dereference failure: pointer NULL in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.50] line 138 dereference failure: pointer invalid in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.51] line 138 dereference failure: deallocated dynamic object in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.52] line 138 dereference failure: dead object in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.53] line 138 dereference failure: pointer outside object bounds in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.54] line 138 dereference failure: invalid integer address in tdr_ptr->td_preserving_fields: SUCCESS
[tdh_mng_create.pointer_dereference.55] line 138 dereference failure: pointer NULL in global_data->module_hv: SUCCESS
[tdh_mng_create.pointer_dereference.56] line 138 dereference failure: pointer invalid in global_data->module_hv: SUCCESS
[tdh_mng_create.pointer_dereference.57] line 138 dereference failure: deallocated dynamic object in global_data->module_hv: SUCCESS
[tdh_mng_create.pointer_dereference.58] line 138 dereference failure: dead object in global_data->module_hv: SUCCESS
[tdh_mng_create.pointer_dereference.59] line 138 dereference failure: pointer outside object bounds in global_data->module_hv: SUCCESS
[tdh_mng_create.pointer_dereference.60] line 138 dereference failure: invalid integer address in global_data->module_hv: SUCCESS
[tdh_mng_create.pointer_dereference.61] line 141 dereference failure: pointer NULL in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.62] line 141 dereference failure: pointer invalid in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.63] line 141 dereference failure: deallocated dynamic object in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.64] line 141 dereference failure: dead object in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.65] line 141 dereference failure: pointer outside object bounds in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.66] line 141 dereference failure: invalid integer address in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.67] line 142 dereference failure: pointer NULL in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.68] line 142 dereference failure: pointer invalid in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.69] line 142 dereference failure: deallocated dynamic object in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.70] line 142 dereference failure: dead object in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.71] line 142 dereference failure: pointer outside object bounds in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.pointer_dereference.72] line 142 dereference failure: invalid integer address in tdr_pamt_entry_ptr->$anon0: SUCCESS
[tdh_mng_create.postcondition.1] line 157 target address is valid: SUCCESS
[tdh_mng_create.postcondition.2] line 158 hkid address is valid: SUCCESS
[tdh_mng_create.postcondition.3] line 159 hkid reserved bits is 0: SUCCESS

** 13 of 93 failed (4 iterations)
VERIFICATION FAILED
