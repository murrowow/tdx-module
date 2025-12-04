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
 * @file tdh_sys_config.c
 * @brief TDHSYSCONFIG API handler
 */

#include "include/tdx_api_defs.h"
#include "include/tdx_basic_defs.h"
#include "include/tdx_basic_types.h"
#include "include/tdx_vmm_api_handlers.h"
#include "include/auto_gen/tdx_error_codes_defs.h"
#include "src/common/data_structures/tdx_global_data.h"
#include "src/common/memory_handlers/pamt_manager.h"
#include "src/common/data_structures/loader_data.h"
#include "src/common/accessors/data_accessors.h"
#include "src/common/helpers/helpers.h"
#include "src/common/memory_handlers/keyhole_manager.h"
#include "include/auto_gen/cpuid_configurations.h"

#include "driver/driver.h"
typedef struct pamt_data_s
{
    uint64_t pamt_1g_base; /**< Base address of the PAMT_1G range associated with the above TDMR (HKID bits must be 0). 4K aligned. */
    uint64_t pamt_1g_size; /**< Size of the PAMT_1G range associated with the above TDMR. 4K aligned. */
    uint64_t pamt_2m_base; /**< Base address of the PAMT_2M range associated with the above TDMR (HKID bits must be 0). 4K aligned. */
    uint64_t pamt_2m_size; /**< Size of the PAMT_2M range associated with the above TDMR. 4K aligned. */
    uint64_t pamt_4k_base; /**< Base address of the PAMT_4K range associated with the above TDMR (HKID bits must be 0). 4K aligned. */
    uint64_t pamt_4k_size; /**< Size of the PAMT_4K range associated with the above TDMR. 4K aligned. */
}pamt_data_t;

_STATIC_INLINE_ void update_pamt_array (tdmr_info_entry_t*   tdmr_info_copy, pamt_data_t pamt_data_array[], uint32_t i)
{
    #ifdef SOURCE
    pamt_data_array[i].pamt_1g_base = tdmr_info_copy[i].pamt_1g_base;
    pamt_data_array[i].pamt_1g_size = tdmr_info_copy[i].pamt_1g_size;
    pamt_data_array[i].pamt_2m_base = tdmr_info_copy[i].pamt_2m_base;
    pamt_data_array[i].pamt_2m_size = tdmr_info_copy[i].pamt_2m_size;
    pamt_data_array[i].pamt_4k_base = tdmr_info_copy[i].pamt_4k_base;
    pamt_data_array[i].pamt_4k_size = tdmr_info_copy[i].pamt_4k_size;
    #endif // SOURCE
}

_STATIC_INLINE_ void copy_tdmr_info_entry (tdmr_info_entry_t* tdmr_info_src, tdmr_info_entry_t* tdmr_info_target)
{
    tdmr_info_target->tdmr_base = tdmr_info_src->tdmr_base;
    tdmr_info_target->tdmr_size = tdmr_info_src->tdmr_size;
    tdmr_info_target->pamt_1g_base = tdmr_info_src->pamt_1g_base;
    tdmr_info_target->pamt_1g_size = tdmr_info_src->pamt_1g_size;
    tdmr_info_target->pamt_2m_base = tdmr_info_src->pamt_2m_base;
    tdmr_info_target->pamt_2m_size = tdmr_info_src->pamt_2m_size;
    tdmr_info_target->pamt_4k_base = tdmr_info_src->pamt_4k_base;
    tdmr_info_target->pamt_4k_size = tdmr_info_src->pamt_4k_size;

    for(uint64_t j = 0; j < MAX_RESERVED_AREAS; j++)
    {
        tdmr_info_target->rsvd_areas[j].offset = tdmr_info_src->rsvd_areas[j].offset;
        tdmr_info_target->rsvd_areas[j].size = tdmr_info_src->rsvd_areas[j].size;
    }

}

static bool_t is_area_in_cmr(uint64_t area_start, uint64_t area_start_plus_size)
{
    sysinfo_table_t* sysinfo_table_ptr = get_sysinfo_table();

    uint64_t last_cmr_area_start = 0;
    uint64_t last_cmr_area_start_plus_size = 0;

    for (uint64_t i = 0; i < MAX_CMR; i++)
    {
        uint64_t cmr_area_start = sysinfo_table_ptr->cmr_data[i].cmr_base;
        uint64_t cmr_area_start_plus_size = sysinfo_table_ptr->cmr_data[i].cmr_base
                + sysinfo_table_ptr->cmr_data[i].cmr_size;

        if (sysinfo_table_ptr->cmr_data[i].cmr_size != 0)
        {
            if (cmr_area_start == last_cmr_area_start_plus_size)
            {
                cmr_area_start = last_cmr_area_start;
            }

            tdx_debug_assert(cmr_area_start_plus_size >= cmr_area_start);

            if ((area_start >= cmr_area_start) && (area_start_plus_size <= cmr_area_start_plus_size))
            {
                return true;
            }

            last_cmr_area_start = cmr_area_start;
            last_cmr_area_start_plus_size = cmr_area_start_plus_size;
        }

    }

    return false;
}

