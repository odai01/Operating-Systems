#include "os.h"


void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){
  /*we have 5 levels, since the page table node occupies a framse size:4KB
  and each PTE is 64bits=8B, thus each node has 512 children, thus we need 9bits
  and since the used bits are 57-12=45 so we have 45/9=5 levels */
  const int levels=5;
  const uint16_t children=512;
  uint64_t curr_ind;
  uint64_t *pte=(uint64_t *)phys_to_virt(pt<<12);
  uint64_t temp;
  for(int i=levels-1;i>0;i--){
    curr_ind= (vpn >> (i*9)) & (children-1);
    if ((pte[curr_ind]&1)==0)/*this means valid bit=0*/
    {
      if(ppn!=NO_MAPPING){
        temp=alloc_page_frame()<<12;
        pte[curr_ind]= temp + 1;
        }
      else{return;}
    }
    pte=(uint64_t *)phys_to_virt((pte[curr_ind]>>12)<<12);
    /*the shifts above are made to guarantee that offset is zero*/
  }
  curr_ind= (vpn) & (children-1);
  if(ppn!=NO_MAPPING){
    temp=ppn<<12;
    pte[curr_ind]= temp +1;/*to make the valid bit=1*/
  }
  else{
    pte[curr_ind]=(pte[curr_ind]>>1)<<1;/*to make vaild=0*/
  }
}


uint64_t page_table_query(uint64_t pt, uint64_t vpn){
  const int levels=5;
  const uint16_t children=512;
  uint64_t curr_ind;
  uint64_t *pte=(uint64_t *)phys_to_virt(pt<<12);
  for(int i=levels-1;i>0;i--){
    curr_ind= (vpn >> (i*9)) & (children-1);
    if ((pte[curr_ind]&1)==0){return NO_MAPPING;}
    pte=(uint64_t *)phys_to_virt((pte[curr_ind]>>12)<<12);
  }
  curr_ind= (vpn) & (children-1);
  if ((pte[curr_ind]&1)==0){return NO_MAPPING;}
  return (pte[curr_ind]>>12);
}
