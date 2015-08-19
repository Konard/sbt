// седьмой пример
// добавление большого количества нод в дерево, и последующее их удаление
// 1) добавить 10 млн. нод
// 2) удалить 10 млн. нод

#include "sbt.h"

#include <stdio.h> // printf
#include <stdlib.h> // atoi

int main(int argc, char **argv) {

	int N = 10000000;

	printf("добавление ...\n");
	for (int i = 0; i < N; i++) {
		SBT_AddNode(i);
	}

	printf("удаление ...\n");
	for (int i = 0; i < N; i++) {
		SBT_DeleteNode(i);
	}

	return 0;
}
