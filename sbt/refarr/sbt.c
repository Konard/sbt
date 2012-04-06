
#include "sbt.h"

#include <stdio.h> // printf

#ifdef __LINUX__
#include <pthread.h> // pthread_mutex_*
// узкое место при доступе из многих потоков - глобальная блокировка таблицы _nodes
pthread_mutex_t _lock_nodes = PTHREAD_MUTEX_INITIALIZER;
#endif

// переменные модуля


#ifdef SBT_ARRAYS
TNumber _a_value[SBT_MAX_NODES]; // значение, привязанное к ноде
/* TNodeIndex _a_parent[SBT_MAX_NODES]; // ссылка на уровень выше (избавляемся) */
TNodeIndex _a_left[SBT_MAX_NODES];  // ссылка на левое поддерево, = -1, если нет дочерних вершин
TNodeIndex _a_right[SBT_MAX_NODES]; // ссылка на правое поддерево
TNodeSize _a_size[SBT_MAX_NODES]; // size в понимании SBT
int _a_unused[SBT_MAX_NODES]; // «удалённая»; это поле можно использовать и для других флагов
#else
#define _Tree(idx,kind) _nodes[idx].kind
TNode _nodes[SBT_MAX_NODES];
#endif
/* to replace, 
typedef struct TNode {
	TNumber value; // значение, привязанное к ноде
	TNodeIndex parent; // ссылка на уровень выше
	TNodeIndex left;  // ссылка на левое поддерево, = -1, если нет дочерних вершин
	TNodeIndex right; // ссылка на правое поддерево
	TNodeSize size; // size в понимании SBT
	int unused; // «удалённая»; это поле можно использовать и для других флагов
} TNode;
*/

#ifdef SBT_ARRAYS
#define _Tree(idx,kind) _a_##kind[idx]
#else
#define _Tree(idx,kind) _nodes[idx].kind
#endif


TNodeIndex _n_nodes = 0; // число вершин в дереве
TNodeIndex _tree_root = -1; // дерево
TNodeIndex _tree_unused = -1; // список неиспользованных
TNodeIndex _n_clean = SBT_MAX_NODES; // хвост "чистых"; только уменьшается - использованные вершины перемещаются в список unused



// Event-driven technique, функции для оповещения о событиях

FuncOnRotate funcOnRotate = NULL;
FuncOnWalk funcOnWalk = NULL;
FuncOnFind funcOnFind = NULL;

int SBT_SetCallback_OnRotate(FuncOnRotate func_) {
	funcOnRotate = func_;
	return 0;
}

int SBT_SetCallback_OnWalk(FuncOnWalk func_) {
	funcOnWalk = func_;
	return 0;
}

int SBT_SetCallback_OnFind(FuncOnFind func_) {
	funcOnFind = func_;
	return 0;
}


// memory management, эти функции ничего не должны знать о _структуру_ дерева
// значения .left/.right и т.п. они используют лишь для выстраивания элементов в список

// выделение памяти из кольцевого FIFO-буфера (UNUSED-нодов), иначе - из массива CLEAN-нодов

TNodeIndex SBT_AllocateNode() {

	TNodeIndex t = 0; // нет ноды

	// выделить из CLEAN-области (чистые ячейки)
	if (_tree_unused == -1) {
		if (_n_clean > 0) {
			
			t = SBT_MAX_NODES - _n_clean;
			_n_clean--;
		}
		else {
			t = -1; // память закончилась
			return t;
		}
	}

	// выделить из UNUSED-области (списка)
	else {
		// вершины в списке есть
		TNodeIndex t = _tree_unused; // берем из начала
		if (_Tree(t,right) == t) {
			// нет больше элементов
			_tree_unused = -1;
		}
		else {
			_tree_unused = _Tree(t,right); // перемещаем указатель на следующий элемент списка
			TNodeIndex last = _Tree(t,left);
			// теперь first.right - первый элемент, на него ссылается last
			_Tree(_tree_unused,left) = last;
			_Tree(last,right) = _tree_unused;
		}
	}

	// дополнительная очистка
	_Tree(t,left) = -1;
	_Tree(t,right) = -1;
/*	_Tree(t,parent) = -1; */
	_Tree(t,size) = 0;
	_Tree(t,value) = 0;
	_Tree(t,unused) = 0;
	// счетчика _n_unused нет (но можно добавить)
	_n_nodes++;

	return t; // результат (нода)
}

// небезопасная функция ! следите за целостностью дерева самостоятельно !
// подключение в "кольцевой буфер"
// в плане методики, FreeNode не знает ничего о структуре дерева, она работает с ячейками, управляя только ссылками .left и .right

