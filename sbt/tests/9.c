// девятый пример
// проверка на удаление одной вершины в определенном дереве (проверял одну ошибку)

#include "sbt.h"

int main() {
	SBT_AddNode(6);
	SBT_AddNode(4);
	SBT_AddNode(7);

	SBT_AddNode(2);
	SBT_AddNode(5);

	SBT_AddNode(11);
	SBT_AddNode(9);
	SBT_AddNode(12);
	SBT_AddNode(8);

	SBT_CheckAllNodes(); SBT_PrintAllNodes(); SBT_DumpAllNodes();

	SBT_DeleteNode(11);

	SBT_CheckAllNodes(); SBT_PrintAllNodes(); SBT_DumpAllNodes();

	return 0;
}
