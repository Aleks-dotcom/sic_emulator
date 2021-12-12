#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ukazi.c"
#define ull unsigned long long


extern void (*ukazi_functions[0xfc])(ull* mem,int *r1,int*r2);
extern int read_fds[100];
extern int read_fds_cnt;
extern int write_fds[100];
extern int write_fds_cnt;
//registers
int A,B,T,S,X,L,F;
// instructions type 1 opcodes
char ukaz_tipa_1[6] = {0xc4,0xc0,0xf4,0xc8,0xf0,0xf8};
// instructions type 2 opcodes
char ukaz_tipa_2[10] = {0x90, 0xb4,0xa0,0x9c,0x98,0xac,0xa4,0xa8,0x94,0xb0};
// array of register pointers for easier decoding of instructions
int * registers[7] = {&A,&X,&L,&B,&S,&T,&F};
unsigned char rabijo_pointer[13] = {0x78,0x14,0x7c,0xe8,0x84,0x10,0x30,0x34,0x38,0x3c,0x48,0x54,0x0c};
ull IP;
ull HIP;
ull SW;
int a = 0;

struct Header{
	char H [1];
	char program_name[6];
	char start_addr[6];
	char program_legth[6];
};

// abstract structure to easier access the code in T segments
struct T{
	char t[1];
	char start_addr[6];
	char length_of_code[2];
	char code[60]; // hacky way to get the code
};
ull obrni_indijanca_main(ull i){
	ull b0, b1 ,b2; 
	b0 = (i & 0x000000ff) << 16;
	b1 = (i & 0x0000ff00);
	b2 = (i & 0x00ff0000) >> 16;
	ull res = 0x00 | b0 | b1 | b2;
	return res;
}

int rabi_pointer(unsigned char actual_oc){
	for (int i = 0; i< sizeof(rabijo_pointer)/sizeof(unsigned char); ++i){
		if (rabijo_pointer[i] == actual_oc)
			return 1;
	
	}
	return 0;

}

int one_byte(char oc){
	for (int i = 0; i < 6; ++i){
		if (oc == ukaz_tipa_1[i])
			return 1;
	
	}
	return 0;

}
//return 2s complement of <n> based on bitcount
int twos_complement(int n, int bitcount){
	printf("dobil sm %d",n);	
	if (( n & (1 << (bitcount -1))) > 0){
		puts("negativno je");
		printf("negativno je, returnam %d\n",((1 <<bitcount) -n ) *-1);
		return ((1 << bitcount) - n) * -1;
	
	}else{
		return n;
	}
}


int two_byte(char oc){
	for( int i = 0; i< 10; ++i){
		if (oc == ukaz_tipa_2[i])
			return 1;
	}
	return 0;
}


void * char_to_bytes(char * a, int size, char * write){
	int cnt=0;
	for (int i = 0; i < size; i+=2){
		char r[2];
		strncpy(r,a+i,2);
		write[cnt]= (char) strtol(r,NULL,16);
		cnt++;
	}
}