static api_error_type check_tdmr_area_addresses_and_size(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS], uint32_t i)
{
    // TDMR base address must be aligned on 1GB.
    //TDMR size must be greater than 0 and a whole multiple of 1GB.
    #ifdef SOURCE
    if (!is_addr_aligned_pwr_of_2(tdmr_info_copy[i].tdmr_base, _1GB) ||
        !is_addr_aligned_pwr_of_2(tdmr_info_copy[i].tdmr_size, _1GB) ||
        tdmr_info_copy[i].tdmr_size <= 0)
    {
        TDX_ERROR("TDMR_BASE[%d]=0x%llx or TDMR_SIZE[%d]=0x%llx are not 1GB aligned\n",
                i, tdmr_info_copy[i].tdmr_base, i, tdmr_info_copy[i].tdmr_size);
        return api_error_with_multiple_info(TDX_INVALID_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        __CPROVER_assume(is_addr_aligned_pwr_of_2(tdmr_info_copy[i].tdmr_base, _1GB) &&
                         is_addr_aligned_pwr_of_2(tdmr_info_copy[i].tdmr_size, _1GB) &&
                         (tdmr_info_copy[i].tdmr_size >= 0));
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
    if (!is_addr_aligned_pwr_of_2(tdmr_info_copy[i].tdmr_base, _1GB) ||
        !is_addr_aligned_pwr_of_2(tdmr_info_copy[i].tdmr_size, _1GB) ||
        tdmr_info_copy[i].tdmr_size <= 0)
    {
        __CPROVER_assert(is_addr_aligned_pwr_of_2(tdmr_info_copy[i].tdmr_base, _1GB) &&
                         is_addr_aligned_pwr_of_2(tdmr_info_copy[i].tdmr_size, _1GB) &&
                         (tdmr_info_copy[i].tdmr_size > 0), "TDMR address must be 1GB aligned and size greater than 0");
        TDX_ERROR("TDMR_BASE[%d]=0x%llx or TDMR_SIZE[%d]=0x%llx are not 1GB aligned\n",
                i, tdmr_info_copy[i].tdmr_base, i, tdmr_info_copy[i].tdmr_size);
        return api_error_with_multiple_info(TDX_INVALID_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // FLOW_PROOF

    #ifdef SOURCE
    // TDMR base address must comply with the platform’s maximum PA and their HKID bits must be 0.
    if (!is_pa_smaller_than_max_pa(tdmr_info_copy[i].tdmr_base) ||
         get_hkid_from_pa((pa_t)tdmr_info_copy[i].tdmr_base) != 0)
    {
        TDX_ERROR("TDMR_BASE[%d]=0x%llx doesn't comply with platform max PA = 0x%llx, or HKID=0x%x!=0\n",
                i, tdmr_info_copy[i].tdmr_base, BIT(get_global_data()->max_pa),
                get_hkid_from_pa((pa_t)tdmr_info_copy[i].tdmr_base));
        return api_error_with_multiple_info(TDX_INVALID_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
    __CPROVER_assume(is_pa_smaller_than_max_pa(tdmr_info_copy[i].tdmr_base)); 
    __CPROVER_assume(((tdmr_info_copy[i].tdmr_base & global_data.hkid_mask) >> global_data.hkid_start_bit) == 0);
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
    if (!is_pa_smaller_than_max_pa(tdmr_info_copy[i].tdmr_base) ||
        (((tdmr_info_copy[i].tdmr_base) >> global_data.hkid_start_bit) != 0)) {
        __CPROVER_assert(is_pa_smaller_than_max_pa(tdmr_info_copy[i].tdmr_base), "tdmr base pa is not smaller than max pa");
        __CPROVER_assert(((tdmr_info_copy[i].tdmr_base & global_data.hkid_mask) >> global_data.hkid_start_bit) == 0, "tdmr hkid shifted all the way is not 0");
        return api_error_with_multiple_info(TDX_INVALID_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // FLOW_PROOF

    #ifdef SOURCE
    // TDMR end address must comply with the platform’s maximum PA and their HKID bits must be 0.
    uint64_t tdmr_end = tdmr_info_copy[i].tdmr_base + tdmr_info_copy[i].tdmr_size - 1;
    if (!is_pa_smaller_than_max_pa(tdmr_end) || get_hkid_from_pa((pa_t)tdmr_end) != 0)
    {
        TDX_ERROR("TDMR_END[%d]=0x%llx doesn't comply with platform max PA = 0x%llx, or HKID=0x%x!=0\n",
                i, tdmr_end, BIT(get_global_data()->max_pa), get_hkid_from_pa((pa_t)tdmr_end));
        return api_error_with_multiple_info(TDX_INVALID_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        uint64_t tdmr_end = tdmr_info_copy[i].tdmr_base + tdmr_info_copy[i].tdmr_size - 1;
        __CPROVER_assume(((tdmr_end & global_data.hkid_mask) >> global_data.hkid_start_bit) == 0);
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF // SOPHIA HERE
        uint64_t tdmr_end = tdmr_info_copy[i].tdmr_base + tdmr_info_copy[i].tdmr_size - 1;
        if (!is_pa_smaller_than_max_pa(tdmr_end) || 
            (((tdmr_end & global_data.hkid_mask) >> global_data.hkid_start_bit) != 0)) {
            __CPROVER_assert(((tdmr_end & global_data.hkid_mask) >> global_data.hkid_start_bit) == 0, "tdmr end exceeds the maximum possible size");
        }
    #endif // FLOW_PROOF
    return TDX_SUCCESS;
}

static bool_t check_pamt_addresses_and_size(uint64_t pamt_base, uint64_t pamt_size,
                                       uint64_t entry_size, uint64_t tdmr_size)
{
    // PAMT size should not cause integer overflow when added to the base
    if (!is_valid_integer_range(pamt_base, pamt_size))
    {
        TDX_ERROR("PAMT size 0x%llx causes integer overflow when added to base 0x%llx\n", pamt_base, pamt_size);
        return false;
    }

    // PAMT base address must comply with the alignment requirements.
    if (!is_addr_aligned_pwr_of_2(pamt_base, _4KB) || !is_addr_aligned_pwr_of_2(pamt_size, _4KB))
    {
        TDX_ERROR("PAMT base=0x%llx or size=0x%llx are not 4KB aligned\n", pamt_base, pamt_size);
        return false;
    }

    // PAMT base address must comply with the platform’s maximum PA and their HKID bits must be 0.
    if (!is_pa_smaller_than_max_pa(pamt_base) || get_hkid_from_pa((pa_t)pamt_base) != 0)
    {
        TDX_ERROR("PAMT base=0x%llx doesn't comply with platform max PA = 0x%llx, or HKID=0x%x!=0\n",
                pamt_base, BIT(get_global_data()->max_pa), get_hkid_from_pa((pa_t)pamt_base));
        return false;
    }

    // PAMT end address must comply with the platform’s maximum PA and their HKID bits must be 0.
    uint64_t pamt_end = pamt_base + pamt_size - 1;
    if (!is_pa_smaller_than_max_pa(pamt_end) || get_hkid_from_pa((pa_t)pamt_end) != 0)
    {
        TDX_ERROR("PAMT end=0x%llx doesn't comply with platform max PA = 0x%llx, or HKID=0x%x!=0\n",
                pamt_end, BIT(get_global_data()->max_pa), get_hkid_from_pa((pa_t)pamt_end));
        return false;
    }

    // The size of each PAMT region must be large enough to contain the PAMT for its associated TDMR.
    if (pamt_size < ((tdmr_size / entry_size) * sizeof(pamt_entry_t)))
    {
        TDX_ERROR("PAMT size=0x%llx isn't big enough to contain entries (0x%llx) for current TDMR\n",
                pamt_size, (tdmr_size / entry_size) * sizeof(pamt_entry_t));
        return false;
    }

    return true;
}

static bool_t is_pamt_overlaps_available_area(tdmr_info_entry_t* tdmr_info_ptr,
                                              uint64_t pamt_base, uint64_t pamt_size)
{
    uint64_t available_start = tdmr_info_ptr->tdmr_base;
    uint64_t available_end;
    bool_t valid_rsvd_area = false;

    for (uint32_t j = 0; j < MAX_RESERVED_AREAS; j++)
    {
        uint64_t reserved_start = tdmr_info_ptr->tdmr_base + tdmr_info_ptr->rsvd_areas[j].offset;
        uint64_t reserved_end = reserved_start + tdmr_info_ptr->rsvd_areas[j].size;

        valid_rsvd_area = (tdmr_info_ptr->rsvd_areas[j].size != 0);

        // In case when no TDMR space left to check
        if (available_start == (tdmr_info_ptr->tdmr_base + tdmr_info_ptr->tdmr_size))
        {
            break;
        }

        // In case there's reserved area at the beginning of TDMR, or two consequent reserved areas
        // Continue to the next reserved area.
        if (valid_rsvd_area && (available_start == reserved_start))
        {
            available_start = reserved_end;
            continue;
        }

        if (!valid_rsvd_area) // NULL entry is last - no more reserved areas
        {
            available_end = tdmr_info_ptr->tdmr_base + tdmr_info_ptr->tdmr_size;
        }
        else
        {
            available_end = reserved_start;
        }

        uint64_t available_size = available_end - available_start;

        // At this point PAMT areas, TDMR reserved areas, and TDMR area
        // were already checked to not cause integer overflow
        if (is_overlap(pamt_base, pamt_size, available_start, available_size))
        {
            TDX_ERROR("TDMR: PAMT [0x%llx - 0x%llx] overlaps with available area [0x%llx - 0x%llx]\n",
                    pamt_base, pamt_base + pamt_size, available_start, available_end);
            return true;
        }

        if (!valid_rsvd_area) // NULL entry is last - no more reserved areas
        {
            break;
        }

        available_start = reserved_end;
    }

    if (valid_rsvd_area)
    {
        available_end = tdmr_info_ptr->tdmr_base + tdmr_info_ptr->tdmr_size;
        uint64_t available_size = available_end - available_start;

        // At this point PAMT areas, TDMR reserved areas, and TDMR area
        // were already checked to not cause integer overflow
        if ((available_size > 0) && is_overlap(pamt_base, pamt_size, available_start, available_size))
        {
            TDX_ERROR("TDMR: PAMT [0x%llx - 0x%llx] overlaps with available area [0x%llx - 0x%llx]\n",
                pamt_base, pamt_base + pamt_size, available_start, available_end);
            return true;
        }
    }

    return false;
}

static api_error_type check_pamt_overlaps_available_areas(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS], uint32_t i)
{
    // TDMRs non-reserved parts and PAMTs must not overlap (PAMTs may reside
    // within all TDMRs reserved areas).

    for (uint32_t j=0; j <= i ; j++)
    {
        if (is_pamt_overlaps_available_area(&tdmr_info_copy[j], tdmr_info_copy[i].pamt_4k_base, tdmr_info_copy[i].pamt_4k_size))
        {
            TDX_ERROR("TDMR[%d].PAMT_4KB overlaps available area in TDMR[%d]\n", i, j);
            return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)i, PT_4KB, (uint8_t)j, 0);
        }

        if (is_pamt_overlaps_available_area(&tdmr_info_copy[j], tdmr_info_copy[i].pamt_2m_base, tdmr_info_copy[i].pamt_2m_size))
        {
            TDX_ERROR("TDMR[%d].PAMT_2MB overlaps available area in TDMR[%d]\n", i, j);
            return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)i, PT_2MB, (uint8_t)j, 0);
        }

        if (is_pamt_overlaps_available_area(&tdmr_info_copy[j], tdmr_info_copy[i].pamt_1g_base, tdmr_info_copy[i].pamt_1g_size))
        {
            TDX_ERROR("TDMR[%d].PAMT_1GB overlaps available area in TDMR[%d]\n", i, j);
            return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)i, PT_1GB, (uint8_t)j, 0);
        }

        if (j < i)
        {
            if (is_pamt_overlaps_available_area(&tdmr_info_copy[i], tdmr_info_copy[j].pamt_4k_base, tdmr_info_copy[j].pamt_4k_size))
            {
                TDX_ERROR("TDMR[%d].PAMT_4KB overlaps available area in TDMR[%d]\n", j, i);
                return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)j, PT_4KB, (uint8_t)i, 0);
            }

            if (is_pamt_overlaps_available_area(&tdmr_info_copy[i], tdmr_info_copy[j].pamt_2m_base, tdmr_info_copy[j].pamt_2m_size))
            {
                TDX_ERROR("TDMR[%d].PAMT_2MB overlaps available area in TDMR[%d]\n", j, i);
                return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)j, PT_2MB, (uint8_t)i, 0);
            }

            if (is_pamt_overlaps_available_area(&tdmr_info_copy[i], tdmr_info_copy[j].pamt_1g_base, tdmr_info_copy[j].pamt_1g_size))
            {
                TDX_ERROR("TDMR[%d].PAMT_1GB overlaps available area in TDMR[%d]\n", j, i);
                return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)j, PT_1GB, (uint8_t)i, 0);
            }
        }

    }

    return TDX_SUCCESS;
}

