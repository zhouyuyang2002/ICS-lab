/**********************************/
// Name: Yuyang Zhou
// Mail: 2000013061@stu.pku.edu.cn
// Homework: Cache_lab Phase_A
/**********************************/

#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*

*/
FILE* inf;            // Input flow                                 
char* File_in = NULL; // Input file
int Verbose_tag;      // Whether I should output the info. 
int s,E,b;
int evict_count;
int miss_count;
int hit_count;        // counter of eviction,miss & hit.

struct Line{
	int Valid;        // Whether this line contains info in it
	int LRUTag;       // The Last time visit the Line.
	long long Tag;          // The tag of info in it
};
struct Line **Cache_Mem = NULL; 
                      // Array used to save the Lines, (1 << S) * E in size.

#define IHIT 0
#define IMISS 1
#define IEVICT 2
#define Req_Type int
                      

/*
Quit: Quit the programs, and free the memories which 
is malloced in the program. (Cache_Mem & File_in)
*/
void Quit(){
	if (File_in != NULL){
		free(File_in);
		fclose(inf);
	}
	if (Cache_Mem != NULL){
		for (int i = 0; i < 1 << s; i++)
			if (Cache_Mem[i] != NULL)
				free(Cache_Mem[i]);
		free(Cache_Mem);
	}
	exit(0);
}

/*
print_Help: Print the Help infomation of the program
,And then exit the program. (will be called when -h 
is used, or lost -s,-E,-b,-t)
*/
void print_Help(){
	printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
	printf("Options:\n");
	printf("  -h         Print this help message.\n");
	printf("  -v         Optional verbose flag.\n");
 	printf("  -s <num>   Number of set index bits.\n");
 	printf("  -E <num>   Number of lines per set.\n");
	printf("  -b <num>   Number of block offset bits.\n");
 	printf("  -t <file>  Trace file.\n");
	printf("\n");
	printf("Examples:\n");
	printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
	printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
	Quit();
}

/*
init_Args: Init args of the program
*/
void init_Args(int argc,char **argv){
	int ch = -1, l;
	s = E = b = -1;
	for (;;){
		ch = getopt(argc, argv, "hvs:E:b:t:");
		if (ch == -1) break;
		switch (ch){
			case 'h':
				print_Help();
				break;
			case 'v':
				Verbose_tag = 1;
				break;
			case 's':
				sscanf(optarg,"%d",&s);
				break;
			case 'E':
				sscanf(optarg,"%d",&E);
				break;
			case 'b':
				sscanf(optarg,"%d",&b);
				break;
			case 't':
				l = strlen(optarg);
				File_in = (char *)malloc(l * sizeof(char));
				strcpy(File_in, optarg);
				break;
			default:
				printf("Unknown options!\n");
		}
	}
}

/*
init_Cache: init the Cache, and print the help information
if something is lost in -s,-E,-b,-t or -h is used.
*/
void init_Cache(){
	if (s == -1 || E == -1 || b == -1 || File_in == NULL)
		print_Help();
	inf = fopen(File_in,"r");
	Cache_Mem = (struct Line **) malloc(sizeof(struct Line *) * (1 << s));
	for (int i = 0; i < 1 << s; i ++){
		Cache_Mem[i] = (struct Line *) malloc(sizeof(struct Line) * E);
		for (int j = 0; j < E; j ++)
			Cache_Mem[i][j].Valid = 0;
	}
}

/*
Report: when the Cache simulator find situation A,
print the Verbose information, and update the counter.
*/
void Report(Req_Type a){
	switch (a){
		case IHIT:
			if (Verbose_tag)
				printf(" hit");
			++hit_count;
			break;
		case IMISS:
			if (Verbose_tag)
				printf(" miss");
			++miss_count;
			break;
		case IEVICT:
			if (Verbose_tag)
				printf(" eviction");
			++evict_count;
			break;
	}
}

/*
Cache_Usage: A Memory Usage in location, update the
information in Set [Set_id].
*/
void Cache_Usage(long long location){
	int Set_id = (location >> b) & ((1 << s) - 1);
	long long Tag = location >> (b + s);
	static int Cache_clock = 0;
	++Cache_clock;
	for (int i = 0; i < E; i++)
		if (Cache_Mem[Set_id][i].Tag == Tag)
			if (Cache_Mem[Set_id][i].Valid == 1){
				Cache_Mem[Set_id][i].LRUTag = Cache_clock;
				Report(IHIT); // Hit the Memory
				return;
			}

	Report(IMISS);
	for (int i = 0; i < E; i++)
		if (Cache_Mem[Set_id][i].Valid == 0){
			Cache_Mem[Set_id][i].Valid = 1;
			Cache_Mem[Set_id][i].Tag = Tag;
			Cache_Mem[Set_id][i].LRUTag = Cache_clock;
			return;                  // some line is empty
		}

	Report(IEVICT);
	int mnv = Cache_Mem[Set_id][0].LRUTag;
	int mnp = 0;
	for (int i = 0; i < E; i++)
		if (Cache_Mem[Set_id][i].LRUTag < mnv){
			mnv = Cache_Mem[Set_id][i].LRUTag;
			mnp = i;
		}
	Cache_Mem[Set_id][mnp].Tag = Tag;
	Cache_Mem[Set_id][mnp].LRUTag = Cache_clock;
									 // Eviction happened
}

/*
Run_Cache: Read instructions from inf, Judge the times
Used of the memory [location], and call Cache_Usage
*/
void Run_Cache(){
	char type[5];
	long long location;
	int size;
	while (fscanf(inf, "%s %llx,%d", type, &location, &size) != EOF){
		printf("%s %llx,%d\n",type,location,size);
		switch (type[0]){
			case 'I':
				break;
			case 'M':
			case 'L':
			case 'S':
				if (Verbose_tag == 1)
					printf("%c %llx,%d", type[0], location, size);
				Cache_Usage(location);
				if (type[0] == 'M')
					Cache_Usage(location);
				if (Verbose_tag == 1)
					puts("");
				break;
			default:
				printf("Instruction Err!!!");
				break;
		}
	}
}

int main(int argc,char **argv){
	init_Args(argc,argv);
	init_Cache();
	Run_Cache();
	printSummary(hit_count, miss_count, evict_count);
	Quit();
	return 0;
}