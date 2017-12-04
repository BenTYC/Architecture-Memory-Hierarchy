#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "simulator.h"
#include "cmp.h"

int main(int argc, char *argv[]) 
{
	//build CMP
	CMP iCMP, dCMP;
	CMP *iCmp = &iCMP, *dCmp = &dCMP;
	
	NewCmp(iCmp);
	NewCmp(dCmp);
	initialCmp(iCmp);
	initialCmp(dCmp);
	
	
	//Insert parameter
	InsertPara(argc, argv, iCmp, dCmp);
	
	/*printf("iCmp:\n");
	Print_CMPPara(iCmp);
	printf("dCmp:\n");
	Print_CMPPara(dCmp);*/
	
	
	//Insert input data
	FILE *fp, *fps, *fpr;
	int PCa[4], SPa[4], wordsI[4], wordsD[4], temp[4];
	int iNum, dNum, cycleCount = 0;
	unsigned int PC, SP, PC_origin, Instruction;
	unsigned int registers[RegSize];
	int i, j;
	
	
	//Initialize registers 	
	for(i = 0; i < RegSize; i++) {
		registers[i] = 0x0;
	}
	
	
	//Insert input data (Disk SP PC)
	fp = fopen("iimage.bin","rb");
	assert(fp != NULL);
	
	for(i = 0; i < 4; i++) {
		PCa[i]=fgetc(fp);
	}
	for(i = 0; i < 4; i++) {
		wordsI[i]=fgetc(fp);
	}
	
	iNum = BtoW(wordsI);
	PC_origin = BtoW(PCa);
	
	for(i = PC_origin/4; i < PC_origin/4 + iNum; i++) {
		for( j = 0; j < 4; j++) {
			temp[j]=fgetc(fp);
		}
		iCmp->disk->words[i] = BtoW(temp);
	}
	
	fclose(fp);
		
	fp = fopen("dimage.bin","rb");
	assert(fp != NULL);
	for(i = 0; i < 4; i++) {
		SPa[i]=fgetc(fp);
	}
	for(i = 0; i < 4; i++) {
		wordsD[i]=fgetc(fp);
	}
	
	dNum = BtoW(wordsD);
	SP = BtoW(SPa);
	
	for(i = 0; i < dNum; i++) {
		for( j = 0; j < 4; j++) {
			temp[j] = fgetc(fp);
		}
		dCmp->disk->words[i] = BtoW(temp);
	}
	
	fclose(fp);
			
	
	PC = PC_origin;
	registers[29] = SP;
	
	
	//Setup for simulator
	unsigned char opcode, funct, rs, rt, rd, shamt;
	unsigned int address, immediate, DataAddress, Data;
	int signedImmediate, tempn, temp1, temp2;
	char optype[10];	
	
	
	//Run simulator
	fps = fopen("snapshot.rpt", "w");
	
	while( 1 ) {
		
		//Print 
	    Print_reg(registers, PC, cycleCount, fps);
		
		
		//advance cycle
		cycleCount++;
		/*if(cycleCount > 11200){
			break;
		}
		printf("cycle:%2d PC=%2d \n", cycleCount, (PC - PC_origin)/4);*/
		 
		 
		//Get instruction 
		Instruction = CMP_Read(PC, iCmp);
		
		
		//Get opcode & Check halt		
		opcode = getOpcode( Instruction );
		/*getOptype(optype, Instruction);
		printf("optype:%4s ins:%08x\n", optype, Instruction);*/
		//printf("\nICMP:\n");Print_CMP(iCmp);
		
		if( opcode == HALT ){
			break;
		}		
		
		//PC+4
		PC += 4;
		
		
		//get para for Simulation
		funct = getFunct(Instruction);
		shamt = getShamt(Instruction);
		immediate = getImmediate(Instruction); 
		address = getAddress(Instruction);
		rd = getRd(Instruction); 
		rt = getRt(Instruction);
		rs = getRs(Instruction);
		signedImmediate = unsignedToSigned( immediate, 16);
		DataAddress	= registers[rs] + signedImmediate;
		
		//Simulate
		if( opcode == RTYPEOP ) {
			switch (funct) {
				case ADD:
					registers[rd] = registers[rs] + registers[rt];	
					break;
				case SUB:
					registers[rd] = registers[rs] - registers[rt];	
					break;
				case AND:
					registers[rd] = registers[rs] & registers[rt];	
					break;
				case OR:
					registers[rd] = registers[rs] | registers[rt];	
					break;
				case XOR:
					registers[rd] = registers[rs] ^ registers[rt];	
					break;
				case NOR:
					registers[rd] = ~(registers[rs] | registers[rt]);	
					break;
				case NAND:
					registers[rd] = ~(registers[rs] & registers[rt]);	
					break;
				case SLT:
					temp1 = registers[rs];
					temp2 = registers[rt];
					registers[rd] = (temp1 < temp2);
					break;
				case SLL:
					registers[rd] = registers[rt] << shamt;	
					break;
				case SRL:
					registers[rd] = registers[rt] >> shamt;	
					break;
				case SRA:
					tempn = registers[rt];
					registers[rd] = tempn >> shamt;	
					break;
				case JR:
					PC = registers[rs];
					break;
				default:
					printf("opcodeR wrong\n");	
					break;			
			}			
		}else if( opcode == J || opcode == JAL ){
			switch (opcode) {
				case J:
					PC = (PC & 0xF0000000) | (address << 2);
					break;
				case JAL:
					registers[31] = PC;	
					PC = (PC & 0xF0000000) | (address << 2);
					break;
				default:
					printf("opcodeJ wrong\n");
					break;					
			}
		}else{ 
			switch (opcode) {
				case ADDI:	
					registers[rt] = registers[rs] + signedImmediate;
					break;
				case LW:  
					Data = CMP_Read( DataAddress, dCmp);
					registers[rt] = Data;	
					break;
				case LH:
					Data = CMP_Read( DataAddress, dCmp);
					tempn = Data;
					if( DataAddress % 4 == 0){
						tempn >>= 16;
					}else{
						tempn <<= 16;
						tempn >>= 16;
					}
					registers[rt] = tempn;	
					break;
				case LHU:
					Data = CMP_Read( DataAddress, dCmp);
					if( DataAddress % 4 == 0){
						registers[rt] = Data >> 16;
					}else{
						registers[rt] = Data & 0xFFFF;
					}	
					break;
				case LB:
					Data = CMP_Read( DataAddress, dCmp);
					tempn = Data;
					if( DataAddress % 4 == 0){
						tempn >>= 24;
					}else if( DataAddress % 4 == 1){
						tempn <<= 8;
						tempn >>= 24;
					}else if( DataAddress % 4 == 2){
						tempn <<= 16;
						tempn >>= 24;
					}else {
						tempn <<= 24;
						tempn >>= 24;
					}
					registers[rt] = tempn;	
					break;
				case LBU:
					Data = CMP_Read( DataAddress, dCmp);
					if( DataAddress % 4 == 0){
						registers[rt] = Data >> 24;
					}else if( DataAddress % 4 == 1){
						registers[rt] = (Data & 0xFF0000) >> 16;
					}else if( DataAddress % 4 == 2){
						registers[rt] = (Data & 0xFF00) >> 8;
					}else {
						registers[rt] = Data & 0xFF;
					}	
					break;
				case SW: 
					Data = CMP_Read( DataAddress, dCmp);
					Data = registers[rt];
					CMP_Write( Data, DataAddress, dCmp);
					break;
				case SH:
					Data = CMP_Read( DataAddress, dCmp);
					if( DataAddress % 4 == 0){
						Data = (Data & 0xFFFF) | (registers[rt] << 16);
					}else{
						Data = (Data & 0xFFFF0000) | (registers[rt] & 0xFFFF);	
					}
					CMP_Write( Data, DataAddress, dCmp);
					break;
				case SB:
					Data = CMP_Read( DataAddress, dCmp);
					if( DataAddress % 4 == 0){
						Data = (Data & 0x00FFFFFF) | (registers[rt] << 24);
					}else if( DataAddress % 4 == 1){
						Data = (Data & 0xFF00FFFF) | ((registers[rt] << 16) & 0xFF0000);
					}else if( DataAddress % 4 == 2){
						Data = (Data & 0xFFFF00FF) | ((registers[rt] << 8) & 0xFF00);
					}else {
						Data = (Data & 0xFFFFFF00) | (registers[rt] & 0xFF);
					}
					CMP_Write( Data, DataAddress, dCmp);
					break;
				case LUI:
					registers[rt] = immediate << 16;	
					break;
				case ANDI:
					registers[rt] = registers[rs] & immediate;	
					break;	
				case ORI:
					registers[rt] = registers[rs] | immediate;	
					break;
				case NORI:
					registers[rt] = ~(registers[rs] | immediate);	
					break;
				case SLTI:
					tempn = registers[rs];
					registers[rt] = (tempn < signedImmediate);
					break;
				case BEQ:
					if (registers[rs] == registers[rt]) {
						PC = PC + signedImmediate*4;
					}	
					break;	
				case BNE:
					if (registers[rs] != registers[rt]) {
						PC = PC + signedImmediate*4;
					}	
					break;		
				default:
					printf("opcode wrong\n");
					break;				
			}
		}
		registers[0] = 0;	
		
		/*if(cycleCount > 49800){
			printf("\nDCMP:\n");Print_CMP(dCmp);
		}*/		
	}	
	fclose(fps);	
	
	//Print report
	fpr = fopen("report.rpt", "w");		
	Print_report( fpr, iCmp, dCmp);		
	fclose(fpr);
	
	return 0;
}
