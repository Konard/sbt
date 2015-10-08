#include "sbt.h"
#include <stdio.h>

int main() {

	SBT_Initialise();

	SBT_AddNode(0);
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
	SBT_AddNode(123);
	SBT_PrintAllNodes();
	SBT_DumpAllNodes();
	for (int k = 0; k < 20; k++) SBT_AddNode(k);

	printf("Value %2I64d\n",(long long int)aValue[0]);
	printf("UT    %2I64d\n",(long long int)aUT[0]);
	printf("LT    %2I64d\n",(long long int)aLT[0]);
	printf("RT    %2I64d\n",(long long int)aRT[0]);
	printf("Size  %2I64d\n",(long long int)aSize[0]);
	printf("FREE  %2I64d\n",(long long int)aFREE[0]);

	SBT_Deinitialise();
	return 0;
}
