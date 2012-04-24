// третий пример
// генерация псевдослучайных чисел
// 1) добавление N псевдослучайных чисел
// 2) удаление этих же чисел, с проверкой на сохранение баланса

#include <stdio.h> // printf
#include <stdlib.h> // atoi

#include "sbt.h"

int main(int argc, char **argv) {

	if (argc < 2) return 0;
	int N = atoi(argv[1]);

#define RND_SEED 100
#define RND_A 9
#define RND_B 9
#define RND_C 7
	printf("добавление ...\n");
	int rnd = RND_SEED;
	for(int i = 0; i < N; i++) {
		rnd ^= (rnd << RND_A);
		rnd ^= (rnd >> RND_B);
		rnd ^= (rnd << RND_C);
		SBT_AddNodeUniq((rnd)&0x00FFFFFF); // вставка с отказами
		//SBT_AddNode((rnd)&0x00FFFFFF); // вставка без отказов
	}

	printf("удаление ...\n");
	rnd = RND_SEED + 1;
	for(int i = 0; i < N; i++) {
		rnd ^= (rnd << RND_A);
		rnd ^= (rnd >> RND_B);
		rnd ^= (rnd << RND_C);
		SBT_DeleteNode((rnd)&0x000000FF); // вставка без отказов
		SBT_CheckAllNodesBalance(); // эту строчку можно закомментировать, чтобы не тормозило на большом числе нодов
	}
	// SBT_PrintAllNodes();

	return 0;
}
