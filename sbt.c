
#include "sbt.h"
#include <stdio.h>
#include <malloc.h>

// Массив, его размер
const uint64_t NNSTART = 1000;
const uint64_t NNPLUS = 20000;

// Разделяем массив структур на массивы по отдельным полям

TNodeIndex NN = 0;
TValue *aValue;		// значение, привязанное к ноде
TNodeIndex *aUT;		// ссылка на уровень выше
TNodeIndex *aLT;		// ссылка на левое поддерево, = -1, если нет дочерних вершин
TNodeIndex *aRT;		// ссылка на правое поддерево
TNodeSize *aSize;	// size в понимании SBT

uint8_t *aFREE;	// «удалённая»; это поле можно использовать и для других флагов

TNodeIndex NU = 0; // число вершин в дереве, использованные (USED)
TNodeIndex ROOT = -1; // дерево
TNodeIndex FREE = -1; // список неиспользованных
TNodeIndex NCLEAN; // количество "чистых"; только уменьшается

void SBT_Initialise() {
	NN = NNSTART;
	TNodeIndex t;

	aValue = (TValue *)malloc(NN*sizeof(TValue));
	aUT = (TNodeIndex *)malloc(NN*sizeof(TNodeIndex));
	aLT = (TNodeIndex *)malloc(NN*sizeof(TNodeIndex));
	aRT = (TNodeIndex *)malloc(NN*sizeof(TNodeIndex));
	aSize = (TNodeSize *)malloc(NN*sizeof(TNodeSize));
	aFREE = (uint8_t *)malloc(NN*sizeof(uint8_t));

	SBT_Initialise_Opt();

	NU = 0; // число вершин в дереве, использованные (USED)
	ROOT = -1; // дерево
	FREE = -1; // список неиспользованных
	NCLEAN = NN; // количество "чистых"; только уменьшается
}

void SBT_Deinitialise() {
	free(aValue);
	free(aUT);
	free(aLT);
	free(aRT);
	free(aSize);
	free(aFREE);
	NN = 0; // число вершин в дереве, использованные (USED)
	ROOT = -1; // дерево
	FREE = -1; // список неиспользованных
	NCLEAN = NN; // количество "чистых"; только уменьшается
}

// Функции для оповещения о событиях, event-driven 

FuncOnRotate funcOnRotate = NULL;
FuncOnWalk funcOnWalk = NULL;
FuncOnFind funcOnFind = NULL;

int8_t SBT_SetCallback_OnRotate(FuncOnRotate func_) {
	funcOnRotate = func_;
	return 0;
}

int8_t SBT_SetCallback_OnWalk(FuncOnWalk func_) {
	funcOnWalk = func_;
	return 0;
}

int8_t SBT_SetCallback_OnFind(FuncOnFind func_) {
	funcOnFind = func_;
	return 0;
}


// Вращение влево, t - слева, перевешиваем туда (вершины не пропадают, NU сохраняет значение)

int8_t SBT_LeftRotate(TNodeIndex t) {

        if (t < 0) return 0;
        TNodeIndex k = aRT[t];
        if (k < 0) return 0;
        TNodeIndex p = aUT[t];

        // поворачиваем ребро дерева
        aRT[t] = aLT[k];
        aLT[k] = t;

        // корректируем size
        aSize[k] = aSize[t];
        TNodeIndex n_l = aLT[t];
        TNodeIndex n_r = aRT[t]; // ? для ускорения — выборку из кэша
        TNodeSize s_l = ((n_l != -1) ? aSize[n_l] : 0);
        TNodeSize s_r = ((n_r != -1) ? aSize[n_r] : 0);
        aSize[t] = s_l + s_r + 1;

        // меняем трёх предков
        // 1. t.right.parent = t
        // 2. k.parent = t.parent
        // 3. t.parent = k
        if (aRT[t] != -1) aUT[aRT[t]] = t; // ? из кэша
        aUT[k] = p;
        aUT[t] = k;

        // меняем корень, parent -> t, k
        if (p == -1) ROOT = k; // это root
        else {
                if (aLT[p] == t) aLT[p] = k;
                else aRT[p] = k; // вторую проверку можно не делать
        }
        return 1;
}

