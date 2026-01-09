/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "string.h"
#include "mm.h"
#include "mm64.h"
#include "syscall.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

//struct vm_rg_struct *currg = &caller->krnl->mm->symrgtbl[rgid];

/*enlist_vm_freerg_list - add new rg to freerg_list */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;
  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  mm->mmap->vm_freerg_list = rg_elmt;
  return 0;
}

/*get_symrg_byid - get mem region by region ID */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ) {
    return NULL;
  }
  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory */
// TRONG __alloc() - libmem.c
int __alloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
    if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ) {
        return -1;
    }

    struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
    if (cur_vma == NULL) {
        return -1;
    }

    struct vm_rg_struct *rgnode = &caller->krnl->mm->symrgtbl[rgid];

    if (rgnode->rg_start != 0 || rgnode->rg_end != 0) {
        return -1;
    }

    addr_t alloc_start = cur_vma->sbrk;
    addr_t alloc_end = alloc_start + size;
    
    if (alloc_end > cur_vma->vm_end) {
        addr_t needed_size = alloc_end - cur_vma->sbrk;
        
        if (inc_vma_limit(caller, vmaid, needed_size) != 0) {
            printf("[ERROR] __alloc: Failed to expand VMA\n");
            return -1;
        }
        // Sau khi expand, sbrk đã được cập nhật
        alloc_start = cur_vma->sbrk;
        alloc_end = alloc_start + size;
    }
    
    if (alloc_start == 0) {
        alloc_start = PAGING_PAGESZ;
        alloc_end = alloc_start + size;
    }
    
    rgnode->rg_start = alloc_start;
    rgnode->rg_end = alloc_end;
    *alloc_addr = alloc_start;
    
    cur_vma->sbrk = alloc_end;


    return 0;
}

/*__free - remove a region memory */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
    pthread_mutex_lock(&mmvm_lock);

    if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ) {
        pthread_mutex_unlock(&mmvm_lock);
        return -1;
    }
    
    struct vm_rg_struct *rgnode = &caller->krnl->mm->symrgtbl[rgid];

    if (rgnode->rg_start == 0 && rgnode->rg_end == 0) {
        printf("[WARNING] __free: Process %d trying to free unallocated region %d\n", 
               caller->pid, rgid);
        pthread_mutex_unlock(&mmvm_lock);
        return -1;  // Không free region chưa được allocate
    }

    if (rgnode->rg_start >= rgnode->rg_end) {
        printf("[WARNING] __free: Process %d region %d has invalid range [%lu-%lu]\n",
               caller->pid, rgid, rgnode->rg_start, rgnode->rg_end);
        pthread_mutex_unlock(&mmvm_lock);
        return -1;
    }
    
    // Reset region
    rgnode->rg_start = 0;
    rgnode->rg_end = 0;

    pthread_mutex_unlock(&mmvm_lock);
    return 0;
}

/*liballoc - PAGING-based allocate a region memory */
int liballoc(struct pcb_t *proc, addr_t size, uint32_t reg_index)
{
  addr_t addr;
  int val = __alloc(proc, 0, reg_index, size, &addr);
  
  if (val == 0) {
    proc->regs[reg_index] = addr;
  }
  
#ifdef IODUMP
  printf("liballoc:178\n");
  print_pgtbl(proc, 0, 0);  
#endif

  return val;
}

/*libfree - PAGING-based free a region memory */
/*libfree - PAGING-based free a region memory */
int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  if (proc->regs[reg_index] == 0) {
      printf("[WARNING] libfree: Process %d reg_index %d contains address 0, skipping free\n",
             proc->pid, reg_index);
      return -1;
  }
  
  int val = __free(proc, 0, reg_index);
  
#ifdef IODUMP
  printf("libfree:218\n");
  print_pgtbl(proc, 0, 0);
#endif
  
  return val;
}

/*pg_getpage - get the page in ram */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  if (pgn < 0 || pgn >= PAGING_MAX_PGN) {
    return -1;
  }

  // Kiểm tra page đã được map chưa
  uint32_t pte = pte_get_entry(caller, pgn);
  
  if (PAGING_PAGE_PRESENT(pte)) {
    *fpn = PAGING_FPN(pte);
    return 0;
  }

  // Page is not present, allocate and map it
  addr_t new_fpn;
  if (MEMPHY_get_freefp(caller->krnl->mram, &new_fpn) == 0) {
    // Map the page
    if (pte_set_fpn(caller, pgn, new_fpn) == 0) {
      // Thêm vào FIFO list cho page replacement
      if (caller->krnl->mm->fifo_pgn == NULL) {
        caller->krnl->mm->fifo_pgn = malloc(sizeof(struct pgn_t));
        caller->krnl->mm->fifo_pgn->pgn = pgn;
        caller->krnl->mm->fifo_pgn->pg_next = NULL;
      } else {
        struct pgn_t *new_node = malloc(sizeof(struct pgn_t));
        new_node->pgn = pgn;
        new_node->pg_next = caller->krnl->mm->fifo_pgn;
        caller->krnl->mm->fifo_pgn = new_node;
      }
      
      *fpn = new_fpn;
      return 0;
    } else {
      MEMPHY_put_freefp(caller->krnl->mram, new_fpn);
      return -1;
    }
  } else {
    return -1;
  }
}

