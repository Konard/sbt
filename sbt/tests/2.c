// второй пример
// 1) добавить 12 вершин (они будут передвигаться налево,
// 2) посреди них добавить ещё три вершины
// 3) распечатать результат
// 4) удалить три вершины
// 5) распечатать результат

#include "sbt.h"

int main() {

	// так как подвешиваются справа - идут по-возрастанию)
	SBT_AddNode(1);
	SBT_AddNode(2);
	SBT_AddNode(3);
	SBT_AddNode(4);
	SBT_AddNode(5);
	SBT_AddNode(6);
	SBT_AddNode(7);
	SBT_AddNode(8);
	SBT_AddNode(9);
	SBT_AddNode(10);
	SBT_AddNode(11);
	SBT_AddNode(12);

	SBT_AddNode(5);
	SBT_AddNode(6);
	SBT_AddNode(7);

	SBT_CheckAllNodes(); SBT_PrintAllNodes(); SBT_DumpAllNodes(); // распечатать результат

	SBT_DeleteNode(6);
	SBT_DeleteNode(8);
	SBT_DeleteNode(11);
	SBT_CheckAllNodes(); SBT_PrintAllNodes(); SBT_DumpAllNodes(); // распечатать результат

	return 0;
}