// Вращение вправо, t - справа, перевешиваем туда (вершины не пропадают, NU сохраняет значение)

int8_t SBT_RightRotate(TNodeIndex t) {

	if (t < 0) return 0;
	TNodeIndex k = aLT[t];
	if (k < 0) return 0;
	TNodeIndex p = aUT[t];

	// поворачиваем ребро дерева
	aLT[t] = aRT[k];
	aRT[k] = t;

	// корректируем size
	aSize[k] = aSize[t];
	TNodeIndex n_l = aLT[t];
	TNodeIndex n_r = aRT[t];
	TNodeSize s_l = ((n_l != -1) ? aSize[n_l] : 0);
	TNodeSize s_r = ((n_r != -1) ? aSize[n_r] : 0);
	aSize[t] = s_l + s_r + 1;

	// меняем трёх предков
	// 1. t.left.parent = t
	// 2. k.parent = t.parent
	// 3. t.parent = k
	if (aLT[t] != -1) aUT[aLT[t]] = t;
	aUT[k] = p;
	aUT[t] = k;

	// меняем корень, parent -> t, k
	if (p == -1) ROOT = k; // это root
	else {
		if (aLT[p] == t) aLT[p] = k;
		else aRT[p] = k; // вторую проверку можно не делать
	}
	return 1;
}

// Размеры вершин, size

TNodeSize SBT_Left_Left_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex l = aLT[t];
	if (l == -1) return 0;
	TNodeIndex ll = aLT[l];
	return ((ll == -1) ? 0 : aSize[ll]);
}

TNodeSize SBT_Left_Right_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex l = aLT[t];
	if (l == -1) return 0;
	TNodeIndex lr = aRT[l];
	return ((lr == -1) ? 0 : aSize[lr]);
}

TNodeSize SBT_Right_Right_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex r = aRT[t];
	if (r == -1) return 0;
	TNodeIndex rr = aRT[r];
	return ((rr == -1) ? 0 : aSize[rr]);
}

TNodeSize SBT_Right_Left_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex r = aRT[t];
	if (r == -1) return 0;
	TNodeIndex rl = aLT[r];
	return ((rl == -1) ? 0 : aSize[rl]);
}

TNodeSize SBT_Right_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex r = aRT[t];
	return ((r == -1) ? 0 : aSize[r]);
}

TNodeSize SBT_Left_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex l = aLT[t];
	return ((l == -1) ? 0 : aSize[l]);
}

// Сбалансировать дерево (более быстрый алгоритм)

int8_t SBT_Maintain_Simpler(TNodeIndex t, int8_t flag) {

	if (t < 0) return 0;
	TNodeIndex parent = aUT[t]; // есть "родитель"
	int8_t at_left = 0;
	if (parent == -1) { // t - корень дерева, он изменяется; запоминать нужно не индекс, а "топологию"
	}
	else {
	    if (aLT[parent] == t) at_left = 1; // "слева" от родителя - индекс родителя не изменился
	    else at_left = 0; // "справа" от родителя
	}

	// поместили слева, flag == 0
	if (flag == 0) {
		if (SBT_Left_Left_size(t) > SBT_Right_size(t)) {
			SBT_RightRotate_Opt(t);
		}
		else if (SBT_Left_Right_size(t) > SBT_Right_size(t)) {
			SBT_LeftRotate_Opt(aLT[t]);
			SBT_RightRotate_Opt(t);
		}
		else { return 0; }
	}
	// поместили справа, flag == 1
	else {
		if (SBT_Right_Right_size(t) > SBT_Left_size(t)) {
			SBT_LeftRotate_Opt(t);
		}
		else if (SBT_Right_Left_size(t) > SBT_Left_size(t)) {
			SBT_RightRotate_Opt(aRT[t]);
			SBT_LeftRotate_Opt(t);
		}
		else { return 0; }
	}

	TNodeIndex t0 = -1;
	if (parent == -1) t0 = ROOT;
	else {
	    if (at_left) t0 = aLT[parent];
	    else t0 = aRT[parent];
	}

	SBT_Maintain_Simpler(aLT[t0], 0); // false
	SBT_Maintain_Simpler(aRT[t0], 1); // true
	SBT_Maintain_Simpler(t0, 0); // false
	SBT_Maintain_Simpler(t0, 1); // true

	return 0;
}

