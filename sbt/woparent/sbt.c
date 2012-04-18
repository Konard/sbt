
/*
 * Само-балансирующееся по размеру дерево : Size-balanced tree
 * документация по проекту преимущественно на русском языке
 * 
 * Лицензия: CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/legalcode
 * перевод лицензии на русский язык: http://wiki.creativecommons.org/images/6/61/CC0_rus.pdf
 */

#include "sbt.h"
#include <stdio.h> // printf

// внутренние переменные модуля

#ifdef SBT_ARRAYS
  
  // раздельными массивами
  
TNumber _a_value[SBT_MAX_NODES]; // значение, привязанное к ноде
TNodeIndex _a_left[SBT_MAX_NODES];  // ссылка на левое поддерево, = -1, если нет дочерних вершин
TNodeIndex _a_right[SBT_MAX_NODES]; // ссылка на правое поддерево
TNodeSize _a_size[SBT_MAX_NODES]; // size в понимании SBT
int _a_unused[SBT_MAX_NODES]; // «удалённая»; это поле можно использовать и для других флагов
#else

  // одним массивом
  
#define _Tree(idx,kind) _nodes[idx].kind
TNode _nodes[SBT_MAX_NODES];
#endif

/*
typedef struct TNode {
	TNumber value; // значение, привязанное к ноде
	TNodeIndex left;  // ссылка на левое поддерево, = -1, если нет дочерних вершин
	TNodeIndex right; // ссылка на правое поддерево
	TNodeSize size; // size в понимании SBT
	int unused; // «удалённая»; это поле можно использовать и для других флагов
} TNode;
*/


// функция-макрос для доступа к массиву

#ifdef SBT_ARRAYS
#define _Tree(idx,kind) _a_##kind[idx]
#else
#define _Tree(idx,kind) _nodes[idx].kind
#endif

TNodeIndex _n_nodes = 0; // число вершин в дереве
TNodeIndex _tree_root = -1; // дерево, использованные ноды
TNodeIndex _tree_unused = -1; // список неиспользованных нодов
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

/*
  Функции memory management - эти функции ничего не должны знать про _структуру_ дерева,
  значения left/right и т.п. они используют лишь для выстраивания элементов в список.
*/

// AllocateNode() - выделение памяти из кольцевого FIFO-буфера (UNUSED-нодов), иначе - из массива CLEAN-нодов.

TNodeIndex SBT_AllocateNode() {

	TNodeIndex t = -1; // нет ноды

	// выделить ноду из UNUSED-области (из кольцевого списка), если они есть
	if (_tree_unused != -1) {
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
			// этот приём сработает даже в случае, если в списке останется всего-лишь один элемент
		}
	}

	// выделить из CLEAN-области (чистые ячейки), если они остались
	else {
		if (_n_clean > 0) {
			
			t = SBT_MAX_NODES - _n_clean;
			_n_clean--;
		}
		else {
			// память закончилась, t = -1
			return t;
		}
	}

	// дополнительная очистка
	_Tree(t,left) = -1;
	_Tree(t,right) = -1;
	_Tree(t,size) = 0;
	_Tree(t,value) = 0;
	_Tree(t,unused) = 0;
	// счетчика _n_unused нет (но можно добавить)
	_n_nodes++; // можно увеличивать счетчик занятых нодов вне функции Allocate

	return t; // результат (нода)
}

// FreeNode - перемещение ноды в "кольцевой буфер"
// в плане методики, FreeNode не знает ничего о структуре дерева, она работает с ячейками, управляя только ссылками left и right
// ( небезопасная функция! следите за целостностью дерева самостоятельно! )