static bool_t check_pamt_overlap(uint64_t pamt_base, uint64_t pamt_size,
        pamt_data_t pamt_data_array[], uint32_t i)
{
    if (is_overlap(pamt_base, pamt_size, pamt_data_array[i].pamt_4k_base, pamt_data_array[i].pamt_4k_size) ||
        is_overlap(pamt_base, pamt_size, pamt_data_array[i].pamt_2m_base, pamt_data_array[i].pamt_2m_size) ||
        is_overlap(pamt_base, pamt_size, pamt_data_array[i].pamt_1g_base, pamt_data_array[i].pamt_1g_size))
    {
        return true;
    }

    return false;
}

static api_error_type check_all_pamt_overlap(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS], uint32_t i,
        pamt_data_t pamt_data_array[])
{
    // PAMTs must not overlap with other PAMTs.
    for (uint32_t j = 0; j < i; j++)
    {

        if (check_pamt_overlap(tdmr_info_copy[i].pamt_4k_base, tdmr_info_copy[i].pamt_4k_size, pamt_data_array, j))
        {
            TDX_ERROR("TDMR[%d].PAMT_4KB overlaps other PAMT in TDMR[%d]\n", i, j);
            return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)i, PT_4KB, (uint8_t)j, 0);
        }
        if (check_pamt_overlap(tdmr_info_copy[i].pamt_2m_base, tdmr_info_copy[i].pamt_2m_size, pamt_data_array, j))
        {
            TDX_ERROR("TDMR[%d].PAMT_2MB overlaps other PAMT in TDMR[%d]\n", i, j);
            return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)i, PT_2MB, (uint8_t)j, 0);
        }

        if (check_pamt_overlap(tdmr_info_copy[i].pamt_1g_base, tdmr_info_copy[i].pamt_1g_size, pamt_data_array, j))
        {
            TDX_ERROR("TDMR[%d].PAMT_1GB overlaps other PAMT in TDMR[%d]\n", i, j);
            return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)i, PT_1GB, (uint8_t)j, 0);
        }
    }

    if (is_overlap(tdmr_info_copy[i].pamt_4k_base, tdmr_info_copy[i].pamt_4k_size,
                   tdmr_info_copy[i].pamt_2m_base, tdmr_info_copy[i].pamt_2m_size))
    {
        TDX_ERROR("TDMR[%d].PAMT_4KB overlaps PAMT_2MB\n", i);
        return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)i, PT_4KB, (uint8_t)i, 0);
    }

    if (is_overlap(tdmr_info_copy[i].pamt_4k_base, tdmr_info_copy[i].pamt_4k_size,
                   tdmr_info_copy[i].pamt_1g_base, tdmr_info_copy[i].pamt_1g_size))
    {
        TDX_ERROR("TDMR[%d].PAMT_4KB overlaps PAMT_1GB\n", i);
        return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)i, PT_4KB, (uint8_t)i, 0);
    }

    if (is_overlap(tdmr_info_copy[i].pamt_2m_base, tdmr_info_copy[i].pamt_2m_size,
                   tdmr_info_copy[i].pamt_1g_base, tdmr_info_copy[i].pamt_1g_size))
    {
        TDX_ERROR("TDMR[%d].PAMT_2MB overlaps PAMT_1GB\n", i);
        return api_error_with_multiple_info(TDX_PAMT_OVERLAP, (uint8_t)i, PT_2MB, (uint8_t)i, 0);
    }

    return TDX_SUCCESS;
}

