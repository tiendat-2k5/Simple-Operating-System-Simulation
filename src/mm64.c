/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#if defined(MM64)

/*
 * init_pte - Initialize PTE entry
 */
static addr_t *ensure_pte_mapping(struct pcb_t *caller, 
                                  addr_t pgd_idx, addr_t p4d_idx, addr_t pud_idx, 
                                  addr_t pmd_idx, addr_t pt_idx);

int init_pte(addr_t *pte,
             int pre,    // present
             addr_t fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1;  // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}


/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Extract page direactories */
	*pgd = (addr & PAGING64_ADDR_PGD_MASK) >> PAGING64_ADDR_PGD_LOBIT;
	*p4d = (addr & PAGING64_ADDR_P4D_MASK) >> PAGING64_ADDR_P4D_LOBIT;
	*pud = (addr & PAGING64_ADDR_PUD_MASK) >> PAGING64_ADDR_PUD_LOBIT;
	*pmd = (addr & PAGING64_ADDR_PMD_MASK) >> PAGING64_ADDR_PMD_LOBIT;
	*pt = (addr & PAGING64_ADDR_PT_MASK) >> PAGING64_ADDR_PT_LOBIT;

	/* Validate page directory indices */
        if (*pgd >= PAGING64_PGD_ENTRIES) return -1;
        if (*p4d >= PAGING64_P4D_ENTRIES) return -1;
        if (*pud >= PAGING64_PUD_ENTRIES) return -1;
        if (*pmd >= PAGING64_PMD_ENTRIES) return -1;
        if (*pt >= PAGING64_PT_ENTRIES) return -1;
	return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Shift the address to get page num and perform the mapping*/
	return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
                         pgd,p4d,pud,pmd,pt);
}


/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  struct krnl_t *krnl = caller->krnl;

  addr_t *pte;
  addr_t pgd;
  addr_t p4d;
  addr_t pud;
  addr_t pmd;
  addr_t pt;
	
#ifdef MM64	
  /* Get value from the system */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  if (get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt) != 0) {
      return -1; /* Invalid page number */
  }
  
  addr_t *pgd_entry = &krnl->mm->pgd[pgd];
  if(!PAGING_PTE_PRESENT(*pgd_entry)){
     addr_t p4d_fpn;
     if (MEMPHY_get_freefp(krnl->mram, &p4d_fpn) != 0) {
      return -1; /* No free frames */
    }
     init_pte(pgd_entry, 1, p4d_fpn, 0, 0, 0, 0);
  }
  
  addr_t p4d_fpn = PAGING_PTE_FPN(*pgd_entry);
  addr_t *p4d_table = (addr_t*)(krnl->mram->storage + (p4d_fpn * PAGING_PAGESZ));
  addr_t *p4d_entry = &p4d_table[p4d];

  if (!PAGING_PTE_PRESENT(*p4d_entry)) {
      addr_t pud_fpn;
      if (MEMPHY_get_freefp(krnl->mram, &pud_fpn) != 0) {
        return -1;
      }
        init_pte(p4d_entry, 1, pud_fpn, 0, 0, 0, 0);
    }
    
  addr_t pud_fpn = PAGING_PTE_FPN(*p4d_entry);
  addr_t *pud_table = (addr_t*)(krnl->mram->storage + (pud_fpn * PAGING_PAGESZ));
  addr_t *pud_entry = &pud_table[pud];
  
  if (!PAGING_PTE_PRESENT(*pud_entry)) {
      addr_t pmd_fpn;
      if (MEMPHY_get_freefp(krnl->mram, &pmd_fpn) != 0) {
        return -1;
       }
        init_pte(pud_entry, 1, pmd_fpn, 0, 0, 0, 0);
    }
    
  addr_t pmd_fpn = PAGING_PTE_FPN(*pud_entry);
  addr_t *pmd_table = (addr_t*)(krnl->mram->storage + (pmd_fpn * PAGING_PAGESZ));
  addr_t *pmd_entry = &pmd_table[pmd];
    
  if (!PAGING_PTE_PRESENT(*pmd_entry)) {
     addr_t pt_fpn;
    if (MEMPHY_get_freefp(krnl->mram, &pt_fpn) != 0) {
     return -1;
    }
        init_pte(pmd_entry, 1, pt_fpn, 0, 0, 0, 0);
    }
    
    /* Get PT table from PMD entry */
  addr_t pt_fpn = PAGING_PTE_FPN(*pmd_entry);
  addr_t *pt_table = (addr_t*)(krnl->mram->storage + (pt_fpn * PAGING_PAGESZ));
  pte = &pt_table[pt];
#else
  pte = &krnl->mm->pgd[pgn];
#endif
	
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  struct krnl_t *krnl = caller->krnl;
  if (krnl == NULL || krnl->mm == NULL) {
    return -1;
  }