int SBT_FreeNode(TNodeIndex t) {

	if (_n_nodes <= 0) return 0; // нет ни одного занятого нода - нечего удалять
	if (t < 0) return 0; // некорректный индекс нода
	if (_Tree(t,unused) == 1) return 0; // нода уже удалена

	// UNUSED пуст, сделать первым элементом в UNUSED-пространстве
	if (_tree_unused == -1) {
		_Tree(t,left) = t;
		_Tree(t,right) = t;
		_tree_unused = t;
	}

	// UNUSED уже частично заполнен, добавить в UNUSED-пространство
	else {
		TNodeIndex last = _Tree(_tree_unused,left); // так как существует root-вершина (first), то и last <> -1
		// ссылки внутри
		_Tree(t,left) = last;
		_Tree(last,right) = t;
		// ссылки снаружи
		_Tree(_tree_unused,left) = t;
		_Tree(t,right) = _tree_unused;
		// _tree_unused не изменяется,
		// поэтому в следующий раз, при Allocate, возьмется вершина _tree_unused,
		// FIFO-список
	}
	_Tree(t,size) = 0;
	_Tree(t,value) = 0;
	_Tree(t,unused) = 1;
	// счетчика _n_unused нет
	_n_nodes--; // вспомогательный счетчик: эти вершины можно пересчитать в WORK-пространстве

	return 1;
}

/*
 * Размеры для вершин, size
 */

TNodeSize SBT_Left_Left_size(TNodeIndex t) {
	if (t < 0) return 0;
	TNodeIndex l = _Tree(t,left);
	if (l < 0) return 0;
	TNodeIndex ll = _Tree(l,left);
	return ((ll < 0) ? 0 : _Tree(ll,size));
}

TNodeSize SBT_Left_Right_size(TNodeIndex t) {
	if (t < 0) return 0;
	TNodeIndex l = _Tree(t,left);
	if (l < 0) return 0;
	TNodeIndex lr = _Tree(l,right);
	return ((lr < 0) ? 0 : _Tree(lr,size));
}

TNodeSize SBT_Right_Right_size(TNodeIndex t) {
	if (t < 0) return 0;
	TNodeIndex r = _Tree(t,right);
	if (r < 0) return 0;
	TNodeIndex rr = _Tree(r,right);
	return ((rr < 0) ? 0 : _Tree(rr,size));
}

TNodeSize SBT_Right_Left_size(TNodeIndex t) {
	if (t < 0) return 0;
	TNodeIndex r = _Tree(t,right);
	if (r < 0) return 0;
	TNodeIndex rl = _Tree(r,left);
	return ((rl < 0) ? 0 : _Tree(rl,size));
}

TNodeSize SBT_Right_size(TNodeIndex t) {
	if (t < 0) return 0;
	TNodeIndex r = _Tree(t,right);
	return ((r < 0) ? 0 : _Tree(r,size));
}

TNodeSize SBT_Left_size(TNodeIndex t) {
	if (t < 0) return 0;
	TNodeIndex l = _Tree(t,left);
	return ((l < 0) ? 0 : _Tree(l,size));
}

/*
 * Размеры для вершин, size (unsafe-версии функций,
 * без проверки на t < 0)
 * (для оптимизации по скорости)
 */

TNodeSize SBT_Left_Left_size_unsafe(TNodeIndex t) {
	TNodeIndex l = _Tree(t,left);
	if (l < 0) return 0;
	TNodeIndex ll = _Tree(l,left);
	return ((ll < 0) ? 0 : _Tree(ll,size));
}

TNodeSize SBT_Left_Right_size_unsafe(TNodeIndex t) {
	TNodeIndex l = _Tree(t,left);
	if (l < 0) return 0;
	TNodeIndex lr = _Tree(l,right);
	return ((lr < 0) ? 0 : _Tree(lr,size));
}

TNodeSize SBT_Right_Right_size_unsafe(TNodeIndex t) {
	TNodeIndex r = _Tree(t,right);
	if (r < 0) return 0;
	TNodeIndex rr = _Tree(r,right);
	return ((rr < 0) ? 0 : _Tree(rr,size));
}

