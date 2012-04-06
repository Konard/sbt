#include <stdio.h> // printf
#define __USE_XOPEN_EXTENDED
#include <stdlib.h> // random
#include <time.h> // time

#include "sbt.h"

int main() {

#define N (10*1000*1000)
//#define N 100000
//#define N 100
	// седьмой пример
	printf("добавление ...\n");
	for (int i = 0; i < N; i++) {
		SBT_AddNode(i);
//		SBT_AddNodeUniq(i);
//		SBT_CheckAllNodesSize();
	}
//	SBT_DeleteNode(10);
//	SBT_PrintAllNodes();
//	return 0;

	printf("удаление ...\n");
	for (int i = 0; i < N; i++) {
		SBT_DeleteNode(i);
//		SBT_CheckAllNodesBalance();
	}

//	printf("перед удалением %d\n", i);
//	SBT_DumpAllNodes();
//	SBT_PrintAllNodes();
//		printf("после удаления %d\n", i);
//	SBT_DumpAllNodes();
//	SBT_PrintAllNodes();
//		SBT_CheckAllNodesSize();

	SBT_CheckAllNodesSize();
	SBT_CheckAllNodesBalance();

//	    SBT_DeleteAllNodes(i);
	SBT_PrintAllNodes();
//	SBT_DumpAllNodes();

	return 0;
}
