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
#include <string.h>

#if defined(MM64)

/* * Helper function to walk the page table tree
 * Returns a pointer to the PTE (Page Table Entry) at the lowest level (PT).
 * If intermediate tables are missing and 'alloc' is true, they are created.
 */
/* [FIX] Changed pgn type from int to addr_t to prevent overflow when shifting */
static addr_t *__get_pte(struct mm_struct *mm, addr_t pgn, int alloc) {
    if (!mm || !mm->pgd) return NULL;

    /* Calculate indices for 5 levels based on PGN */
    /* PGN includes indices for PGD, P4D, PUD, PMD, PT */
    /* Shift amounts assume 9 bits per level (512 entries) */
    /* Total 9*5 = 45 bits for PGN, + 12 bits offset = 57 bits address */
    int pgd_idx = (pgn >> 36) & 0x1FF;
    int p4d_idx = (pgn >> 27) & 0x1FF;
    int pud_idx = (pgn >> 18) & 0x1FF;
    int pmd_idx = (pgn >> 9)  & 0x1FF;
    int pt_idx  = (pgn >> 0)  & 0x1FF;

    /* Level 5: PGD */
    if (mm->pgd[pgd_idx] == 0) {
        if (!alloc) return NULL;
        mm->pgd[pgd_idx] = (addr_t)malloc(512 * sizeof(addr_t));
        memset((void *)mm->pgd[pgd_idx], 0, 512 * sizeof(addr_t));
    }
    addr_t *p4d = (addr_t *)mm->pgd[pgd_idx];

    /* Level 4: P4D */
    if (p4d[p4d_idx] == 0) {
        if (!alloc) return NULL;
        p4d[p4d_idx] = (addr_t)malloc(512 * sizeof(addr_t));
        memset((void *)p4d[p4d_idx], 0, 512 * sizeof(addr_t));
    }
    addr_t *pud = (addr_t *)p4d[p4d_idx];

    /* Level 3: PUD */
    if (pud[pud_idx] == 0) {
        if (!alloc) return NULL;
        pud[pud_idx] = (addr_t)malloc(512 * sizeof(addr_t));
        memset((void *)pud[pud_idx], 0, 512 * sizeof(addr_t));
    }
    addr_t *pmd = (addr_t *)pud[pud_idx];

    /* Level 2: PMD */
    if (pmd[pmd_idx] == 0) {
        if (!alloc) return NULL;
        pmd[pmd_idx] = (addr_t)malloc(512 * sizeof(addr_t));
        memset((void *)pmd[pmd_idx], 0, 512 * sizeof(addr_t));
    }
    addr_t *pt = (addr_t *)pmd[pmd_idx];

    /* Level 1: PT - Return pointer to the PTE entry */
    return &pt[pt_idx];
}

int init_pte(addr_t *pte,
             int pre,    // present
             addr_t fpn, // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1; // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    } else { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }
  return 0;
}

int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
    if (!caller || !caller->krnl->mm) return -1;
    
    /* Get PTE, allocating path if necessary to store swap info */
    addr_t *pte = __get_pte(caller->krnl->mm, pgn, 1); 
    if (!pte) return -1;

    CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
    SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
    CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

    SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
    SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

    return 0;
}

int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
    if (!caller || !caller->krnl->mm) return -1;

    /* Get PTE, allocating path if necessary */
    addr_t *pte = __get_pte(caller->krnl->mm, pgn, 1);
    if (!pte) return -1;

    SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
    CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
    SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

    return 0;
}

uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
    if (!caller || !caller->krnl->mm) return 0;

    /* Don't alloc if just reading */
    addr_t *pte = __get_pte(caller->krnl->mm, pgn, 0); 
    if (!pte) return 0; /* Page not mapped yet */

    return (uint32_t)*pte;
}

int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
    if (!caller || !caller->krnl->mm) return -1;

    addr_t *pte = __get_pte(caller->krnl->mm, pgn, 1);
    if (!pte) return -1;

    *pte = pte_val;
    return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller, 
                    addr_t addr, 
                    int pgnum, 
                    struct framephy_struct *frames, 
                    struct vm_rg_struct *ret_rg)
{
    struct framephy_struct *fpit = frames;
    int pgit = 0;
    /* [FIX] Changed pgn type from int to addr_t */
    addr_t pgn = PAGING_PGN(addr);

    if (ret_rg) {
        ret_rg->rg_start = addr;
        // [FIX] Use PAGING64_PAGESZ instead of PAGING_PAGESZ
        ret_rg->rg_end = addr + pgnum * PAGING64_PAGESZ;
    }

    for (pgit = 0; pgit < pgnum; pgit++) {
       if (fpit == NULL) break;
       
       pte_set_fpn(caller, pgn + pgit, fpit->fpn);
       enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn + pgit);
       
       fpit = fpit->fp_next;
    }

    return 0;
}

/* * alloc_pages_range - allocate req_pgnum of frame in ram
 */
addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
    int pgit, count = 0;
    struct framephy_struct *newfp_str;
    addr_t fpn;

    *frm_lst = NULL;

    for(pgit = 0; pgit < req_pgnum; pgit++) {
        if(MEMPHY_get_freefp(caller->krnl->mram, &fpn) == 0) {
            newfp_str = (struct framephy_struct*)malloc(sizeof(struct framephy_struct));
            newfp_str->fpn = fpn;
            newfp_str->fp_next = *frm_lst;
            *frm_lst = newfp_str;
            count++;
        } else {
            /* Rollback if failed */
            while(*frm_lst) {
                struct framephy_struct *tmp = *frm_lst;
                *frm_lst = (*frm_lst)->fp_next;
                MEMPHY_put_freefp(caller->krnl->mram, tmp->fpn);
                free(tmp);
            }
            return -3000; /* Out of memory */
        }
    }
    return 0;
}