// Сбалансировать дерево ("тупой" алгоритм)

int8_t SBT_Maintain(TNodeIndex t) {

	if (t < 0) return 0;

	TNodeIndex parent = aUT[t]; // есть "родитель"
	int8_t at_left = 0;
	if (parent == -1) { // t - корень дерева, он изменяется; запоминать нужно не индекс, а "топологию"
	}
	else {
	    if (aLT[parent] == t) at_left = 1; // "слева" от родителя - индекс родителя не изменился
	    else at_left = 0; // "справа" от родителя
	}

#define CALC_T0 \
	TNodeIndex t0 = -1; \
	if (parent == -1) t0 = ROOT; \
	else { \
	    if (at_left) t0 = aLT[parent]; \
	    else t0 = aRT[parent]; \
	}

	// поместили слева (?)
	if (SBT_Left_Left_size(t) > SBT_Right_size(t)) {
		SBT_RightRotate_Opt(t);
		CALC_T0
		SBT_Maintain(aRT[t0]);
		SBT_Maintain(t0);
	}
	else if (SBT_Left_Right_size(t) > SBT_Right_size(t)) {
		SBT_LeftRotate_Opt(aLT[t]);
		SBT_RightRotate_Opt(t);
		CALC_T0
		SBT_Maintain(aLT[t0]);
		SBT_Maintain(aRT[t0]);
		SBT_Maintain(t0);
	}
	// поместили справа (?)
	else if (SBT_Right_Right_size(t) > SBT_Left_size(t)) {
		SBT_LeftRotate_Opt(t);
		CALC_T0
		SBT_Maintain(aLT[t0]);
		SBT_Maintain(t0);
	}
	else if (SBT_Right_Left_size(t) > SBT_Left_size(t)) {
		SBT_RightRotate_Opt(aRT[t]);
		SBT_LeftRotate_Opt(t);
		CALC_T0
		SBT_Maintain(aLT[t0]);
		SBT_Maintain(aRT[t0]);
		SBT_Maintain(t0);
	}

	return 0;
}

// Добавить вершину в поддерево t, без проверки уникальности (куда - t, родительская - parent)

int8_t SBT_AddNode_At(TValue value, TNodeIndex t, TNodeIndex parent) {
	if (ROOT != -1) aSize[t]++;
	if (NU <= 0) {
		TNodeIndex t_new = SBT_AllocateNode();
		if (t_new == -1) return 0;
		aValue[t_new] = value;
		aUT[t_new] = parent;
		aLT[t_new] = -1;
		aRT[t_new] = -1;
		aSize[t_new] = 1;
		ROOT = 0;
		return 1;
	}
	else {
		if(value < aValue[t]) {
			if(aLT[t] == -1) {
				TNodeIndex t_new = SBT_AllocateNode();
				if (t_new == -1) return 0;
				aLT[t] = t_new;
				aValue[t_new] = value;
				aUT[t_new] = t;
				aLT[t_new] = -1;
				aRT[t_new] = -1;
				aSize[t_new] = 1;
			}
			else {
				SBT_AddNode_At(value, aLT[t], t);
			}
		}
		else {
			if(aRT[t] == -1) {
				TNodeIndex t_new = SBT_AllocateNode();
				if (t_new == -1) return 0;
				aRT[t] = t_new; // по-умолчанию, добавляем вправо (поэтому "левых" вращений больше)
				aValue[t_new] = value;
				aUT[t_new] = t;
				aLT[t_new] = -1;
				aRT[t_new] = -1;
				aSize[t_new] = 1;
			}
			else {
				SBT_AddNode_At(value, aRT[t], t);
			}
		}
	}
	// SBT_Maintain(t);
	SBT_Maintain_Simpler(t, (value >= aValue[t]) ? 1 : 0);
	return 1;
}