static api_error_type check_pamt_addresses(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS], uint32_t i)
{
    // For each PAMT region (1G, 2M and 4K) of each TDMR:
    //  PAMT base address must comply with the alignment requirements.
    //  PAMT base address must comply with the platform’s maximum PA and their HKID bits must be 0.
    //  The size of each PAMT region must be large enough to contain the PAMT for its associated TDMR.

    if (!check_pamt_addresses_and_size(tdmr_info_copy[i].pamt_1g_base,
            tdmr_info_copy[i].pamt_1g_size, _1GB, tdmr_info_copy[i].tdmr_size))
    {
        TDX_ERROR("TDMR[%d].PAMT_1GB info is invalid\n", i);
        return api_error_with_multiple_info(TDX_INVALID_PAMT, (uint8_t)i, PT_1GB, 0, 0);
    }

    if (!check_pamt_addresses_and_size(tdmr_info_copy[i].pamt_2m_base,
            tdmr_info_copy[i].pamt_2m_size, _2MB, tdmr_info_copy[i].tdmr_size))
    {
        TDX_ERROR("TDMR[%d].PAMT_2MB info is invalid\n", i);
        return api_error_with_multiple_info(TDX_INVALID_PAMT, (uint8_t)i, PT_2MB, 0, 0);
    }

    if (!check_pamt_addresses_and_size(tdmr_info_copy[i].pamt_4k_base,
            tdmr_info_copy[i].pamt_4k_size, _4KB, tdmr_info_copy[i].tdmr_size))
    {
        TDX_ERROR("TDMR[%d].PAMT_4KB info is invalid\n", i);
        return api_error_with_multiple_info(TDX_INVALID_PAMT, (uint8_t)i, PT_4KB, 0, 0);
    }

    return TDX_SUCCESS;
}

static api_error_type check_pamt_in_cmr(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS], uint32_t i)
{
    // PAMTs must be contained in convertible memory, i.e., in CMRs.

    if (!is_area_in_cmr(tdmr_info_copy[i].pamt_1g_base,
            tdmr_info_copy[i].pamt_1g_base + tdmr_info_copy[i].pamt_1g_size))
    {
        TDX_ERROR("TDMR[%d].PAMT_1GB info is not contained in any CMR\n", i);
        return api_error_with_multiple_info(TDX_PAMT_OUTSIDE_CMRS, (uint8_t)i, PT_1GB, 0, 0);
    }

    if (!is_area_in_cmr(tdmr_info_copy[i].pamt_2m_base,
            tdmr_info_copy[i].pamt_2m_base + tdmr_info_copy[i].pamt_2m_size))
    {
        TDX_ERROR("TDMR[%d].PAMT_2MB info is not contained in any CMR\n", i);
        return api_error_with_multiple_info(TDX_PAMT_OUTSIDE_CMRS, (uint8_t)i, PT_2MB, 0, 0);
    }

    if (!is_area_in_cmr(tdmr_info_copy[i].pamt_4k_base,
            tdmr_info_copy[i].pamt_4k_base + tdmr_info_copy[i].pamt_4k_size))
    {
        TDX_ERROR("TDMR[%d].PAMT_4KB info is not contained in any CMR\n", i);
        return api_error_with_multiple_info(TDX_PAMT_OUTSIDE_CMRS, (uint8_t)i, PT_4KB, 0, 0);
    }

    return TDX_SUCCESS;

}

static api_error_type check_tdmr_pamt_areas(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS],
        uint32_t  i, pamt_data_t pamt_data_array[])
{
    api_error_type err;
    if (((err = check_pamt_addresses(tdmr_info_copy, i)) != TDX_SUCCESS))
    {
        TDX_ERROR("TDMR[%d] PAMT addresses/sizes are incorrect\n", i);
        return err;
    }
    if (((err = check_all_pamt_overlap(tdmr_info_copy, i,pamt_data_array)) != TDX_SUCCESS))
    {
        TDX_ERROR("TDMR[%d] PAMT areas are overlapping\n", i);
        return err;
    }

    if (((err = check_pamt_overlaps_available_areas(tdmr_info_copy, i)) != TDX_SUCCESS))
    {
        TDX_ERROR("TDMR[%d] PAMT areas are not contained in reserved area\n", i);
        return err;
    }

    if (((err = check_pamt_in_cmr(tdmr_info_copy, i)) != TDX_SUCCESS))
    {
        TDX_ERROR("TDMR[%d] PAMT areas are not contained in convertible areas (CMR)\n", i);
        return err;
    }

    return TDX_SUCCESS;
}

