#include <stdlib.h>
#include <stdio.h>
#define ull unsigned long long 
#define ll long long
extern unsigned long long IP;
extern unsigned long long HIP;
extern unsigned long long SW;
extern int A,B,X,T,S,F,L;
// use funkcije bodo imele isti signature ampak ker jih bo vecina delal z registri
// ki so global variables bodo argument uporabljale sam tiste k delajo z memorijem direkt
// this is subject to change
void (*ukazi_functions[0xfc])(ull *mem, int * r1, int * r2);

int read_fds [100];
int read_fds_cnt = 0;

int write_fds [100];
int write_fds_cnt =0;

// poklicemo iz main fila da se ukazi_functions populata za pravimi vrednostmi

ull obrni_indijanca(ull i){
	ull b0, b1 ,b2; 
	b0 = (i & 0x000000ff) << 16;
	b1 = (i & 0x0000ff00);
	b2 = (i & 0x00ff0000) >> 16;
	ull res = 0x00 | b0 | b1 | b2;
	return res;
}

int active_read_descriptor(int descriptor){
	for(int i = 0; i< read_fds_cnt; ++i){
		if (descriptor == read_fds[i])
			return 1;
	}
	return 0;
}

int active_write_descriptor(int descriptor){
	for(int i = 0; i< write_fds_cnt; ++i){
		if (descriptor == write_fds[i])
			return 1;
	}
	return 0;
}


void memory_view(int offset, int size){
	unsigned char * where = (unsigned char*) HIP+offset;
	puts("======================MEMORY====================");
	for ( int i = 0; i< size; ++i){
		printf("%x",where[i]);
	}
	puts("");
}

ull jump_store_memory_sanitize(ull * mem){
	// addres -> set IP oz store there
	// value  -> set IP = HIP +value oz store HIP + val
	if ( ((ull) mem > 0xffffff)  && ((ll)mem > 0)){
		//memory
		return (ull) mem;
	}else{
		return HIP + (ull)mem;
	}
}


int twos_complement_util(ull n, int bitcount){
	printf("dobil sm %llu\n",n);	
	int res;
	if (( n & (1 << (bitcount -1))) > 0){
		printf("negativno je, returnam %llu\n",((1 <<bitcount) -n ) *-1);
		return ((1 << bitcount) - n) * -1;
	
	}else{
		res = n& 0xffffff;
		return res;
	}
}

void wd(ull * mem,int * r1, int* r2){
	puts("klicem wd");
	ull res = (ull) mem;
	printf("res: %llu",res);
	char file_name[3];	//BOF WINK WINK ;)
	char path[10];
	int byte_res = (int)((res&0xff0000)>>16);
	printf("byte_res: %d",byte_res);
	if (byte_res == 1){
		write(1, &A,1);
	}else if(active_write_descriptor(byte_res)){
		write(byte_res,&A,1);
	}else{
		if ( write_fds_cnt >= 100){
			perror("Dej se umiri s file descriptorji");
			exit(-1);
		}
		sprintf(file_name, "%d", byte_res);
		strcat(path,"./");
		strcat(path,file_name);
		int fd = open(path, O_WRONLY|O_CREAT,S_IRWXU);
		if ( fd < 0){
			perror("file descriptor se ni odpru");
			exit(-1);
		}
		write(fd,&A,1);
		write_fds[write_fds_cnt] = fd;
		write_fds_cnt++;
	}
}

void rmo(ull * mem, int*r1, int*r2){
	puts("klicem rmo");
	*r2 = *r1;
}



void rd(ull * mem,int * r1, int* r2){
	puts("klicem rd");
	ull res = (ull)mem;
	printf("res: %llu",res);
	char file_name[3];	//BOF WINK WINK ;)
	char path[10];
	int byte_res = (int)((res&0xff0000) >> 16);
	printf("byte_res: %d",byte_res);
	if (byte_res == 0){
		read(0, &A,1);
	}else if(active_read_descriptor(byte_res)){
		read(byte_res,&A,1);
	}else{
		if ( read_fds_cnt >= 100){
			perror("Dej se umiri s file descriptorji");
			exit(-1);
		}
		sprintf(file_name, "%d", byte_res);
		strcat(path,"./");
		strcat(path,file_name);
		int fd = open(path, O_RDONLY|O_CREAT,S_IRWXU);
		if ( fd <0){
			perror("file descriptor se ni odpru");
			exit(-1);
		}
		read(fd,&A,1);
		read_fds[read_fds_cnt] = fd;
		read_fds_cnt++;
	}
	printf("read %x",byte_res);
}


