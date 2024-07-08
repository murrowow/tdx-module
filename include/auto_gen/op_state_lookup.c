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
 *  Spreadsheet Format Version - '2'
 **/

#include "auto_gen/op_state_lookup.h"


const bool_t seamcall_state_lookup[MAX_SEAMCALL_LEAF][NUM_OP_STATES] = {
    [0] = {0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0 },
    [1] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    [2] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    [3] = {0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0 },
    [4] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    [5] = {0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0 },
    [6] = {0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0 },
    [7] = {0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0 },
    [10] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    [11] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    [12] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    [13] = {0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1 },
    [14] = {0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1 },
    [15] = {0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0 },
    [16] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    [17] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    [18] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    [21] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    [22] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    [23] = {0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0 },
    [25] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    [26] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    [29] = {0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1 },
    [30] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    [38] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    [39] = {0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0 },
    [43] = {0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1 },
    [48] = {1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0 },
    [49] = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    [64] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
    [65] = {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    [66] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    [68] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
    [70] = {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    [71] = {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
    [72] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0 },
    [73] = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    [74] = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    [75] = {0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
    [80] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1 },
    [81] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 },
    [82] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    [83] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 },
    [84] = {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0 },
    [85] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    [86] = {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    [87] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    [96] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 }
};



const bool_t servtd_bind_othertd_state_lookup[NUM_OP_STATES] = { 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0 };




const bool_t tdcall_state_lookup[MAX_TDCALL_LEAF][NUM_OP_STATES] = {
    [18] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    [20] = {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1 }
};



const state_flags_t state_flags_lookup[NUM_OP_STATES] = {
  [0] = {
    .tlb_tracking_required = 0,
    .any_initialized = 0,
    .any_finalized = 0,
    .export_in_order = 0,
    .import_in_order = 0,
    .import_out_of_order = 0,
    .import_in_progress = 0
  },
  [1] = {
    .tlb_tracking_required = 0,
    .any_initialized = 1,
    .any_finalized = 0,
    .export_in_order = 0,
    .import_in_order = 0,
    .import_out_of_order = 0,
    .import_in_progress = 0
  },
  [2] = {
    .tlb_tracking_required = 1,
    .any_initialized = 1,
    .any_finalized = 1,
    .export_in_order = 0,
    .import_in_order = 0,
    .import_out_of_order = 0,
    .import_in_progress = 0
  },
  [3] = {
    .tlb_tracking_required = 1,
    .any_initialized = 1,
    .any_finalized = 1,
    .export_in_order = 1,
    .import_in_order = 0,
    .import_out_of_order = 0,
    .import_in_progress = 0
  },
  [4] = {
    .tlb_tracking_required = 0,
    .any_initialized = 1,
    .any_finalized = 1,
    .export_in_order = 1,
    .import_in_order = 0,
    .import_out_of_order = 0,
    .import_in_progress = 0
  },
  [5] = {
    .tlb_tracking_required = 0,
    .any_initialized = 1,
    .any_finalized = 1,
    .export_in_order = 0,
    .import_in_order = 0,
    .import_out_of_order = 0,
    .import_in_progress = 0
  },
  [6] = {
    .tlb_tracking_required = 0,
    .any_initialized = 1,
    .any_finalized = 1,
    .export_in_order = 0,
    .import_in_order = 1,
    .import_out_of_order = 0,
    .import_in_progress = 1
  },
  [7] = {
    .tlb_tracking_required = 0,
    .any_initialized = 1,
    .any_finalized = 1,
    .export_in_order = 0,
    .import_in_order = 1,
    .import_out_of_order = 0,
    .import_in_progress = 1
  },
  [8] = {
    .tlb_tracking_required = 0,
    .any_initialized = 1,
    .any_finalized = 1,
    .export_in_order = 0,
    .import_in_order = 0,
    .import_out_of_order = 1,
    .import_in_progress = 1
  },
  [9] = {
    .tlb_tracking_required = 1,
    .any_initialized = 1,
    .any_finalized = 1,
    .export_in_order = 0,
    .import_in_order = 0,
    .import_out_of_order = 1,
    .import_in_progress = 1
  },
  [10] = {
    .tlb_tracking_required = 0,
    .any_initialized = 1,
    .any_finalized = 0,
    .export_in_order = 0,
    .import_in_order = 0,
    .import_out_of_order = 0,
    .import_in_progress = 0
  }
};



