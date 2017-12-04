#include <stdio.h>
#include <string.h>

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

// R-Type Instructions
#define RTYPEOP 0x00
#define ADD	 0x20
#define SUB	 0x22
#define AND	 0x24
#define OR	 0x25
#define XOR	 0x26
#define NOR	 0x27
#define NAND 0x28
#define SLT	 0x2A
#define SLL	 0x00
#define SRL	 0x02
#define SRA	 0x03
#define JR	 0x08

// I-Type Instructions
#define ADDI 0x08
#define LW	 0x23
#define LH	 0x21
#define LHU  0x25
#define LB   0x20
#define LBU  0x24
#define SW	 0x2B
#define SH	 0x29
#define SB	 0x28
#define LUI	 0x0F
#define ANDI 0x0C
#define ORI	 0x0D
#define NORI 0x0E
#define SLTI 0x0A
#define BEQ	 0x04
#define BNE	 0x05

// J-Type Instructions
#define J	 0x02
#define JAL	 0x03

//Specialized Instruction
#define HALT 0x3F

//Array number
#define RegSize 32

//function
unsigned int BtoW( unsigned int H[4] );
void Print_reg( unsigned int registers[RegSize], unsigned int PC, int cycleCount, FILE *fps);
unsigned char getOpcode( unsigned int Instruction);
unsigned char getFunct( unsigned int Instruction);
unsigned char getRs( unsigned int Instruction);
unsigned char getRt( unsigned int Instruction);
unsigned char getRd( unsigned int Instruction);
unsigned char getShamt( unsigned int Instruction);
unsigned int getImmediate( unsigned int Instruction);
unsigned int getAddress(unsigned int Instruction);
int unsignedToSigned(unsigned int unsignedN, int OrigianlDigits);


unsigned int BtoW( unsigned int H[4] )
{
	return ( H[0] << 24 ) + ( H[1] << 16 ) + ( H[2] << 8 ) + H[3];
}

void Print_reg(unsigned int registers[RegSize], unsigned int PC, int cycleCount, FILE *fps)
{
	int i;
	
	fprintf(fps, "cycle %d\n", cycleCount);
	for(i = 0; i < RegSize; i++){
		fprintf(fps, "$%02d: 0x%08X\n", i, registers[i]);
	}
	fprintf(fps, "PC: 0x%08X\n\n\n", PC);
	return;
}

unsigned char getOpcode( unsigned int Instruction)
{
	return (Instruction >> 26) & 0x3F;
}

unsigned char getFunct( unsigned int Instruction)
{
	return Instruction & 0x3F;
}

unsigned char getRs( unsigned int Instruction)
{
	return (Instruction >> 21) & 0x1F;
}

unsigned char getRt(unsigned int Instruction)
{
	return (Instruction >> 16) & 0x1F;
}

unsigned char getRd( unsigned int Instruction)
{
	return (Instruction >> 11) & 0x1F;
}

unsigned char getShamt( unsigned int Instruction)
{
	return (Instruction >> 6) & 0x1F;
}

unsigned int getImmediate( unsigned int Instruction)
{
	return Instruction & 0xFFFF;
}

unsigned int getAddress(unsigned int Instruction)
{
	return Instruction & 0x3FFFFFF;
}

int unsignedToSigned(unsigned int unsignedN, int OrigianlDigits)
{
	if(OrigianlDigits == 0){
		return 0;
	}
	
	int signedN = unsignedN;
	signedN <<= (32 - OrigianlDigits);
	signedN >>= (32 - OrigianlDigits);
	return signedN;
}

void *getOptype( char *OPtype, unsigned int Instruction)
{
	unsigned char opcode = getOpcode( Instruction );
	unsigned char funct = getFunct( Instruction );	
	
	if(opcode == HALT){
		strcpy(OPtype, "HALT");
	}else if( opcode == RTYPEOP){	
		switch(funct){
			case ADD:
				strcpy(OPtype, "ADD");	
				break;
			case SUB:
				strcpy(OPtype, "SUB");
				break;
			case AND:
				strcpy(OPtype, "AND");
				break;
			case OR:
				strcpy(OPtype, "OR");
				break;
			case XOR:
				strcpy(OPtype, "XOR");
				break;
			case NOR:
				strcpy(OPtype, "NOR");
				break;
			case NAND:
				strcpy(OPtype, "NAND");
				break;
			case SLT:
				strcpy(OPtype, "SLT");
				break;
			case SLL:
				if( getRd(Instruction)==0 && getRt(Instruction)==0 && getShamt(Instruction)==0){
					strcpy(OPtype, "NOP");
				}else{
					strcpy(OPtype, "SLL");
				}
				break;
			case SRL:
				strcpy(OPtype, "SRL");
				break;
			case SRA:
				strcpy(OPtype, "SRA");
				break;
			case JR:
				strcpy(OPtype, "JR");
				break;
			default:
				strcpy(OPtype, "Rwrong");	
				break;			
		}		
	}else{
		switch(opcode){			
			case ADDI:					
				strcpy(OPtype, "ADDI");
				break;
			case LW:
				strcpy(OPtype, "LW");
				break;
			case LH:
				strcpy(OPtype, "LH");
				break;
			case LHU:
				strcpy(OPtype, "LHU");	
				break;
			case LB:
				strcpy(OPtype, "LB");	
				break;
			case LBU:
				strcpy(OPtype, "LBU");	
				break;
			case SW:
				strcpy(OPtype, "SW");
				break;
			case SH:
				strcpy(OPtype, "SH");
				break;
			case SB:
				strcpy(OPtype, "SB");
				break;
			case LUI:
				strcpy(OPtype, "LUI");
				break;
			case ANDI:
				strcpy(OPtype, "ANDI");	
				break;	
			case ORI:
				strcpy(OPtype, "ORI");	
				break;
			case NORI:
				strcpy(OPtype, "NORI");	
				break;
			case SLTI:
				strcpy(OPtype, "SLTI");
				break;
			case BEQ:
				strcpy(OPtype, "BEQ");
				break;	
			case BNE:
				strcpy(OPtype, "BNE");
				break;
			case J:
				strcpy(OPtype, "J");
				break;
			case JAL:
				strcpy(OPtype, "JAL");
				break;
			default:
				strcpy(OPtype, "OPwrong");
				break;
		}
	}	
}

#endif /* SIMULATOR_H_ */