// Добавить вершину без проверки уникальности value

int8_t SBT_AddNode(TValue value) {
	return SBT_AddNode_At(value, ROOT, -1);
}

// Добавление вершины только если такой же (с таким же значением) в дереве нет

int8_t SBT_AddNodeUniq(TValue value) {

	int8_t result = SBT_FindNode(value); // fail, если вершина с таким value уже существует
	if (result == -1) {
		SBT_AddNode(value);
	}
	return result;
}

// Удалить вершину, в дереве t
// Алгоритм (взят из статьи про AVL на Википедии) :
// 1. Ищем вершину на удаление,
// 2. Ищем ей замену, спускаясь по левой ветке (value-),
// 3. Перевешиваем замену на место удаленной,
// 4. Выполняем балансировку вверх от родителя места, где находилась замена (от parent замены).

int8_t SBT_DeleteNode_At(TValue value, TNodeIndex t, TNodeIndex parent) {

	if ((NU <= 0) || (t < 0)) {
		return -1; // ответ: "Не найден"
	}
	TNodeIndex d = SBT_FindNode(value);
	// надо ли что-то делать? (вершину нашли?)
	if (d == -1) return -1;
	// d != -1
	TNodeIndex d_p = (d != -1) ? _Tree(d,parent) : -1;
	TNodeIndex l = SBT_FindNode_NearestAndLesser_ByIndex(d); // d.left -> right
	// l == -1, только если у t_d нет дочерних вершин слева (хотя бы одной, <= t_d),
	// в таком случае - просто удаляем t_d (без перевешивания)
	

	// l != -1 (Diagram No.1)
	if (l != -1) {
		TNodeIndex l_p = _Tree(l,parent);
		TNodeIndex l_l = _Tree(l,left); // l_r = l.right == -1
		TNodeIndex d_r = _Tree(d,right);

		// меняем правую часть l
		_Tree(l,right) = d_r;
		if (d_r != -1) _Tree(d_r,parent) = l;
		// меняем левую часть l
		if (l_p == d) { // не надо менять левую часть и l_p
		}
		else {
			TNodeIndex d_l = _Tree(d,left);
			// меняем верхнюю часть
			_Tree(l_p,right) = l_l;
			_Tree(l_l,parent) = l_p;
			// меняем левую часть l
			_Tree(l,left) = d_l;
			_Tree(d_l,parent) = l;
		}
		
		// меняем ссылку на корень
		if (d_p == -1){
			// меняем l <-> root = t_d_p = -1 (1)
			ROOT = l;
			_Tree(l,parent) = -1;
		}
		else {
			// меняем l <-> root = t_d_p != -1
			if (_Tree(d_p,left) == d) _Tree(d_p,left) = l; // ?
			else _Tree(d_p,right) = l; // ?
			_Tree(l,parent) = d_p;
		}

		TNodeIndex q;
		if (l_p == d) q = l;
		else q = l_p;
		while(q != -1) {
			_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
			q = _Tree(q,parent);
		}


#ifndef SBT_DONT_MAINTAIN
		if (l_l != -1) q = l_l;
		else q = l_p;
		while(q != -1) {
			//_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
			SBT_Maintain(q);
			q = _Tree(q,parent);
		}
#endif

		SBT_FreeNode(d);

	}

	else {
		TNodeIndex r = SBT_FindNode_NearestAndGreater_ByIndex(d); // d.right -> left
		// (Diagram No.3)
		if (r == -1) {
			// вершина d является листом
			// меняем ссылку на корень
			if (d_p == -1) ROOT = -1;
			else {
				if (_Tree(d_p,left) == d) _Tree(d_p,left) = -1;
				else _Tree(d_p,right) = -1;
			}
			// можно не делать: _Tree(d,parent) = -1;
			// больше ничего делать не надо
			TNodeIndex q = d_p;
			while(q != -1) {
				_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
				q = _Tree(q,parent);
			}

#ifndef SBT_DONT_MAINTAIN
			q = d_p;
			while(q != -1) {
				SBT_Maintain(q);
				q = _Tree(q,parent);
			}
#endif

			SBT_FreeNode(d);

		}
		// r != -1 (Diagram No.2)
		else {
			// меняем справа
			TNodeIndex r_p = _Tree(r,parent);
			TNodeIndex r_r = _Tree(r,right); // r_l = r.left == -1
			TNodeIndex d_l = _Tree(d,left); // == -1 всегда?

			// меняем левую часть r <-> d_l
			_Tree(r,left) = d_l;
			if (d_l != -1) _Tree(d_l,parent) = r;
			if (r_p == d) { // правую часть менять не надо, r_r
			}
			else {
				TNodeIndex d_r = _Tree(d,right); // != -1 всегда?
				// меняем верхнюю часть
				_Tree(r_p,left) = r_r;
				_Tree(r_r,parent) = r_p;
				// меняем правую часть r <-> d_r
				_Tree(r,right) = d_r;
				_Tree(d_r,parent) = r;
			}
			
			// меняем ссылку на корень
			if (d_p == -1){
				// меняем l <-> root = t_d_p = -1 (1)
				ROOT = r;
				_Tree(r,parent) = -1;
			}
			else {
				// меняем l <-> root = t_d_p != -1
				if(_Tree(d_p,right) == d) _Tree(d_p,right) = r;
				else _Tree(d_p,left) = r;
				_Tree(r,parent) = d_p;
			}

			TNodeIndex q;
			if (r_p == d) q = r;
			else q = r_p;
			while(q != -1) {
				_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
				q = _Tree(q,parent);
			}


#ifndef SBT_DONT_MAINTAIN
			if (r_r != -1) q = r_r;
			else q = r_p;
			while(q != -1) {
				SBT_Maintain(q);
				q = _Tree(q,parent);
			}
#endif

			SBT_FreeNode(d);

		}
	}

	return d;
}


