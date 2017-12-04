#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmpdevice.h"
#include "cmpfunction.h"
#include "queue.h"

void InsertPara(int argc, char *argv[], CMP *iCmp, CMP *dCmp)
{	
	if( argc > 1 ){
		iCmp->memory->size = atoi(argv[1])/4;
		dCmp->memory->size = atoi(argv[2])/4;
		iCmp->pagetable->pageSize = atoi(argv[3])/4;
		dCmp->pagetable->pageSize = atoi(argv[4])/4;
		iCmp->cache->size = atoi(argv[5])/4;
		iCmp->cache->blockSize = atoi(argv[6])/4;
		iCmp->cache->SetAsso = atoi(argv[7]);
		dCmp->cache->size = atoi(argv[8])/4;
		dCmp->cache->blockSize = atoi(argv[9])/4;
		dCmp->cache->SetAsso = atoi(argv[10]);
	}else{
		iCmp->memory->size = 16;
		dCmp->memory->size = 8;
		iCmp->pagetable->pageSize = 2;
		dCmp->pagetable->pageSize = 4;
		iCmp->cache->size = 4;
		iCmp->cache->blockSize = 1;
		iCmp->cache->SetAsso = 4;
		dCmp->cache->size = 4;
		dCmp->cache->blockSize = 1;
		dCmp->cache->SetAsso = 1;	
		/*iCmp->memory->size = 128;
		dCmp->memory->size = 256;
		iCmp->pagetable->pageSize = 32;
		dCmp->pagetable->pageSize = 16;
		iCmp->cache->size = 16;
		iCmp->cache->blockSize = 1;
		iCmp->cache->SetAsso = 8;
		dCmp->cache->size = 8;
		dCmp->cache->blockSize = 1;
		dCmp->cache->SetAsso = 4;*/
	}
	Insert_initial_CMP(iCmp);
	Insert_initial_CMP(dCmp);	
}

void Print_report( FILE *fpr, CMP *iCmp, CMP *dCmp)
{
	fprintf( fpr, "ICache :\n"); 
	fprintf( fpr, "# hits: %u\n", iCmp->cache->hitNum ); 
	fprintf( fpr, "# misses: %u\n\n", iCmp->cache->missNum ); 
 
	fprintf( fpr, "DCache :\n"); 
	fprintf( fpr, "# hits: %u\n", dCmp->cache->hitNum ); 
	fprintf( fpr, "# misses: %u\n\n", dCmp->cache->missNum ); 
 
	fprintf( fpr, "ITLB :\n"); 
	fprintf( fpr, "# hits: %u\n", iCmp->tlb->hitNum ); 
	fprintf( fpr, "# misses: %u\n\n", iCmp->tlb->missNum ); 
 
	fprintf( fpr, "DTLB :\n"); 
	fprintf( fpr, "# hits: %u\n", dCmp->tlb->hitNum ); 
	fprintf( fpr, "# misses: %u\n\n", dCmp->tlb->missNum ); 
 
	fprintf( fpr, "IPageTable :\n"); 
	fprintf( fpr, "# hits: %u\n", iCmp->pagetable->hitNum ); 
	fprintf( fpr, "# misses: %u\n\n", iCmp->pagetable->missNum ); 
 
	fprintf( fpr, "DPageTable :\n"); 
	fprintf( fpr, "# hits: %u\n", dCmp->pagetable->hitNum ); 
	fprintf( fpr, "# misses: %u\n\n", dCmp->pagetable->missNum ); 
}

