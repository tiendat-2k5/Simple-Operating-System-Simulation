/*
 * System Call: Memory Statistics  
 * SYSCALL: mmstats - get memory usage statistics for current process
 */

#include "syscall.h"
#include "os-mm.h"
#include "common.h"
#include "queue.h"
#include <stdio.h>

// Định nghĩa các macro cơ bản cho paging
#ifndef PAGING_PTE_PRESENT
#define PAGING_PTE_PRESENT(pte) ((pte) != 0)
#endif

int __sys_mmstats(struct krnl_t *krnl, uint32_t pid, struct sc_regs* regs)
{
    // Lấy PID từ parameter thứ nhất (regs->a1)
    uint32_t target_pid = regs->a1;
    
    printf("[DEBUG] sys_mmstats called by PID %d for target PID: %d\n", pid, target_pid);
    
    struct pcb_t *caller = NULL;
    
    printf("[MMSTATS] Looking for process with PID: %d\n", target_pid);
    
    // Tìm process bằng target_pid
    if (krnl == NULL || krnl->running_list == NULL) {
        printf("[MMSTATS] ERROR: Kernel or running_list is NULL\n");
        return -1;
    }
    
    for (int i = 0; i < krnl->running_list->size; i++) {
        if (krnl->running_list->proc[i] != NULL && 
            krnl->running_list->proc[i]->pid == target_pid) {
            caller = krnl->running_list->proc[i];
            printf("[MMSTATS] Found process %d\n", target_pid);
            break;
        }
    }
    
    if (caller == NULL) {
        printf("[MMSTATS] ERROR: Process %d not found in running_list\n", target_pid);
        return -1;
    }
    
    if (caller->krnl == NULL || caller->krnl->mm == NULL) {
        printf("[MMSTATS] ERROR: Memory management not initialized for process %d\n", target_pid);
        return -1;
    }
    
    struct mm_struct *mm = caller->krnl->mm;
    
    // DEBUG CHI TIẾT CẤU TRÚC MEMORY
    printf("[MMSTATS DEBUG] === MEMORY STRUCTURE DEBUG ===\n");
    printf("[MMSTATS DEBUG] mm_struct: %p\n", mm);
    printf("[MMSTATS DEBUG] pgd: %p\n", mm->pgd);
    printf("[MMSTATS DEBUG] mmap: %p\n", mm->mmap);
    
#ifdef MM64
    printf("[MMSTATS DEBUG] 64-bit paging structures:\n");
    printf("[MMSTATS DEBUG] p4d: %p\n", mm->p4d);
    printf("[MMSTATS DEBUG] pud: %p\n", mm->pud); 
    printf("[MMSTATS DEBUG] pmd: %p\n", mm->pmd);
    printf("[MMSTATS DEBUG] pt: %p\n", mm->pt);
#endif

    // Thống kê memory usage
    int ram_pages = 0;
    int total_pages = 0;

    printf("[MMSTATS] Memory Statistics for PID %d:\n", target_pid);
    
    // KIỂM TRA CẤU TRÚC PAGE TABLE
    if (mm->pgd == NULL) {
        printf("[MMSTATS] Page table (pgd) is NULL\n");
        return 0;
    }
    
    printf("[MMSTATS] Page table address: %p\n", mm->pgd);
    
    // Đếm pages trong page table - cách đơn giản
    int max_entries = 1024;
    
    for (int i = 0; i < max_entries; i++) {
        if (mm->pgd[i] != 0) { // Page table entry không rỗng
            total_pages++;
            ram_pages++; // Giả sử tất cả đều trong RAM
            
            // In thông tin chi tiết cho một số entries đầu tiên
            if (i < 10) { // Chỉ in 10 entries đầu để tránh spam
                printf("[MMSTATS] PGD Entry[%d]: 0x%lx -> %s\n", 
                       i, mm->pgd[i], 
                       (mm->pgd[i] != 0) ? "VALID" : "INVALID");
            }
        }
    }
    
    // THỬ ĐẾM PAGES TỪ CÁC LEVEL KHÁC NẾU LÀ 64-BIT
#ifdef MM64
    if (mm->p4d != NULL) {
        printf("[MMSTATS DEBUG] Counting P4D entries...\n");
        int p4d_count = 0;
        for (int i = 0; i < 512; i++) {
            if (mm->p4d[i] != 0) p4d_count++;
        }
        printf("[MMSTATS DEBUG] P4D entries used: %d/512\n", p4d_count);
    }
    
    if (mm->pud != NULL) {
        printf("[MMSTATS DEBUG] Counting PUD entries...\n");
        int pud_count = 0;
        for (int i = 0; i < 512; i++) {
            if (mm->pud[i] != 0) pud_count++;
        }
        printf("[MMSTATS DEBUG] PUD entries used: %d/512\n", pud_count);
    }
    
    if (mm->pmd != NULL) {
        printf("[MMSTATS DEBUG] Counting PMD entries...\n");
        int pmd_count = 0;
        for (int i = 0; i < 512; i++) {
            if (mm->pmd[i] != 0) pmd_count++;
        }
        printf("[MMSTATS DEBUG] PMD entries used: %d/512\n", pmd_count);
    }
#endif
    
    // Kiểm tra memory regions từ vm_area_struct
    int vm_regions = 0;
    struct vm_area_struct *vm_area = mm->mmap;
    while (vm_area != NULL) {
        vm_regions++;
        printf("[MMSTATS] VM Area %d: 0x%lx - 0x%lx (size: %ld bytes)\n",
               vm_regions, vm_area->vm_start, vm_area->vm_end,
               vm_area->vm_end - vm_area->vm_start);
        
        // DEBUG THÊM THÔNG TIN VMA
        printf("[MMSTATS DEBUG] VMA %d: vm_id=%lu, sbrk=0x%lx, vm_mm=%p\n",
               vm_regions, vm_area->vm_id, vm_area->sbrk, vm_area->vm_mm);
               
        vm_area = vm_area->vm_next;
    }

    // Hiển thị statistics
    printf("[MMSTATS] === MEMORY STATISTICS ===\n");
    printf("[MMSTATS] Process ID: %d\n", target_pid);
    printf("[MMSTATS] Total pages allocated: %d\n", total_pages);
    printf("[MMSTATS] Pages in RAM: %d\n", ram_pages);
    printf("[MMSTATS] VM Memory regions: %d\n", vm_regions);
    
    if (total_pages > 0) {
        printf("[MMSTATS] Memory usage: %d pages (%d KB)\n", 
               total_pages, total_pages * 4); // Giả sử page size 4KB
    } else {
        printf("[MMSTATS] No pages allocated\n");
    }
    
    // Return số pages trong RAM
    return ram_pages;
}