// Удалить первую попавшуюся вершину по значению value

int8_t SBT_DeleteNode(TValue value) {

	TNodeIndex t = SBT_DeleteNode_At(value, ROOT, -1);

	return t;
}

// Удалить все вершины с данным значением value

int8_t SBT_DeleteAllNodes(TValue value) {
	int8_t result = -1;
	while ((result = SBT_DeleteNode_At(value, ROOT, -1)) != -1);
	return result;
}

// Напечатать вершины поддерева t

void SBT_PrintAllNodes_At(int8_t depth, TNodeIndex t) {
	int8_t i;

	if ((NU <= 0) || (t < 0)) {
		return; // выйти, если вершины нет
	}

	// сверху - большие вершины
	if (aRT[t] >= 0) SBT_PrintAllNodes_At(depth + 1, aRT[t]);

	if (!((aUT[t] == -1) && (aLT[t] == -1) && (aRT[t] == -1)) || (t == ROOT)) { // ? что будет если напечатать при -1
		for (i = 0; i < depth; i++) printf(" "); // отступ
		printf("+%d, id = %lld, value = %lld, size = %lld\n",
			(int)depth,
			(long long int)t,
			(long long int)aValue[t],
			(long long int)aSize[t]
		); // иначе: напечатать "тело" узла
	}

	// снизу - меньшие
	if (aLT[t] >= 0) SBT_PrintAllNodes_At(depth + 1, aLT[t]);
}

// Напечатать все вершины, начиная от корня дерева

void SBT_PrintAllNodes() {
	printf("USED = %lld\n", (long long int)NU);
	SBT_PrintAllNodes_At(0, ROOT);
}


// Пройтись по поддереву t

