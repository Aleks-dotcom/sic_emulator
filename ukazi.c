#include <stdlib.h>
#include <stdio.h>
extern void *IP;
extern int A,B,X,T,S,F,L;
// use funkcije bodo imele isti signature ampak ker jih bo vecina delal z registri
// ki so global variables bodo argument uporabljale sam tiste k delajo z memorijem direkt
// this is subject to change
void (*ukazi_functions[0xfc])(void * mem, int * r1, int * r2);
// poklicemo iz main fila da se ukazi_functions populata za pravimi vrednostmi

void addr(void * mem, int * r1, int * r2 ){
	int res = *r1 + *r2;
	*r2 = res;
}

void add(void *mem, int * r1,int *r2){
	int res = (*(int *)mem);
	res = res & 0xffffff;
	A  = A + res;
}

void sta(void * mem, int * r1, int * r2){
	int tmp = A & 0xffffff;
	int cnt=0;
	while (tmp > 0){
		(*(char *) mem) = tmp &0xff;
		tmp >>= 0x8;
		mem+=1;
	}
}

void lda(void *mem, int * r1, int *r2){
	int res = (*(int*)mem);
	res = res & 0xffffff;
	A = res;	
}

void init_ukazi(){
	// tukaj nasopamo funkcije za vse opcode oz tiste k se mi jih bo dalo implementirat
	ukazi_functions[0x18] = add;
	ukazi_functions[0x58] = addr;
	
}

