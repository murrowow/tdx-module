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
 *  based on architecture commit id "5c1d21c9" 
 *  Spreadsheet Format Version - '2'
 **/

#ifndef _AUTO_GEN_SEPT_STATE_LOOKUP_H_
#define _AUTO_GEN_SEPT_STATE_LOOKUP_H_


#include "tdx_api_defs.h"


#define MAX_SEAMCALL_LEAF 128
#define MAX_TDCALL_LEAF 32
#define MAX_SEPT_STATE_ENC 128
#define MAX_L2_SEPT_STATE_ENC 16
#define NUM_SEPT_STATES 16


typedef struct sept_special_flags_s
{
    uint32_t public_state;
    bool_t live_export_allowed;
    bool_t paused_export_allowed;
    bool_t first_time_export_allowed;
    bool_t re_export_allowed;
    bool_t export_cancel_allowed;
    bool_t first_time_import_allowed;
    bool_t re_import_allowed;
    bool_t import_cancel_allowed;
    bool_t mapped_or_pending;
    bool_t any_exported;
    bool_t any_exported_and_dirty;
    bool_t any_exported_and_non_dirty;
    bool_t any_pending;
    bool_t any_pending_and_guest_acceptable;
    bool_t any_blocked;
    bool_t any_blockedw;
    bool_t guest_fully_accessible_leaf;
    bool_t tlb_tracking_required;
    bool_t guest_accessible_leaf;
    uint32_t index;
} sept_special_flags_t;


extern const bool_t seamcall_sept_state_lookup[MAX_SEAMCALL_LEAF][NUM_SEPT_STATES];


extern const bool_t tdcall_sept_state_lookup[MAX_TDCALL_LEAF][NUM_SEPT_STATES];


extern const sept_special_flags_t sept_special_flags_lookup[MAX_SEPT_STATE_ENC];

extern const sept_special_flags_t l2_sept_special_flags_lookup[MAX_L2_SEPT_STATE_ENC];

// State encoding for SEPT states - Bits PS:D 

#define SEPT_STATE_FREE_ENCODING 0xe
#define SEPT_STATE_REMOVED_ENCODING 0xc
#define SEPT_STATE_NL_MAPPED_ENCODING 0x0
#define SEPT_STATE_NL_BLOCKED_ENCODING 0x8
#define SEPT_STATE_MAPPED_ENCODING 0x60
#define SEPT_STATE_BLOCKED_ENCODING 0x68
#define SEPT_STATE_BLOCKEDW_ENCODING 0x64
#define SEPT_STATE_EXPORTED_BLOCKEDW_ENCODING 0x66
#define SEPT_STATE_EXPORTED_DIRTY_ENCODING 0x63
#define SEPT_STATE_EXPORTED_DIRTY_BLOCKEDW_ENCODING 0x67
#define SEPT_STATE_PENDING_ENCODING 0x70
#define SEPT_STATE_PENDING_BLOCKED_ENCODING 0x78
#define SEPT_STATE_PENDING_BLOCKEDW_ENCODING 0x74
#define SEPT_STATE_PENDING_EXPORTED_BLOCKEDW_ENCODING 0x76
#define SEPT_STATE_PENDING_EXPORTED_DIRTY_ENCODING 0x73
#define SEPT_STATE_PENDING_EXPORTED_DIRTY_BLOCKEDW_ENCODING 0x77
#define SEPT_STATE_L2_FREE_ENCODING 0x0
#define SEPT_STATE_L2_NL_MAPPED_ENCODING 0x8
#define SEPT_STATE_L2_NL_BLOCKED_ENCODING 0x1
#define SEPT_STATE_L2_MAPPED_ENCODING 0x6
#define SEPT_STATE_L2_BLOCKED_ENCODING 0x7


#endif /* _AUTO_GEN_SEPT_STATE_LOOKUP_H_ */