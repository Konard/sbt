// пятый пример
// 1) добавить N x 3 элементов
// 2) найти определенный элемент по его значению, через FindNode()

#include <stdio.h> // printf
#include "sbt.h"

int main() {

#define N 10
	for (int i = 0; i < N; i++)
	    SBT_AddNodeUniq(i*2);
	for (int i = 0; i < N; i++)
	    SBT_AddNodeUniq(i*2 + 1);
	for (int i = 0; i < N; i++)
	    SBT_AddNodeUniq(i*2 + 1); // fail

	SBT_CheckAllNodes();
	SBT_PrintAllNodes();
	
	TNumber idx = SBT_FindNode(2); // = обычный Find для уникального ключа (number)
	printf("idx(2) = %lld\n", (long long int)idx);

	return 0;
}