static api_error_type check_tdmr_reserved_areas(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS], uint32_t i)
{
    // For each reserved area within TDMR:

    for (uint32_t j = 0; j < MAX_RESERVED_AREAS; j++)
    {
        uint64_t area_offset = tdmr_info_copy[i].rsvd_areas[j].offset;
        uint64_t area_size = tdmr_info_copy[i].rsvd_areas[j].size;
        uint64_t prev_area_offset, prev_area_size;

        // A NULL reserved area (indicated by a size of 0) may only be followed by other NULL reserved areas.
        if (area_size == 0)
        {
            if (j < MAX_RESERVED_AREAS-1 && tdmr_info_copy[i].rsvd_areas[j+1].size != 0)
            {
                return api_error_with_multiple_info(TDX_NON_ORDERED_RESERVED_IN_TDMR,
                        (uint8_t)i, (uint8_t)j, 0, 0);
            }
        }
        else
        {
            // Check for integer overflow
            if (!is_valid_integer_range(area_offset, area_size))
            {
                TDX_ERROR("TDMR[%d]: integer overflow on reserved area %d\n", i, j);
                return api_error_with_multiple_info(TDX_INVALID_RESERVED_IN_TDMR,
                        (uint8_t)i, (uint8_t)j, 0, 0);
            }

            if (j > 0)
            {
                prev_area_offset = tdmr_info_copy[i].rsvd_areas[j-1].offset;
                prev_area_size = tdmr_info_copy[i].rsvd_areas[j-1].size;
            }

            // Reserved areas within TDMR must be sorted in an ascending offset order.

            if ((j > 0) && (area_offset < prev_area_offset))
            {
                TDX_ERROR("TDMR[%d]: RSVD_AREA[%d]=0x%llx is smaller than RSVD_AREA[%d]=0x%llx\n",
                        i, j, area_offset, j-1, prev_area_offset);
                return api_error_with_multiple_info(TDX_NON_ORDERED_RESERVED_IN_TDMR,
                        (uint8_t)i, (uint8_t)j, 0, 0);
            }


            // Reserved areas must not overlap.
            // Check will be correct due to previous (ascencion) check correctness.
            if ((j > 0) && (area_offset < prev_area_offset + prev_area_size))
            {
                TDX_ERROR("TDMR[%d]: RSVD_AREA[%d] (from 0x%llx to 0x%llx) overlaps RSVD_AREA[%d] at 0x%llx\n",
                        i, j-1, prev_area_offset, prev_area_offset + prev_area_size, j, area_offset);

                return api_error_with_multiple_info(TDX_NON_ORDERED_RESERVED_IN_TDMR,
                        (uint8_t)i, (uint8_t)j, 0, 0);
            }

            // Offset and size must comply with the alignment and granularity requirements.
            // TDMR base address and size must comply with the alignment and granularity requirements.
            if (!is_addr_aligned_pwr_of_2(area_offset, _4KB) ||
                !is_addr_aligned_pwr_of_2(area_size, _4KB))
            {
                TDX_ERROR("TDMR[%d]: RSVD_AREA[%d] offset 0x%llx or size 0x%llx are not 4KB aligned\n",
                        i, j, area_offset, area_size);
                return api_error_with_multiple_info(TDX_INVALID_RESERVED_IN_TDMR,
                        (uint8_t)i, (uint8_t)j, 0, 0);
            }

            // Reserved areas must be fully contained within their TDMR.
            uint64_t tdmr_start =  tdmr_info_copy[i].tdmr_base;
            uint64_t tdmr_end = tdmr_info_copy[i].tdmr_base + tdmr_info_copy[i].tdmr_size;
            if (!is_valid_integer_range(tdmr_start, area_offset))
            {
                TDX_ERROR("TDMR[%d]: integer overflow on reserved area %d\n", i, j);
                return api_error_with_multiple_info(TDX_INVALID_RESERVED_IN_TDMR,
                        (uint8_t)i, (uint8_t)j, 0, 0);
            }
            uint64_t rsvd_start = tdmr_start + area_offset;
            if (!is_valid_integer_range(rsvd_start, area_size))
            {
                TDX_ERROR("TDMR[%d]: integer overflow on reserved area %d\n", i, j);
                return api_error_with_multiple_info(TDX_INVALID_RESERVED_IN_TDMR,
                        (uint8_t)i, (uint8_t)j, 0, 0);
            }
            uint64_t rsvd_end = rsvd_start + area_size;
            if (rsvd_start < tdmr_start || rsvd_end > tdmr_end)
            {
                TDX_ERROR("RSVD_AREA[%d] [0x%llx - 0x%llx] is not contained in TDMR[%d]: [0x%llx - 0x%llx]\n",
                        j, rsvd_start, rsvd_end, i, tdmr_start, tdmr_end);
                return api_error_with_multiple_info(TDX_INVALID_RESERVED_IN_TDMR,
                        (uint8_t)i, (uint8_t)j, 0, 0);
            }
        }
    }

    return TDX_SUCCESS;
}

static api_error_type check_tdmr_available_areas(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS], uint32_t i)
{
    // TDMRs non-reserved parts must be contained in convertible memory, i.e., in CMRs.

    uint64_t available_start = tdmr_info_copy[i].tdmr_base;
    uint64_t available_end;
    bool_t valid_rsvd_area = false;

    for (uint32_t j = 0; j < MAX_RESERVED_AREAS; j++)
    {
        uint64_t reserved_start = tdmr_info_copy[i].tdmr_base + tdmr_info_copy[i].rsvd_areas[j].offset;
        uint64_t reserved_start_plus_size = reserved_start + tdmr_info_copy[i].rsvd_areas[j].size;

        valid_rsvd_area = (tdmr_info_copy[i].rsvd_areas[j].size != 0);

        // In case when no TDMR space left to check
        if (available_start == (tdmr_info_copy[i].tdmr_base + tdmr_info_copy[i].tdmr_size))
        {
            break;
        }

        // In case there's reserved area at the beginning of TDMR, or two consequent reserved areas
        // Continue to the next reserved area.
        if (valid_rsvd_area && (available_start == reserved_start))
        {
            available_start = reserved_start_plus_size;
            continue;
        }

        if (!valid_rsvd_area) // NULL entry is last - no more reserved areas
        {
            available_end = tdmr_info_copy[i].tdmr_base + tdmr_info_copy[i].tdmr_size;
        }
        else
        {
            available_end = reserved_start;
        }

        if (!is_area_in_cmr(available_start, available_end))
        {
            TDX_ERROR("TDMR[%d]: Non-reserved area [0x%llx - 0x%llx] is not in any CMR\n",
                    i, available_start, available_end);

            return api_error_with_multiple_info(TDX_TDMR_OUTSIDE_CMRS, (uint8_t)i, 0, 0, 0);
        }

        if (!valid_rsvd_area) // NULL entry is last - no more reserved areas
        {
            break;
        }

        available_start = reserved_start_plus_size;
    }

    if (valid_rsvd_area)
    {
        available_end = tdmr_info_copy[i].tdmr_base + tdmr_info_copy[i].tdmr_size;
        if ((available_end > available_start) && !is_area_in_cmr(available_start, available_end))
        {
            TDX_ERROR("TDMR[%d]: Non-reserved area [0x%llx - 0x%llx] is not in any CMR\n",
                    i, available_start, available_end);

            return api_error_with_multiple_info(TDX_TDMR_OUTSIDE_CMRS, (uint8_t)i, 0, 0, 0);
        }
    }

    return TDX_SUCCESS;
}

