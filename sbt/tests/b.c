// b-й пример
// генерация псевдослучайных чисел, и их удаление

#include <stdlib.h> // random, atoll
#include "sbt.h"

int main(int argc, char **argv) {

	if (argc < 2) return 0;
	long long int N = atoll(argv[1]);

	// ... можно написать другой тест (удалённый тест уже не нужен)

	return 0;
}