int SBT_FreeNode(TNodeIndex t) {

	if (_n_nodes <= 0) return 0; // нечего удалять, >0 нам нужно
	if (t < 0) return 0;
	if (_Tree(t,unused) == 1) return 0; // уже удалено

	// UNUSED пуст, сделать первым элементом в unused-пространстве
	if (_tree_unused == -1) {
		_Tree(t,left) = t;
		_Tree(t,right) = t;
		_tree_unused = t;
	}

	// UNUSED уже частично или полностью заполнен, добавить в unused-пространство
	else {
		TNodeIndex last = _Tree(_tree_unused,left); // так как существует root-вершина (first), то и last <> -1
		// ссылки внутри
		_Tree(t,left) = last;
		_Tree(last,right) = t;
		// ссылки снаружи
		_Tree(_tree_unused,left) = t;
		_Tree(t,right) = _tree_unused;
		// _tree_unused не изменяется
	}
/*	_Tree(t,parent) = -1; */
	_Tree(t,size) = 0;
	_Tree(t,value) = 0;
	_Tree(t,unused) = 1;
	// счетчика _n_unused нет
	_n_nodes--; // условный счетчик (вспомогательный): эти вершины можно пересчитать в WORK-пространстве

	return 1;
}


// Функции Rotate, Maintain & Add, Delete //

// Вращение влево, t - слева, перевешиваем туда (вершины не пропадают, _n_nodes сохраняет значение)

int SBT_LeftRotate(TNodeIndex t, TNodeIndex parent) {

        if (t < 0) return 0;
        TNodeIndex k = _Tree(t,right);
        if (k < 0) return 0;
/*        TNodeIndex p = _Tree(t,parent); */
//	if (funcOnRotate != NULL) funcOnRotate(k, t, "LEFT_ROTATE");

        // поворачиваем ребро дерева
        _Tree(t,right) = _Tree(k,left);
        _Tree(k,left) = t;

        // корректируем size
        _Tree(k,size) = _Tree(t,size);
        TNodeIndex n_l = _Tree(t,left);
        TNodeIndex n_r = _Tree(t,right); // для ускорения — выборку из кэша
        TNodeSize s_l = ((n_l != -1) ? _Tree(n_l,size) : 0);
        TNodeSize s_r = ((n_r != -1) ? _Tree(n_r,size) : 0);
        _Tree(t,size) = s_l + s_r + 1;

/*
        // меняем трёх предков
        // 1. t.right.parent = t
        // 2. k.parent = t.parent
        // 3. t.parent = k
        if (_Tree(t,right) != -1) _Tree(_Tree(t,right),parent) = t; // из кэша
        _Tree(k,parent) = p;
        _Tree(t,parent) = k;
*/
        // меняем корень, parent -> t, k
        if (parent == -1) _tree_root = k; // это root
        else {
                if (_Tree(parent,left) == t) _Tree(parent,left) = k;
                else _Tree(parent,right) = k; // вторую проверку можно не делать
        }
        return 1;
}

// Вращение вправо, t - справа, перевешиваем туда (вершины не пропадают, _n_nodes сохраняет значение)

int SBT_RightRotate(TNodeIndex t, TNodeIndex parent) {

	if (t < 0) return 0;
	TNodeIndex k = _Tree(t,left);
	if (k < 0) return 0;
/*	TNodeIndex p = _Tree(t,parent); */
//	if (funcOnRotate != NULL) funcOnRotate(k, t, "RIGHT_ROTATE");

	// поворачиваем ребро дерева
	_Tree(t,left) = _Tree(k,right);
	_Tree(k,right) = t;

	// корректируем size
	_Tree(k,size) = _Tree(t,size);
	TNodeIndex n_l = _Tree(t,left);
	TNodeIndex n_r = _Tree(t,right);
	TNodeSize s_l = ((n_l != -1) ? _Tree(n_l,size) : 0);
	TNodeSize s_r = ((n_r != -1) ? _Tree(n_r,size) : 0);
	_Tree(t,size) = s_l + s_r + 1;

/*
	// меняем трёх предков
	// 1. t.left.parent = t
	// 2. k.parent = t.parent
	// 3. t.parent = k
	if (_Tree(t,left) != -1) _Tree(_Tree(t,left),parent) = t;
	_Tree(k,parent) = p;
	_Tree(t,parent) = k;
*/
	// меняем корень, parent -> t, k
	if (parent == -1) _tree_root = k; // это root
	else {
		if (_Tree(parent,left) == t) _Tree(parent,left) = k;
		else _Tree(parent,right) = k; // вторую проверку можно не делать
	}
	return 1;
}

// Размеры для вершин, size