void SBT_WalkAllNodes_At(int8_t depth, TNodeIndex t) {

	if ((NU <= 0) || (t < 0)) {
		funcOnWalk(t, -1, "WALK_UP");
		return; // выйти, если вершины нет
	}

	// меньшие значения
	if (aLT[t] >= 0) {
		funcOnWalk(t, aLT[t], "WALK_DOWN_LEFT");
		SBT_WalkAllNodes_At(depth + 1, aLT[t]);
	}

	funcOnWalk(t, t, "WALK_NODE");

	// бо'льшие значения
	if (aRT[t] >= 0) {
		funcOnWalk(t, aRT[t], "WALK_DOWN_RIGHT");
		SBT_WalkAllNodes_At(depth + 1, aRT[t]);
	}

	funcOnWalk(t, -1, "WALK_UP");

}

// Пройтись по всем вершинам, сгенерировать события WALK_ (посылаются в зарегистрированные обработчики событий)

void SBT_WalkAllNodes() {
	funcOnWalk(-1, -1, "WALK_STARTS");
	SBT_WalkAllNodes_At(0, ROOT);
	funcOnWalk(-1, -1, "WALK_FINISH");
}

// Найти значение value в поддереве t (корректно при использовании AddNodeUniq)

TNodeIndex SBT_FindNode_At(TValue value, TNodeIndex t) {

	if ((NU <= 0) || (t < 0)) return -1; // нечего обрабатывать, [тернарный] элемент не найден

	if (value == aValue[t]) return t; // Среагировать на найденный элемент, вернуть индекс этой ноды
	else if (value < aValue[t]) {
		// влево
		return SBT_FindNode_At(value, aLT[t]);
	}
	// можно не делать дополнительное сравнение для целых чисел
	else {
		// вправо
		return SBT_FindNode_At(value, aRT[t]);
	}

	// не выполняется
	return -1; // "не найден"
}

// Найти вершину от корня ROOT

TNodeIndex SBT_FindNode(TValue value) {
	if (NU <= 0) return -1; // нечего искать
	return SBT_FindNode_At(value, ROOT); // надо искать от корня по всему дереву
}


// ? надо ли (не реализовано)
void SBT_FindAllNodes_At(TValue value, TNodeIndex t) {
	return;
}

// ? надо ли (не реализовано)
void SBT_FindAllNodes(TValue value) {
	return SBT_FindAllNodes_At(value, ROOT);
}


// Проверка дерева на SBT-balance-корректность, начиная с ноды t
void SBT_CheckAllNodesBalance_At(int8_t depth, TNodeIndex t) {

	if ((NU <= 0) || (t < 0)) return; // нечего обрабатывать

	// меньшие значения
	if (aLT[t] >= 0) {
		SBT_CheckAllNodesBalance_At(depth + 1, aLT[t]);
	}
	// проверить
	if ((SBT_Left_Left_size(t) > SBT_Right_size(t)) && (SBT_Right_size(t) > 0)) {
		printf("ERROR %lld LL > R (%lld > %lld)\n",
			(long long int)t,
			(long long int)SBT_Left_Left_size(t),
			(long long int)SBT_Right_size(t)
		);
	}
	if ((SBT_Left_Right_size(t) > SBT_Right_size(t)) && (SBT_Right_size(t) > 0)) {
		printf("ERROR %lld LR > R (%lld > %lld)\n",
			(long long int)t,
			(long long int)SBT_Left_Right_size(t),
			(long long int)SBT_Right_size(t)
		);
	}
	if ((SBT_Right_Right_size(t) > SBT_Left_size(t)) && (SBT_Left_size(t) > 0)) {
		printf("ERROR %lld RR > L (%lld > %lld)\n",
			(long long int)t,
			(long long int)SBT_Right_Right_size(t),
			(long long int)SBT_Left_size(t)
		);
	}
	if ((SBT_Right_Left_size(t) > SBT_Left_size(t)) && (SBT_Left_size(t) > 0)) {
		printf("ERROR %lld RL > L (%lld > %lld)\n",
			(long long int)t,
			(long long int)SBT_Right_Left_size(t),
			(long long int)SBT_Left_size(t)
		);
	}
	
	// бо'льшие значения
	if (aRT[t] >= 0) {
		SBT_CheckAllNodesBalance_At(depth + 1, aRT[t]);
	}

}