void cmpr(ull * mem ,int *r1, int * r2){
	puts("klicem cmpr");
	printf("SW prej: %llx\n",SW);
	// c=0 -> 0
	// c=1 -> >
	// c=2 -> <
	SW &= 0x9f;
	if( *r1 > *r2){
		SW |= 0x20;	
	}
	if ( *r1 < *r2){
		SW |= 0x40;
	}
	printf("SW potem: %llx\n",SW);
}

void cmp(ull * mem ,int *r1, int * r2){
	puts("klicem cmp");
	printf("SW prej: %llx\n",SW);
	// c=0 -> 0
	// c=1 -> >
	// c=2 -> <
	ull res = (ull) mem;
	SW &= 0x9f;
	int c = res & 0xffffff;
	printf("compare: %d",c);
	printf("A: %d",A);
	if( A > c){
		SW |= 0x20;	
	}
	if ( A < c){
		SW |= 0x40;
	}
	printf("SW potem: %llx\n",SW);
}


void jeq(ull * mem, int*r1,int*r2){
	puts("klicem jeq");
	ull to_write = (ull) mem;
	if ( (SW & 0x60) == 0x00)
		IP = to_write;
	printf("instruction pointer: %llu",IP);
}

void jgt(ull * mem, int*r1, int*r2){
	puts("klicem jgt");
	ull to_write = (ull) mem;
	if ( (SW & 0x60) == 0x40)
		IP = to_write;
	printf("instruction pointer: %llu",IP);

}

void jlt(ull * mem, int*r1, int*r2){
	puts("klicem jlt");
	ull to_write = (ull) mem;
	if ( (SW & 0x60) == 0x20)
		IP = to_write;
	printf("instruction pointer: %llu",IP);
}

void jsub(ull * mem, int*r1, int*r2){
	puts("klicem jsub");
	ull to_write = (ull)mem;
	L = IP -HIP;
	IP = to_write;
}

void mulr(ull * mem, int*r1, int*r2){
	puts("klicem mulr");
	*r2 = *r1 * *r2;
}


void mul(ull * mem, int*r1, int*r2){
	puts("klicem mul");
	ull res = (ull) mem;
	A = A * res;
}


void rsub( ull* mem, int*r1, int*r2){
	puts("klicem rsub");
	IP = L + HIP;
}


void div_r(ull * mem, int*r1, int*r2){
	puts("klicem divr");
	int res = *r2 / *r1;
	*r2 = res;
}

void div_(ull * mem, int*r1, int*r2){
	puts("klicem div");
	ull res = (ull) mem;
	A = A / res;
}

void sub(ull *mem, int * r1,int *r2){
	puts("klicem sub");
	ull res = (ull) mem;	
	res = twos_complement_util(res,24);
	A  = A -res;
}


void subr(ull *mem, int * r1, int * r2 ){
	puts("klicem subr");
	int res = *r2 - *r1;
	*r2 = res;
}



void clear( ull * mem, int * r1, int *r2){
	puts("klicem clear");
	*r1 = 0;
}

void jump(ull * mem, int * r1, int *r2){
	puts("klicem jump");
	ull to_write = (ull) mem;
	IP = to_write;
}

void addr(ull *mem, int * r1, int * r2 ){
	puts("klicem addr");
	int res = *r1 + *r2;
	printf("res: %x\n",res);
	*r2 = res;
}

void add(ull *mem, int * r1,int *r2){
	puts("klicem add");
	ull res = (ull) mem; 
	res = twos_complement_util(res,24);	
	A  = A + res;
}

void sta(ull *mem, int * r1, int * r2){
	puts("klicem sta");
	unsigned char * to_write = (unsigned char*) mem;
	printf("to_write: %p\n",to_write);
	ull tmp = obrni_indijanca((ull)A);
	printf("na adresi %p je vrednost %d\n",to_write,*to_write);
	for (int i = 0 ;i < 3; ++i){
		*to_write = tmp &0xff;
		tmp >>= 0x8;
		to_write++;
	}
	printf("na adresi %p je vrednost %d\n",to_write-1,*(to_write-1));
}

void lda(ull *mem, int * r1, int *r2){
	puts("klicem lda");
	printf("ll: %lld\n",(ll)mem);
	ull res = (ull)mem;
	A = res;	
}

void ldb(ull *mem, int * r1, int *r2){
	puts("klicem ldb");
	printf("ll: %lld\n",(ll)mem);
	ull res = (ull) mem;
	B = res;	
}

void ldl(ull *mem, int * r1, int *r2){
	puts("klicem ldl");
	printf("ll: %lld\n",(ll)mem);
	ull res = (ull) mem;
	L = res;	
}

void lds(ull *mem, int * r1, int *r2){
	puts("klicem lds");
	printf("ll: %lld\n",(ll)mem);
	ull res = (ull) mem;
	S = res;	
}

