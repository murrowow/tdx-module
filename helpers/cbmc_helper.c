#include "helpers/cbmc_helper.h"
#include "src/common/helpers/helpers.h"

pamt_entry_t * get_pamt_entry(uint64_t target_tdr_pa, hkid_api_input_t hkid_info)
{
    __CPROVER_assume(target_tdr_pa != 0);
    __CPROVER_assume(hkid_info.hkid != 0);
    __CPROVER_assume(hkid_info.reserved == 0);

    tdx_module_global_t * global_data = get_global_data();

    // TDR related variables
    pa_t                  tdr_pa;                   // TDR physical address
    tdr_t               * tdr_ptr;                  // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;           // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;       // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;  // Indicate TDR is locked

    //uint16_t              td_hkid;
    //bool_t                kot_locked_flag = false;  // Indicate KOT is locked

    api_error_type        return_val = UNINITIALIZE_ERROR;

    tdr_pa.raw = target_tdr_pa;
    check_lock_and_map_explicit_tdr(tdr_pa, //input target_tdr_pa
                                OPERAND_ID_RCX, //constant
                                TDX_RANGE_RW, //constant
                                TDX_LOCK_EXCLUSIVE, //constant
                                PT_NDA, //constant 
                                &tdr_pamt_block,
                                &tdr_pamt_entry_ptr,
                                &tdr_locked_flag,
                                &tdr_ptr);

    /* pamt_get_block(tdr_pa, tdr_pamt_block); 

    api_error_code_e errc = pamt_walk(hpa, *pamt_block, lock_type, leaf_size,
                                      walk_to_leaf_size, is_guest, &pamt_entry_lp);*/
    return tdr_pamt_entry_ptr;
}

