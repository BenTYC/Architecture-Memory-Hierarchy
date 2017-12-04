#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

#ifndef CMPDEVICE_H_
#define CMPDEVICE_H_

#define DiskSize 256

struct tlb{
	int PPN[DiskSize];
	int validBit[DiskSize];	
	unsigned int tag[DiskSize];
	int pageSize;
	int entryNum;
	int hitNum;
	int missNum;
	Queue *LRUQ;
	Queue *invalidLRUQ;
};
typedef struct tlb TLB;

void initialTLB(TLB *t)
{
	int i;
	
	t->hitNum = 0;
	t->missNum = 0;
	for(i = 0; i < DiskSize; i++){		
		t->validBit[i] = 0;
	}
	init(t->LRUQ);
	init(t->invalidLRUQ);
}

struct pagetable{
	int PPN[DiskSize];	
	int validBit[DiskSize];
	int dirtyBit[DiskSize];
	int pageSize;
	int entryNum;
	int hitNum;
	int missNum;
};
typedef struct pagetable PageTable;

void initialPageTable(PageTable *PT)
{
	int i;
	
	for(i = 0; i < DiskSize; i++){
		PT->validBit[i] = 0;
		PT->dirtyBit[i] = 0;
	}
	PT->hitNum = 0;
	PT->missNum = 0;
}

struct cache{
	unsigned int words[DiskSize][DiskSize];
	unsigned int PA[DiskSize][DiskSize];
	unsigned int VA[DiskSize][DiskSize];
	unsigned int PPN[DiskSize];
	int validBit[DiskSize];
	int dirtyBit[DiskSize];
	int tag[DiskSize];
	int size;
	int blockSize;
	int entryNum;
	int SetAsso;
	int sets;
	int hitNum;
	int missNum;
	Queue* LRUQ_set[DiskSize];
	Queue* invalidLRUQ_set[DiskSize];
};
typedef struct cache Cache;

void initialCache(Cache *ca)
{
	int i;
	
	for(i = 0; i < DiskSize; i++){
		ca->validBit[i] = 0;
		ca->dirtyBit[i] = 0;
		init(ca->LRUQ_set[i]);
		init(ca->invalidLRUQ_set[i]);
	}
	ca->hitNum = 0;
	ca->missNum = 0;
}

struct memory{
	unsigned int words[DiskSize];
	unsigned int addressInD[DiskSize];
	unsigned int PPN[DiskSize];	
	int size;
	int sets;
	Queue *LRUQ;	
};
typedef struct memory Memory;

void initialMemory(Memory *mem)
{
	init(mem->LRUQ);
}

struct disk{
	unsigned int words[DiskSize];	
};
typedef struct disk Disk;

void initialDisk(Disk *disk)
{
	int i;	
	for(i = 0; i < DiskSize; i++){
		disk->words[i] = 0;
	}
}

struct cmp{
	TLB *tlb;
	PageTable *pagetable;
	Cache *cache;
	Memory *memory;
	Disk *disk;
	int VAOffsetDigits;
	int PAOffsetDigits;
	int PAIndexDigits;
	int PATagDigits;	
};
typedef struct cmp CMP;

void NewCmp(CMP *Cmp)
{
	int i;
	Cmp->tlb = (TLB *)malloc(sizeof(TLB));
	Cmp->pagetable = (PageTable *)malloc(sizeof(PageTable));
	Cmp->cache = (Cache *)malloc(sizeof(Cache));
	Cmp->memory = (Memory *)malloc(sizeof(Memory));
	Cmp->disk = (Disk *)malloc(sizeof(Disk));
	
	Cmp->tlb->invalidLRUQ = (Queue *)malloc(sizeof(Queue));
	Cmp->tlb->LRUQ = (Queue *)malloc(sizeof(Queue));
	Cmp->memory->LRUQ = (Queue *)malloc(sizeof(Queue));
	for(i = 0; i < DiskSize; i++){
		Cmp->cache->LRUQ_set[i] = (Queue *)malloc(sizeof(Queue));
		Cmp->cache->invalidLRUQ_set[i] = (Queue *)malloc(sizeof(Queue));
	}
}

void initialCmp(CMP *Cmp)
{
	initialTLB(Cmp->tlb);
	initialPageTable(Cmp->pagetable);
	initialCache(Cmp->cache);
	initialMemory(Cmp->memory);
	initialDisk(Cmp->disk);	
}

