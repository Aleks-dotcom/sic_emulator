#include <stdlib.h>
#include <stdio.h>
extern unsigned long long IP;
extern int A,B,X,T,S,F,L;
#define ull unsigned long long 
// use funkcije bodo imele isti signature ampak ker jih bo vecina delal z registri
// ki so global variables bodo argument uporabljale sam tiste k delajo z memorijem direkt
// this is subject to change
void (*ukazi_functions[0xfc])(ull *mem, int * r1, int * r2);
// poklicemo iz main fila da se ukazi_functions populata za pravimi vrednostmi
ull obrni_indijanca(ull i){
	ull b0, b1 ,b2; 
	b0 = (i & 0x000000ff) << 16;
	b1 = (i & 0x0000ff00);
	b2 = (i & 0x00ff0000) >> 16;
	ull res = 0x00 | b0 | b1 | b2;
	return res;
}


void addr(ull *mem, int * r1, int * r2 ){
	puts("klicem addr");
	int res = *r1 + *r2;
	*r2 = res;
}

void add(ull *mem, int * r1,int *r2){
	puts("klicem add");
	ull res;
	if ( (ull) mem > 0xffffff){
		//indirect
		res = obrni_indijanca(*mem);
	}else{
		//direct
		res = ((ull)mem & 0xffffff);	
	}
	A  = A + res;
}

void sta(ull *mem, int * r1, int * r2){
	puts("klicem sta");
	char * to_write = (char*) mem;
	ull tmp = obrni_indijanca((ull)A);
	int cnt=0;
	printf("na adresi %p je vrednost %lld\n",mem,*mem);
	while (tmp > 0){
		*to_write = tmp &0xff;
		tmp >>= 0x8;
		to_write++;
	}
	printf("na adresi %p je vrednost %lld\n",mem,*mem);
}

void lda(ull *mem, int * r1, int *r2){
	puts("klicem lda");
	ull res;
	if ( (ull) mem > 0xffffff){
		//memory
		res = obrni_indijanca(*mem);
	}else{
		res = (ull)mem & 0xffffff;
	}
	A = res;	
}

void init_ukazi(){
	// tukaj nasopamo funkcije za vse opcode oz tiste k se mi jih bo dalo implementirat
	ukazi_functions[0x18] = add;
	ukazi_functions[0x58] = addr;
	ukazi_functions[0x0c] = sta;
	ukazi_functions[0x00] = lda;
	
}

