// седьмой пример
// добавление большого количества нод в дерево, и последующее их удаление
// 1) добавить 10 млн. нод
// 2) удалить 10 млн. нод

#include "sbt.h"

#include <stdio.h> // printf
#include <stdlib.h> // atoi

int main(int argc, char **argv) {

	if (argc < 2) return 0;
	int N = atoi(argv[1]);

	printf("добавление ...\n");
	for (int i = 0; i < N; i++) {
		SBT_AddNode(i);
//		SBT_AddNodeUniq(i);
//		SBT_CheckAllNodesSize();
	}

	printf("удаление ...\n");
	for (int i = 0; i < N; i++) {
		SBT_DeleteNode(i);
//		SBT_CheckAllNodesBalance();
	}

//	SBT_CheckAllNodes();
//	SBT_PrintAllNodes();

	return 0;
}
