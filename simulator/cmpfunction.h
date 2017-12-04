#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cmpdevice.h"
#include "queue.h"

#ifndef CMPFUNCTION_H_
#define CMPFUNCTION_H_

void Insert_initialQ(Queue *q, int entryNum, int start)
{
	int i;
	for(i = start; i < start + entryNum; i++){
		enqueue(q, i);
	}
}

void Insert_initial_CMP(CMP *Cmp)
{
	Cmp->tlb->pageSize = Cmp->pagetable->pageSize;
	Cmp->pagetable->entryNum = 256 / Cmp->pagetable->pageSize;
	Cmp->tlb->entryNum = Cmp->pagetable->entryNum / 4;
	Cmp->cache->entryNum = Cmp->cache->size / Cmp->cache->blockSize;
	Cmp->cache->sets = Cmp->cache->entryNum / Cmp->cache->SetAsso;
	Cmp->memory->sets = Cmp->memory->size / Cmp->pagetable->pageSize;
	Cmp->VAOffsetDigits = log(Cmp->pagetable->pageSize) / log(2);
	Cmp->PAOffsetDigits = log(Cmp->cache->blockSize) / log(2);
	Cmp->PAIndexDigits = log(Cmp->cache->entryNum) / log(2);
	Cmp->PATagDigits = 32 - Cmp->PAOffsetDigits - Cmp->PAIndexDigits + log(Cmp->cache->SetAsso) / log(2);
	
	Insert_initialQ(Cmp->tlb->invalidLRUQ, Cmp->tlb->entryNum, 0);
	Insert_initialQ(Cmp->memory->LRUQ, Cmp->memory->sets, 0);
	
	int i;
	for(i = 0; i < Cmp->cache->sets; i++){
		Insert_initialQ(Cmp->cache->invalidLRUQ_set[i], Cmp->cache->SetAsso, i*Cmp->cache->SetAsso);
	}
	
	for(i = 0; i < Cmp->memory->size; i++){
		Cmp->memory->PPN[i] = i >> Cmp->VAOffsetDigits;
		Cmp->memory->words[i] = 0;
	}
}

unsigned int getAddressN(unsigned int address, int offsetDigits)
{
	return address >> offsetDigits;
}

unsigned int getOffset(unsigned int address, int offsetDigits)
{
	if(offsetDigits == 0){
		return 0;
	}
	
	unsigned int offset = address;
	int shift = 32 - offsetDigits;
	
	offset <<= shift;
	offset >>= shift;
	return offset;
}

unsigned int getIndex(unsigned int PA, int PAOffsetDigits, int PAIndexDigits)
{
	unsigned int index = PA;
	int PATagDigits = 32 - PAOffsetDigits-PAIndexDigits;
	index <<= PATagDigits;
	index >>= (32 - PAIndexDigits);
	return index;
}

unsigned int getTag(unsigned int PA, int TagDigits)
{
	return PA >> (32 - TagDigits);
}

int TLB_search(unsigned int VPN, TLB *tlb)
{
	int i;
		
	for(i = 0; i < tlb->entryNum; i++){
		if( VPN == tlb->tag[i] && tlb->validBit[i] ){ //hit
			return i;
		}
	}
	return -1;	
}

int Cache_search(unsigned int PATag, int setN, Cache *cache)
{
	int i;
	
	//在第N個set中找是否有hit 
	for(i = setN*cache->SetAsso; i < (setN+1)*cache->SetAsso; i++){
		if(cache->tag[i] == PATag && cache->validBit[i] ){ //hit
			return i;
		}
	}
	return -1;	
}

void CacheWriteBack(int Cache_indexToUnvalid, CMP *Cmp)
{
	unsigned int PA;
	int i, j;
	
	//用存好的PA整個block寫回Memory 
	for(j = 0; j < Cmp->cache->blockSize; j++){
		PA = Cmp->cache->PA[Cache_indexToUnvalid][j];
		Cmp->memory->words[PA] = Cmp->cache->words[Cache_indexToUnvalid][j];
	}
	
	//將寫入的Memory對應的PageTable項的dirty bit打開 
	for(i = 0; i < Cmp->pagetable->entryNum; i++){
		if( Cmp->pagetable->PPN[i] == Cmp->cache->PPN[Cache_indexToUnvalid] && Cmp->pagetable->validBit[i] ){
			Cmp->pagetable->dirtyBit[i] = 1;
		}
	}	
}

void PageFault_Handle(int PPN_ToReplaced, CMP *Cmp)
{
	int PT_indexToUnvaid, TLB_indexToUnvalid, Cache_indexToUnvalid, Cache_setToReQ; 
	unsigned int addressToD;
	int i, j;
	
	//printf("PageFault:\nPPN:%d \n", PPN_ToReplaced);
	
	//tlb pop to invalid q
	for(i = 0; i < Cmp->tlb->entryNum; i++){
		if(Cmp->tlb->PPN[i] == PPN_ToReplaced && Cmp->tlb->validBit[i] ){
			TLB_indexToUnvalid = i;
			Cmp->tlb->validBit[TLB_indexToUnvalid] = 0;
			enqueue( Cmp->tlb->invalidLRUQ, pop_queue( Cmp->tlb->LRUQ, TLB_indexToUnvalid));
		}
	}
	
	//cache pop to invalid q
	for(i = 0; i < Cmp->cache->entryNum; i++){ //搜尋整個Cache找要unvalid的index 
		if(Cmp->cache->PPN[i] == PPN_ToReplaced && Cmp->cache->validBit[i] ){ 
			Cache_indexToUnvalid = i;					
			Cache_setToReQ = Cache_indexToUnvalid / Cmp->cache->SetAsso;
			enqueue( Cmp->cache->invalidLRUQ_set[Cache_setToReQ], pop_queue( Cmp->cache->LRUQ_set[Cache_setToReQ], Cache_indexToUnvalid));
					
			//Cache block write back
			if( Cmp->cache->dirtyBit[Cache_indexToUnvalid] ){ 
				CacheWriteBack(Cache_indexToUnvalid, Cmp);
				Cmp->cache->dirtyBit[Cache_indexToUnvalid] = 0;
			}
			
			Cmp->cache->validBit[Cache_indexToUnvalid] = 0;
		}
	}
	
	//pagetable上原對應PPN的entry的處理 
	for(i = 0; i < Cmp->pagetable->entryNum; i++){ //找出pagetable上原使用此PNN的entry 
		if( PPN_ToReplaced == Cmp->pagetable->PPN[i] && Cmp->pagetable->validBit[i] == 1 ){ //mem取代發生 
			PT_indexToUnvaid = i;
			//Memory data write back
			if( Cmp->pagetable->dirtyBit[PT_indexToUnvaid] ){
				for(j = 0; j < Cmp->memory->size; j++){ //搜尋整個Memory找PPN一樣的write back 					
					if( Cmp->memory->PPN[j] == PPN_ToReplaced ){
						addressToD = Cmp->memory->addressInD[j];
						Cmp->disk->words[addressToD] = Cmp->memory->words[j];
					}
				}
				Cmp->pagetable->dirtyBit[PT_indexToUnvaid] = 0; 
			}		
			Cmp->pagetable->validBit[PT_indexToUnvaid] = 0;							
			
			break;
		}
	}	
}

#endif