#ifdef MM64	
  addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
  
  // Parse page number to multi-level indices
  if (get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx) != 0) {
    return -1;
  }

  // Ensure all page table levels are allocated and get the final PTE
  addr_t *pte = ensure_pte_mapping(caller, pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx);
  if (pte == NULL) {
    return -1;
  }

  // Set the PTE with the frame number
  *pte = 0;
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
  
  return 0;
#else
  // 32-bit implementation
  if (pgn >= PAGING_MAX_PGN) {
    return -1;
  }
  
  krnl->mm->pgd[pgn] = 0;
  SETBIT(krnl->mm->pgd[pgn], PAGING_PTE_PRESENT_MASK);
  CLRBIT(krnl->mm->pgd[pgn], PAGING_PTE_SWAPPED_MASK);
  SETVAL(krnl->mm->pgd[pgn], fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
  
  return 0;
#endif
}

/* Ensure all page table levels are allocated and return final PTE */
static addr_t *ensure_pte_mapping(struct pcb_t *caller, 
                                  addr_t pgd_idx, addr_t p4d_idx, addr_t pud_idx, 
                                  addr_t pmd_idx, addr_t pt_idx)
{
  struct krnl_t *krnl = caller->krnl;
  struct mm_struct *mm = krnl->mm;
  
  // PGD level - this is in mm->pgd array (virtual memory of kernel)
  if (!PAGING_PTE_PRESENT(mm->pgd[pgd_idx])) {
    addr_t p4d_fpn;
    if (MEMPHY_get_freefp(krnl->mram, &p4d_fpn) != 0) {
      return NULL;
    }
    init_pte(&mm->pgd[pgd_idx], 1, p4d_fpn, 0, 0, 0, 0);
    
    // Initialize the P4D table to zeros
    addr_t *p4d_table = (addr_t*)(krnl->mram->storage + (p4d_fpn * PAGING_PAGESZ));
    for (int i = 0; i < PAGING64_P4D_ENTRIES; i++) {
      p4d_table[i] = 0;
    }
  }
  
  addr_t p4d_fpn = PAGING_PTE_FPN(mm->pgd[pgd_idx]);
  addr_t *p4d_table = (addr_t*)(krnl->mram->storage + (p4d_fpn * PAGING_PAGESZ));
  
  // P4D level  
  if (!PAGING_PTE_PRESENT(p4d_table[p4d_idx])) {
    addr_t pud_fpn;
    if (MEMPHY_get_freefp(krnl->mram, &pud_fpn) != 0) {
      return NULL;
    }
    init_pte(&p4d_table[p4d_idx], 1, pud_fpn, 0, 0, 0, 0);
    
    // Initialize the PUD table to zeros
    addr_t *pud_table = (addr_t*)(krnl->mram->storage + (pud_fpn * PAGING_PAGESZ));
    for (int i = 0; i < PAGING64_PUD_ENTRIES; i++) {
      pud_table[i] = 0;
    }
  }
  
  addr_t pud_fpn = PAGING_PTE_FPN(p4d_table[p4d_idx]);
  addr_t *pud_table = (addr_t*)(krnl->mram->storage + (pud_fpn * PAGING_PAGESZ));
  
  // PUD level
  if (!PAGING_PTE_PRESENT(pud_table[pud_idx])) {
    addr_t pmd_fpn;
    if (MEMPHY_get_freefp(krnl->mram, &pmd_fpn) != 0) {
      return NULL;
    }
    init_pte(&pud_table[pud_idx], 1, pmd_fpn, 0, 0, 0, 0);
    
    // Initialize the PMD table to zeros
    addr_t *pmd_table = (addr_t*)(krnl->mram->storage + (pmd_fpn * PAGING_PAGESZ));
    for (int i = 0; i < PAGING64_PMD_ENTRIES; i++) {
      pmd_table[i] = 0;
    }
  }
  
  addr_t pmd_fpn = PAGING_PTE_FPN(pud_table[pud_idx]);
  addr_t *pmd_table = (addr_t*)(krnl->mram->storage + (pmd_fpn * PAGING_PAGESZ));
  
  // PMD level
  if (!PAGING_PTE_PRESENT(pmd_table[pmd_idx])) {
    addr_t pt_fpn;
    if (MEMPHY_get_freefp(krnl->mram, &pt_fpn) != 0) {
      return NULL;
    }
    init_pte(&pmd_table[pmd_idx], 1, pt_fpn, 0, 0, 0, 0);
    
    // Initialize the PT table to zeros
    addr_t *pt_table = (addr_t*)(krnl->mram->storage + (pt_fpn * PAGING_PAGESZ));
    for (int i = 0; i < PAGING64_PT_ENTRIES; i++) {
      pt_table[i] = 0;
    }
  }
  
  addr_t pt_fpn = PAGING_PTE_FPN(pmd_table[pmd_idx]);
  addr_t *pt_table = (addr_t*)(krnl->mram->storage + (pt_fpn * PAGING_PAGESZ));
  
  return &pt_table[pt_idx];
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
/* Get PTE page table entry */
uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
    if (caller == NULL || caller->krnl == NULL || caller->krnl->mm == NULL) {
        return 0;
    }

#ifdef MM64
    addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
    
    if (get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx) != 0) {
        return 0;
    }

    struct krnl_t *krnl = caller->krnl;
    struct mm_struct *mm = krnl->mm;
    
    // printf("[MM64-Trace] Access PGN: %05lx | Indices: PGD[%lx] -> P4D[%lx] -> PUD[%lx] -> PMD[%lx] -> PT[%lx]\n", 
    //        (unsigned long)pgn, 
    //        (unsigned long)pgd_idx, (unsigned long)p4d_idx, 
    //        (unsigned long)pud_idx, (unsigned long)pmd_idx, 
    //        (unsigned long)pt_idx);
    // Traverse multi-level page tables
    if (!PAGING_PTE_PRESENT(mm->pgd[pgd_idx])) {
        return 0;
    }
    
    addr_t p4d_fpn = PAGING_PTE_FPN(mm->pgd[pgd_idx]);
    // 🚨 QUAN TRỌNG: Kiểm tra bounds
    if (p4d_fpn * PAGING_PAGESZ >= krnl->mram->maxsz) {
        return 0;
    }
    
    addr_t *p4d_table = (addr_t*)(krnl->mram->storage + (p4d_fpn * PAGING_PAGESZ));
    
    if (!PAGING_PTE_PRESENT(p4d_table[p4d_idx])) {
        return 0;
    }
    
    addr_t pud_fpn = PAGING_PTE_FPN(p4d_table[p4d_idx]);
    if (pud_fpn * PAGING_PAGESZ >= krnl->mram->maxsz) {
        return 0;
    }
    
    addr_t *pud_table = (addr_t*)(krnl->mram->storage + (pud_fpn * PAGING_PAGESZ));
    
    if (!PAGING_PTE_PRESENT(pud_table[pud_idx])) {
        return 0;
    }
    
    addr_t pmd_fpn = PAGING_PTE_FPN(pud_table[pud_idx]);
    if (pmd_fpn * PAGING_PAGESZ >= krnl->mram->maxsz) {
        return 0;
    }
    
    addr_t *pmd_table = (addr_t*)(krnl->mram->storage + (pmd_fpn * PAGING_PAGESZ));
    
    if (!PAGING_PTE_PRESENT(pmd_table[pmd_idx])) {
        return 0;
    }
    
    addr_t pt_fpn = PAGING_PTE_FPN(pmd_table[pmd_idx]);
    if (pt_fpn * PAGING_PAGESZ >= krnl->mram->maxsz) {
        return 0;
    }
    
    addr_t *pt_table = (addr_t*)(krnl->mram->storage + (pt_fpn * PAGING_PAGESZ));
    
    return pt_table[pt_idx];