TNodeSize SBT_Left_Left_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex l = _Tree(t,left);
	if (l == -1) return 0;
	TNodeIndex ll = _Tree(l,left);
	return ((ll == -1) ? 0 : _Tree(ll,size));
}

TNodeSize SBT_Left_Right_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex l = _Tree(t,left);
	if (l == -1) return 0;
	TNodeIndex lr = _Tree(l,right);
	return ((lr == -1) ? 0 : _Tree(lr,size));
}

TNodeSize SBT_Right_Right_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex r = _Tree(t,right);
	if (r == -1) return 0;
	TNodeIndex rr = _Tree(r,right);
	return ((rr == -1) ? 0 : _Tree(rr,size));
}

TNodeSize SBT_Right_Left_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex r = _Tree(t,right);
	if (r == -1) return 0;
	TNodeIndex rl = _Tree(r,left);
	return ((rl == -1) ? 0 : _Tree(rl,size));
}

TNodeSize SBT_Right_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex r = _Tree(t,right);
	return ((r == -1) ? 0 : _Tree(r,size));
}

TNodeSize SBT_Left_size(TNodeIndex t) {
	if (t == -1) return 0;
	TNodeIndex l = _Tree(t,left);
	return ((l == -1) ? 0 : _Tree(l,size));
}


// Сбалансировать дерево (более быстрый алгоритм)

int SBT_Maintain_Simpler(TNodeIndex t, TNodeIndex parent, int flag) {
// если меняются узлы - надо поменять ссылку на них

	if (t < 0) return 0;
/*	TNodeIndex parent = _Tree(t,parent); // есть "родитель" */
	int at_left = 0;
	if (parent == -1) { // t - корень дерева, он изменяется; запоминать нужно не индекс, а "топологию"
	}
	else {
	    if (_Tree(parent,left) == t) at_left = 1; // "слева" от родителя - индекс родителя не изменился
	    else at_left = 0; // "справа" от родителя
	}

	// поместили слева, flag == 0
	if (flag == 0) {
		if (SBT_Left_Left_size(t) > SBT_Right_size(t)) {
			SBT_RightRotate(t, parent);
		}
		else if (SBT_Left_Right_size(t) > SBT_Right_size(t)) {
			SBT_LeftRotate(_Tree(t,left), t);
			SBT_RightRotate(t, parent);
		}
		else { return 0; }
	}
	// поместили справа, flag == 1
	else {
		if (SBT_Right_Right_size(t) > SBT_Left_size(t)) {
			SBT_LeftRotate(t, parent);
		}
		else if (SBT_Right_Left_size(t) > SBT_Left_size(t)) {
			SBT_RightRotate(_Tree(t,right), t);
			SBT_LeftRotate(t, parent);
		}
		else { return 0; }
	}

	TNodeIndex t0 = -1;
	if (parent == -1) t0 = _tree_root;
	else {
	    if (at_left) t0 = _Tree(parent,left);
	    else t0 = _Tree(parent,right);
	}
	SBT_Maintain_Simpler(_Tree(t0,left), t0, 0); // false
	SBT_Maintain_Simpler(_Tree(t0,right), t0, 1); // true
	SBT_Maintain_Simpler(t0, parent, 0); // false
	SBT_Maintain_Simpler(t0, parent, 1); // true

	return 0;
}

// Сбалансировать дерево ("тупой" алгоритм)

int SBT_Maintain(TNodeIndex t, TNodeIndex parent) {

	if (t < 0) return 0;

/*	TNodeIndex parent = _Tree(t,parent); // есть "родитель" */
	int at_left = 0;
	if (parent == -1) { // t - корень дерева, он изменяется; запоминать нужно не индекс, а "топологию"
	}
	else {
	    if (_Tree(parent,left) == t) at_left = 1; // "слева" от родителя - индекс родителя не изменился
	    else at_left = 0; // "справа" от родителя
	}

#define CALC_T0 \
	TNodeIndex t0 = -1; \
	if (parent == -1) t0 = _tree_root; \
	else { \
	    if (at_left) t0 = _Tree(parent,left); \
	    else t0 = _Tree(parent,right); \
	}

	// поместили слева (?)
	if (SBT_Left_Left_size(t) > SBT_Right_size(t)) {
		SBT_RightRotate(t, parent);
		CALC_T0
		SBT_Maintain(_Tree(t0,right), t0);
		SBT_Maintain(t0, parent);
	}
	else if (SBT_Left_Right_size(t) > SBT_Right_size(t)) {
		SBT_LeftRotate(_Tree(t,left), t);
		SBT_RightRotate(t, parent);
		CALC_T0
		SBT_Maintain(_Tree(t0,left), t0);
		SBT_Maintain(_Tree(t0,right), t0);
		SBT_Maintain(t0, parent);
	}
	// поместили справа (?)
	else if (SBT_Right_Right_size(t) > SBT_Left_size(t)) {
		SBT_LeftRotate(t, parent);
		CALC_T0
		SBT_Maintain(_Tree(t0,left), t0);
		SBT_Maintain(t0, parent);
	}
	else if (SBT_Right_Left_size(t) > SBT_Left_size(t)) {
		SBT_RightRotate(_Tree(t,right), t);
		SBT_LeftRotate(t, parent);
		CALC_T0
		SBT_Maintain(_Tree(t0,left), t0);
		SBT_Maintain(_Tree(t0,right), t0);
		SBT_Maintain(t0, parent);
	}

	return 0;
}

