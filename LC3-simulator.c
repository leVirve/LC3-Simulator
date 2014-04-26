#include<stdio.h>
#define ENTRY_POINT 0x30
#define OP_BR  0x0 
#define OP_ADD 0x1
#define OP_LD  0x2
#define OP_ST  0x3
#define OP_JSR_JSRR 0x4
#define OP_AND 0x5
#define OP_LDR 0x6
#define OP_STR 0x7
#define OP_NOT 0x9
#define OP_LDI 0xa
#define OP_STI 0xb
#define OP_JMP_RET 0xc
#define OP_LEA 0xe
#define OP_TRAP 0xf

struct CPU{
	unsigned short reg[8];
	unsigned short PC;
};

struct CPU cpu;
unsigned short memory[128];
unsigned short last_altered_reg;

unsigned short SExt(unsigned short targ, int n)
{
	if(targ >> n-1 == 0)  return targ;
	else  return (0xffff << n) + targ;
}

unsigned short ZExt(unsigned short targ)
{
	return targ; 
}

void OpNOT(unsigned short inst)
{
	unsigned short src1,dst;
	src1 = ((inst >> 6) & 0x7); /*sro1*/
	dst = ((inst >> 9) & 0x7); /*dst*/
	cpu.reg[dst] = ~ cpu.reg[src1];
	last_altered_reg = dst;
}

void OpAND(unsigned short inst)
{
	unsigned short src1,src2,imm5,dst;
	src1 = ((inst >> 6) & 0x7); /*src1*/
	dst = ((inst >> 9) & 0x7); /*dst*/
	if(((inst >> 5) & 0x1) == 0) {
		src2 = inst & 0x7; /*src2*/
		cpu.reg[dst] = cpu.reg[src1] & cpu.reg[src2];
	} else {
		imm5 = SExt(inst & 0x1f, 5); /*imm5*/
		cpu.reg[dst] = cpu.reg[src1] & imm5; 
	}
	last_altered_reg = dst;
}

void OpADD(unsigned short inst)
{
	unsigned short src1,src2,imm5,dst;
	src1 = ((inst >> 6) & 0x7); /*src1*/
	dst = ((inst >> 9) & 0x7); /*dst*/
	if(((inst >> 5) & 0x1) == 0) {
		src2 = inst & 0x7; /*src2*/
		cpu.reg[dst] = cpu.reg[src1] + cpu.reg[src2];
	} else if(((inst >> 5) & 0x1) == 1) {
		imm5 = SExt(inst & 0x1f,5); /*imm5*/
		cpu.reg[dst] = cpu.reg[src1] + imm5;
	}
	last_altered_reg = dst;
}

void OpST(unsigned short inst)
{
	unsigned short src,pcoffset9,madr;
	src = ((inst >> 9) & 0x7); /*src*/
	pcoffset9 = SExt(inst & 0x1ff, 9); /*PCoffset9*/
	madr = cpu.PC + pcoffset9;
	memory[madr] = cpu.reg[src];
}

void OpSTI(unsigned short inst)
{
	unsigned short src,pcoffset9,madr;
	src = ((inst >> 9) & 0x7); /*src*/
	pcoffset9 = SExt(inst & 0x1ff, 9); /*PCoffset9*/
	madr = cpu.PC + pcoffset9;
	memory[memory[madr]] = cpu.reg[src];
}

void OpSTR(unsigned short inst)
{
	unsigned short src,baser,offset6,madr;
	src = ((inst >> 9) & 0x7); /*src*/
	baser = ((inst >> 6) & 0x7); /*baser*/
	offset6 = SExt(inst & 0x3f, 6); /*offset6*/
	madr = cpu.reg[baser] + offset6;
	memory[madr] = cpu.reg[src];
}

void OpLD(unsigned short inst)
{
	unsigned short dst,pcoffset9,madr;
	dst = ((inst >> 9) & 0x7); /*dst*/
	pcoffset9 = SExt(inst & 0x1ff, 9); /*PCoffset9*/
	madr = cpu.PC + pcoffset9;
	cpu.reg[dst] = memory[madr];
	last_altered_reg = dst;
}

void OpLDI(unsigned short inst)
{
	unsigned short dst,pcoffset9,madr;
	dst = ((inst >> 9) & 0x7); /*dst*/
	pcoffset9 = SExt(inst & 0x1ff, 9); /*PCoffset9*/
	madr = cpu.PC + pcoffset9;
	cpu.reg[dst] = memory[memory[madr]];
	last_altered_reg = dst;
}
 
