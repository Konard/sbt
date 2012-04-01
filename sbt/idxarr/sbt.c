
#include "sbt.h"

#include <stdio.h> // printf

#ifdef __LINUX__
#include <pthread.h> // pthread_mutex_*
// узкое место при доступе из многих потоков - глобальная блокировка таблицы _nodes
pthread_mutex_t _lock_nodes = PTHREAD_MUTEX_INITIALIZER;
#endif

// переменные модуля

/* to replace, TNode _nodes[SBT_MAX_NODES];
typedef struct TNode {
	TNumber value; // значение, привязанное к ноде
	TNodeIndex parent; // ссылка на уровень выше
	TNodeIndex left;  // ссылка на левое поддерево, = -1, если нет дочерних вершин
	TNodeIndex right; // ссылка на правое поддерево
	TNodeSize size; // size в понимании SBT
	int unused; // «удалённая»; это поле можно использовать и для других флагов
} TNode;
*/

TNumber _a_value[SBT_MAX_NODES]; // значение, привязанное к ноде
TNodeIndex _a_parent[SBT_MAX_NODES]; // ссылка на уровень выше
TNodeIndex _a_left[SBT_MAX_NODES];  // ссылка на левое поддерево, = -1, если нет дочерних вершин
TNodeIndex _a_right[SBT_MAX_NODES]; // ссылка на правое поддерево
TNodeSize _a_size[SBT_MAX_NODES]; // size в понимании SBT
int _a_unused[SBT_MAX_NODES]; // «удалённая»; это поле можно использовать и для других флагов

#define SBT_ARRAYS
#ifdef SBT_ARRAYS
#define _Tree(idx,kind) _a_##kind[idx]
#else
#define _Tree(idx,kind) _nodes[idx].##kind
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


// Функции Rotate, Maintain & Add, Delete //

// Вращение влево, t - слева, перевешиваем туда (вершины не пропадают, _n_nodes сохраняет значение)

int SBT_LeftRotate(TNodeIndex t) {

        if (t < 0) return 0;
        TNodeIndex k = _Tree(t,right);
        if (k < 0) return 0;
        TNodeIndex p = _Tree(t,parent);
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

        // меняем трёх предков
        // 1. t.right.parent = t
        // 2. k.parent = t.parent
        // 3. t.parent = k
        if (_Tree(t,right) != -1) _Tree(_Tree(t,right),parent) = t; // из кэша
        _Tree(k,parent) = p;
        _Tree(t,parent) = k;

        // меняем корень, parent -> t, k
        if (p == -1) _tree_root = k; // это root
        else {
                if (_Tree(p,left) == t) _Tree(p,left) = k;
                else _Tree(p,right) = k; // вторую проверку можно не делать
        }
        return 1;
}

// Вращение вправо, t - справа, перевешиваем туда (вершины не пропадают, _n_nodes сохраняет значение)

int SBT_RightRotate(TNodeIndex t) {

	if (t < 0) return 0;
	TNodeIndex k = _Tree(t,left);
	if (k < 0) return 0;
	TNodeIndex p = _Tree(t,parent);
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

	// меняем трёх предков
	// 1. t.left.parent = t
	// 2. k.parent = t.parent
	// 3. t.parent = k
	if (_Tree(t,left) != -1) _Tree(_Tree(t,left),parent) = t;
	_Tree(k,parent) = p;
	_Tree(t,parent) = k;

	// меняем корень, parent -> t, k
	if (p == -1) _tree_root = k; // это root
	else {
		if (_Tree(p,left) == t) _Tree(p,left) = k;
		else _Tree(p,right) = k; // вторую проверку можно не делать
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

int SBT_Maintain_Simpler(TNodeIndex t, int flag) {

	if (t < 0) return 0;
	TNodeIndex parent = _Tree(t,parent); // есть "родитель"
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
			SBT_RightRotate(t);
		}
		else if (SBT_Left_Right_size(t) > SBT_Right_size(t)) {
			SBT_LeftRotate(_Tree(t,left));
			SBT_RightRotate(t);
		}
		else { return 0; }
	}
	// поместили справа, flag == 1
	else {
		if (SBT_Right_Right_size(t) > SBT_Left_size(t)) {
			SBT_LeftRotate(t);
		}
		else if (SBT_Right_Left_size(t) > SBT_Left_size(t)) {
			SBT_RightRotate(_Tree(t,right));
			SBT_LeftRotate(t);
		}
		else { return 0; }
	}

	TNodeIndex t0 = -1;
	if (parent == -1) t0 = _tree_root;
	else {
	    if (at_left) t0 = _Tree(parent,left);
	    else t0 = _Tree(parent,right);
	}
	SBT_Maintain_Simpler(_Tree(t0,left), 0); // false
	SBT_Maintain_Simpler(_Tree(t0,right), 1); // true
	SBT_Maintain_Simpler(t0, 0); // false
	SBT_Maintain_Simpler(t0, 1); // true

	return 0;
}

// Сбалансировать дерево ("тупой" алгоритм)