void Print_CMPPara(CMP *Cmp)
{
	printf("MemSize:   %3d\n", Cmp->memory->size);
	printf("PageSize:  %3d\n", Cmp->pagetable->pageSize);
	printf("CacheSize: %3d\n", Cmp->cache->size);
	printf("BlockSize: %3d\n", Cmp->cache->blockSize);
	printf("SetAsso:   %3d\n", Cmp->cache->SetAsso);
	printf("MemSets:   %3d\n", Cmp->memory->sets);
	printf("tPageSize: %3d\n", Cmp->tlb->pageSize);
	printf("PTentries: %3d\n", Cmp->pagetable->entryNum);
	printf("Tentries:  %3d\n", Cmp->tlb->entryNum);
	printf("Centries:  %3d\n", Cmp->cache->entryNum);	
	printf("CacheSets: %3d\n", Cmp->cache->sets);
	printf("VAOffsetDigits: %2d\n", Cmp->VAOffsetDigits);
	printf("PAOffsetDigits: %2d\n", Cmp->PAOffsetDigits);
	printf("PAIndexDigits : %2d\n", Cmp->PAIndexDigits);	
	printf("PATagDigits   : %2d\n", Cmp->PATagDigits);
	
	printf("TLBiQ: ");   print_queue(Cmp->tlb->invalidLRUQ);
	printf("TLBQ: ");    print_queue(Cmp->tlb->LRUQ);
	printf("MemoryQ: "); print_queue(Cmp->memory->LRUQ);
	
	int i;
	printf("\nCacheQ:\n");
	for(i = 0; i < Cmp->cache->sets; i++){		
		printf("Set[%d]:\n", i);
		printf("invalid: "); print_queue(Cmp->cache->invalidLRUQ_set[i]);
		printf("valid  : "); print_queue(Cmp->cache->LRUQ_set[i]);
	}		
	printf("\n");	
}

void Print_CMP(CMP *Cmp)
{
	int i, j;
	
	printf("TLB:\n");
	printf("Entry  Tag  PPN\n");
	for(i = 0; i < Cmp->tlb->entryNum; i++){
		if( Cmp->tlb->validBit[i] ){
			printf("%4d %4d %4d\n", i, Cmp->tlb->tag[i], Cmp->tlb->PPN[i]);
		}
	}
	printf("TLBiQ: ");   print_queue(Cmp->tlb->invalidLRUQ);
	printf("TLBQ: ");    print_queue(Cmp->tlb->LRUQ);
	printf("hits:%2d miss:%2d\n\n", Cmp->tlb->hitNum, Cmp->tlb->missNum);
	
	printf("Page Table:\n");
	printf("Entry  PPN  dirty\n");
	for(i = 0; i < Cmp->pagetable->entryNum; i++){
		if( Cmp->pagetable->validBit[i] ){
			printf("%4d %4d %4d\n", i, Cmp->pagetable->PPN[i], Cmp->pagetable->dirtyBit[i]);
		}
	}
	printf("MemoryQ: "); print_queue(Cmp->memory->LRUQ);
	printf("hits:%2d miss:%2d\n\n", Cmp->pagetable->hitNum, Cmp->pagetable->missNum);
	
	/*printf("Memory:\n");
	printf("Entry  VA     Data\n");
	for(i = 0; i < Cmp->memory->size; i++){
		printf("%4d   %2d %08x\n", i, Cmp->memory->addressInD[i], Cmp->memory->words[i]);
	}*/
	
	printf("\nCache:\n");
	printf("Entry  PPN  Tag  dirty  VA    Data\n");
	for(i = 0; i < Cmp->cache->entryNum; i++){
		if( Cmp->cache->validBit[i] ){
			printf("%4d %4d %4d  %4d", i, Cmp->cache->PPN[i], Cmp->cache->tag[i], Cmp->cache->dirtyBit[i]);			
			for(j = 0; j < Cmp->cache->blockSize; j++){	
				printf("  %4d", Cmp->cache->VA[i][0]);	
				printf("  %08x", Cmp->cache->words[i][j]);
			}
			printf("\n");
		}
	}
	
	printf("CacheQ:\n");
	for(i = 0; i < Cmp->cache->sets; i++){		
		printf("Set[%d]:\n", i);
		printf("invalid: "); print_queue(Cmp->cache->invalidLRUQ_set[i]);
		printf("valid  : "); print_queue(Cmp->cache->LRUQ_set[i]);
	}
	printf("hits:%2d miss:%2d\n\n", Cmp->cache->hitNum, Cmp->cache->missNum);
	
}


#endif

