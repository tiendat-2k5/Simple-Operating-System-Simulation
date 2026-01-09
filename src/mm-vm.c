/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


/* * get_vma_by_num - get vm area by numID 
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
    struct vm_area_struct *pvma = mm->mmap;

    if (mm->mmap == NULL)
        return NULL;

    int vmait = 0;
    while (pvma != NULL && vmait < vmaid)
    {
        pvma = pvma->vm_next;
        vmait++;
    }

    return pvma;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn , addr_t swpfpn)
{
    return __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
}

/* * get_vm_area_node_at_brk
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
    if (cur_vma == NULL) return NULL;

    if (cur_vma->sbrk + size > cur_vma->vm_end) return NULL;

    struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
    if (newrg == NULL) return NULL;

    newrg->rg_start = cur_vma->sbrk;
    newrg->rg_end = newrg->rg_start + size;
    newrg->rg_next = NULL;

    return newrg;
}

/* * validate_overlap_vm_area
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
    if (vmastart >= vmaend) return -1;

    struct vm_area_struct *vma = caller->krnl->mm->mmap;
    if (vma == NULL) return -1;

    struct vm_area_struct *cur_area = get_vma_by_num(caller->krnl->mm, vmaid);
    if (cur_area == NULL) return -1;

    while (vma != NULL)
    {
        if (vma != cur_area && OVERLAP(cur_area->vm_start, cur_area->vm_end, vma->vm_start, vma->vm_end))
        {
            return -1;
        }
        vma = vma->vm_next;
    }

    return 0;
}

/* * inc_vma_limit - increase vm area limits to reserve space for new variable
 * FIX: Chỉ mở rộng vm_end (vùng nhớ vật lý), KHÔNG TĂNG sbrk (để libmem tự làm)
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
    if(caller == NULL || caller->krnl == NULL || caller->krnl->mm == NULL) return -1;
  
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
    if(cur_vma == NULL) return -1;
  
    /* Tính toán kích thước cần mở rộng (align theo page) */
    addr_t mm64_inc_sz;
#ifdef MM64
    mm64_inc_sz = (inc_sz + PAGING_PAGESZ - 1) & ~(PAGING_PAGESZ - 1);
#else
    mm64_inc_sz = PAGING_PAGE_ALIGNSZ(inc_sz);
#endif

    /* * Logic quan trọng tương thích với libmem.c:
     * libmem gọi hàm này khi (alloc_end > vm_end).
     * Nhiệm vụ của ta là map thêm RAM từ [old_vm_end] đến [new_vm_end].
     * Ta KHÔNG được đụng vào cur_vma->sbrk vì libmem sẽ dùng giá trị cũ của sbrk làm start.
     */

    addr_t old_limit = cur_vma->vm_end;
    addr_t new_limit = old_limit + mm64_inc_sz;
  
    // Đảm bảo map đủ cho yêu cầu của libmem (từ sbrk đến sbrk+inc_sz)
    // Tuy nhiên libmem truyền vào inc_sz = (alloc_end - sbrk), tức là phần dư ra.
    // Đơn giản nhất là mở rộng vm_end thêm đúng lượng aligned đó.

    int incnumpage = mm64_inc_sz / PAGING_PAGESZ;
    struct vm_rg_struct ret_rg;
         
    /* Map RAM vật lý cho vùng mới mở rộng */
    if (vm_map_ram(caller, old_limit, new_limit, old_limit, incnumpage, &ret_rg) < 0) {
        printf("[ERROR] inc_vma_limit: vm_map_ram failed\n");
        return -1;
    }

    /* CHỈ CẬP NHẬT vm_end, ĐỂ YÊN sbrk CHO LIBMEM */
    cur_vma->vm_end = new_limit;

    return 0;
}

/* * pgalloc - Wrapper gọi đến hàm __alloc của libmem.c
 * Cần thiết vì một số module khác (như loader/cpu) có thể gọi pgalloc
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
    addr_t addr;
    // Gọi hàm __alloc đã có sẵn trong libmem.c
    return __alloc(proc, 0, reg_index, size, &addr);
}

/* * pgfree - Wrapper gọi đến hàm __free của libmem.c
 */
int pgfree(struct pcb_t *proc, uint32_t reg_index)
{
    // Gọi hàm __free đã có sẵn trong libmem.c
    return __free(proc, 0, reg_index);
}

// #endif