void ldt(ull *mem, int * r1, int *r2){
	puts("klicem ldt");
	printf("ll: %lld\n",(ll)mem);
	ull res = (ull) mem;
	T = res;	
}

void ldx(ull *mem, int * r1, int *r2){
	puts("klicem ldx");
	printf("ll: %lld\n",(ll)mem);
	ull res = (ull) mem;
	X = res;	
}

void stl(ull *mem, int * r1, int * r2){
	puts("klicem stl");
	unsigned char * to_write = (unsigned char*) mem;
	ull tmp = obrni_indijanca((ull)L);
	for (int i = 0 ;i < 3; ++i){
		*to_write = tmp &0xff;
		tmp >>= 0x8;
		to_write++;
	}
}
void stb(ull *mem, int * r1, int * r2){
	puts("klicem stb");
	unsigned char * to_write = (unsigned char*) mem;
	ull tmp = obrni_indijanca((ull)B);
	for (int i = 0 ;i < 3; ++i){
		*to_write = tmp &0xff;
		tmp >>= 0x8;
		to_write++;
	}
}
void sts(ull *mem, int * r1, int * r2){
	puts("klicem sts");
	unsigned char * to_write = (unsigned char*) mem;
	ull tmp = obrni_indijanca((ull)S);
	for (int i = 0 ;i < 3; ++i){
		*to_write = tmp &0xff;
		tmp >>= 0x8;
		to_write++;
	}
}
void stsw(ull *mem, int * r1, int * r2){
	puts("klicem stsw");
	unsigned char * to_write = (unsigned char*) mem;
	ull tmp = obrni_indijanca((ull)SW);
	for (int i = 0 ;i < 3; ++i){
		*to_write = tmp &0xff;
		tmp >>= 0x8;
		to_write++;
	}
}
void stt(ull *mem, int * r1, int * r2){
	puts("klicem stt");
	unsigned char * to_write = (unsigned char*) mem;
	ull tmp = obrni_indijanca((ull)T);
	for (int i = 0 ;i < 3; ++i){
		*to_write = tmp &0xff;
		tmp >>= 0x8;
		to_write++;
	}
}
void stx(ull *mem, int * r1, int * r2){
	puts("klicem stx");
	unsigned char * to_write = (unsigned char*) mem;
	ull tmp = obrni_indijanca((ull)X);
	for (int i = 0 ;i < 3; ++i){
		*to_write = tmp &0xff;
		tmp >>= 0x8;
		to_write++;
	}
}

void ldch(ull * mem, int*r1, int *r2){
	puts("klicem ldch");
	ull res = (ull)mem;
	A = (res & 0xff);
}

void stch(ull * mem, int*r1, int * r2){
	puts("klicem stch");
	unsigned char * to_write = (unsigned char*) mem;
	*to_write = A &0xff;
}


void init_ukazi(){
	// tukaj nasopamo funkcije za vse opcode oz tiste k se mi jih bo dalo implementirat
	ukazi_functions[0x18] = add;
	ukazi_functions[0x90] = addr;

	ukazi_functions[0x1c] = sub;
	ukazi_functions[0x94] = subr;

	ukazi_functions[0xb4] = clear;

	ukazi_functions[0x24] = div_;
	ukazi_functions[0x9c] = div_r;

	ukazi_functions[0x20] = mul;
	ukazi_functions[0x98] = mulr;

	ukazi_functions[0x28] = cmp;
	ukazi_functions[0xa0] = cmpr;

	ukazi_functions[0x0c] = sta;
	ukazi_functions[0x54] = stch;
	
	ukazi_functions[0x00] = lda;
	ukazi_functions[0x50] = ldch;
	
	ukazi_functions[0x30] = jeq;
	ukazi_functions[0x34] = jgt;
	ukazi_functions[0x38] = jlt;
	ukazi_functions[0x3c] = jump;
	ukazi_functions[0x4c] = rsub;
	ukazi_functions[0x48] = jsub;


	ukazi_functions[0x78] = stb;
	ukazi_functions[0x14] = stl;
	ukazi_functions[0x7c] = sts;
	ukazi_functions[0xe8] = stsw;
	ukazi_functions[0x84] = stt;
	ukazi_functions[0x10] = stx;
	
	ukazi_functions[0x68] = ldb;
	ukazi_functions[0x08] = ldl;
	ukazi_functions[0x6c] = lds;
	ukazi_functions[0x74] = ldt;
	ukazi_functions[0x04] = ldx;

	ukazi_functions[0xdc] = wd;
	ukazi_functions[0xd8] = rd;

	ukazi_functions[0xac] = rmo;
}