// Проверка дерева на SBT-size-корректность, начиная с ноды t
void SBT_CheckAllNodesSize_At(int8_t depth, TNodeIndex t) {

	if ((NU <= 0) || (t < 0)) return; // нечего обрабатывать

	// меньшие значения
	if (aLT[t] >= 0) {
		SBT_CheckAllNodesSize_At(depth + 1, aLT[t]);
	}

	// проверить
	if (aSize[t] !=  SBT_Left_size(t) + SBT_Right_size(t) + 1) {
		printf("size error: idx = %lld (size: %lld <> %lld + %lld + 1)\n",
			(long long int)t,
			(long long int)aSize[t],
			(long long int)SBT_Left_size(t),
			(long long int)SBT_Right_size(t)
		);
		printf("----\n");
		SBT_PrintAllNodes();
		printf("----\n");
	}

	// бо'льшие значения
	if (aRT[t] >= 0) {
		SBT_CheckAllNodesSize_At(depth + 1, aRT[t]);
	}

}

// Проверить всё дерево на SBT-balance-корректность, начиная с корневой ноды ROOT
void SBT_CheckAllNodesBalance() {
	SBT_CheckAllNodesBalance_At(0, ROOT);
}

// Проверить всё дерево на SBT-size-корректность, начиная с корневой ноды ROOT
void SBT_CheckAllNodesSize() {
	SBT_CheckAllNodesSize_At(0, ROOT);
}

// Проверить всё дерево на SBT-корректность, начиная с корневой ноды ROOT
void SBT_CheckAllNodes() {
	SBT_CheckAllNodesBalance(); // проверить на правильность балансировки
	SBT_CheckAllNodesSize(); // проверить простановку sizes
}


// Полная распечатка всех WORK-,UNUSED-nodes (всё из unCLEAN-области)
void SBT_DumpAllNodes() {
	uint64_t i;
	for (i = 0; i < NN - NCLEAN; i++) {
		printf("[%lld] = %lld L:%lld R:%lld U:%lld SZ:%lld\n",
			(long long int)i,
			(long long int)aValue[i],
			(int)aFREE[i],
			(long long int)aLT[i],
			(long long int)aRT[i],
			(long long int)aUT[i],
			(long long int)aSize[i]
		);
	}
}

// Корневой элемент, Index
TNodeIndex GetRootIndex() {
	return ROOT;
}


// Относится к memory management, эти функции ничего не должны знать о _структуре_ дерева.
// Значения left, right и т.п. они используют лишь для выстраивания элементов в список.
// Выделение памяти - из кольцевого FIFO-буфера [FREE-нодов], либо из массива CLEAN-нодов.

TNodeIndex SBT_AllocateNode() {

/*
	at Begin

	NU = 0; // число вершин в дереве, использованные (USED)
	ROOT = -1; // дерево
	FREE = -1; // список неиспользованных
	NCLEAN = NN; // количество "чистых"; только уменьшается
*/

	TNodeIndex t = -1; // нет ноды

	// выделить из CLEAN-области (чистые ячейки)
	if (FREE == -1) {
		if (NCLEAN > 0) {

			t = NN - NCLEAN; // первый из CLEAN-элементов
			NCLEAN--;
		}
		else {
			t = -1; // память закончилась
			return t;
		}
	}

	// выделить из UNUSED-области, есть FREE-ноды
	else {
		t = FREE; // берем из начала
		if (aRT[t] == t) {
			FREE = -1; // нет больше элементов
		}
		else {
			FREE = aRT[t]; // перемещаем указатель на следующий элемент списка
			TNodeIndex last = aLT[t];
			// теперь R(first) - первый элемент, на него ссылается last
			aLT[FREE] = last;
			aRT[last] = FREE;
		}
	}

	// дополнительная очистка
	aLT[t] = -1;
	aRT[t] = -1;
	aUT[t] = -1;
	aSize[t] = 0;
	aValue[t] = 0;
	aFREE[t] = 0x00; // ЗАНЯТО

	NU++; // USED

	return t; // TNodeIndex
}

