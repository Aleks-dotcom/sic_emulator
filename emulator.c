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


extern void (*ukazi_functions[0xfc])(void * mem,int *r1,int*r2);
//registers
int A,B,T,S,X,L,F;
// instructions type 1 opcodes
char ukaz_tipa_1[6] = {0xc4,0xc0,0xf4,0xc8,0xf0,0xf8}
// instructions type 2 opcodes
char ukaz_tipa_2[10] = {0x90, 0xb4,0xa0,0x9c,0x98,0xac,0xa4,0xa8,0x94,0xb0}
// array of register pointers for easier decoding of instructions
int * registers[7] = {&A,&X,&L,&B,&S,&T,&F};

void * IP;
void * HIP;
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

int one_byte(char oc){
	for (int i = 0; i < 6; ++i){
		if oc == ukaz_tipa_1[i]
			return 1;
	
	}
	return 0;

}

int two_byte(char oc){
	for( int i = 0; i< 10; ++i){
		if oc == ukaz_tipa_2[i]
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
	IP = emulated_memory;
	// helper instruction pointer damo tudi na zacetek memorija in tuki bo tud
	// ostal
	HIP = emulated_memory;
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
	
	int * r1,r2;
	void * m;
	char register_data, r1_oc,r2_oc;
	int increment_ip = 0;
	// runnamom loop dokler nam mati da kruha in mleka xD
	
	while (0){
		//preberemo 1 byte iz IPja
		char data  = (*(int*)IP);
	
		// izluscimo dejanksi opcode
		char actual_oc =  data & 0xfc;	
	
		// ce je tipa ena recemo da nardi kar mora -> povecamo IP za 1
	
		if (one_byte(actual_oc) ==1){
			ukazi_functions[actual_oc](m,r1,r2);
			increment_ip =1;

		}else if( two_byte(actual_oc) == 1){
			m = NULL;
			IP++1;
			register_data = (*(char*)IP);
			r1_oc = register_data >> 0x8;
			r2_oc = register_data & 0xff;
			r1 = registers[r1_oc];
			r2 = registers[r2_oc];
			increment_ip = 2;
		}else{
			// ce smo prsli do tle potem mamo instruction tipa 3/4
			// let the fun begin!!
			
			// ce je tip 3/4 ne rabimo registrov
			r1 = 0; 
			r2 = 0;

			// dobimo vn n in i
			char n = opcode & 0x2;
			char i = opcode & 0x1;

			// potegnemo vn se 2 byta ker itak bo vsaj 3 byte dolg
			// instruction
			//
			data  = data >> 0x8;
			char b2 = data & 0xff;
			char x = b2 & 0x80;
			char b = b2 & 0x40;
			char p = b2 & 0x20;
			char e = b2 & 0x10;

			
			data = data >> 0x8;

			char b3 = data & 0xff;
			// displacement loh zracunamo ze zdej ker ga rabimo v obeh
			// primerih

			int displacemant = (b3&0x0f) << 8;
			displacemant = displacemant | (b3 & 0xff);
			
			//pogruntamo a mamo 3 al 4
		
			if (e == 1){
				// imamo instruction tipa 4 addr je absoluten 
				data = data >> 0x8;
				char b4 = data & 0xff;
				displacemant <<= 0x8;
				displacemant |= (b4&0xff);	
				m = displacement;
				increment_ip =4;
			}else{
				// imamo instruction tipa 3
				if p == 1
					m = IP + displacemant;
				if b == 1
					m = B + displacement;
				if x == 1
					m += X;
				if n == 1
					m = *(HIP + mem); // dereference mem;

				increment ip = 3;
			}
		}
		ukazi_functions[actual_oc](m,r1,r2);
		//TODO: zdj k smo poflexali s kodo jo je treba pa se stestira.
		//to pa kot dobri programerji, pustimo za ju3 :)
	}


	for (int i = 0 ;i < cnt; ++i){
		free(t_segments[i]);
	}
	munmap(emulated_memory,0x1000);
	free(t_segments);
	free(file);
}