static void set_tdmr_info_in_global_data(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS], uint32_t i)
{
    tdx_module_global_t* global_data_ptr = get_global_data();

    global_data_ptr->tdmr_table[i].base = tdmr_info_copy[i].tdmr_base;
    global_data_ptr->tdmr_table[i].size = tdmr_info_copy[i].tdmr_size;
    global_data_ptr->tdmr_table[i].last_initialized = global_data_ptr->tdmr_table[i].base;
    global_data_ptr->tdmr_table[i].lock = 0;
    global_data_ptr->tdmr_table[i].pamt_1g_base = tdmr_info_copy[i].pamt_1g_base;
    global_data_ptr->tdmr_table[i].pamt_2m_base = tdmr_info_copy[i].pamt_2m_base;
    global_data_ptr->tdmr_table[i].pamt_4k_base = tdmr_info_copy[i].pamt_4k_base;
    global_data_ptr->tdmr_table[i].num_of_pamt_blocks = (uint32_t)(tdmr_info_copy[i].tdmr_size / _1GB);

    global_data_ptr->tdmr_table[i].num_of_rsvd_areas = 0;
    for (uint32_t j = 0; j < MAX_RESERVED_AREAS; j++)
    {
        global_data_ptr->tdmr_table[i].rsvd_areas[j].offset = tdmr_info_copy[i].rsvd_areas[j].offset;
        global_data_ptr->tdmr_table[i].rsvd_areas[j].size = tdmr_info_copy[i].rsvd_areas[j].size;

        if (global_data_ptr->tdmr_table[i].rsvd_areas[j].size == 0)
        {
            // NULL entry is last
            break;
        }

        global_data_ptr->tdmr_table[i].num_of_rsvd_areas++;
    }
}