/*                 
api_error_type check_lock_and_map_explicit_tdr(
        tdr_pa, 
        uint64_t operand_id, //OPERAND_ID_RCX
        mapping_type_t mapping_type, //TDX_RANGE_RW
        lock_type_t lock_type, //TDX_LOCK_EXCLUSIVE
        page_type_t expected_pt, //PT_NDA
        pamt_block_t* pamt_block, //tdr_pamt_block
        pamt_entry_t** pamt_entry, //tdr_pamt_entry_ptr
        bool_t* is_locked, //tdr_locked_flag
        tdr_t** tdr_p //tdr_ptr
        )
{
    return check_lock_and_map_explicit_private_4k_hpa(tdr_pa, OPERAND_ID_RCX, NULL, TDX_RANGE_RW,
            TDX_LOCK_EXCLUSIVE, PT_NDA, tdr_pamt_block, tdr_pamt_entry_ptr, tdr_lock_flag, (void**)tdr_ptr);
                                
}
                              
api_error_type check_lock_and_map_explicit_private_4k_hpa(
        pa_t hpa, //tdr_pa
        uint64_t operand_id, //OPERAND_ID_RCX 
        tdr_t* tdr_p, // NULL
        mapping_type_t mapping_type, //TDX_RANGE_RW
        lock_type_t lock_type, //TDX_LOCK_EXCLUSIVE
        page_type_t expected_pt, //PT_NDA
        pamt_block_t* pamt_block, //tdr_pamt_block
        pamt_entry_t** pamt_entry, //tdr_pamt_entry_ptr
        bool_t* is_locked, //tdr_lock_flag
        void**         la // (void**) tdr_ptr
        )
{
    api_error_type errc;

    errc = check_and_lock_explicit_4k_private_hpa( tdr_pa, OPERAND_ID_RCX,
             TDX_LOCK_EXCLUSIVE, PT_NDA, tdr_pamt_block, tdr_pamt_entry_ptr, tdr_lock_flag);
    if (errc != TDX_SUCCESS)
    {
        return errc;
    }

    pa_t hpa_with_hkid = assign_hkid_to_hpa(tdr_p, hpa);

    *la = map_pa((void*)hpa_with_hkid.full_pa, mapping_type);

    return TDX_SUCCESS;
}  

api_error_type check_and_lock_explicit_4k_private_hpa(
        pa_t hpa, //tdr_pa
        uint64_t operand_id, //OPERAND_ID_RCX
        lock_type_t lock_type, //TDX_LOCK_EXCLUSIVE
        page_type_t expected_pt, //PT_NDA
        pamt_block_t* pamt_block, //tdr_pamt_block
        pamt_entry_t** pamt_entry, //tdr_pamt_entry_ptr
        bool_t* is_locked //tdr_lock_flag
        )
{
    api_error_type errc;

    page_size_t leaf_size = PT_4KB;

    errc = check_and_lock_explicit_private_hpa(tdr_pa, OPERAND_ID_RCX, _4KB, TDX_LOCK_EXCLUSIVE,
              PT_NDA, tdr_pamt_block, tdr_pamt_entry_ptr, &leaf_size, true, tdr_lock_flag);

    if (errc != TDX_SUCCESS)
    {
        return errc;
    }

    return TDX_SUCCESS;
}

api_error_type check_and_lock_explicit_private_hpa(
        pa_t hpa, //tdr_pa
        uint64_t operand_id, //OPERAND_ID_RCX
        uint64_t alignment, //_4KB 
        lock_type_t lock_type, //TDX_LOCK_EXCLUSIVE
        page_type_t expected_pt, //PT_NDA
        pamt_block_t* pamt_block, //tdr_pamt_block
        pamt_entry_t** pamt_entry, //tdr_pamt_entry_ptr
        page_size_t* leaf_size, // pointer to some value that is PT_4KB
        bool_t walk_to_leaf_size, //true
        bool_t* is_locked //tdr_lock_flag
        )
{
    api_error_code_e errc;

    errc = hpa_check_with_pwr_2_alignment(hpa, alignment);
    if (errc != TDX_SUCCESS)
    {
        return api_error_with_operand_id(TDX_OPERAND_INVALID, operand_id);
    }

    errc = non_shared_hpa_metadata_check_and_lock(tdr_pa, TDX_LOCK_EXCLUSIVE,
            PT_NDA, tdr_pamt_block, tdr_pamt_entry_ptr, &_4KB, trues, false);

    if (errc != TDX_SUCCESS)
    {
        return api_error_with_operand_id(errc, operand_id);
    }

    *is_locked = true;

    return TDX_SUCCESS;
}

api_error_code_e non_shared_hpa_metadata_check_and_lock(
        pa_t hpa, //tdr_pa
        lock_type_t lock_type, //TDX_LOCK_EXCLUSIVE
        page_type_t expected_pt, //PT_NDA
        pamt_block_t* pamt_block, //tdr_pamt_block
        pamt_entry_t** pamt_entry, //tdr_pamt_entry_ptr
        page_size_t*   leaf_size, //&_4KB
        bool_t walk_to_leaf_size, //true
        bool_t is_guest //false 
        )
{
    // 1) Check that the operand’s HPA is within a TDMR (Trust Domain Memory Range) which is covered by a PAMT.
    if (!pamt_get_block(tdr_pa, tdr_pamt_block))
    {
        TDX_ERROR("pamt_get_block error hpa = 0x%llx\n", hpa.raw);
        return TDX_OPERAND_ADDR_RANGE_ERROR;
    }

    pamt_entry_t* pamt_entry_lp;
    page_size_t requested_leaf_size = *leaf_size;

    // 2) Find the PAMT entry for the page and verify that its metadata is as expected.
    api_error_code_e errc = pamt_walk(tdr_pa, *tdr_pamt_block, TDX_LOCK_EXCLUSIVE, &_4KB,
                                      walk_to_leaf_size, is_guest, &pamt_entry_lp);

    if (errc != TDX_SUCCESS)
    {
        TDX_ERROR("pamt_walk error\n");
        return errc;
    }

    if (walk_to_leaf_size && (requested_leaf_size != *leaf_size))
    {
        TDX_ERROR("PAMT entry level = %d , Expected level = %d\n", *leaf_size, requested_leaf_size);
        pamt_unwalk(hpa, *pamt_block, pamt_entry_lp, lock_type, *leaf_size);
        return TDX_PAGE_METADATA_INCORRECT;
    }

    if (pamt_entry_lp->pt != expected_pt)
    {
        TDX_ERROR("pamt_entry_lp->pt = %d , expected_pt = %d\n", pamt_entry_lp->pt, expected_pt);
        pamt_unwalk(hpa, *pamt_block, pamt_entry_lp, lock_type, *leaf_size);
        return TDX_PAGE_METADATA_INCORRECT;
    }

    *pamt_entry = pamt_entry_lp;

    return TDX_SUCCESS;
}

// SOPHIA: assign the HKID value associated from the TD from the TDR page
pa_t assign_hkid_to_hpa(tdr_t* tdr_p, pa_t hpa)
{
    uint16_t hkid;

    // 1) If the target page is TDR (given TDR pointer is NULL), then use the TDX-SEAM global private HKID.

    if (tdr_p == NULL)
    {
        hkid = get_global_data()->hkid;
    }
    // 2) Else, read the HKID value associated with the TD from the TDR page.
    else
    {
        hkid = tdr_p->key_management_fields.hkid;
    }

    return set_hkid_to_pa(hpa, hkid);
}
*/