// Добавить вершину в поддерево t, без проверки уникальности (куда - t, родительская - parent)

int SBT_AddNode_At(TNumber value, TNodeIndex t, TNodeIndex parent) {
	_Tree(t,size)++;
	if (_n_nodes <= 0) {
		TNodeIndex t_new = SBT_AllocateNode();
		_Tree(t_new,value) = value;
/*		_Tree(t_new,parent) = parent; */
		_Tree(t_new,left) = -1;
		_Tree(t_new,right) = -1;
		_Tree(t_new,size) = 1;
		_tree_root = t_new;
	}
	else {
		if(value < _Tree(t,value)) {
			if(_Tree(t,left) == -1) {
				TNodeIndex t_new = SBT_AllocateNode();
				_Tree(t,left) = t_new;
				_Tree(t_new,value) = value;
				_Tree(t_new,value) = value;
/*				_Tree(t_new,parent) = t; */
				_Tree(t_new,left) = -1;
				_Tree(t_new,right) = -1;
				_Tree(t_new,size) = 1;
			}
			else {
				SBT_AddNode_At(value, _Tree(t,left), t);
			}
		}
		else {
			if(_Tree(t,right) == -1) {
				TNodeIndex t_new = SBT_AllocateNode();
				_Tree(t,right) = t_new; // по-умолчанию, добавляем вправо (поэтому "левых" вращений больше)
				_Tree(t_new,value) = value;
				_Tree(t_new,value) = value;
/*				_Tree(t_new,parent) = t; */
				_Tree(t_new,left) = -1;
				_Tree(t_new,right) = -1;
				_Tree(t_new,size) = 1;
			}
			else {
				SBT_AddNode_At(value, _Tree(t,right), t);
			}
		}
	}
	//SBT_Maintain(t, parent);
	SBT_Maintain_Simpler(t, parent, (value >= _Tree(t,value)) ? 1 : 0);
	return 0;
}

// Добавить вершину без проверки уникальности value

int SBT_AddNode(TNumber value) {
	return SBT_AddNode_At(value, _tree_root, -1);
}

// Добавление вершины только если такой же (с таким же значением) в дереве нет

