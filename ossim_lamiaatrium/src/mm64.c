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
	*pgd = (addr&PAGING64_ADDR_PGD_MASK)>>PAGING64_ADDR_PGD_LOBIT;
	*p4d = (addr&PAGING64_ADDR_P4D_MASK)>>PAGING64_ADDR_P4D_LOBIT;
	*pud = (addr&PAGING64_ADDR_PUD_MASK)>>PAGING64_ADDR_PUD_LOBIT;
	*pmd = (addr&PAGING64_ADDR_PMD_MASK)>>PAGING64_ADDR_PMD_LOBIT;
	*pt = (addr&PAGING64_ADDR_PT_MASK)>>PAGING64_ADDR_PT_LOBIT;

	/* TODO: implement the page direactories mapping */

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

  // addr_t *pte;
  // addr_t pgd=0;
  // addr_t p4d=0;
  // addr_t pud=0;
  // addr_t pmd=0;
  // addr_t pt=0;
  addr_t *pte = NULL;
  // dummy pte alloc to avoid runtime error
  //pte = malloc(sizeof(addr_t));
#ifdef MM64	
  /* Get value from the system */
  /* TODO Perform multi-level page mapping */
  addr_t pdg_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
  get_pd_from_pagenum(pgn, &pdg_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);
  //get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  //... krnl->mm->pgd
  //... krnl->mm->pt
  //pte = &krnl->mm->pt;
  return -1;
#else
  pte = (addr_t*)&krnl->mm->pgd[pgn];
#endif
	if(pte != NULL){
    CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
    //SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
    SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
    CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

    SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
    SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
  }
  // SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  // SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  // SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  // SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

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

  // addr_t *pte;
  // addr_t pgd=0;
  // addr_t p4d=0;
  // addr_t pud=0;
  // addr_t pmd=0;
  // addr_t pt=0;
  addr_t *pte = NULL;
	
  // dummy pte alloc to avoid runtime error
  //pte = malloc(sizeof(addr_t));
#ifdef MM64	
  /* Get value from the system */
  /* TODO Perform multi-level page mapping */
  addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);
  //get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  //... krnl->mm->pgd
  //... krnl->mm->pt
  //pte = &krnl->mm->pt;
  return -1;
#else
  pte = (addr_t*)&krnl->mm->pgd[pgn];
#endif
  if(pte != NULL){
    SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
    CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

    SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
  }
  // SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  // CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  // SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
  struct krnl_t *krnl = caller->krnl;
  // uint32_t pte = 0;
  // addr_t pgd=0;
  // addr_t p4d=0;
  // addr_t pud=0;
  // addr_t pmd=0;
  // addr_t	pt=0;
  uint32_t pte = 0;
#ifdef MM64
  addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);
	
  /* TODO Perform multi-level page mapping */
  //get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  //... krnl->mm->pgd
  //... krnl->mm->pt
  //pte = &krnl->mm->pt;	
#else
  if(pgn < PAGING_MAX_PGN){
    pte = krnl->mm->pgd[pgn];
  }
#endif
	
  return pte;
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
	struct krnl_t *krnl = caller->krnl;
#ifdef MM64
  addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
  addr_t *pte_addr;
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);

  return -1;

#else
	krnl->mm->pgd[pgn]=pte_val;
#endif

	return 0;
}


/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
  //int pgit = 0;
  //uint64_t pattern = 0xdeadbeef;

  /* TODO memset the page table with given pattern
   */
  struct krnl_t *krnl = caller->krnl;
  addr_t pgn_start = PAGING_PGN(addr);
  int i;

#ifdef MM64
  return 0;

#else
  if(pgn_start + pgnum > PAGING_MAX_PGN){
    pgnum = PAGING_MAX_PGN - pgn_start;
  }
  for(i = 0; i < pgnum; i++){
    krnl->mm->pgd[pgn_start + i] = 0;
  }

#endif

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
 addr_t pgn_start = PAGING_PGN(addr);

  /* TODO: update the rg_end and rg_start of ret_rg 
  //ret_rg->rg_end =  ....
  //ret_rg->rg_start = ...
  //ret_rg->vmaid = ...
  */
  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + pgnum * PAGING_PAGESZ;

  /* TODO map range of frame to address space
   *      [addr to addr + pgnum*PAGING_PAGESZ
   *      in page table caller->krnl->mm->pgd,
   *                    caller->krnl->mm->pud...
   *                    ...
   */
  for(pgit = 0; pgit < pgnum; pgit++){
    if(fpit == NULL){
      break;
    }
    pte_set_fpn(caller, pgn_start + pgit, fpit->fpn);
    enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn_start + pgit);
    fpit = fpit->fp_next;
  }

  /* Tracking for later page replacement activities (if needed)
   * Enqueue new usage page */
  //enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn64 + pgit);

  //return 0;
  return pgit * PAGING_PAGESZ;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  addr_t fpn;
  int pgit;
  struct framephy_struct *newfp_str;
  int count = 0;

  /* TODO: allocate the page 
  //caller-> ...
  //frm_lst-> ...
  */
  *frm_lst = NULL;