TNodeSize SBT_Right_Left_size_unsafe(TNodeIndex t) {
	TNodeIndex r = _Tree(t,right);
	if (r < 0) return 0;
	TNodeIndex rl = _Tree(r,left);
	return ((rl < 0) ? 0 : _Tree(rl,size));
}

TNodeSize SBT_Right_size_unsafe(TNodeIndex t) {
	TNodeIndex r = _Tree(t,right);
	return ((r < 0) ? 0 : _Tree(r,size));
}

TNodeSize SBT_Left_size_unsafe(TNodeIndex t) {
	TNodeIndex l = _Tree(t,left);
	return ((l < 0) ? 0 : _Tree(l,size));
}

/*
 * Функции работы с деревом,
 * Rotate, Maintain & Add, Delete
 */

// LeftRotate, Вращение влево - перемещение вершины t влево, (вершины не пропадают, _n_nodes сохраняет значение)

int SBT_LeftRotate(TNodeIndex t, TNodeIndex parent) {

        // проверять на условие _n_nodes > 0 не обязательно
        if (t < 0) return 0;
        TNodeIndex k = _Tree(t,right);
        if (k < 0) return 0;
//	if (funcOnRotate != NULL) funcOnRotate(k, t, "LEFT_ROTATE");

        // поворачиваем ребро дерева
        _Tree(t,right) = _Tree(k,left);
        _Tree(k,left) = t;

        // корректируем size
        _Tree(k,size) = _Tree(t,size);
        _Tree(t,size) = SBT_Left_size(t) + SBT_Right_size(t) + 1;

        // меняем корень, parent -> t, k
        if (parent == -1) _tree_root = k; // это root
        else {
                if (_Tree(parent,left) == t) _Tree(parent,left) = k;
                else _Tree(parent,right) = k; // вторую проверку можно не делать
        }
        return 1; // нет отказа
}

// Вращение вправо, t - справа, перевешиваем туда (вершины не пропадают, _n_nodes сохраняет значение)

int SBT_RightRotate(TNodeIndex t, TNodeIndex parent) {

	if (t < 0) return 0;
	TNodeIndex k = _Tree(t,left);
	if (k < 0) return 0;
//	if (funcOnRotate != NULL) funcOnRotate(k, t, "RIGHT_ROTATE");

	// поворачиваем ребро дерева
	_Tree(t,left) = _Tree(k,right);
	_Tree(k,right) = t;

	// корректируем size
	_Tree(k,size) = _Tree(t,size);
	_Tree(t,size) = SBT_Left_size(t) + SBT_Right_size(t) + 1;

	// меняем корень, parent -> t, k
	if (parent == -1) _tree_root = k; // это root
	else {
		if (_Tree(parent,left) == t) _Tree(parent,left) = k;
		else _Tree(parent,right) = k; // вторую проверку можно не делать
	}
	return 1; // не отказа
}

// Maintain (Simpler), Сбалансировать дерево (более быстрый алгоритм)

