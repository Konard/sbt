// пример c.c
// проверка добавления вершин + распечатка результата (небольшого дерева)

#include "sbt.h"

int main() {

	SBT_AddNode(10);
	SBT_AddNode(20);
	SBT_AddNode(30);

	SBT_AddNode(5);
	SBT_AddNode(15);
	SBT_AddNode(25);
	SBT_AddNode(35);

	SBT_CheckAllNodes(); SBT_PrintAllNodes(); SBT_DumpAllNodes();

	return 0;
}
