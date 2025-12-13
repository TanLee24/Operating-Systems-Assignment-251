/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "os-mm.h"
#include "syscall.h"
#include "libmem.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef MM64
#include "mm64.h"
#else
#include "mm.h"
#endif

/* Khai báo prototype để tránh warning implicit declaration */
extern int vmap_pgd_memset(struct pcb_t *caller, addr_t addr, int pgnum);
extern int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz);
extern int MEMPHY_read(struct memphy_struct *mp, addr_t addr, BYTE *value);
extern int MEMPHY_write(struct memphy_struct *mp, addr_t addr, BYTE data);

/* Helper: Tìm PCB an toàn */
static struct pcb_t *find_proc_safe(struct krnl_t *krnl, uint32_t pid) {
    if (!krnl) return NULL;

    struct queue_t *q;

    /* 1. Tìm trong running_list */
    q = krnl->running_list;
    if (q) { /* PHẢI KIỂM TRA q KHÁC NULL TRƯỚC */
        for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
            if (q->proc[i] && q->proc[i]->pid == pid) {
                return q->proc[i];
            }
        }
    }

    /* 2. Tìm trong ready_queue */
    q = krnl->ready_queue;
    if (q) { /* PHẢI KIỂM TRA q KHÁC NULL TRƯỚC */
        for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
            if (q->proc[i] && q->proc[i]->pid == pid) {
                return q->proc[i];
            }
        }
    }

    return NULL;
}

int __sys_memmap(struct krnl_t *krnl, uint32_t pid, struct sc_regs* regs)
{
    if (!krnl) return -1;
    
    int memop = regs->a1;
    int ret = 0;
    struct pcb_t *caller = NULL;
    addr_t paddr = regs->a2; 

    switch (memop) {
    /* Các lệnh cần caller (Alloc, Map, Swap) */
    case SYSMEM_MAP_OP:
    case SYSMEM_INC_OP:
    case SYSMEM_SWP_OP:
            caller = find_proc_safe(krnl, pid);
            if (!caller) return -1;

            if (memop == SYSMEM_MAP_OP) ret = vmap_pgd_memset(caller, regs->a2, regs->a3);
            else if (memop == SYSMEM_INC_OP) ret = inc_vma_limit(caller, regs->a2, regs->a3);
            else ret = __mm_swap_page(caller, regs->a2, regs->a3);
            break;

    /* Không cần tìm caller, không cần dịch địa chỉ lần 2 */
    case SYSMEM_IO_READ:
            if (krnl->mram) {
                BYTE data;
                /* Đọc từ RAM vật lý vào biến tạm data */
                if (MEMPHY_read(krnl->mram, paddr, &data) < 0) 
                    ret = -1;
                else {
                    /* Ghi data vào thanh ghi đích của process */
                    if (caller && regs->a3 < 10) { 
                        caller->regs[regs->a3] = data; 
                    }
                }
            } else ret = -1;
            break;

    case SYSMEM_IO_WRITE:
            if (krnl->mram) {
                /* Lấy dữ liệu từ thanh ghi nguồn để ghi xuống RAM */
                BYTE data = (BYTE)regs->a3; 
                if (MEMPHY_write(krnl->mram, paddr, data) < 0) 
                    ret = -1;
            } else ret = -1;
            break;

    default:
            return -1;
    }

    return ret;
}