int SBT_Maintain_Simpler(TNodeIndex t, TNodeIndex parent, int flag) {
// если меняются узлы - надо поменять ссылку на них

	if (t < 0) return 0;
	int at_left = 0;
	if (parent == -1) { // t - корень дерева, он изменяется; запоминать нужно не индекс, а "топологию"
	}
	else {
	    if (_Tree(parent,left) == t) at_left = 1; // t - "слева" от родителя; индекс родителя не изменился
	    else at_left = 0; // t - "справа" от родителя
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

// Maintain, Сбалансировать дерево ("тупой" алгоритм)
// возможно, получится написать специализированную версию maintain для DeleteNode

int SBT_Maintain(TNodeIndex t, TNodeIndex parent) {

	if (t < 0) return 0;

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

// AddNode, Добавить вершину в поддерево t, без проверки уникальности (t - куда, parent - родительская)

int SBT_AddNode_At(TNumber value, TNodeIndex t, TNodeIndex parent) {
	if (_n_nodes <= 0) {
		TNodeIndex t_new = SBT_AllocateNode();
		_Tree(t_new,value) = value;
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
				_Tree(t_new,left) = -1;
				_Tree(t_new,right) = -1;
				_Tree(t_new,size) = 1;
			}
			else {
				SBT_AddNode_At(value, _Tree(t,right), t);
			}
		}
	}
	_Tree(t,size)++;
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

    TNodeIndex t = SBT_FindNode(value);
	if (t == -1) {
		SBT_AddNode(value);
	}
	// fail, если вершина с таким value уже существует
	return t;
}

// Удалить вершину, в дереве t
// Алгоритм (взят из статьи про AVL на Википедии) :
// 1. Ищем вершину на удаление,
// 2. Ищем ей замену, спускаясь по левой ветке (value-),
// 3. Перевешиваем замену на место удаленной,
// 4. Выполняем балансировку вверх от родителя места, где находилась замена (от parent замены).

// Удалить первую попавшуюся вершину по значению value
// (функция нерекурсивная)

#define SBT_MAX_DEPTH 256
static TNodeIndex _a_path[SBT_MAX_DEPTH];

int SBT_DeleteNode(TNumber value) {

	if (_n_nodes <= 0) return 0; // ответ: "Не найден"
	if (_tree_root == -1) return 0; // эта проверка не обязательна

	// d = FindNode +++
	int _path_idx = 0; // свободный элемент
	TNodeIndex d = _tree_root; // <> -1, от корня
	TNodeIndex d_p = -1;
	TNumber current = -1;
	do {
		_a_path[_path_idx++] = d;
		current = _Tree(d,value);
		// переходим к следующему элементу (может оказаться q = -1)
		if(value > current) { d_p = d; d = _Tree(d,right);}
		else {
			if(value < current) { d_p = d; d = _Tree(d,left);}
			// остаёмся на найденном элементе
			else { break;}
		}
	} while(d != -1);
	// на выходе current = value, или d = -1 (left/right)
	if (d == -1) return 0; // нечего удалять; d != -1, элемент найден
	int _path_idx_d = _path_idx - 1; // последний элемент списка

	// l = SBT_FindNode_NearestAndLesser_ByIndex(d)
	TNodeIndex d_l = _Tree(d,left);
	if (d_l == -1) {
		TNodeIndex d_r = _Tree(d,right);
		// вершина d является листом
		if (d_r == -1) {
			// меняем ссылку на корень
			if (d_p == -1) {
				_tree_root = -1;
			}
			else {
				if (_Tree(d_p,left) == d) _Tree(d_p,left) = -1;
				else _Tree(d_p,right) = -1;

				int idx = 0;
				idx = _path_idx_d - 1;
				while(idx >= 0) {
					TNodeIndex q = _a_path[idx]; // не должно = -1
					_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
					idx--;
				}
				idx = _path_idx_d - 1;
				while(idx >= 0) {
					TNodeIndex q = _a_path[idx];
					TNodeIndex q_p = (idx > 0) ? _a_path[idx-1] : -1;
					SBT_Maintain(q, q_p);
					idx--;
				}
			}
			SBT_FreeNode(d);
		}
		// справа есть
		else {
			_a_path[_path_idx++] = d_r;
			TNodeIndex r = d_r;
			TNodeIndex r_p = d;
			while (_Tree(r,left) != -1) {
				r_p = r;
				r = _Tree(r,left);
				_a_path[_path_idx++] = r;
			}
			int _path_idx_r = _path_idx - 1;
			// меняем левую часть (r => d)
			TNodeIndex r_r = _Tree(r,right);
			_Tree(r,left) = d_l;
			if (r == d_r) { // правую часть менять не надо, r_r
			}
			else {
				_Tree(r_p,left) = r_r; // меняем верхнюю часть, r_r => r
				_Tree(r,right) = d_r; // меняем правую часть r => d
			}
			
			// меняем ссылку на корень: d на r
			if (d_p == -1) {
				_tree_root = r;
			}
			else {
				if(_Tree(d_p,right) == d) _Tree(d_p,right) = r;
				else _Tree(d_p,left) = r;
			}
			// простановка Sizes
			int idx = _path_idx_r - 1; // начиная с r_p
			while(idx >= 0) {
				TNodeIndex q = _a_path[idx];
				if (q == d) q = r;
				_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
				idx--;
			}
			// балансировка вверх, до корня
			idx = _path_idx_r - 1; // начиная с r_p
			while(idx >= 0) {
				TNodeIndex q = _a_path[idx];
				TNodeIndex q_p = (idx > 0) ? _a_path[idx-1] : -1;
				if (q == d) q = r;
				SBT_Maintain(q, q_p);
				idx--;
			}
			SBT_FreeNode(d);
		}
	}
	// слева есть
	else {
		TNodeIndex d_r = _Tree(d,right);
		_a_path[_path_idx++] = d_l;
		TNodeIndex l = d_l;
		TNodeIndex l_p = d;
		while (_Tree(l,right) != -1) {
			l_p = l;
			l = _Tree(l,right);
			_a_path[_path_idx++] = l;
		}
		int _path_idx_l = _path_idx - 1;
		// меняем правую часть (l => d)
		TNodeIndex l_l = _Tree(l,left);
		_Tree(l,right) = d_r;
		if (l == d_l) { // левую часть менять не надо, l_l
		}
		else {
			_Tree(l_p,right) = l_l; // меняем верхнюю часть, l_l => l
			_Tree(l,left) = d_l; // меняем левую часть l => d
		}
		
		// меняем ссылку на корень: d на l
		if (d_p == -1){
			_tree_root = l;
		}
		else {
			if (_Tree(d_p,left) == d) _Tree(d_p,left) = l;
			else _Tree(d_p,right) = l;
		}
		// простановка Sizes
		int idx = _path_idx_l - 1; // начиная с l_p
		while(idx >= 0) {
			TNodeIndex q = _a_path[idx];
			if (q == d) q = l;
			_Tree(q,size) =  SBT_Left_size(q) + SBT_Right_size(q) + 1;
			idx--;
		}
		// балансировка на "обратном отходе"
		idx = _path_idx_l - 1; // начиная с l_p
		while(idx >= 0) {
			TNodeIndex q = _a_path[idx];
			TNodeIndex q_p = (idx > 0) ? _a_path[idx-1] : -1;
			if (q == d) q = l;
			SBT_Maintain(q, q_p);
			idx--;
		}
		SBT_FreeNode(d);
	}
	return d;
}

// Удалить все вершины с данным значением (value)

int SBT_DeleteAllNodes(TNumber value) {
	TNodeIndex t = -1;
	while ((t = SBT_DeleteNode(value)) != -1);
	return t;
}

// Напечатать вершины в поддереве t

void SBT_PrintAllNodes_At(int depth, TNodeIndex t) {

	if ((_n_nodes <= 0) || (t < 0)) {
		return; // выйти, если вершины нет
	}

	// сверху - большие вершины
	if (_Tree(t,right) >= 0) SBT_PrintAllNodes_At(depth + 1, _Tree(t,right));

	for (int i = 0; i < depth; i++) printf(" "); // отступ
	printf("+%d, id = %lld, value = %lld, size = %lld\n",
		depth,
		(long long int)t,
		(long long int)_Tree(t,value),
		(long long int)_Tree(t,size)
	); // иначе: напечатать "тело" узла

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
		printf("idx = %lld, value = %lld [unused = %d], left = %lld, right = %lld, size = %lld\n",
			(long long int)i,
			(long long int)_Tree(i,value),
			(int)_Tree(i,unused),
			(long long int)_Tree(i,left),
			(long long int)_Tree(i,right),
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