unsigned int CMP_Read(unsigned int address, CMP *Cmp)
{
	unsigned int Data;
	unsigned int virtualAddress, VAOffset, VPN, PPN, physicalAddress, PA_toPut, DAddress_moveToM;
	int TLB_indexOrMiss, PT_indexOrMiss, PT_indexToUnvaid;
	int i;
	
	//分解 
	virtualAddress = address/4;
	VAOffset = getOffset(virtualAddress, Cmp->VAOffsetDigits);
	VPN = getAddressN(virtualAddress, Cmp->VAOffsetDigits);	
	//printf("VA:%08x VPN:%08x Offset:%08x\n", virtualAddress, VPN, VAOffset);
	
	
	//查PPN是否在TLB上                             Full-Asso: VPN即Tag 
	TLB_indexOrMiss = TLB_search(VPN, Cmp->tlb); //hit回傳index number, miss則回傳-1 	
	//printf("TLB_indexOrMiss:%d\n", TLB_indexOrMiss);	
	
	if( TLB_indexOrMiss >= 0 ){ //TLB hit handle
		Cmp->tlb->hitNum++;
		PPN = Cmp->tlb->PPN[TLB_indexOrMiss];
		enqueue( Cmp->tlb->LRUQ, pop_queue( Cmp->tlb->LRUQ, TLB_indexOrMiss));		
	}else{ //TLB miss handle
		Cmp->tlb->missNum++;
		
		//下到PageTable找		
		if( Cmp->pagetable->validBit[VPN] ){//PT hit
			Cmp->pagetable->hitNum++;
			PPN = Cmp->pagetable->PPN[VPN];
			enqueue( Cmp->memory->LRUQ, pop_queue( Cmp->memory->LRUQ, PPN));			
		}else{//page fault
			Cmp->pagetable->missNum++;
			
			//生成PPN			
			PPN = dequeue(Cmp->memory->LRUQ); 
			enqueue( Cmp->memory->LRUQ, PPN);
			
			//關三個valid bit + update 兩個Q(Mem的在上一行) + Write back
			PageFault_Handle(PPN, Cmp);
			
			//Swap: 將Data和information傳到memory 一個Page Size 
			for(i = 0; i < Cmp->pagetable->pageSize; i++){
				PA_toPut = (PPN << Cmp->VAOffsetDigits) + i;
				DAddress_moveToM = (VPN << Cmp->VAOffsetDigits) + i;
				Cmp->memory->words[PA_toPut] = Cmp->disk->words[DAddress_moveToM];
				Cmp->memory->addressInD[PA_toPut] = DAddress_moveToM;
			}
			
			//Page table update						
			Cmp->pagetable->PPN[VPN] = PPN;
			Cmp->pagetable->validBit[VPN] = 1;				
		}
				
		//選TLB的index 
		if( empty(Cmp->tlb->invalidLRUQ) ){
			TLB_indexOrMiss = dequeue(Cmp->tlb->LRUQ);
		}else{
			TLB_indexOrMiss = dequeue(Cmp->tlb->invalidLRUQ);
		}		
		enqueue( Cmp->tlb->LRUQ, TLB_indexOrMiss);
		
		//page table放上TLB 
		//Cmp->tlb->PPN[TLB_indexOrMiss] = PPN;
		Cmp->tlb->PPN[TLB_indexOrMiss] = Cmp->pagetable->PPN[VPN];
		Cmp->tlb->tag[TLB_indexOrMiss] = VPN;
		Cmp->tlb->validBit[TLB_indexOrMiss] = 1;
	}	
	
	
	//得出PA 
	physicalAddress = ( PPN << Cmp->VAOffsetDigits ) | VAOffset;
	//printf("PPN:%07x VAOffset:%0x", PPN, VAOffset);
	
	
	//Cache	
	unsigned int PAOffset, PAIndex, PATag, PBN, PA_MemToCache;
	int Cache_indexOrMiss, Cache_setN;	
	
	PAOffset = getOffset( physicalAddress, Cmp->PAOffsetDigits);
	PBN = getAddressN( physicalAddress, Cmp->PAOffsetDigits);
	PAIndex = getIndex( physicalAddress, Cmp->PAOffsetDigits, Cmp->PAIndexDigits);
	PATag = getTag( physicalAddress, Cmp->PATagDigits);
	Cache_setN = PBN % Cmp->cache->sets;
	//printf("PA:%d PATag:%d PAOffset:%d setN:%d\n", physicalAddress, PATag, PAOffset, setN);
	
	Cache_indexOrMiss = Cache_search(PATag, Cache_setN, Cmp->cache);
	//printf("Cache_indexOrMiss:%d\n", Cache_indexOrMiss);
	
	if( Cache_indexOrMiss >= 0 ){ //Cache hit
		Cmp->cache->hitNum++;
		Cache_indexOrMiss = pop_queue( Cmp->cache->LRUQ_set[Cache_setN], Cache_indexOrMiss);
		enqueue( Cmp->cache->LRUQ_set[Cache_setN], Cache_indexOrMiss);	
	}else{ //Cache miss
		Cmp->cache->missNum++;
		
		//從Q選擇要放的block 
		if( empty(Cmp->cache->invalidLRUQ_set[Cache_setN]) ){
			Cache_indexOrMiss = dequeue(Cmp->cache->LRUQ_set[Cache_setN]);
		}else{
			Cache_indexOrMiss = dequeue(Cmp->cache->invalidLRUQ_set[Cache_setN]);
		}
		enqueue( Cmp->cache->LRUQ_set[Cache_setN], Cache_indexOrMiss);
		
		//若原本block為dirty, write back to memory
		if( Cmp->cache->dirtyBit[Cache_indexOrMiss] && Cmp->cache->validBit[Cache_indexOrMiss] ){ 
			CacheWriteBack(Cache_indexOrMiss, Cmp);
			Cmp->cache->dirtyBit[Cache_indexOrMiss] = 0;
			Cmp->cache->validBit[Cache_indexOrMiss] = 0;
		}		 
								
		//從Mem抓 根據PBN一次抓一個block Size 				
		for(i = 0; i < Cmp->cache->blockSize; i++){
			PA_MemToCache = (PBN << Cmp->PAOffsetDigits) + i;
			Cmp->cache->words[Cache_indexOrMiss][i] = Cmp->memory->words[PA_MemToCache];
			Cmp->cache->PA[Cache_indexOrMiss][i] = PA_MemToCache;
			Cmp->cache->VA[Cache_indexOrMiss][i] = Cmp->memory->addressInD[PA_MemToCache];
		}
		Cmp->cache->validBit[Cache_indexOrMiss] = 1;
		Cmp->cache->PPN[Cache_indexOrMiss] = PPN;
		Cmp->cache->tag[Cache_indexOrMiss] = PATag;		
	}
	//update 
			
	Data = Cmp->cache->words[Cache_indexOrMiss][PAOffset];	
	//printf("Cache_indexOrMiss:%d PAOffset:%d", Cache_indexOrMiss, PAOffset);
	
	return Data;
}

void CMP_Write(unsigned int Data, unsigned int DataAddress, CMP *Cmp)
{
	unsigned int virtualAddress = DataAddress/4;
	int i, j;
	
	//搜尋整個Cache, 找出VA所在的W, 寫入 	
	for(i = 0; i < Cmp->cache->entryNum; i++){
		for(j = 0; j < Cmp->cache->blockSize; j++){
			if(Cmp->cache->VA[i][j] == virtualAddress && Cmp->cache->validBit[i] ){
				Cmp->cache->words[i][j] = Data;
				Cmp->cache->dirtyBit[i] = 1;
				return;
			}			
		}
	}
}