int SBT_AddNodeUniq(TNumber value) {

	int result = SBT_FindNode(value); // fail, если вершина с таким value уже существует
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

// Удалить первую попавшуюся вершину по значению value

#define SBT_MAX_DEPTH 256
static TNodeIndex _a_path_delete[SBT_MAX_DEPTH];

int SBT_DeleteNode(TNumber value) {

	if (_n_nodes <= 0) return 0; // ответ: "Не найден"
	if (_tree_root == -1) return 0;

	// FindNode d
//	printf("[0] FindNode d\n");

	TNodeIndex d = -1;
	TNodeIndex d_p = -1;

	TNodeIndex q_p = -1;
	TNodeIndex q = _tree_root; // <> -1

	int _path_idx = -1; // на последнем занятом элементе
	int _path_idx_d = -1;
	do {
		_path_idx++;
		_a_path_delete[_path_idx] = q;
		// переходим к следующему элементу (может оказаться q = -1)
		if(value > _Tree(q,value)) {
			q_p = q;
			q = _Tree(q,right);
		}
		else {
			if(value < _Tree(q,value)) {
				q_p = q;
				q = _Tree(q,left);
			}
			else {
				// остаёмся на найденном элементе
				d = q;
				d_p = q_p;
				break;
			}
		}
	} while(q != -1);
	// на выходе d = q (value) или q = -1 (left/right)
//	printf("d_p = %lld\n", (long long int)d_p);
//	printf("d = %lld\n", (long long int)d);
	if (d == -1) return 0; // нечего возвращать
	// q != -1, d != -1, элемент найден; d остался последним элементом в стековом буфере
	
	_path_idx_d = _path_idx; // последний элемент списка
	_path_idx++; // переходим на свободные элементы

	// l = SBT_FindNode_NearestAndLesser_ByIndex(d)
//	printf("[1] l = SBT_FindNode_NearestAndLesser_ByIndex(d)\n");

	TNodeIndex left = _Tree(d,left);
//	printf("left = %lld\n", (long long int)left);
	_a_path_delete[_path_idx] = left;
	_path_idx++;

	TNodeIndex l = -1; // d.left to right
	TNodeIndex l_p = -1;
	if (left != -1) {
		// left != -1
		TNodeIndex right = left;
		TNodeIndex right_parent = d;
		while (_Tree(right,right) != -1) {
			right_parent = right;
			right = _Tree(right,right);
			if (right != -1) {
				_a_path_delete[_path_idx] = right;
				_path_idx++;
			}
		}
		l_p = right_parent;
		l = right;
	}
	// если не найдено (if not found), l = -1; l_p = -1
	int _path_idx_l = _path_idx;
	// l = right записан последним в _path_idx_l

	// l == -1, только если у t_d нет дочерних вершин слева (хотя бы одной, <= t_d),
	// в таком случае - просто удаляем t_d (без перевешивания)
	

	// l != -1 (Diagram No.1)
	if (l != -1) {
		TNodeIndex l_l = _Tree(l,left); // l_r = l.right == -1
		_a_path_delete[_path_idx] = l_l; // последний элемент
		_path_idx++; // опять на свободный конец
		TNodeIndex d_r = _Tree(d,right);

		// меняем правую часть l
		_Tree(l,right) = d_r;
/*		if (d_r != -1) _Tree(d_r,parent) = l; */
		// меняем левую часть l
		if (l_p == d) { // не надо менять левую часть и l_p
		}
		else {
			TNodeIndex d_l = _Tree(d,left); // d_l != -1
			// меняем верхнюю часть
			_Tree(l_p,right) = l_l;
/*			_Tree(l_l,parent) = l_p; */
			// меняем левую часть l
			_Tree(l,left) = d_l;
/*			_Tree(d_l,parent) = l; */
		}
		
		// меняем ссылку на корень
		if (d_p == -1){
			// меняем l <-> root = t_d_p = -1 (1)
//			printf("_tree_root = l;\n");
			_tree_root = l;
/*			_Tree(l,parent) = -1; */
		}
		else {
			// меняем l <-> root = t_d_p != -1
			if (_Tree(d_p,left) == d) _Tree(d_p,left) = l; // ?
			else _Tree(d_p,right) = l; // ?
/*			_Tree(l,parent) = d_p; */
		}

		// Нужен буфер глубины до SBT_MAX_DEPTH, = 0+
		int idx = -1;

/*		TNodeIndex q; */
		if (l_p == d) {
/*			q = l; */
			idx = _path_idx_l - 1;
		}
		else {
/*			q = l_p; */
			idx = _path_idx_l - 2;
		}
/*		while(q != -1) { */
		while(idx >= 0) {
			TNodeIndex q = _a_path_delete[idx];
			_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
/*			q = _Tree(q,parent); */
			idx--;
		}

		// балансировка на "обратном отходе"

		if (l_l != -1) {
/*			q = l_l; */
			idx = _path_idx_l;
		}
		else {
/*			q = l_p; */
			idx = _path_idx_l - 2;
		}
/*		while(q != -1) { */
		while(idx >= 0) {
			TNodeIndex q = _a_path_delete[idx];
			TNodeIndex q_p = (idx > 0) ? _a_path_delete[idx-1] : -1;
			SBT_Maintain(q, q_p);
/*			q = _Tree(q,parent); */
			idx--;
		}

		SBT_FreeNode(d);

	}

	else {

		// найти r = SBT_FindNode_NearestAndGreater_ByIndex(d)
/*		TNodeIndex r = SBT_FindNode_NearestAndGreater_ByIndex(d); // d.right -> left */
//		printf("[2] r = SBT_FindNode_NearestAndGreater_ByIndex(d)\n");

		_path_idx = _path_idx_d; // последний элемент списка
		_path_idx++; // переходим на свободные элементы

		// d != -1
		TNodeIndex right = _Tree(d,right);
//		printf("right = %lld\n", (long long int)right);

		_a_path_delete[_path_idx] = right;
		_path_idx++;
//		printf("[2.1]\n");


		TNodeIndex r = -1; // d.left to right
		TNodeIndex r_p = -1;
		if (right != -1) {
			TNodeIndex left_parent = d;
			TNodeIndex left = right;
			while (_Tree(left,left) != -1) {
				left_parent = left;
				left = _Tree(left,left);
				if (left != -1) {
					_a_path_delete[_path_idx] = left;
					_path_idx++;
				}
			}
			r_p = left_parent; // if parent != t[value]
			r = left;
		}
		int _path_idx_r = _path_idx;

//		printf("[2.2]\n");

		// (Diagram No.3)
		if (r == -1) {

//			printf("[3] (r == -1)\n");

			// вершина d является листом
			// меняем ссылку на корень
			if (d_p == -1) {
//				printf("_tree_root = -1;\n");
				_tree_root = -1;
			}
			else {
				if (_Tree(d_p,left) == d) _Tree(d_p,left) = -1;
				else _Tree(d_p,right) = -1;
			}

//			printf("[3.1]\n");
			// можно не делать: _Tree(d,parent) = -1;
			// больше ничего делать не надо
			int idx = -1;
/*			TNodeIndex q = d_p; */
			idx = _path_idx_d - 1;
/*			while(q != -1) { */
			while(idx >= 0) {
				TNodeIndex q = _a_path_delete[idx];
				_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
				idx--;
/*				q = _Tree(q,parent); */
			}

//			printf("[3.2]\n");
/*			q = d_p; */
//printf("idx = %d\n", idx);
			idx = _path_idx_d - 1;
/*			while(q != -1) { */
			while(idx >= 0) {
				TNodeIndex q = _a_path_delete[idx];
				TNodeIndex q_p = (idx > 0) ? _a_path_delete[idx-1] : -1;
				SBT_Maintain(q, q_p);
				idx--;
/*				q = _Tree(q,parent); */
			}

//			printf("[3.3]\n");
			SBT_FreeNode(d);

		}
		// r != -1 (Diagram No.2)
		else {
//			printf("[4] (r != -1)\n");

			// меняем справа
/*			TNodeIndex r_p = _Tree(r,parent); */
			TNodeIndex r_r = _Tree(r,right); // r_l = r.left == -1
			_a_path_delete[_path_idx] = r_r; // последний элемент
			_path_idx++; // опять на свободный конец
			TNodeIndex d_l = _Tree(d,left); // == -1 всегда?

			// меняем левую часть r <-> d_l
			_Tree(r,left) = d_l;
/*			if (d_l != -1) _Tree(d_l,parent) = r;*/

			if (r_p == d) { // правую часть менять не надо, r_r
			}
			else {
				TNodeIndex d_r = _Tree(d,right); // != -1 всегда?
				// меняем верхнюю часть
				_Tree(r_p,left) = r_r;
/*				_Tree(r_r,parent) = r_p; */
				// меняем правую часть r <-> d_r
				_Tree(r,right) = d_r;
/*				_Tree(d_r,parent) = r; */
			}
			
			// меняем ссылку на корень
			if (d_p == -1){
				// меняем l <-> root = t_d_p = -1 (1)
//				printf("_tree_root = r;\n");
				_tree_root = r;
/*				_Tree(r,parent) = -1; */
			}
			else {
				// меняем l <-> root = t_d_p != -1
				if(_Tree(d_p,right) == d) _Tree(d_p,right) = r;
				else _Tree(d_p,left) = r;
/*				_Tree(r,parent) = d_p; */
			}

			int idx = -1;
/*			TNodeIndex q; */
			if (r_p == d) {
/*				q = r; */
				idx = _path_idx_r - 1;
			}
			else {
/*				q = r_p; */
				idx = _path_idx_r - 2;
			}
/*			while(q != -1) { */
			while(idx >= 0) {
				TNodeIndex q = _a_path_delete[idx];
				_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
/*				q = _Tree(q,parent); */
				idx--;
			}


			if (r_r != -1) {
				q = r_r;
				idx = _path_idx_r;
			}
			else {
				q = r_p;
				idx = _path_idx_r - 2;
			}
/*			while(q != -1) { */
			while(idx >= 0) {
				TNodeIndex q = _a_path_delete[idx];
				TNodeIndex q_p = (idx > 0) ? _a_path_delete[idx-1] : -1;
				SBT_Maintain(q, q_p);
/*				q = _Tree(q,parent); */
				idx--;
			}

			SBT_FreeNode(d);

		}
	}

	return d;
}

// Удалить все вершины с данным значением (value)

int SBT_DeleteAllNodes(TNumber value) {
	int result = -1;
//	while ((result = SBT_DeleteNode_At(value, _tree_root, -1)) != -1);
	while ((result = SBT_DeleteNode(value)) != -1);
	return result;
}

// Напечатать вершины в поддереве t

void SBT_PrintAllNodes_At(int depth, TNodeIndex t) {

	if ((_n_nodes <= 0) || (t < 0)) {
		return; // выйти, если вершины нет
	}

	// сверху - большие вершины
	if (_Tree(t,right) >= 0) SBT_PrintAllNodes_At(depth + 1, _Tree(t,right));

/*	if (!((_Tree(t,parent) == -1) && (_Tree(t,left) == -1) && (_Tree(t,right) == -1)) || (t == _tree_root)) { */
//	if (!((_Tree(t,left) == -1) && (_Tree(t,right) == -1)) || (t == _tree_root)) {
		for (int i = 0; i < depth; i++) printf(" "); // отступ
		printf("+%d, id = %lld, value = %lld, size = %lld\n",
			depth,
			(long long int)t,
			(long long int)_Tree(t,value),
			(long long int)_Tree(t,size)
		); // иначе: напечатать "тело" узла
//	}

	// снизу - меньшие
	if (_Tree(t,left) >= 0) SBT_PrintAllNodes_At(depth + 1, _Tree(t,left));
}

// Напечатать все вершины, начиная от корня дерева

void SBT_PrintAllNodes() {
	printf("n_nodes = %lld; _tree_root = %lld\n",
		(long long int)_n_nodes,
		(long long int)_tree_root
	);
	SBT_PrintAllNodes_At(0, _tree_root);
}

// Пройтись с вершины t (по поддереву)

void SBT_WalkAllNodes_At(int depth, TNodeIndex t) {

	if ((_n_nodes <= 0) || (t < 0)) {
		funcOnWalk(t, -1, "WALK_UP");
		return; // выйти, если вершины нет
	}

	// снизу - меньшие
	if (_Tree(t,left) >= 0) {
		funcOnWalk(t, _Tree(t,left), "WALK_DOWN_LEFT");
		SBT_WalkAllNodes_At(depth + 1, _Tree(t,left));
	}

	funcOnWalk(t, t, "WALK_NODE");

	// сверху - большие вершины
	if (_Tree(t,right) >= 0) {
		funcOnWalk(t, _Tree(t,right), "WALK_DOWN_RIGHT");
		SBT_WalkAllNodes_At(depth + 1, _Tree(t,right));
	}

	funcOnWalk(t, -1, "WALK_UP");

}

// Пройтись по всем вершинам, сгенерировать события WALK_ (посылаются в зарегистрированные перед этим обработчики событий)

void SBT_WalkAllNodes() {
	funcOnWalk(-1, -1, "WALK_STARTS");
	SBT_WalkAllNodes_At(0, _tree_root);
	funcOnWalk(-1, -1, "WALK_FINISH");
}

// Найти значение value в поддереве t (корректно при использовании AddNodeUniq)

TNodeIndex SBT_FindNode_At(TNumber value, TNodeIndex t) {

	if ((_n_nodes <= 0) || (t < 0)) {
		return -1; // ответ: "Не найден"
	}

	if (value == _Tree(t,value)) {
		// Среагировать на найденный элемент, вернуть индекс этой ноды
		return t;
	}
	else if (value < _Tree(t,value)) {
		// влево
		return SBT_FindNode_At(value, _Tree(t,left));
	}
	// можно не делать это сравнение для целых чисел
	else 
	// if (value > _Tree(t,value))
	{
		// вправо
		return SBT_FindNode_At(value, _Tree(t,right));
	}

	// не выполняется
	return -1; // "не найден"
}

// Найти вершину от корня

TNodeIndex SBT_FindNode(TNumber value) {
	if (_n_nodes <= 0) return -1;
	return SBT_FindNode_At(value, _tree_root);
}

// (не реализовано)

void SBT_FindAllNodes_At(TNumber value, TNodeIndex t) {
	return;
}

// (не реализовано)

void SBT_FindAllNodes(TNumber value) {
	return SBT_FindAllNodes_At(value, _tree_root);
}


// Проверка дерева на SBT-корректность, начиная с ноды t

void SBT_CheckAllNodesBalance_At(int depth, TNodeIndex t) {

	if ((_n_nodes <= 0) || (t < 0)) {
		return; // выйти, если вершины нет
	}

	// снизу - меньшие
	if (_Tree(t,left) >= 0) {
		SBT_CheckAllNodesBalance_At(depth + 1, _Tree(t,left));
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
	
	// сверху - большие вершины
	if (_Tree(t,right) >= 0) {
		SBT_CheckAllNodesBalance_At(depth + 1, _Tree(t,right));
	}

}

// Проверить всё дерево на SBT-корректность (начиная с корневой ноды)

void SBT_CheckAllNodesBalance() {
	SBT_CheckAllNodesBalance_At(0, _tree_root);
}

// Проверка дерева на SBT-корректность (sizes), начиная с ноды t

void SBT_CheckAllNodesSize_At(int depth, TNodeIndex t) {

	if ((_n_nodes <= 0) || (t < 0)) {
		return; // выйти, если вершины нет
	}

	// снизу - меньшие
	if (_Tree(t,left) >= 0) {
		SBT_CheckAllNodesSize_At(depth + 1, _Tree(t,left));
	}

	// проверить
	if (_Tree(t,size) !=  SBT_Left_size(t) + SBT_Right_size(t) + 1) {
		printf("size error: idx = %lld (size: %lld <> %lld + %lld + 1)\n",
			(long long int)t,
			(long long int)_Tree(t,size),
			(long long int)SBT_Left_size(t),
			(long long int)SBT_Right_size(t)
		);
		printf("----\n");
		SBT_PrintAllNodes();
		printf("----\n");
	}

	// сверху - большие вершины
	if (_Tree(t,right) >= 0) {
		SBT_CheckAllNodesSize_At(depth + 1, _Tree(t,right));
	}

}

// Проверить всё дерево на SBT-корректность (sizes) (начиная с корневой ноды)

void SBT_CheckAllNodesSize() {
	SBT_CheckAllNodesSize_At(0, _tree_root);
}

void SBT_CheckAllNodes() {
	SBT_CheckAllNodesBalance();
	SBT_CheckAllNodesSize();
}

// распечатка WORK- и UNUSED- nodes (всё, что до CLEAN-)

void SBT_DumpAllNodes() {
	for (uint64_t i = 0; i < SBT_MAX_NODES - _n_clean; i++) {
/*		printf("idx = %lld, value = %lld [unused = %d], left = %lld, right = %lld, parent = %lld, size = %lld\n", */
		printf("idx = %lld, value = %lld [unused = %d], left = %lld, right = %lld, size = %lld\n",
			(long long int)i,
			(long long int)_Tree(i,value),
			(int)_Tree(i,unused),
			(long long int)_Tree(i,left),
			(long long int)_Tree(i,right),
/*			(long long int)_Tree(i,parent), */
			(long long int)_Tree(i,size)
		);
	}
}

/* to replace,
TNode *GetPointerToNode(TNodeIndex t) {
	return &(_Tree(t]);
}
*/

TNodeIndex GetRootIndex() {
	return _tree_root;
}


// Найти самый близкий по значению элемент в "левом" поддереве, by Index

/* необходимо полностью переписать без parent, встроить код в функцию DeleteNode() */

TNodeIndex SBT_FindNode_NearestAndLesser_ByIndex(TNodeIndex t) {
	if (t != -1) {
		TNodeIndex left = _Tree(t,left);
		if (left == -1) {
			t = -1;
		}
		else {
			TNodeIndex parent = t;
			TNodeIndex right = left;
			while (right != -1) {
				parent = right;
				right = _Tree(right,right);
			}
			t = parent; // if parent != t[value]
		}
	}
	// else not found, = -1
	return t;
}

// Найти самый близкий по значению элемент в "правом" поддереве, by Index

TNodeIndex SBT_FindNode_NearestAndGreater_ByIndex(TNodeIndex t) {
	if (t != -1) {
		TNodeIndex right = _Tree(t,right);
		if (right == -1) {
			t = -1;
		}
		else {
			TNodeIndex parent = t;
			TNodeIndex left = right;
			while (left != -1) {
				parent = left;
				left = _Tree(left,left);
			}
			t = parent; // if parent != t[value]
		}
	}
	return t;
}

// Найти самый близкий по значению элемент в "левом" поддереве

TNodeIndex SBT_FindNode_NearestAndLesser_ByValue(TNumber value) {
	return SBT_FindNode_NearestAndLesser_ByIndex(SBT_FindNode(value));
}

// Найти самый близкий по значению элемент в "правом" поддереве

TNodeIndex SBT_FindNode_NearestAndGreater_ByValue(TNumber value) {
	return SBT_FindNode_NearestAndGreater_ByIndex(SBT_FindNode(value));
}

TNodeIndex SBT_FindNextUsedNode(TNodeIndex s) {
	int f = 0;
	long long int t = 0;
	// ищем от указанной позиции
	for(t = s; t < SBT_MAX_NODES - _n_clean; t++)
	if (_Tree(t,unused) == 0) { f = 1; break; }

	// если ещё не найдена использующаяся ячейка:
	if (!f) {
		// ищем от начала
		for(t = 0; t < s; t++)
			if (_Tree(t,unused) == 0) { f = 1; break; }
	}
	if (f) return t;
	else return -1;
}

// INT64_MAX, если не можем найти элемент по индексу

TNumber GetValueByIndex(TNodeIndex t) {
	return (t != 1) ? _Tree(t,value) : INT64_MAX;
}

