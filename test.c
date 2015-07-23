#include "sbt.h"

int main() {

	SBT_Initialise();
	int i;
	for(i = 0; i < 1000000; i++) {
            SBT_AddNode(i%100000);
	}

//        SBT_PrintAllNodes();
//        SBT_DumpAllNodes();

	SBT_Deinitialise();
        return 0;
}
