// Copyright (C) 2023 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT
/**
 *  This File is Automatically generated by the TDX xls extract tool
 *  based on architecture commit id "40ef60d2" 
 *  Spreadsheet Format Version - '5'
 **/

#ifndef _AUTO_GEN_ERROR_CODES_H_
#define _AUTO_GEN_ERROR_CODES_H_

// Error codes
#define TDX_SUCCESS                                  0x0000000000000000ULL
#define TDX_NON_RECOVERABLE_VCPU                     0x4000000100000000ULL
#define TDX_NON_RECOVERABLE_TD                       0x6000000200000000ULL
#define TDX_INTERRUPTED_RESUMABLE                    0x8000000300000000ULL
#define TDX_INTERRUPTED_RESTARTABLE                  0x8000000400000000ULL
#define TDX_NON_RECOVERABLE_TD_NON_ACCESSIBLE        0x6000000500000000ULL
#define TDX_INVALID_RESUMPTION                       0xC000000600000000ULL
#define TDX_NON_RECOVERABLE_TD_WRONG_APIC_MODE       0xE000000700000000ULL
#define TDX_CROSS_TD_FAULT                           0x8000000800000000ULL
#define TDX_CROSS_TD_TRAP                            0x9000000900000000ULL
#define TDX_NON_RECOVERABLE_TD_CORRUPTED_MD          0x6000000A00000000ULL
#define TDX_OPERAND_INVALID                          0xC000010000000000ULL
#define TDX_OPERAND_ADDR_RANGE_ERROR                 0xC000010100000000ULL
#define TDX_OPERAND_BUSY                             0x8000020000000000ULL
#define TDX_PREVIOUS_TLB_EPOCH_BUSY                  0x8000020100000000ULL
#define TDX_SYS_BUSY                                 0x8000020200000000ULL
#define TDX_RND_NO_ENTROPY                           0x8000020300000000ULL
#define TDX_OPERAND_BUSY_HOST_PRIORITY               0x8000020400000000ULL
#define TDX_HOST_PRIORITY_BUSY_TIMEOUT               0x9000020500000000ULL
#define TDX_PAGE_METADATA_INCORRECT                  0xC000030000000000ULL
#define TDX_PAGE_ALREADY_FREE                        0x0000030100000000ULL
#define TDX_PAGE_NOT_OWNED_BY_TD                     0xC000030200000000ULL
#define TDX_PAGE_NOT_FREE                            0xC000030300000000ULL
#define TDX_TD_ASSOCIATED_PAGES_EXIST                0xC000040000000000ULL
#define TDX_SYS_INIT_NOT_PENDING                     0xC000050000000000ULL
#define TDX_SYS_LP_INIT_NOT_DONE                     0xC000050200000000ULL
#define TDX_SYS_LP_INIT_DONE                         0xC000050300000000ULL
#define TDX_SYS_NOT_READY                            0xC000050500000000ULL
#define TDX_SYS_SHUTDOWN                             0xC000050600000000ULL
#define TDX_SYS_KEY_CONFIG_NOT_PENDING               0xC000050700000000ULL
#define TDX_SYS_STATE_INCORRECT                      0xC000050800000000ULL
#define TDX_SYS_INVALID_HANDOFF                      0xC000050900000000ULL
#define TDX_SYS_INCOMPATIBLE_SIGSTRUCT               0xC000050A00000000ULL
#define TDX_SYS_LP_INIT_NOT_PENDING                  0xC000050B00000000ULL
#define TDX_SYS_CONFIG_NOT_PENDING                   0xC000050C00000000ULL
#define TDX_INCOMPATIBLE_SEAM_CAPABILITIES           0xC000050D00000000ULL
#define TDX_TD_FATAL                                 0xE000060400000000ULL
#define TDX_TD_NON_DEBUG                             0xC000060500000000ULL
#define TDX_TDCS_NOT_ALLOCATED                       0xC000060600000000ULL
#define TDX_LIFECYCLE_STATE_INCORRECT                0xC000060700000000ULL
#define TDX_OP_STATE_INCORRECT                       0xC000060800000000ULL
#define TDX_NO_VCPUS                                 0xC000060900000000ULL
#define TDX_TDCX_NUM_INCORRECT                       0xC000061000000000ULL
#define TDX_X2APIC_ID_NOT_UNIQUE                     0xC000062100000000ULL
#define TDX_VCPU_STATE_INCORRECT                     0xC000070000000000ULL
#define TDX_VCPU_ASSOCIATED                          0x8000070100000000ULL
#define TDX_VCPU_NOT_ASSOCIATED                      0x8000070200000000ULL
#define TDX_NO_VALID_VE_INFO                         0xC000070400000000ULL
#define TDX_MAX_VCPUS_EXCEEDED                       0xC000070500000000ULL
#define TDX_TSC_ROLLBACK                             0xC000070600000000ULL
#define TDX_TD_VMCS_FIELD_NOT_INITIALIZED            0xC000073000000000ULL
#define TD_VMCS_FIELD_ERROR                          0xC000073100000000ULL
#define TDX_KEY_GENERATION_FAILED                    0x8000080000000000ULL
#define TDX_TD_KEYS_NOT_CONFIGURED                   0x8000081000000000ULL
#define TDX_KEY_STATE_INCORRECT                      0xC000081100000000ULL
#define TDX_KEY_CONFIGURED                           0x0000081500000000ULL
#define TDX_WBCACHE_NOT_COMPLETE                     0x8000081700000000ULL
#define TDX_HKID_NOT_FREE                            0xC000082000000000ULL
#define TDX_NO_HKID_READY_TO_WBCACHE                 0x0000082100000000ULL
#define TDX_WBCACHE_RESUME_ERROR                     0xC000082300000000ULL
#define TDX_FLUSHVP_NOT_DONE                         0x8000082400000000ULL
#define TDX_NUM_ACTIVATED_HKIDS_NOT_SUPPORTED        0xC000082500000000ULL
#define TDX_INCORRECT_CPUID_VALUE                    0xC000090000000000ULL
#define TDX_LIMIT_CPUID_MAXVAL_SET                   0xC000090100000000ULL
#define TDX_INCONSISTENT_CPUID_FIELD                 0xC000090200000000ULL
#define TDX_CPUID_MAX_SUBLEAVES_UNRECOGNIZED         0xC000090300000000ULL
#define TDX_CPUID_LEAF_1F_FORMAT_UNRECOGNIZED        0xC000090400000000ULL
#define TDX_INVALID_WBINVD_SCOPE                     0xC000090500000000ULL
#define TDX_INVALID_PKG_ID                           0xC000090600000000ULL
#define TDX_ENABLE_MONITOR_FSM_NOT_SET               0xC000090700000000ULL
#define TDX_CPUID_LEAF_NOT_SUPPORTED                 0xC000090800000000ULL
#define TDX_SMRR_NOT_LOCKED                          0xC000091000000000ULL
#define TDX_INVALID_SMRR_CONFIGURATION               0xC000091100000000ULL
#define TDX_SMRR_OVERLAPS_CMR                        0xC000091200000000ULL
#define TDX_SMRR_LOCK_NOT_SUPPORTED                  0xC000091300000000ULL
#define TDX_SMRR_NOT_SUPPORTED                       0xC000091400000000ULL
#define TDX_INCONSISTENT_MSR                         0xC000092000000000ULL
#define TDX_INCORRECT_MSR_VALUE                      0xC000092100000000ULL
#define TDX_SEAMREPORT_NOT_AVAILABLE                 0xC000093000000000ULL
#define TDX_SEAMDB_GETREF_NOT_AVAILABLE              0xC000093100000000ULL
#define TDX_SEAMDB_REPORT_NOT_AVAILABLE              0xC000093200000000ULL
#define TDX_SEAMVERIFYREPORT_NOT_AVAILABLE           0xC000093300000000ULL
#define TDX_INVALID_TDMR                             0xC0000A0000000000ULL
#define TDX_NON_ORDERED_TDMR                         0xC0000A0100000000ULL
#define TDX_TDMR_OUTSIDE_CMRS                        0xC0000A0200000000ULL
#define TDX_TDMR_ALREADY_INITIALIZED                 0x00000A0300000000ULL
#define TDX_INVALID_PAMT                             0xC0000A1000000000ULL
#define TDX_PAMT_OUTSIDE_CMRS                        0xC0000A1100000000ULL
#define TDX_PAMT_OVERLAP                             0xC0000A1200000000ULL
#define TDX_INVALID_RESERVED_IN_TDMR                 0xC0000A2000000000ULL
#define TDX_NON_ORDERED_RESERVED_IN_TDMR             0xC0000A2100000000ULL
#define TDX_CMR_LIST_INVALID                         0xC0000A2200000000ULL
#define TDX_EPT_WALK_FAILED                          0xC0000B0000000000ULL
#define TDX_EPT_ENTRY_FREE                           0xC0000B0100000000ULL
#define TDX_EPT_ENTRY_NOT_FREE                       0xC0000B0200000000ULL
#define TDX_EPT_ENTRY_NOT_PRESENT                    0xC0000B0300000000ULL
#define TDX_EPT_ENTRY_NOT_LEAF                       0xC0000B0400000000ULL
#define TDX_EPT_ENTRY_LEAF                           0xC0000B0500000000ULL
#define TDX_GPA_RANGE_NOT_BLOCKED                    0xC0000B0600000000ULL
#define TDX_GPA_RANGE_ALREADY_BLOCKED                0x00000B0700000000ULL
#define TDX_TLB_TRACKING_NOT_DONE                    0xC0000B0800000000ULL
#define TDX_EPT_INVALID_PROMOTE_CONDITIONS           0xC0000B0900000000ULL
#define TDX_PAGE_ALREADY_ACCEPTED                    0x00000B0A00000000ULL
#define TDX_PAGE_SIZE_MISMATCH                       0xC0000B0B00000000ULL
#define TDX_GPA_RANGE_BLOCKED                        0xC0000B0C00000000ULL
#define TDX_EPT_ENTRY_STATE_INCORRECT                0xC0000B0D00000000ULL
#define TDX_EPT_PAGE_NOT_FREE                        0xC0000B0E00000000ULL
#define TDX_L2_SEPT_WALK_FAILED                      0xC0000B0F00000000ULL
#define TDX_L2_SEPT_ENTRY_NOT_FREE                   0xC0000B1000000000ULL
#define TDX_PAGE_ATTR_INVALID                        0xC0000B1100000000ULL
#define TDX_L2_SEPT_PAGE_NOT_PROVIDED                0xC0000B1200000000ULL
#define TDX_METADATA_FIELD_ID_INCORRECT              0xC0000C0000000000ULL
#define TDX_METADATA_FIELD_NOT_WRITABLE              0xC0000C0100000000ULL
#define TDX_METADATA_FIELD_NOT_READABLE              0xC0000C0200000000ULL
#define TDX_METADATA_FIELD_VALUE_NOT_VALID           0xC0000C0300000000ULL
#define TDX_METADATA_LIST_OVERFLOW                   0xC0000C0400000000ULL
#define TDX_INVALID_METADATA_LIST_HEADER             0xC0000C0500000000ULL
#define TDX_REQUIRED_METADATA_FIELD_MISSING          0xC0000C0600000000ULL
#define TDX_METADATA_ELEMENT_SIZE_INCORRECT          0xC0000C0700000000ULL
#define TDX_METADATA_LAST_ELEMENT_INCORRECT          0xC0000C0800000000ULL
#define TDX_METADATA_FIELD_CURRENTLY_NOT_WRITABLE    0xC0000C0900000000ULL
#define TDX_METADATA_WR_MASK_NOT_VALID               0xC0000C0A00000000ULL
#define TDX_METADATA_FIRST_FIELD_ID_IN_CONTEXT       0x00000C0B00000000ULL
#define TDX_METADATA_FIELD_SKIP                      0x00000C0C00000000ULL
#define TDX_SERVTD_ALREADY_BOUND_FOR_TYPE            0xC0000D0000000000ULL
#define TDX_SERVTD_TYPE_MISMATCH                     0xC0000D0100000000ULL
#define TDX_SERVTD_ATTR_MISMATCH                     0xC0000D0200000000ULL
#define TDX_SERVTD_INFO_HASH_MISMATCH                0xC0000D0300000000ULL
#define TDX_SERVTD_UUID_MISMATCH                     0xC0000D0400000000ULL
#define TDX_SERVTD_NOT_BOUND                         0xC0000D0500000000ULL
#define TDX_SERVTD_BOUND                             0xC0000D0600000000ULL
#define TDX_TARGET_UUID_MISMATCH                     0xC0000D0700000000ULL
#define TDX_TARGET_UUID_UPDATED                      0xC0000D0800000000ULL
#define TDX_INVALID_MBMD                             0xC0000E0000000000ULL
#define TDX_INCORRECT_MBMD_MAC                       0xC0000E0100000000ULL
#define TDX_NOT_WRITE_BLOCKED                        0xC0000E0200000000ULL
#define TDX_ALREADY_WRITE_BLOCKED                    0x00000E0300000000ULL
#define TDX_NOT_EXPORTED                             0xC0000E0400000000ULL
#define TDX_MIGRATION_STREAM_STATE_INCORRECT         0xC0000E0500000000ULL
#define TDX_MAX_MIGS_NUM_EXCEEDED                    0xC0000E0600000000ULL
#define TDX_EXPORTED_DIRTY_PAGES_REMAIN              0xC0000E0700000000ULL
#define TDX_MIGRATION_DECRYPTION_KEY_NOT_SET         0xC0000E0800000000ULL
#define TDX_TD_NOT_MIGRATABLE                        0xC0000E0900000000ULL
#define TDX_PREVIOUS_EXPORT_CLEANUP_INCOMPLETE       0xC0000E0A00000000ULL
#define TDX_NUM_MIGS_HIGHER_THAN_CREATED             0xC0000E0B00000000ULL
#define TDX_IMPORT_MISMATCH                          0xC0000E0C00000000ULL
#define TDX_MIGRATION_EPOCH_OVERFLOW                 0xC0000E0D00000000ULL
#define TDX_MAX_EXPORTS_EXCEEDED                     0xC0000E0E00000000ULL
#define TDX_INVALID_PAGE_MAC                         0xC0000E0F00000000ULL
#define TDX_MIGRATED_IN_CURRENT_EPOCH                0xC0000E1000000000ULL
#define TDX_DISALLOWED_IMPORT_OVER_REMOVED           0xC0000E1100000000ULL
#define TDX_SOME_VCPUS_NOT_MIGRATED                  0xC0000E1200000000ULL
#define TDX_ALL_VCPUS_IMPORTED                       0xC0000E1300000000ULL
#define TDX_MIN_MIGS_NOT_CREATED                     0xC0000E1400000000ULL
#define TDX_VCPU_ALREADY_EXPORTED                    0xC0000E1500000000ULL
#define TDX_INVALID_MIGRATION_DECRYPTION_KEY         0xC0000E1600000000ULL
#define TDX_INVALID_CPUSVN                           0xC000100000000000ULL
#define TDX_INVALID_REPORTMACSTRUCT                  0xC000100100000000ULL
#define TDX_L2_EXIT_HOST_ROUTED_ASYNC                0x0000110000000000ULL
#define TDX_L2_EXIT_HOST_ROUTED_TDVMCALL             0x0000110100000000ULL
#define TDX_L2_EXIT_PENDING_INTERRUPT                0x0000110200000000ULL
#define TDX_PENDING_INTERRUPT                        0x0000112000000000ULL
#define TDX_TD_EXIT_BEFORE_L2_ENTRY                  0x0000114000000000ULL
#define TDX_TD_EXIT_ON_L2_VM_EXIT                    0x0000114100000000ULL
#define TDX_TD_EXIT_ON_L2_TO_L1                      0x0000114200000000ULL
#define TDX_GLA_NOT_CANONICAL                        0xC000116000000000ULL
#define UNINITIALIZE_ERROR                           0xFFFFFFFFFFFFFFFFULL