// ? Небезопасная функция (следите за целостностью дерева самостоятельно)
// Делает переключение вершины в "кольцевой буфер" aFREE[NN]
// В плане алгоритмов, FreeNode() не знает ничего о структуре дерева,
// она работает с ячейками, управляя только ссылками left и right.

int8_t SBT_FreeNode(TNodeIndex t) {

	if (NU <= 0) return 0; // нечего удалять, нет USED-вершин
	if (t < 0) return 0; // не туда указали by Index
	if (aFREE[t] == 0x01) return 0; // уже удалено

	// FREE-список пуст, тогда сделать удалённую вершину первым элементом в UNUSED-пространстве
	if (FREE == -1) {
		aLT[t] = t;
		aRT[t] = t;
		FREE = t; // ? указывает сама на себя, циклический список
	}

	// Добавить в UNUSED-пространство
	else {
		TNodeIndex last = aLT[FREE]; // так как уже существует первая FREE-вершина; вставляем слева от FREE
		// ссылки внутри
		aLT[t] = last;
		aRT[last] = t;
		// ссылки снаружи
		aLT[FREE] = t;
		aRT[t] = FREE;
		// FREE не изменяется
	}
	aUT[t] = -1;
	aSize[t] = 0;
	aValue[t] = 0;
	aFREE[t] = 0x01; // освободили

	NU--; // USED вспомогательный полезный счетчик, эти вершины также можно пересчитать в WORK-пространстве

	return 1;
}

// Найти самый близкий по значению элемент в "левом" поддереве, start by Index
TNodeIndex SBT_FindNode_NearestAndLesser_ByIndex(TNodeIndex t) {
	if (t != -1) {
		TNodeIndex left = aLT[t];
		if (left == -1) t = -1; // левого поддерева нет
		else {
			TNodeIndex parent = t;
			TNodeIndex right = left; // ? можно немного оптимизировать amd64
			while (right != -1) {
				parent = right;
				right = aRT[right];
			}
			t = parent;
		}
	}
	return t;
}

// Найти самый близкий по значению элемент в "правом" поддереве, start by Index
TNodeIndex SBT_FindNode_NearestAndGreater_ByIndex(TNodeIndex t) {
	if (t != -1) {
		TNodeIndex right = aRT[t];
		if (right == -1) t = -1; // правого поддерева нет
		else {
			TNodeIndex parent = t;
			TNodeIndex left = right; // ? можно немного оптимизировать amd64
			while (left != -1) {
				parent = left;
				left = aLT[left];
			}
			t = parent;
		}
	}
	return t;
}

// Найти самый близкий по значению элемент в "левом" поддереве
TNodeIndex SBT_FindNode_NearestAndLesser_ByValue(TValue value) {
	return SBT_FindNode_NearestAndLesser_ByIndex(SBT_FindNode(value));
}

// Найти самый близкий по значению элемент в "правом" поддереве
TNodeIndex SBT_FindNode_NearestAndGreater_ByValue(TValue value) {
	return SBT_FindNode_NearestAndGreater_ByIndex(SBT_FindNode(value));
}

// Найти следующую USED-вершину
TNodeIndex SBT_FindNextUsedNode(TNodeIndex s) {
	uint8_t f = 0;
	uint64_t t = 0;
	// ищем от указанной позиции
	for(t = s; t < NN - NCLEAN; t++) if (aFREE[t] == 0x00) { f = 1; break; }

	// если ещё не найдена использующаяся ячейка
	if (f == 0) {
		// ищем от начала
		for(t = 0; t < s; t++) if (aFREE[t] == 0x00) { f = 1; break; }
	}
	if (f == 1) return t;
	else return -1; // не нашли
}

// Прямая выборка Value, = INT64_MAX, если не можем найти элемент по индексу
// ? Доработать
TValue GetValueByIndex(TNodeIndex t) {
	return (t != -1) ? aValue[t] : INT64_MAX; // ? MAX
}
