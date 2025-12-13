/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */
 
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

#if !defined(MM64)
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

/*
 * init_pte - Initialize PTE entry
 */
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
  printf("[ERROR] %s: This feature 32 bit mode is deprecated\n", __func__);
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
  printf("[ERROR] %s: This feature 32 bit mode is deprecated\n", __func__);
  return 0;
}

/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  addr_t *pte = &caller->krnl->mm->pgd[pgn];
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  addr_t *pte = &caller->krnl->mm->pgd[pgn];
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
  return 0;
}

/* Get PTE page table entry */
uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
  if(caller == NULL || caller->krnl->mm == NULL || caller->krnl->mm->pgd == NULL)
      return 0; 
  return caller->krnl->mm->pgd[pgn];
}

/* Set PTE page table entry */
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
	caller->krnl->mm->pgd[pgn] = pte_val;
	return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region
{                                                   
  int pgit = 0;
  int pgn = PAGING_PGN(addr);
  
  if (ret_rg != NULL) {
      ret_rg->rg_start = addr;
      ret_rg->rg_end = addr + pgnum * PAGING_PAGESZ;
  }
  
  struct framephy_struct *fpit = frames;
  for (pgit = 0; pgit < pgnum; pgit++) {
      if (fpit == NULL) break; 
      
      pte_set_fpn(caller, pgn + pgit, fpit->fpn); 
      
      enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn + pgit); 

      fpit = fpit->fp_next;
  }
  return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 */
addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  int pgit;
  int count = 0;
  struct framephy_struct *newfp_str;
  addr_t fpn; 

  *frm_lst = NULL;

  for(pgit = 0; pgit < req_pgnum; pgit++){
      if(MEMPHY_get_freefp(caller->krnl->mram, &fpn) == 0){
          newfp_str = (struct framephy_struct*) malloc(sizeof(struct framephy_struct));
          newfp_str->fpn = fpn;
          newfp_str->fp_next = *frm_lst;
          *frm_lst = newfp_str;
          count++;
      } else {
          return -3000;
      }
  }
  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  addr_t ret_alloc = 0;
  int pgnum = incpgnum;

  ret_alloc = alloc_pages_range(caller, pgnum, &frm_lst);

  if(ret_alloc < 0){
      return -1;
  }

  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  return 0;
}

/* Swap copy content page from source frame to destination frame */
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
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct * vma = malloc(sizeof(struct vm_area_struct));

  mm->pgd = malloc(PAGING_MAX_PGN * sizeof(addr_t)); 
  
  for(int i = 0; i < PAGING_MAX_PGN; i++)
    mm->pgd[i] = 0;

  vma->vm_id = 0;
  vma->vm_start = 0;
  vma->vm_end = PAGING_MAX_PGN * PAGING_PAGESZ; 
  
  vma->sbrk = vma->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma->vm_start, vma->vm_end);
  enlist_vm_rg_node(&vma->vm_freerg_list, first_rg);

  vma->vm_next = NULL;
  vma->vm_mm = mm; /* Gán ngược lại pointer mm */
  mm->mmap = vma;
  mm->fifo_pgn = NULL; 

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

// int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
// {
//   int pgn_start, pgn_end;
//   int pgit;

//   if(caller == NULL || caller->mm == NULL) return -1;
  
//   pgn_start = PAGING_PGN(start);
//   pgn_end = PAGING_PGN(end);
  
//   if (end == (uint32_t)-1) pgn_end = PAGING_MAX_PGN;

//   printf("print_pgtbl: \n");

//   for(pgit = pgn_start; pgit < pgn_end; pgit++)
//   {
//     uint32_t pte = caller->mm->pgd[pgit];
    
//     if ((pte & PAGING_PTE_PRESENT_MASK)) 
//     {
//         printf("\t%08ld: %08x\n", pgit * sizeof(uint32_t), pte);
//     }
//   }

//   return 0;
// }

int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
 {
   int pgn_start, pgn_end;
   int pgit;
 
   if (end == -1)
   {
     pgn_start = 0;
     struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, 0);
     end = cur_vma->vm_end;
   }
   pgn_start = PAGING_PGN(start);
   pgn_end = PAGING_PGN(end);
 
   printf("print_pgtbl: %d - %d", start, end);
   if (caller == NULL) { printf("NULL caller\n"); return -1;}
   printf("\n");
 
   for (pgit = pgn_start; pgit < pgn_end; pgit++)
   {
     printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->krnl->mm->pgd[pgit]);
   }
 
   return 0;
 }

/* Các hàm Deprecated / Stub */
int vmap_pgd_memset(struct pcb_t *caller, addr_t addr, int pgnum) { return 0; }
int print_list_fp(struct framephy_struct *ifp) { return 0; }
int print_list_rg(struct vm_rg_struct *irg) { return 0; }
int print_list_vma(struct vm_area_struct *ivma) { return 0; }
int print_list_pgn(struct pgn_t *ip) { return 0; }

#endif //ndef MM64