static api_error_type check_and_set_tdmrs(tdmr_info_entry_t tdmr_info_copy[MAX_TDMRS],
        uint64_t i, pamt_data_t pamt_data_array[])
{
    // Check TDMR_INFO and update the internal TDMR_TABLE with TDMR, reserved areas and PAMT setup:

    uint64_t tdmr_base = tdmr_info_copy[i].tdmr_base;
    uint64_t prev_tdmr_base, prev_tdmr_size;

    // Check for integer overflow
    #ifdef SOURCE
    if (!is_valid_integer_range(tdmr_info_copy[i].tdmr_base, tdmr_info_copy[i].tdmr_size))
    {
        TDX_ERROR("TDMR[%d]: base+size cues integer overflow\n", i);
        return api_error_with_multiple_info(TDX_INVALID_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
    __CPROVER_assume(is_valid_integer_range(tdmr_info_copy[i].tdmr_base, tdmr_info_copy[i].tdmr_size)); 
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
    if (!is_valid_integer_range(tdmr_info_copy[i].tdmr_base, tdmr_info_copy[i].tdmr_size))
    {
        __CPROVER_assert(is_valid_integer_range(tdmr_info_copy[i].tdmr_base, tdmr_info_copy[i].tdmr_size), "TMDR base+size cues integer overflow"); 
        TDX_ERROR("TDMR[%d]: base+size cues integer overflow\n", i);
        return api_error_with_multiple_info(TDX_INVALID_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // FLOW_PROOF

    #ifdef FLOW_PROOF
    #else 
    if (i > 0)
    {
        prev_tdmr_base = tdmr_info_copy[i-1].tdmr_base;
        prev_tdmr_size = tdmr_info_copy[i-1].tdmr_size;
    }
    #endif // FLOW_PROOF

    #ifdef SOURCE
    // TDMRs must be sorted in an ascending base address order.
    if ((i > 0) && tdmr_base < prev_tdmr_base)
    {
        TDX_ERROR("TDMR_BASE[%d]=0x%llx is smaller than TDMR_BASE[%d]=0x%llx\n",
                i, tdmr_info_copy[i].tdmr_base, i-1, tdmr_info_copy[i-1].tdmr_base);
        return api_error_with_multiple_info(TDX_NON_ORDERED_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
    __CPROVER_assume(tdmr_base >= prev_tdmr_base);
    #endif // MODDULAR_PROOF

    #ifdef FLOW_PROOF
        if ((i > 0) && tdmr_base < prev_tdmr_base)
    {
        __CPROVER_assert((i > 0) && (tdmr_base >= prev_tdmr_base), "TDMRs must be sorted in an ascending base address order");
        TDX_ERROR("TDMR_BASE[%d]=0x%llx is smaller than TDMR_BASE[%d]=0x%llx\n",
                i, tdmr_info_copy[i].tdmr_base, i-1, tdmr_info_copy[i-1].tdmr_base);
        return api_error_with_multiple_info(TDX_NON_ORDERED_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // FLOW_PROOF

    // TDMRs must not overlap with other TDMRs.
    // Check will be correct due to previous (ascension) check correctness.
    #ifdef SOURCE
    if ((i > 0) && (tdmr_base < prev_tdmr_base + prev_tdmr_size))
    {
        TDX_ERROR("TDMR[%d]: (from 0x%llx to 0x%llx) overlaps TDMR[%d] at 0x%llx\n",
                i-1, prev_tdmr_base, prev_tdmr_base + prev_tdmr_size, i, tdmr_base);
        return api_error_with_multiple_info(TDX_NON_ORDERED_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
    __CPROVER_assume((tdmr_base >= prev_tdmr_base + prev_tdmr_size));
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
    if ((i > 0) && (tdmr_base < prev_tdmr_base + prev_tdmr_size))
    {
        __CPROVER_assert((tdmr_base >= prev_tdmr_base + prev_tdmr_size), "TDMRs must not overlap with other TDMRs");
        TDX_ERROR("TDMR[%d]: (from 0x%llx to 0x%llx) overlaps TDMR[%d] at 0x%llx\n",
                i-1, prev_tdmr_base, prev_tdmr_base + prev_tdmr_size, i, tdmr_base);
        return api_error_with_multiple_info(TDX_NON_ORDERED_TDMR, (uint8_t)i, 0, 0, 0);
    }
    #endif // FLOW_PROOF
    
    #ifdef SOURCE
    api_error_type err;
    if ((err = check_tdmr_area_addresses_and_size(tdmr_info_copy, (uint32_t)i)) != TDX_SUCCESS)
    {
        return err;
    }
    #endif // SOURCE

    #ifdef MODULAR_PROOF
        api_error_type err;
        err = check_tdmr_area_addresses_and_size(tdmr_info_copy, (uint32_t)i);
    #endif //MODULAR_PROOF

    #ifdef FLOW_PROOF
        api_error_type err;
        if ((err = check_tdmr_area_addresses_and_size(tdmr_info_copy, (uint32_t)i)) != TDX_SUCCESS)
        {
            return err;
        }
    #endif // FLOW_PROOF

    // if ((err = check_tdmr_reserved_areas(tdmr_info_copy, (uint32_t)i)) != TDX_SUCCESS)
    // {
    //     return err;
    // }
    // if ((err = check_tdmr_pamt_areas(tdmr_info_copy, (uint32_t)i, pamt_data_array)) != TDX_SUCCESS)
    // {
    //     return err;
    // }

    // if ((err = check_tdmr_available_areas(tdmr_info_copy, (uint32_t)i)) != TDX_SUCCESS)
    // {
    //     return err;
    // }
    // All checks passed for current TDMR, fill it in our module data:

    //set_tdmr_info_in_global_data(tdmr_info_copy, (uint32_t)i);

    return TDX_SUCCESS;

}

api_error_type tdh_sys_config(uint64_t tdmr_info_array_pa,
                             uint64_t num_of_tdmr_entries,
                             hkid_api_input_t global_private_hkid)
{
    // Temporary Variables
    tdmr_info_entry_t*   tdmr_info_p;   // Pointer to TDMR info
    tdmr_info_entry_t*   tdmr_info_copy;// Pointer to TDMR info array
    bool_t               tdmr_info_p_init = false;
    pa_t                 tdmr_info_pa = {.raw = tdmr_info_array_pa};  // Physical address of an array of physical addresses of the TDMR info structure
    uint64_t*            tdmr_pa_array = NULL; // Pointer to an array of physical addresses of the TDMR info structure
    uint16_t             hkid = global_private_hkid.hkid;
    bool_t               global_lock_acquired = false;
    #ifdef SOURCE
    tdx_module_global_t* tdx_global_data_ptr = get_global_data();
    #else 
    tdx_module_global_t* tdx_global_data_ptr = &global_data;
    #endif // SOURCE

    api_error_type       retval = TDX_SYS_BUSY;
    #ifdef SOURCE
    if (acquire_sharex_lock_ex(&tdx_global_data_ptr->global_lock) != LOCK_RET_SUCCESS)
    {
        TDX_ERROR("Failed to acquire global lock\n");
        retval = TDX_SYS_BUSY;
        goto EXIT;
    }
    #endif // SOURCE
    #ifdef MODULAR_PROOF
        __CPROVER_assume(tdx_global_data_ptr->global_lock.raw == SHAREX_FREE); 
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
        if (tdx_global_data_ptr->global_lock.raw != SHAREX_FREE)
        {
            __CPROVER_assert(tdx_global_data_ptr->global_lock.raw == SHAREX_FREE, "obtain lock for global data pointer"); 
            TDX_ERROR("Failed to acquire global lock\n");
            retval = TDX_SYS_BUSY;
            goto EXIT;
        }
    #endif // FLOW_PROOF
    global_lock_acquired = true;

    #ifdef SOURCE
    if (tdx_global_data_ptr->global_state.sys_state != SYSINIT_DONE)
    {
        TDX_ERROR("Wrong sys_init state: %d\n", tdx_global_data_ptr->global_state.sys_state);
        retval = TDX_SYS_CONFIG_NOT_PENDING;
        goto EXIT;
    }

    if (tdx_global_data_ptr->num_of_init_lps < tdx_global_data_ptr->num_of_lps)
    {
        TDX_ERROR("Num of initialized lps %d is smaller than total num of lps %d\n",
                    tdx_global_data_ptr->num_of_init_lps, tdx_global_data_ptr->num_of_lps);

        retval = TDX_SYS_CONFIG_NOT_PENDING;
        goto EXIT;
    }

    retval = shared_hpa_check_with_pwr_2_alignment(tdmr_info_pa, TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT);
    if (retval != TDX_SUCCESS)
    {
        retval = api_error_with_operand_id(retval, OPERAND_ID_RCX);
        TDX_ERROR("TDMR info array PA is not a valid shared HPA pa=0x%llx, error=0x%llx\n", tdmr_info_pa.raw, retval);
        goto EXIT;
    }

    if (num_of_tdmr_entries > MAX_TDMRS)
    {
        TDX_ERROR("Num of TDMR entries %llu bigger than MAX_TDMRS (%d)\n", num_of_tdmr_entries, MAX_TDMRS);
        retval = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
        goto EXIT;
    }

    if (num_of_tdmr_entries < 1)
    {
        TDX_ERROR("Num of TDMR entries %llu smaller than 1 \n", num_of_tdmr_entries);
        retval = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
        goto EXIT;
    }

    if ((global_private_hkid.reserved != 0) || !is_private_hkid(hkid))
    {
        TDX_ERROR("HKID 0x%x is not private\n", hkid);
        retval = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_R8);
        goto EXIT;
    }
    #endif // SOURCE 

    #ifdef MODULAR_PROOF
        __CPROVER_assume(global_data.global_state.sys_state == SYSINIT_DONE); 
        __CPROVER_assume(global_data.num_of_init_lps == global_data.num_of_lps);
        __CPROVER_assume(is_addr_aligned_pwr_of_2(tdmr_info_array_pa, TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT));
        __CPROVER_assume(num_of_tdmr_entries <= MAX_TDMRS);
        __CPROVER_assume(num_of_tdmr_entries >= 1);
        __CPROVER_assume((global_private_hkid.reserved == 0));
        __CPROVER_assume((hkid >= global_data.private_hkid_min) && 
                        (hkid <= global_data.private_hkid_max));
    #endif // MODULAR_PROOF

    #ifdef FLOW_PROOF
        if (tdx_global_data_ptr->global_state.sys_state != SYSINIT_DONE)
        {
            __CPROVER_assert(tdx_global_data_ptr->global_state.sys_state == SYSINIT_DONE, "wrong init state"); 
            TDX_ERROR("Wrong sys_init state: %d\n", tdx_global_data_ptr->global_state.sys_state);
            retval = TDX_SYS_CONFIG_NOT_PENDING;
            goto EXIT;
        }

        if (tdx_global_data_ptr->num_of_init_lps < tdx_global_data_ptr->num_of_lps)
        {
            __CPROVER_assert(tdx_global_data_ptr->num_of_init_lps == tdx_global_data_ptr->num_of_lps, "num of initialized lps is smaller than total num of lps");
            TDX_ERROR("Num of initialized lps %d is smaller than total num of lps %d\n",
                        tdx_global_data_ptr->num_of_init_lps, tdx_global_data_ptr->num_of_lps);

            retval = TDX_SYS_CONFIG_NOT_PENDING;
            goto EXIT;
        }

        if (!is_addr_aligned_pwr_of_2(tdmr_info_pa.raw, TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT))
        {
            __CPROVER_assert(is_addr_aligned_pwr_of_2(tdmr_info_pa.raw, TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT), "TDMR info array PA is not a valid shared HPA");
            retval = api_error_with_operand_id(retval, OPERAND_ID_RCX);
            TDX_ERROR("TDMR info array PA is not a valid shared HPA pa=0x%llx, error=0x%llx\n", tdmr_info_pa.raw, retval);
            goto EXIT;
        }

        if (num_of_tdmr_entries > MAX_TDMRS)
        {
            __CPROVER_assert(num_of_tdmr_entries <= MAX_TDMRS, "Num of TDMR entries bigger than MAX_TDMRS");
            TDX_ERROR("Num of TDMR entries %llu bigger than MAX_TDMRS (%d)\n", num_of_tdmr_entries, MAX_TDMRS);
            retval = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
            goto EXIT;
        }

        if (num_of_tdmr_entries < 1)
        {
            __CPROVER_assert(num_of_tdmr_entries >= 1, "Num of TDMR entries smaller than 1");
            TDX_ERROR("Num of TDMR entries %llu smaller than 1 \n", num_of_tdmr_entries);
            retval = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RDX);
            goto EXIT;
        }

        if ((global_private_hkid.reserved != 0) || 
            !((hkid >= global_data.private_hkid_min) && (hkid <= global_data.private_hkid_max)))
        {
            __CPROVER_assert((global_private_hkid.reserved == 0), "HKID is not private 1");
            __CPROVER_assert((hkid >= global_data.private_hkid_min) && (hkid <= global_data.private_hkid_max), "HKID is not private 2");
            TDX_ERROR("HKID 0x%x is not private\n", hkid);
            retval = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_R8);
            goto EXIT;
        }
    #endif // FLOW_PROOF
    
    #ifdef SOURCE
    tdx_global_data_ptr->kot.entries[hkid].state = KOT_STATE_HKID_RESERVED;
    #else 
    tdx_global_data_ptr->kot.entries[hkid & HKID_MASK].state = KOT_STATE_HKID_RESERVED;
    #endif // SOURCE 

    tdx_global_data_ptr->hkid = hkid;

    #ifdef SOURCE
    tdmr_pa_array = map_pa(tdmr_info_pa.raw_void, TDX_RANGE_RO);
    #else 
    // SOPHIA: index of the tdmr?
    tdmr_pa_array = hkid & HKID_MASK;
    #endif // SOURCE
    tdmr_info_p_init = true;

    // map only 2 tdmr entries each time
    pa_t tdmr_entry;
    pamt_data_t pamt_data_array[MAX_TDMRS];
    api_error_type err; 
    tdmr_info_copy = tdx_global_data_ptr->tdmr_info_copy;
    
    for(uint64_t i = 0; i < num_of_tdmr_entries; i++)
    {
        #ifdef SOURCE
        tdmr_entry.raw = tdmr_pa_array[i];
        #else 
        tdmr_entry.raw = &(tables[i].tdmr_table);
        #endif // SOURCE

        #ifdef SOURCE
        retval = shared_hpa_check_with_pwr_2_alignment(tdmr_entry, TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT);
        if (retval != TDX_SUCCESS)
        {
            retval = api_error_with_operand_id(retval, OPERAND_ID_RCX);
            TDX_ERROR("TDMR entry PA is not a valid shared HPA pa=0x%llx, error=0x%llx\n", tdmr_entry.raw, retval);
            goto EXIT;
        }
        #endif // SOURCE

        #ifdef MODULAR_PROOF
            __CPROVER_assume(shared_hpa_check_with_pwr_2_alignment(tdmr_entry, TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT)); 
        #endif // MODULAR_PROOF

        // #ifdef FLOW_PROOF
        //     retval = shared_hpa_check_with_pwr_2_alignment(tdmr_entry, TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT);
        //     if (retval != TDX_SUCCESS)
        //     {
        //         __CPROVER_assert(shared_hpa_check_with_pwr_2_alignment(tdmr_entry, TDMR_INFO_ENTRY_PTR_ARRAY_ALIGNMENT), "TDMR entry PA is not a va lid shared HPA");
        //         retval = api_error_with_operand_id(retval, OPERAND_ID_RCX);
        //         TDX_ERROR("TDMR entry PA is not a valid shared HPA pa=0x%llx, error=0x%llx\n", tdmr_entry.raw, retval);
        //         goto EXIT;
        //     }
        // #endif // FLOW_PROOF

        #ifdef SOURCE
        tdmr_info_p = (tdmr_info_entry_t*)map_pa(tdmr_entry.raw_void, TDX_RANGE_RO);
        copy_tdmr_info_entry (tdmr_info_p, &tdmr_info_copy[i]);
        free_la(tdmr_info_p);
        #endif // SOURCE
        
        // SOPHIA: keyhole mapping not fully verified yet
        #ifdef MODULAR_PROOF
        tdmr_info_p = (tdmr_info_entry_t*)&(tables[i].tdmr_table); 
        copy_tdmr_info_entry (tdmr_info_p, &tdmr_info_copy[i]);
        #endif // MODULAR_PROOF

        #ifdef SOURCE
        if ((err = check_and_set_tdmrs(tdmr_info_copy, i, pamt_data_array)) != TDX_SUCCESS)
        {
            TDX_ERROR("Check and set TDMRs failed\n");
            retval = err;
            goto EXIT;
        }
        #endif // SOURCE

        #ifdef MODULAR_PROOF
            err = check_and_set_tdmrs(tdmr_info_copy, i, pamt_data_array);
            __CPROVER_assert(err == TDX_SUCCESS, "check and set failed"); 
        #endif // MODULAR_PROOF

        #ifdef FLOW_PROOF
            if ((err = check_and_set_tdmrs(tdmr_info_copy, i, pamt_data_array)) != TDX_SUCCESS)
            {
                __CPROVER_assert(check_and_set_tdmrs(tdmr_info_copy, i, pamt_data_array) == TDX_SUCCESS, "Check and set TDMRs failed");
                TDX_ERROR("Check and set TDMRs failed\n");
                retval = err;
                goto EXIT;
            }
        #endif // FLOW_PROOF

        #ifdef FLOW_PROOF
        #else 
        update_pamt_array(tdmr_info_copy, pamt_data_array, (uint32_t)i); // save tdmr's pamt data
        #endif // SOURCE
    }
    #ifdef SOURCE
    tdx_global_data_ptr->num_of_tdmr_entries = (uint32_t)num_of_tdmr_entries;
    #endif // SOURCE

    #ifdef MODULAR_PROOF
    global_data.num_of_tdmr_entries = (uint32_t)num_of_tdmr_entries; 
    #endif // SOURCE

    // ALL_CHECKS_PASSED:  The function is guaranteed to succeed

    // Complete CPUID handling
    #ifdef FLOW_PROOF
    #else 
    complete_cpuid_handling(tdx_global_data_ptr);
    #endif // SOURCE

    // Prepare state variables for TDHSYSKEYCONFIG
    #ifdef FLOW_PROOF
    #else 
    tdx_global_data_ptr->pkg_config_bitmap = 0ULL;
    #endif // FLOW_PROOF

    // Mark the system initialization as done
    tdx_global_data_ptr->global_state.sys_state = SYSCONFIG_DONE;
    retval = TDX_SUCCESS;

EXIT:

    // SOPHIA: never officially lock or have real memory so can abstract away
    #ifdef SOURCE
    if (global_lock_acquired)
    {
        release_sharex_lock_ex(&tdx_global_data_ptr->global_lock);
    }

    if (tdmr_info_p_init)
    {
        free_la(tdmr_pa_array);
    }
    #endif // SOURCE
    return retval;
}

