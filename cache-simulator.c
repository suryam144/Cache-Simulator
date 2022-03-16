#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
typedef struct cacheBlock{
	unsigned long long t;
	unsigned long long s;
	unsigned long long l;
	unsigned long long timer;
	unsigned long long new;
}cacheBlock;

int logVal(int x){
	int temp = x;
	int count = 0;
	while(temp != 1){
		temp = temp / 2;
		count++;
	}
	return count;
}

int powOf2(unsigned long long x){
	if(((x & (x-1)) == 0) && (x > 0)){return 1;}
	return 0; 
}

int findSet(int cacheSize, int blockSize, int assoc){
	if(!(cacheSize > (blockSize * assoc))){
		printf("error");
		exit(0);
	}
	int blockPow = blockSize;
	int cachePow = cacheSize;	
	int div = cachePow / blockPow;
	int val = div / assoc;
	return logVal(val);
}

int findBlock(int blockSize){
	int blockPow = logVal(blockSize);
	return blockPow;
}

int main(int argc, char* argv[]){

	if(argc != 6){
		printf("error");
		exit(0);
	}

	int cacheSize = atoi(argv[1]);
	char assoc[9];
	strcpy(assoc, argv[2]);
	char policy[5];
	strcpy(policy, argv[3]);
	int blockSize = atoi(argv[4]);
	FILE *tempFile;
	tempFile = fopen(argv[5], "r");

	if(powOf2(cacheSize) == 0 || powOf2(blockSize) == 0){
		printf("error");
		exit(0);
	}
	if(strcmp(policy, "lru") != 0 && strcmp(policy, "fifo") != 0){
		printf("error");
		exit(0);
	}
	if (tempFile == NULL){
		printf("error");
		exit(0);	
	}
	
	int assocVal = 1;
	unsigned long long set = 0;
	unsigned long long line = findBlock(blockSize);
	
	if(strcmp(assoc, "direct") == 0){
		set = findSet(cacheSize, blockSize, 1);
		assocVal = 1; 
	}
	else if(strcmp(assoc, "assoc") == 0){
		set = 0;
	}
	else if(strstr(assoc,"assoc:") != NULL){
		char val[16];
		sscanf(assoc, "assoc:%s", val);
		if(strlen(val) != strspn(val, "0123456789")){
			printf("error");
			exit(0);
		}
		assocVal = atoi(val);
		if(powOf2(assocVal) == 0){
			printf("error");
			exit(0);
		}
		set = findSet(cacheSize, blockSize, assocVal);
	}
	else{
		printf("error");
		exit(0);
	}
	unsigned long long tag = 48 - set - line;
	int readCount = 0;
	int writeCount = 0;
	int cacheHit = 0;
	int cacheMiss = 0;
	
	int directSize = pow(2,set);
	cacheBlock directTable[directSize][assocVal];
	for(int i = 0; i < directSize; i++){
		for(int j = 0; j < assocVal; j++){
			cacheBlock temp;
			temp.t = -1;
			temp.s = -1;
			temp.l = -1;
			temp.timer = 0;
			temp.new = 1;
			directTable[i][j] = temp;
		}
	}
	
	int facVal = (cacheSize / blockSize);
	cacheBlock fac[facVal];
	for(int i = 0; i < facVal; i++){
		cacheBlock temp;
		temp.t = -1;
		temp.s = -1;
		temp.l = -1;
		temp.timer = 0;
		temp.new = 1;
		fac[i] = temp;
	}
	char tempStr[16];
	while(!feof(tempFile)){
		char op[2];
		unsigned long long second;
		fscanf(tempFile, " %s:", tempStr);
		if(strcmp(tempStr, "#eof") != 0){
			fscanf(tempFile, " %s %llx ", op, &second);
			cacheBlock tempBlock;	
			tempBlock.l = second & (unsigned long long)(pow (2,line) - 1);
			tempBlock.s = (second >> line) & (unsigned long long)(pow(2,set)-1);
			tempBlock.t = (second >> (line + set)) & (unsigned long long)(pow(2,tag) - 1);
			tempBlock.timer = 1;
			tempBlock.new = 0;
	
			if(strcmp(assoc, "direct") == 0){
				if(strcmp(op, "R") == 0){
					unsigned long long valid = tempBlock.s;
					if(directTable[valid][0].new == 1){
						cacheMiss++;
						readCount++;	
						directTable[valid][0] = tempBlock;				
					}
					else{
						unsigned long long tagTestOne = directTable[valid][0].t;
						unsigned long long tagTestTwo = tempBlock.t;
						if(tagTestOne == tagTestTwo){
							cacheHit++;
						}
						else{
							cacheMiss++;
							readCount++;
							directTable[valid][0] = tempBlock;
						}
					}
				}
				else if(strcmp(op, "W") == 0){
					unsigned long long valid = tempBlock.s;
					if(directTable[valid][0].new == 1){
						cacheMiss++;
						readCount++;
						writeCount++;
						directTable[valid][0] = tempBlock;
					}
					else{
						unsigned long long tagTestOne = directTable[valid][0].t;
						unsigned long long tagTestTwo = tempBlock.t;
						if(tagTestOne == tagTestTwo){
							cacheHit++;
							writeCount++;
							
						}
						else{
							cacheMiss++;
							readCount++;
							writeCount++;
							directTable[valid][0] = tempBlock;
						}
					}
				}
			}
			else if(strcmp(assoc, "assoc") == 0){
				if(strcmp(op, "R") == 0){
					int stop = 0;
					for(int i = 0; i < facVal && stop != 1; i++){
						if(fac[i].new == 1){
							cacheMiss++;
							readCount++;
							stop = 1;	
							fac[i] = tempBlock;	
								
						}
						else{
							unsigned long long tagTestOne = fac[i].t;
							unsigned long long tagTestTwo = tempBlock.t;
							if(tagTestOne == tagTestTwo){
								cacheHit++;
								if(strcmp(policy, "lru") == 0){fac[i].timer = 0;}
								stop = 1;
							}
						}
					}
					if(stop == 0){
						cacheMiss++;
						readCount++;
						int max = 0;
						for(int i = 0; i < facVal; i++){
							if(fac[i].timer > fac[max].timer){
								max = i;
							}
						}
						fac[max] = tempBlock;
						fac[max].timer = 0;
					}
					for(int i = 0; i < facVal; i++){
						fac[i].timer = fac[i].timer + 1;
					}
				}
				else if(strcmp(op, "W") == 0){
					int stop = 0;
					for(int i = 0; i < facVal && stop != 1; i++){
						if(fac[i].new == 1){
							cacheMiss++;
							readCount++;
							writeCount++;
							stop = 1;	
							fac[i] = tempBlock;	
								
						}
						else{
							unsigned long long tagTestOne = fac[i].t;
							unsigned long long tagTestTwo = tempBlock.t;
							if(tagTestOne == tagTestTwo){
								cacheHit++;
								writeCount++;
								if(strcmp(policy, "lru") == 0){fac[i].timer = 0;}
								stop = 1;
							}
						}
					}
					if(stop == 0){
						cacheMiss++;
						readCount++;
						writeCount++;
						int max = 0;
						for(int i = 0; i < facVal; i++){
							if(fac[i].timer > fac[max].timer){
								max = i;
							}
						}
						fac[max] = tempBlock;
						fac[max].timer = 0;
					}
					for(int i = 0; i < facVal; i++){
						fac[i].timer = fac[i].timer + 1;
					}
				}
			}
			else if(strstr(assoc,"assoc:") != NULL){
				if(strcmp(op, "R") == 0){
					unsigned long long valid = tempBlock.s;
					int stop = 0;
					for(int i = 0; i < assocVal && stop != 1; i++){
						if(directTable[valid][i].new == 1){
							cacheMiss++;
							readCount++;
							stop = 1;	
							directTable[valid][i] = tempBlock;				
						}
						else{
							unsigned long long tagTestOne = directTable[valid][i].t;
							unsigned long long tagTestTwo = tempBlock.t;
							if(tagTestOne == tagTestTwo){
								cacheHit++;
								if(strcmp(policy, "lru") == 0){directTable[valid][i].timer = 0;}
								stop = 1;
							}
						}
					}
					if(stop == 0){
						cacheMiss++;
						readCount++;
						int max = 0;
						for(int i = 0; i < assocVal; i++){
							if(directTable[valid][i].timer > directTable[valid][max].timer){
								max = i;
							}
						}
						directTable[valid][max] = tempBlock;
						directTable[valid][max].timer = 0;
					}
					for(int i = 0; i < directSize; i++){
						for(int j = 0; j < assocVal; j++){
							directTable[i][j].timer = directTable[i][j].timer + 1;
						}
					}
				}
				else if(strcmp(op, "W") == 0){
					unsigned long long valid = tempBlock.s;
					int stop = 0;
					for(int i = 0; i < assocVal && stop != 1; i++){
						if(directTable[valid][i].new == 1){
							cacheMiss++;
							readCount++;
							writeCount++;
							stop = 1;	
							directTable[valid][i] = tempBlock;				
						}
						else{
							unsigned long long tagTestOne = directTable[valid][i].t;
							unsigned long long tagTestTwo = tempBlock.t;
							if(tagTestOne == tagTestTwo){
								cacheHit++;
								if(strcmp(policy, "lru") == 0){directTable[valid][i].timer = 0;}
								writeCount++;
								stop = 1;
							}
						}
					}
					if(stop == 0){
						cacheMiss++;
						readCount++;
						writeCount++;
						int max = 0;
						for(int i = 0; i < assocVal; i++){
							if(directTable[valid][i].timer > directTable[valid][max].timer){
								max = i;
							}
						}
						directTable[valid][max] = tempBlock;
						directTable[valid][max].timer = 0;
					}
					for(int i = 0; i < directSize; i++){
						for(int j = 0; j < assocVal; j++){
							directTable[i][j].timer = directTable[i][j].timer + 1;
						}
					}
				}
			}
		}			
	}	
	printf("Memory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\n", readCount, writeCount, cacheHit, 	cacheMiss);	
	fclose(tempFile);
}