#else
    if (pgn >= PAGING_MAX_PGN) {
        return 0;
    }
    
    return caller->krnl->mm->pgd[pgn];
    
#endif
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
	struct krnl_t *krnl = caller->krnl;
	krnl->mm->pgd[pgn] = pte_val;
	
	return 0;
}


/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
  int pgit = 0;
  uint64_t pattern = 0xdeadbeef;
  
  for (pgit = 0; pgit < pgnum; pgit++) {
    addr_t current_addr = addr + (pgit * PAGING_PAGESZ);
    addr_t pgn = PAGING_PGN(current_addr);
    pte_set_fpn(caller, pgn, (pattern + pgit) & 0xFFFFF);
  }

  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
  struct framephy_struct *fpit = frames;
  int pgit = 0;
  addr_t pgn;

  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + (pgnum * PAGING_PAGESZ);

  for (pgit = 0; pgit < pgnum && fpit != NULL; pgit++) {
          pgn = PAGING_PGN(addr + (pgit * PAGING_PAGESZ));
          
  #ifdef MM64
          /* For 64-bit: use multi-level page tables */
          pte_set_fpn(caller, pgn, fpit->fpn);
  #else
          pte_set_fpn(caller, pgn, fpit->fpn);
  #endif

          /* Tracking for later page replacement activities (if needed)
           * Enqueue new usage page */
          enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn);
          fpit = fpit->fp_next;
      }

  return 0;
}

addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
    addr_t fpn;
    int pgit;
    struct framephy_struct *newfp_str = NULL;
    struct framephy_struct *tail = NULL;
    int allocated = 0;

    for (pgit = 0; pgit < req_pgnum; pgit++)
    {
        if (MEMPHY_get_freefp(caller->krnl->mram, &fpn) == 0)
        {
            struct framephy_struct *new_fp = malloc(sizeof(struct framephy_struct));
            new_fp->fpn = fpn;
            new_fp->fp_next = NULL;
            
            if (newfp_str == NULL) {
                newfp_str = new_fp;
                tail = new_fp;
            } else {
                tail->fp_next = new_fp;
                tail = new_fp;
            }
            allocated++;
        }
        else
        { 
            break; 
        }
    }
    *frm_lst = newfp_str;
    if (allocated == req_pgnum) {
        return 0; 
    } else {
        return allocated;
    }
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  addr_t ret_alloc = 0;

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  /* Out of memory */
  if (ret_alloc == -3000)
  {
    return -1;
  }

  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn)
{
  int cellidx;
  addr_t addrsrc, addrdst;
  for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
/*
 *Initialize a empty Memory Management instance
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  if (vma0 == NULL) {
      printf("[ERROR] init_mm: Failed to allocate VMA\n");
      return -1;
  }

  /* init page table directory */
  #ifdef MM64
      mm->pgd = calloc(PAGING64_PGD_ENTRIES, sizeof(addr_t));
      mm->p4d = calloc(PAGING64_P4D_ENTRIES, sizeof(addr_t));
      mm->pud = calloc(PAGING64_PUD_ENTRIES, sizeof(addr_t));
      mm->pmd = calloc(PAGING64_PMD_ENTRIES, sizeof(addr_t));
      mm->pt = calloc(PAGING64_PT_ENTRIES, sizeof(addr_t));
      
      if (mm->pgd == NULL || mm->p4d == NULL || mm->pud == NULL || mm->pmd == NULL || mm->pt == NULL) {
          printf("[ERROR] init_mm: Failed to allocate page tables\n");
          free(vma0);
          return -1;
      }
  #else
      mm->pgd = calloc(PAGING_MAX_PGN, sizeof(addr_t));
      mm->p4d = NULL;
      mm->pud = NULL;
      mm->pmd = NULL;
      mm->pt = NULL;
      
      if (mm->pgd == NULL) {
          printf("[ERROR] init_mm: Failed to allocate page table\n");
          free(vma0);
          return -1;
      }
  #endif
  
  // Initialize to zeros
  #ifdef MM64
      for (int i = 0; i < PAGING64_PGD_ENTRIES; i++) mm->pgd[i] = 0;
      for (int i = 0; i < PAGING64_P4D_ENTRIES; i++) mm->p4d[i] = 0;
      for (int i = 0; i < PAGING64_PUD_ENTRIES; i++) mm->pud[i] = 0;
      for (int i = 0; i < PAGING64_PMD_ENTRIES; i++) mm->pmd[i] = 0;
      for (int i = 0; i < PAGING64_PT_ENTRIES; i++) mm->pt[i] = 0;
  #else
      for (int i = 0; i < PAGING_MAX_PGN; i++) mm->pgd[i] = 0;
  #endif

  /* Initialize VMA */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  #ifdef MM64
    vma0->vm_end = 0x100000;  /* 4KB for 64-bit */
  #else
    vma0->vm_end = 0x10000;    /* 256B for 32-bit */
  #endif
  
  vma0->sbrk = PAGING_PAGESZ;  // Mỗi process offset khác nhau
  
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  if (first_rg == NULL) {
      printf("[ERROR] init_mm: Failed to allocate first region\n");
      free(vma0);
      return -1;
  }
  
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  vma0->vm_next = NULL;
  vma0->vm_mm = mm; 

  mm->mmap = vma0;
  mm->fifo_pgn = NULL;
  
  // Initialize symbol table
  for (int i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) {
    mm->symrgtbl[i].rg_start = 0;
    mm->symrgtbl[i].rg_end = 0;
    mm->symrgtbl[i].rg_next = NULL;
  }
  return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn)
{
  struct pgn_t *pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
  struct framephy_struct *fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL) { printf("NULL list\n"); return -1;}
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
  if (caller->krnl && caller->krnl->mm) {
    printf("print_pgtbl:\n PDG=%lx P4g=%lx PUD=%lx PMD=%lx\n", 
           (addr_t)caller->krnl->mm->pgd,
           (addr_t)caller->krnl->mm->p4d, 
           (addr_t)caller->krnl->mm->pud,
           (addr_t)caller->krnl->mm->pmd);
  }
  return 0;
}

addr_t *fpn_to_ptr(struct memphy_struct *mp, addr_t fpn) {
    if (mp == NULL || mp->storage == NULL) return NULL;
    if (fpn * PAGING_PAGESZ >= mp->maxsz) return NULL;
    return (addr_t*)(mp->storage + (fpn * PAGING_PAGESZ));
}
#endif  //def MM64
