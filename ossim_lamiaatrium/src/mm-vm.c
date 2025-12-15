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

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  if (mm == NULL) return NULL;

  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn , addr_t swpfpn)
{
  __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{
  struct vm_rg_struct * newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if(cur_vma == NULL || cur_vma->sbrk + alignedsz > cur_vma->vm_end){
    return NULL;
  }

  newrg = malloc(sizeof(struct vm_rg_struct));
  if (newrg == NULL) {
    return NULL;
  }
  
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + alignedsz;
  newrg->rg_next = NULL;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
  if (vmastart >= vmaend) return -1;
  struct vm_area_struct *vma = caller->mm->mmap;
  if (vma == NULL)
  {
    return 0;
  }

  while(vma != NULL){
    if(vma->vm_id == vmaid){
      vma = vma->vm_next;
      continue;
    }
    if(OVERLAP(vmastart, vmaend, vma->vm_start, vma->vm_end)){
      return -1;
    }
    vma = vma->vm_next;
  }

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  if(cur_vma == NULL){
    return -1;
  }

  addr_t aligned_inc_sz = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = aligned_inc_sz / PAGING_PAGESZ;

  addr_t old_sbrk = cur_vma->sbrk;
  addr_t new_sbrk = old_sbrk + aligned_inc_sz;

  if(new_sbrk > cur_vma->vm_end){
    return -1;
  }

  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  newrg->rg_start = old_sbrk;
  newrg->rg_end = new_sbrk;
  newrg->rg_next = NULL;

  if(vm_map_ram(caller, cur_vma->vm_start, cur_vma->vm_end, old_sbrk, incnumpage, newrg) < 0){
    free(newrg);
    return -1;
  }

  cur_vma->sbrk = new_sbrk;

  return 0;
}


// #endif