int main(int argc, char ** argv){
	if (argc != 2){
		perror("usage: ./emulator <prog.obj>");
		exit(-1);
	}
	int offset,size;
	scanf("%d %d",&offset,&size);
	// inicializiraj funkcije za oppcode v ukazi.c
	init_ukazi();

	//testcall

	// delimiter za strtok
	const char delim[1]= "\n";
	// memory za program k bo runnal 
	void * emulated_memory  = mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	printf("%p",emulated_memory);
	FILE * data = fopen(argv[1], "r");
	char * file = (char * ) calloc(0x1000,0);
	fread(file,0x1000,1,data);

	struct Header * h = (struct Header*)calloc(1,sizeof(struct Header));
	h = (struct Header*)strtok(file,delim); 
	int t_count = (int) strtol(h->program_legth,NULL,16);
	char ** t_segments = calloc(1,sizeof(char*) * (t_count/ 29) + sizeof(char*));
	char * tmp = strtok(NULL,delim);
	printf("t_count: %ld\n",sizeof(char*) * t_count/29 +sizeof(char*));
	char cnt = 0;
	while (tmp[0] != 'E'){
		printf("t-> %s\n",tmp);
		char * t = (char *) calloc(1,sizeof(struct T));
		strcpy(t,tmp);
		t_segments[cnt] = t;
		tmp = strtok(NULL,delim);
		cnt++;
	}	

	//nalozimo kodo iz t segmentov v memory <emulated_memory>
	//gdb pravi da to dela prou in js mu vrjamm
	
	int j = 0;
	for (int i = 0; i<cnt; ++i){
		char * a = t_segments[i];
		a +=7;
		char len[2];
		strncpy(len,a,2);
		a+=2;
		int l = (int)strtol(len,NULL,16);
		char copy[l];
		char_to_bytes(a,l*2,copy); 	
		memcpy(emulated_memory+j,copy,l);
		j += l;
		printf("napisali smo %d bytu -> %s\n",l,copy);
	}

	// START EXECUTING
	
	// instruction pointer damo na zacetek memorija
	IP = (ull)emulated_memory;
	// helper instruction pointer damo tudi na zacetek memorija in tuki bo tud
	// ostal
	HIP = (ull)emulated_memory;
	// registre postavimo na 0
	A =0;
	B =0;
	T =0;
	S =0;
	X =0;
	L =0;
	F =0;
	// deklariramo spremenljivke ki jih bomo rabli u loopu da se ne
	// deklarirajo ob vsaki iteraciji
	unsigned char b1,b2,b3,b4;
	int * r1;
	int * r2;
	ull m = (ull)emulated_memory;
	unsigned char register_data, r1_oc,r2_oc;
	// runnamom loop dokler nam mati da kruha in mleka xD
	
	while (1){
		//preberemo 1 byte iz IPja
		ull* ptr_za_picke2=0;
		ull* ptr_za_picke1=0;
		unsigned char b1  = (*(char*)IP);
		IP++;
		if (b1 == 0)
			break;
		printf("IP-> %x\n",b1);	
		// izluscimo dejanksi opcode
		unsigned char actual_oc =  b1 & 0xfc;	
		printf("actual_od -> %x\n",actual_oc);
		// ce je tipa ena recemo da nardi kar mora -> povecamo IP za 1
	
		if (one_byte(actual_oc) ==1){
			puts("ukaz tipa 1");
			ukazi_functions[actual_oc]((ull*)m,r1,r2);

		}else if( two_byte(actual_oc) == 1){
			puts("ukaz tipa 2");
			m = 0;
			register_data = (*(char*)IP);
			IP++;
			printf("register_data: %x",register_data);
			r1_oc = (register_data & 0xf0) >> 4;
			r2_oc = register_data & 0x0f;
			printf("r1_oc: %d, r2_oc: %d",r1_oc,r2_oc);
			r1 = registers[r1_oc];
			r2 = registers[r2_oc];
		}else{
			// ce smo prsli do tle potem mamo instruction tipa 3/4
			// let the fun begin!!
			
			// ce je tip 3/4 ne rabimo registrov
			r1 = 0; 
			r2 = 0;

			// dobimo vn n in i
			char n = (b1 & 0x2) ? 1 : 0;
			char i = (b1 & 0x1) ? 1 : 0;

			// potegnemo vn se 2 byta ker itak bo vsaj 3 byte dolg
			// instruction
			//
			
			b2 = (*(char*)IP);
			IP++;
			char x = (b2 & 0x80) ? 1 : 0;
			char b = (b2 & 0x40) ? 1 : 0;
			char p = (b2 & 0x20) ? 1 : 0;
			char e = (b2 & 0x10) ? 1 : 0;

			b3 = (*(char*)IP);
			IP++;
			// displacement loh zracunamo ze zdej ker ga rabimo v obeh
			// primerih

			printf("b3 = %x\n", b3);
			int displacement = (b2&0x0f) << 8;
			displacement = displacement | (b3 & 0xff);
			printf("displacement: %d\n",displacement);
			//pogruntamo a mamo 3 al 4
		
			if (e == 1){
				// imamo instruction tipa 4 addr je absoluten 
				puts("ukaz tipa 4");
				b4 = (*(char*)IP);
				IP++;
				displacement <<= 0x8;
				displacement |= (b4&0xff);	
				displacement = twos_complement(displacement,20);
				m = displacement + HIP;
			}else{
				// imamo instruction tipa 3
				displacement = twos_complement(displacement,12);
				puts("ukaz tipa 3");
				printf("biti: p->%d, b->%d, x->%d, n->%d,i->%d\n",p,b,x,n,i);
				int tmpIP = IP -HIP;
				m = displacement;
				printf(" disp: %x, tmpIP: %x\n",displacement,tmpIP);
				if (p == 1)
					m += tmpIP;
				if (b == 1)
					m += B;
				if (x == 1)
					m += X;
				if ( !(i== 1 && n == 0)){
					m += HIP;
					ptr_za_picke1 = (ull*)m;
					m = *ptr_za_picke1 & 0xffffff;
					m = obrni_indijanca_main(m);
				}
				printf("mem1: %llx\n",m);
				if (n == 1 && i == 0){
					if ( !((m > 0xffffff) && ((ll)m > 0)))
						m += HIP;
					ptr_za_picke2 = (ull*) m;
					m = *ptr_za_picke2 & 0xffffff; // dereference mem;
					m = obrni_indijanca_main(m);
					
				}
				printf("mem: %llx\n",m);
			}
		}
		printf("mem 0: %p\n",(void*)HIP);
		printf("mem za picke: %p\n",&m);
		if ( rabi_pointer(actual_oc)){
			ull * final;
			if ( ptr_za_picke2 != 0){
				final = ptr_za_picke2;
			}else if (ptr_za_picke1 != 0){
				final = ptr_za_picke1;
			}else{
				final = (ull*) (HIP+m);
			}
			printf("final: %p",final);
			ukazi_functions[actual_oc](final,r1,r2);
		}else{
			ukazi_functions[actual_oc]((ull*)m,r1,r2);
		}
		memory_view(offset,size);
		printf("REGISTRI: A->0x%x, B->0x%x, S->0x%x, X->0x%x, T->0x%x, L->0x%x\n",A,B,S,X,T,L);
		puts("======================================");
		//TODO: zdj k smo poflexali s kodo jo je treba pa se stestira.
		//to pa kot dobri programerji, pustimo za ju3 :)
	}
	// cleanup
	for (int i = 0; i < read_fds_cnt; ++i)
		close(read_fds[i]);
	for (int i = 0; i< write_fds_cnt; ++i)
		close(read_fds[i]);
	for (int i = 0 ;i < cnt; ++i){
		free(t_segments[i]);
	}
	munmap(emulated_memory,0x1000);
	free(t_segments);
	free(file);
}