int SBT_Maintain(TNodeIndex t) {

	if (t < 0) return 0;

	TNodeIndex parent = _Tree(t,parent); // есть "родитель"
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
		SBT_RightRotate(t);
		CALC_T0
		SBT_Maintain(_Tree(t0,right));
		SBT_Maintain(t0);
	}
	else if (SBT_Left_Right_size(t) > SBT_Right_size(t)) {
		SBT_LeftRotate(_Tree(t,left));
		SBT_RightRotate(t);
		CALC_T0
		SBT_Maintain(_Tree(t0,left));
		SBT_Maintain(_Tree(t0,right));
		SBT_Maintain(t0);
	}
	// поместили справа (?)
	else if (SBT_Right_Right_size(t) > SBT_Left_size(t)) {
		SBT_LeftRotate(t);
		CALC_T0
		SBT_Maintain(_Tree(t0,left));
		SBT_Maintain(t0);
	}
	else if (SBT_Right_Left_size(t) > SBT_Left_size(t)) {
		SBT_RightRotate(_Tree(t,right));
		SBT_LeftRotate(t);
		CALC_T0
		SBT_Maintain(_Tree(t0,left));
		SBT_Maintain(_Tree(t0,right));
		SBT_Maintain(t0);
	}

	return 0;
}

// Добавить вершину в поддерево t, без проверки уникальности (куда - t, родительская - parent)

int SBT_AddNode_At(TNumber value, TNodeIndex t, TNodeIndex parent) {
	_Tree(t,size)++;
	if (_n_nodes <= 0) {
		TNodeIndex t_new = SBT_AllocateNode();
		_Tree(t_new,value) = value;
		_Tree(t_new,parent) = parent;
		_Tree(t_new,left) = -1;
		_Tree(t_new,right) = -1;
		_Tree(t_new,size) = 1;
		_tree_root = 0;
	}
	else {
		if(value < _Tree(t,value)) {
			if(_Tree(t,left) == -1) {
				TNodeIndex t_new = SBT_AllocateNode();
				_Tree(t,left) = t_new;
				_Tree(t_new,value) = value;
				_Tree(t_new,value) = value;
				_Tree(t_new,parent) = t;
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
				_Tree(t_new,parent) = t;
				_Tree(t_new,left) = -1;
				_Tree(t_new,right) = -1;
				_Tree(t_new,size) = 1;
			}
			else {
				SBT_AddNode_At(value, _Tree(t,right), t);
			}
		}
	}
	//SBT_Maintain(t);
	SBT_Maintain_Simpler(t, (value >= _Tree(t,value)) ? 1 : 0);
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

int SBT_DeleteNode_At(TNumber value, TNodeIndex t, TNodeIndex parent) {
	if ((_n_nodes <= 0) || (t < 0)) {
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
//	if ((l != -1) && (SBT_Left_size(d) >= SBT_Right_size(d))) {
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
			_tree_root = l;
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


		if (l_l != -1) q = l_l;
		else q = l_p;
		while(q != -1) {
			//_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
			SBT_Maintain(q);
			q = _Tree(q,parent);
		}

		SBT_FreeNode(d);

	}

	else {
		TNodeIndex r = SBT_FindNode_NearestAndGreater_ByIndex(d); // d.right -> left
		// (Diagram No.3)
		if (r == -1) {
			// вершина d является листом
			// меняем ссылку на корень
			if (d_p == -1) _tree_root = -1;
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

			q = d_p;
			while(q != -1) {
				SBT_Maintain(q);
				q = _Tree(q,parent);
			}

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
				_tree_root = r;
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


			if (r_r != -1) q = r_r;
			else q = r_p;
			while(q != -1) {
				SBT_Maintain(q);
				q = _Tree(q,parent);
			}

			SBT_FreeNode(d);

		}
	}

	return d;
}


// Удалить первую попавшуюся вершину по значению value

int SBT_DeleteNode(TNumber value) {

	TNodeIndex t = SBT_DeleteNode_At(value, _tree_root, -1);

	return t;
}

// Удалить все вершины с данным значением (value)

int SBT_DeleteAllNodes(TNumber value) {
	int result = -1;
	while ((result = SBT_DeleteNode_At(value, _tree_root, -1)) != -1);
	return result;
}

// Напечатать вершины в поддереве t

void SBT_PrintAllNodes_At(int depth, TNodeIndex t) {

	if ((_n_nodes <= 0) || (t < 0)) {
		return; // выйти, если вершины нет
	}

	// сверху - большие вершины
	if (_Tree(t,right) >= 0) SBT_PrintAllNodes_At(depth + 1, _Tree(t,right));

	if (!((_Tree(t,parent) == -1) && (_Tree(t,left) == -1) && (_Tree(t,right) == -1)) || (t == _tree_root)) {
		for (int i = 0; i < depth; i++) printf(" "); // отступ
		printf("+%d, id = %lld, value = %lld, size = %lld\n",
			depth,
			(long long int)t,
			(long long int)_Tree(t,value),
			(long long int)_Tree(t,size)
		); // иначе: напечатать "тело" узла
	}

	// снизу - меньшие
	if (_Tree(t,left) >= 0) SBT_PrintAllNodes_At(depth + 1, _Tree(t,left));
}

// Напечатать все вершины, начиная от корня дерева

void SBT_PrintAllNodes() {
	printf("n_nodes = %lld\n", (long long int)_n_nodes);
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
		printf("idx = %lld, value = %lld [unused = %d], left = %lld, right = %lld, parent = %lld, size = %lld\n",
			(long long int)i,
			(long long int)_Tree(i,value),
			(int)_Tree(i,unused),
			(long long int)_Tree(i,left),
			(long long int)_Tree(i,right),
			(long long int)_Tree(i,parent),
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
	_Tree(t,parent) = -1;
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
	_Tree(t,parent) = -1;
	_Tree(t,size) = 0;
	_Tree(t,value) = 0;
	_Tree(t,unused) = 1;
	// счетчика _n_unused нет
	_n_nodes--; // условный счетчик (вспомогательный): эти вершины можно пересчитать в WORK-пространстве

	return 1;
}

// Найти самый близкий по значению элемент в "левом" поддереве, by Index

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