/* * vm_map_ram - do the mapping all vm area to ram storage device
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
    struct framephy_struct *frm_lst = NULL;
    addr_t ret_alloc;

    ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);
    
    if (ret_alloc < 0) return -1;

    vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

    return 0;
}

int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                struct memphy_struct *mpdst, addr_t dstfpn) 
{
  int cellidx;
  addr_t addrsrc, addrdst;
  // [FIX] Use PAGING64_PAGESZ to copy full 4KB page
  for(cellidx = 0; cellidx < PAGING64_PAGESZ; cellidx++)
  {
    // [FIX] Use PAGING64_PAGESZ for address calculation
    addrsrc = srcfpn * PAGING64_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING64_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }
  return 0;
}

int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
    struct vm_area_struct * vma = malloc(sizeof(struct vm_area_struct));

    /* Initialize PGD as a top-level directory (one table of 512 entries) */
    mm->pgd = (addr_t*)malloc(512 * sizeof(addr_t));
    memset(mm->pgd, 0, 512 * sizeof(addr_t));

    /* Other levels are managed dynamically in __get_pte */
    mm->p4d = NULL; 
    mm->pud = NULL; 
    mm->pmd = NULL; 
    mm->pt = NULL;

    vma->vm_id = 0;
    vma->vm_start = 0;
    // [FIX] Use BIT_ULL to prevent 32-bit overflow for 57-bit address space
    vma->vm_end = BIT_ULL(PAGING_CPU_BUS_WIDTH); 
    vma->sbrk = vma->vm_start;
    
    struct vm_rg_struct *first_rg = init_vm_rg(vma->vm_start, vma->vm_end);
    enlist_vm_rg_node(&vma->vm_freerg_list, first_rg);

    vma->vm_next = NULL;
    vma->vm_mm = mm;
    mm->mmap = vma;
    
    /* Initialize symbol table */
    for (int i=0; i<PAGING_MAX_SYMTBL_SZ; i++) {
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

/*
 * Recursive helper to print page table tree
 */
static void print_pgtbl_recursive(void *table, int level, addr_t current_pgn) {
    if (!table) return;
    
    addr_t *entries = (addr_t *)table;
    int i;
    
    for (i = 0; i < 512; i++) {
        if (entries[i] != 0) {
            /* Calculate the PGN contribution of this index at this level */
            /* Level 5 (PGD) shifts by 36, Level 1 (PT) shifts by 0 */
            int shift = (level - 1) * 9;
            addr_t next_pgn = current_pgn | ((addr_t)i << shift);

            if (level > 1) {
                /* Interior node: traverse down */
                print_pgtbl_recursive((void *)entries[i], level - 1, next_pgn);
            } else {
                /* Leaf node (PT): Print PTE */
                printf("  PGN [%05lx]: ", next_pgn);
                uint32_t pte = (uint32_t)entries[i];
                
                if (PAGING_PAGE_PRESENT(pte)) {
                    printf("PRESENT (FPN %05x)", PAGING_FPN(pte));
                } else if (PAGING_PTE_SWAPPED_MASK & pte) {
                    printf("SWAPPED (SWP %05x)", PAGING_SWP(pte));
                } else {
                    printf("INVALID");
                }
                
                if (PAGING_PTE_DIRTY_MASK & pte) printf(" [DIRTY]");
                printf("\n");
            }
        }
    }
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
    printf("--- PCB %d Page Table ---\n", caller->pid);
    if (caller && caller->krnl->mm && caller->krnl->mm->pgd) {
        /* Start traversal from Level 5 (PGD) */
        print_pgtbl_recursive((void *)caller->krnl->mm->pgd, 5, 0);
    }
    printf("----------------------------------------\n");
    return 0;
}

/* * vmap_pgd_memset - Map the directory path for a range without allocating RAM
 * Used to 'reserve' user space structure in kernel
 */
int vmap_pgd_memset(struct pcb_t *caller, addr_t addr, int pgnum) 
{
    int i;
    /* Create the page table path for all pages in range */
    for(i = 0; i < pgnum; i++) {
        addr_t pgn = PAGING_PGN(addr) + i;
        /* Force allocation of intermediate tables, but result PTE is left 0 */
        __get_pte(caller->krnl->mm, pgn, 1); 
    }
    return 0;
}

/* * get_pd_from_address - Extract indices from address (Debugging helper)
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
    if (pgd) *pgd = PAGING64_ADDR_PGD(addr);
    if (p4d) *p4d = PAGING64_ADDR_P4D(addr);
    if (pud) *pud = PAGING64_ADDR_PUD(addr);
    if (pmd) *pmd = PAGING64_ADDR_PMD(addr);
    if (pt)  *pt  = PAGING64_ADDR_PT(addr);
    return 0;
}

/* Dummy/Optional functions */
int print_list_fp(struct framephy_struct *ifp) { return 0; }
int print_list_rg(struct vm_rg_struct *irg) { return 0; }
int print_list_vma(struct vm_area_struct *ivma) { return 0; }
int print_list_pgn(struct pgn_t *ip) { return 0; }

#endif