/*pg_getval - read value at given offset */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0) {
    return -1;
  }

  // Calculate physical address directly
  int phyaddr = (fpn * PAGING_PAGESZ) + off;
  
  // Read directly from physical memory
  if (MEMPHY_read(caller->krnl->mram, phyaddr, data) != 0) {
    return -1;
  }
  
  return 0;
}

/*pg_setval - write value to given offset */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0) {
    return -1;
  }

  // Calculate physical address directly
  int phyaddr = (fpn * PAGING_PAGESZ) + off;
  
  // Write directly to physical memory
  if (MEMPHY_write(caller->krnl->mram, phyaddr, value) != 0) {
    return -1;
  }
  
  return 0;
}

/*__read - read value in region memory */
int __read(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
    if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ) {
        return -1;
    }
    
    // SỬ DỤNG GLOBAL STORAGE
    struct vm_rg_struct *currg = &caller->krnl->mm->symrgtbl[rgid];
    
    // Kiểm tra region hợp lệ - NẾU CHƯA ALLOCATE, TỰ ĐỘNG ALLOCATE
    if (currg->rg_start == 0 && currg->rg_end == 0) {
        // Tự động allocate region với size mặc định 100 bytes
        addr_t alloc_addr;
        if (__alloc(caller, vmaid, rgid, 100, &alloc_addr) != 0) {
            return -1;
        }
    }

    if (currg->rg_start >= currg->rg_end) {
        return -1;
    }

    addr_t region_size = currg->rg_end - currg->rg_start;
    if (offset >= region_size) {
        return -1;
    }

    addr_t target_addr = currg->rg_start + offset;
    
    // Thực hiện read
    return pg_getval(caller->krnl->mm, target_addr, data, caller);
}

/*libread - PAGING-based read a region memory */
int libread(struct pcb_t *proc, uint32_t source, addr_t offset, uint32_t* destination)
{
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);

  if (val == 0) {
    *destination = data;
  }

#ifdef IODUMP
  printf("libread:426\n");
  print_pgtbl(proc, 0, 0);
#endif

  return val;
}

/*__write - write a region memory */
int __write(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
    if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ) {
        return -1;
    }
    
    // SỬ DỤNG GLOBAL STORAGE
    struct vm_rg_struct *currg = &caller->krnl->mm->symrgtbl[rgid];
    
    // Kiểm tra region hợp lệ - NẾU CHƯA ALLOCATE, TỰ ĐỘNG ALLOCATE
    if (currg->rg_start == 0 && currg->rg_end == 0) {
        // Tự động allocate region với size mặc định 100 bytes
        addr_t alloc_addr;
        if (__alloc(caller, vmaid, rgid, 100, &alloc_addr) != 0) {
            return -1;
        }
    }

    if (currg->rg_start >= currg->rg_end) {
        return -1;
    }

    addr_t region_size = currg->rg_end - currg->rg_start;
    if (offset >= region_size) {
        return -1;
    }

    addr_t target_addr = currg->rg_start + offset;
    
    // Thực hiện write
    return pg_setval(caller->krnl->mm, target_addr, value, caller);
}

/*libwrite - PAGING-based write a region memory */
int libwrite(struct pcb_t *proc, BYTE data, uint32_t destination, addr_t offset)
{
  int val = __write(proc, 0, destination, offset, data);

#ifdef IODUMP
  printf("libwrite:502\n");
  print_pgtbl(proc, 0, 0);
#endif

  return val;
}

/*find_victim_page - find victim page */
int find_victim_page(struct mm_struct *mm, addr_t *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;

  if (pg == NULL) {
    return -1;
  }

  *retpgn = pg->pgn;
  mm->fifo_pgn = pg->pg_next;
  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  if (cur_vma == NULL) {
    return -1;
  }

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL) {
    return -1;
  }

  newrg->rg_start = newrg->rg_end = -1;

  while (rgit != NULL) {
    if (rgit->rg_start + size <= rgit->rg_end) {
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      if (rgit->rg_start + size < rgit->rg_end) {
        rgit->rg_start = rgit->rg_start + size;
      } else {
        struct vm_rg_struct *nextrg = rgit->rg_next;
        if (nextrg != NULL) {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;
          rgit->rg_next = nextrg->rg_next;
          free(nextrg);
        } else {
          rgit->rg_start = rgit->rg_end;
          rgit->rg_next = NULL;
        }
      }
      return 0;
    }
    rgit = rgit->rg_next;
  }

  return -1;
}