// Error operands
#define OPERAND_ID_RAX                               0ULL
#define OPERAND_ID_RCX                               1ULL
#define OPERAND_ID_RDX                               2ULL
#define OPERAND_ID_RBX                               3ULL
#define OPERAND_ID_RBP                               5ULL
#define OPERAND_ID_RSI                               6ULL
#define OPERAND_ID_RDI                               7ULL
#define OPERAND_ID_R8                                8ULL
#define OPERAND_ID_R9                                9ULL
#define OPERAND_ID_R10                               10ULL
#define OPERAND_ID_R11                               11ULL
#define OPERAND_ID_R12                               12ULL
#define OPERAND_ID_R13                               13ULL
#define OPERAND_ID_R14                               14ULL
#define OPERAND_ID_R15                               15ULL
#define OPERAND_ID_ATTRIBUTES                        64ULL
#define OPERAND_ID_XFAM                              65ULL
#define OPERAND_ID_EXEC_CONTROLS                     66ULL
#define OPERAND_ID_EPTP_CONTROLS                     67ULL
#define OPERAND_ID_MAX_VCPUS                         68ULL
#define OPERAND_ID_CPUID_CONFIG                      69ULL
#define OPERAND_ID_TSC_FREQUENCY                     70ULL
#define OPERAND_ID_NUM_L2_VMS                        71ULL
#define OPERAND_ID_IA32_ARCH_CAPABILITIES_CONFIG     72ULL
#define OPERAND_ID_PAGE                              95ULL
#define OPERAND_ID_TDMR_INFO_PA                      96ULL
#define OPERAND_ID_GPA_LIST_ENTRY                    97ULL
#define OPERAND_ID_MIG_BUFF_LIST_ENTRY               98ULL
#define OPERAND_ID_NEW_PAGE_LIST_ENTRY               99ULL
#define OPERAND_ID_TDR                               128ULL
#define OPERAND_ID_TDCX                              129ULL
#define OPERAND_ID_TDVPR                             130ULL
#define OPERAND_ID_REG_PAGE                          132ULL
#define OPERAND_ID_TDCS                              144ULL
#define OPERAND_ID_TDVPS                             145ULL
#define OPERAND_ID_SEPT_TREE                         146ULL
#define OPERAND_ID_SEPT_ENTRY                        147ULL
#define OPERAND_ID_RTMR                              168ULL
#define OPERAND_ID_TD_EPOCH                          169ULL
#define OPERAND_ID_L2_VAPIC_GPA                      170ULL
#define OPERAND_ID_MIGSC                             171ULL
#define OPERAND_ID_OP_STATE                          172ULL
#define OPERAND_ID_MIG                               173ULL
#define OPERAND_ID_SERVTD_BINDINGS                   174ULL
#define OPERAND_ID_METADATA_FIELD                    176ULL
#define OPERAND_ID_SYS                               184ULL
#define OPERAND_ID_TDMR                              185ULL
#define OPERAND_ID_KOT                               186ULL
#define OPERAND_ID_KET                               187ULL
#define OPERAND_ID_WBCACHE                           188ULL

#endif /* _AUTO_GEN_ERROR_CODES_H_ */