void OpLDR(unsigned short inst)
{
	unsigned short dst,baser,offset6,madr;
	dst = ((inst >> 9) & 0x7); /*dst*/
	baser = ((inst >> 6) & 0x7); /*baser*/
	offset6 = SExt(inst & 0x3f, 6); /*offset6*/
	madr = cpu.reg[baser] + offset6;
	cpu.reg[dst] = memory[madr];
	last_altered_reg = dst;
}

void OpLEA(unsigned short inst)
{
	unsigned short dst,pcoffset9;
	dst = ((inst >> 9) & 0x7); /*dst*/
	pcoffset9 = SExt(inst & 0x1ff, 9); /*PCoffset9*/
	cpu.reg[dst] = cpu.PC + pcoffset9;
	last_altered_reg = dst;
}

void OpBR(unsigned short inst)
{
	unsigned short n,z,p,pcoffset9;
	n = ((inst >> 11) & 0x1); /*negtive*/
	z = ((inst >> 10) & 0x1); /*zero*/
	p = ((inst >> 9) & 0x1); /*positive*/
	pcoffset9 = SExt(inst & 0x1ff, 9); /*PCoffset9*/
	unsigned short targ_reg = cpu.reg[last_altered_reg];
	if((n == 1 && targ_reg < 0) || (z == 1 && targ_reg == 0) || (p == 1 && targ_reg > 0)) {
		cpu.PC = cpu.PC + pcoffset9;
	}
}

void OpJSR_JSRR(unsigned short inst)
{
	unsigned short pcoffset11,baser;
	cpu.reg[7] = cpu.PC;
	if(((inst >> 11) & 0x1) == 0) {
		baser = (inst >> 6) & 0x7; /*baser*/
		cpu.PC = cpu.reg[baser];
	} else {
		pcoffset11 = SExt(inst & 0x7ff, 11); /*PCoffset11*/
		cpu.PC = cpu.PC + pcoffset11;		
	}
}

void OpTRAP(unsigned short inst)
{
	unsigned short trapvect8;
	trapvect8 = inst & 0xff; /*trapvect8*/
	cpu.reg[7] = cpu.PC;
	cpu.PC = memory[ZExt(trapvect8)];
}

void OpJMP_RET(unsigned short inst)
{
	unsigned short baser;
	baser = (inst >> 6) & 0x7; /*baser*/
	cpu.PC = cpu.reg[baser];
}

void Decode(unsigned short instr)
{
	switch(instr >> 12) {
		case OP_ADD:
			OpADD(instr); 
			break;
		case OP_AND:
			OpAND(instr);
			break;
		case OP_NOT:
			OpNOT(instr);		
			break;
		case OP_ST:
			OpST(instr);		
			break;
		case OP_STI:
			OpSTI(instr);		
			break;
		case OP_STR:
			OpSTR(instr);		
			break;
		case OP_LD:
			OpLD(instr);		
			break;
		case OP_LDI:
			OpLDI(instr);		
			break;
		case OP_LDR:
			OpLDR(instr);		
			break;
		case OP_LEA:
			OpLEA(instr);
			break;
		case OP_BR:
			OpBR(instr);
			break;
		case OP_JSR_JSRR:
			OpJSR_JSRR(instr);
			break;
		case OP_TRAP:
			OpTRAP(instr);
			break;
		case OP_JMP_RET:
			OpJMP_RET(instr);
			break;
		default:
			break;
	}
}

// ==== PRINT ====
static void print()
{
	FILE *out = fopen("output.txt","a");
    int i;
	fprintf(out,"======================================\n");
	fprintf(out,"PC = %2x\n\n", cpu.PC);
	for ( i = 0; i < 8; ++i) {
		fprintf(out,"reg[%d]=%4x\n", i, cpu.reg[i]);
	}
	for ( i = 0; i < 0x30; i+=4) {
		fprintf(out,"memory[0x%2x]=%4x, ", i, memory[i]);
		fprintf(out,"memory[0x%2x]=%4x, ", i+1, memory[i+1]);
		fprintf(out,"memory[0x%2x]=%4x, ", i+2, memory[i+2]);
		fprintf(out,"memory[0x%2x]=%4x\n", i+3, memory[i+3]);
	}
	fprintf(out,"======================================\n");
}

int main(int argc, char *argv[])
{
	FILE *in = fopen(argv[1],"rb");
	
	// ==== LOADER ==== 
	int i = ENTRY_POINT;
	while(fread(&memory[i],sizeof(short),1,in)!=0) 	i++;
	in = fopen("output.txt","w");
	// ==== INSTRUCTION ==== 
	cpu.PC = ENTRY_POINT;
	while(1) {
		int mar = cpu.PC;
		if(memory[mar] == 0x0 ) break;
		cpu.PC = cpu.PC + 1;
		Decode(memory[mar]);
		print();
	}
	
	fclose(in);
	return 0;
}
