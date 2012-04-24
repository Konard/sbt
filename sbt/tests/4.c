// четвертый пример
// 1) добавить N вершин с псевдослучаным значением, выводя сообщения о событиях "Rotate"
// 2* распечатать результат

#include <stdio.h> // printf
#include "sbt.h"

int onRotate(TNodeIndex nodeIndex1, TNodeIndex nodeIndex2, const char *stringAction) {
        printf("idx1 = %lld, idx2 = %lld, action = %s\n",
                (long long int)nodeIndex1,
                (long long int)nodeIndex2,
                stringAction
        );
        return 0;
}

int main() {
	SBT_SetCallback_OnRotate(onRotate);

	// генерация псевдослучайных чисел
#define RND_SEED 100
#define RND_A 9
#define RND_B 9
#define RND_C 7
	int rnd = RND_SEED;
	for(int i = 0; i < 10000000; i++) {
		rnd ^= (rnd << RND_A);
		rnd ^= (rnd >> RND_B);
		rnd ^= (rnd << RND_C);
		//SBT_AddNodeUniq((rnd)&0x00FFFFFF); // вставка с отказами
		SBT_AddNode((rnd)&0x00FFFFFF); // вставка без отказов
	}

	// результат работы
/*	// можно раскомментировать этот блок, чтобы проверить и посмотреть результат выполнения AddNode's
	SBT_CheckAllNodes();
	SBT_PrintAllNodes();
	SBT_DumpAllNodes();
*/
	return 0;
}