/*
  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    // TODO: allocate the page 
    if (MEMPHY_get_freefp(caller->mram, &fpn) == 0)
    {
      newfp_str->fpn = fpn;
    }
    else
    { // TODO: ERROR CODE of obtaining somes but not enough frames
    }
  }
*/
  for(pgit = 0; pgit < req_pgnum; pgit++){
    if(MEMPHY_get_freefp(caller->krnl->mram, &fpn) == 0){
      newfp_str = (struct framephy_struct*) malloc(sizeof(struct framephy_struct));
      newfp_str->fpn = fpn;

      newfp_str->fp_next = *frm_lst;
      *frm_lst = newfp_str;

      count++;
    }
    else{
      return -3000;
    }
  }

  /* End TODO */

  //return 0;
  return count;
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
  int pgnum = incpgnum;

  /*@bksysnet: author provides a feasible solution of getting frames
   *FATAL logic in here, wrong behaviour if we have not enough page
   *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   *Don't try to perform that case in this simple work, it will result
   *in endless procedure of swap-off to get frame and we have not provide
   *duplicate control mechanism, keep it simple
   */
  ret_alloc = alloc_pages_range(caller, pgnum, &frm_lst);

  // if (ret_alloc < 0 && ret_alloc != -3000)
  //   return -1;

  // /* Out of memory */
  // if (ret_alloc == -3000)
  // {
  //   return -1;
  // }
  if(ret_alloc < 0){
    return -1;
  }
  if(ret_alloc == 0){
    return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
  //vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);
  addr_t mapped_bytes = vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);
  if(mapped_bytes == 0){
    return -1;
  }

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
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
#ifdef MM64
  /* TODO init page table directory */
   //mm->pgd = ...
   //mm->p4d = ...
   //mm->pud = ...
   //mm->pmd = ...
   //mm->pt = ...
  mm->pgd = (uint64_t*) malloc(PAGING_MAX_PGN * sizeof(uint64_t));
  memset(mm->pgd, 0, PAGING_MAX_PGN * sizeof(uint64_t));

#else
  mm->pgd = (uint32_t*) malloc(PAGING_MAX_PGN * sizeof(uint32_t));
  memset(mm->pgd, 0, PAGING_MAX_PGN * sizeof(uint32_t));

#endif

  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  //vma0->vm_end = vma0->vm_start;
  vma0->vm_end = BIT(PAGING_CPU_BUS_WIDTH);
  vma0->sbrk = vma0->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  /* TODO update VMA0 next */
  // vma0->next = ...
  vma0->vm_next = NULL;

  /* Point vma owner backward */
  //vma0->vm_mm = mm; 
  vma0->vm_mm = mm;

  /* TODO: update mmap */
  //mm->mmap = ...
  //mm->symrgtbl = ...
  mm->mmap = vma0;
  memset(mm->symrgtbl, 0, PAGING_MAX_SYMTBL_SZ * sizeof(struct vm_rg_struct));

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

// int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
// {
// //  addr_t pgn_start;//, pgn_end;
// //  addr_t pgit;
// //  struct krnl_t *krnl = caller->krnl;

//   addr_t pgd=0;
//   addr_t p4d=0;
//   addr_t pud=0;
//   addr_t pmd=0;
//   addr_t pt=0;

//   get_pd_from_address(start, &pgd, &p4d, &pud, &pmd, &pt);

//   /* TODO traverse the page map and dump the page directory entries */

//   return 0;
// }

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
    struct krnl_t *krnl = caller->krnl;
    addr_t pgn_start;
    addr_t pgn_end;
    addr_t pgit;
    
    // Khởi tạo các biến 64-bit (chỉ cần nếu MM64 được bật)
    // addr_t pgd=0, p4d=0, pud=0, pmd=0, pt=0; 

    // 1. Tính toán PGN (Page Number) bắt đầu và kết thúc
    pgn_start = PAGING_PGN(start);
    
    if (end == (addr_t)-1) {
        // Nếu end là -1, in tất cả các trang
        pgn_end = PAGING_MAX_PGN;
    } else {
        pgn_end = PAGING_PGN(end);
    }

    if (pgn_end > PAGING_MAX_PGN) {
        pgn_end = PAGING_MAX_PGN;
    }

    printf("--- PCB %d Page Table (PGN %d to %d) ---\n", caller->pid, (int)pgn_start, (int)pgn_end);

    /* TODO traverse the page map and dump the page directory entries */
    
    // 2. Duyệt qua các trang ảo trong dải [pgn_start, pgn_end)
    for (pgit = pgn_start; pgit < pgn_end; pgit++)
    {
        uint32_t pte = krnl->mm->pgd[pgit];
        
        // Chỉ in các PTEs không rỗng (PTE != 0)
        if (pte != 0) 
        {
            printf("  PGN [" FORMAT_ADDR "]: ", pgit);
            
            // a. Kiểm tra trạng thái SWAP
            if (PAGING_PTE_SWAPPED_MASK & pte)
            {
                // Trang đang ở SWAP
                addr_t swp_off = PAGING_SWP(pte);
                printf("SWAPPED (SWP %s" FORMAT_ADDR ")", 
                       (PAGING_PAGE_PRESENT(pte) ? "P+ " : ""), swp_off);
            }
            // b. Kiểm tra trạng thái PRESENT
            else if (PAGING_PAGE_PRESENT(pte))
            {
                // Trang đang ở RAM
                addr_t fpn = PAGING_FPN(pte);
                printf("PRESENT (FPN " FORMAT_ADDR ")", fpn);
            }
            // c. Trang không Present và không Swapped (nên là 0, nhưng ta kiểm tra lại)
            else
            {
                printf("INVALID");
            }
            
            // Kiểm tra cờ DIRTY (nếu có)
            if (PAGING_PTE_DIRTY_MASK & pte)
            {
                printf(" [DIRTY]");
            }
            
            printf("\n");
        }
    }
    printf("----------------------------------------\n");

    return 0;
}

#endif  //def MM64
