#ifndef __SBT_H__
#define __SBT_H__

#define SBT_MAX_NODES 10000000
// C language does not mean constant, const int SBT_MAX_NODES = 1000000; // 1 млн. узлов
// см. http://compgroups.net/comp.lang.c/Explanation-needed-for-const-int-error-variably-modified-at-file-scope

#include <stdint.h>

typedef int64_t TNumber;
typedef int64_t TNodeIndex;
typedef uint64_t TNodeSize;

typedef struct TNode {
	TNumber value; // значение, привязанное к ноде
	TNodeIndex parent; // ссылка на уровень выше
	TNodeIndex left;  // ссылка на левое поддерево, = -1, если нет дочерних вершин
	TNodeIndex right; // ссылка на правое поддерево
	TNodeSize size; // size в понимании SBT
	int unused; // «удалённая»; это поле можно использовать и для других флагов
} TNode;

// Разделяем массив структур на массив отдельных полей

// типы функций - обработчиков событий

typedef int (*FuncOnRotate)(TNodeIndex nodeIndex1, TNodeIndex nodeIndex2, const char *stringAction); // LEFT_ROTATE, RIGHT_ROTATE
typedef int (*FuncOnWalk)(TNodeIndex nodeIndex1, TNodeIndex nodeIndex2, const char *stringAction); // WALK_DOWN, WALK_UP, WALK_NODE
typedef int (*FuncOnFind)(TNodeIndex nodeIndex, const char *stringAction); // FOUND

// для оповещения о событиях

int SBT_SetCallback_OnRotate(FuncOnRotate func_);
int SBT_SetCallback_OnWalk(FuncOnWalk func_);
int SBT_SetCallback_OnFind(FuncOnFind func_);

// Основные функции

int SBT_AddNode(TNumber value); // для неуникального ключа
int SBT_AddNodeUniq(TNumber value);
int SBT_DeleteNode(TNumber value); // -1, если нет такого узла в дереве
int SBT_DeleteAllNodes(TNumber value); // для неуникального ключа

// Print, Dump & Check

void SBT_CheckAllNodesBalance_At(int depth, TNodeIndex t);
void SBT_CheckAllNodesBalance();

void SBT_CheckAllNodesSize_At(int depth, TNodeIndex t);
void SBT_CheckAllNodesSize();

void SBT_CheckAllNodes(); // balance + size

void SBT_PrintAllNodes_At(int depth, TNodeIndex t);
void SBT_PrintAllNodes();

void SBT_DumpAllNodes();

// Search & Walk

TNodeIndex SBT_FindNode(TNumber value);
void SBT_FindAllNodes(TNumber value);
void SBT_WalkAllNodes();

TNodeIndex SBT_FindNode_NearestAndLesser_ByIndex(TNodeIndex t);
TNodeIndex SBT_FindNode_NearestAndGreater_ByIndex(TNodeIndex t);

TNodeIndex SBT_FindNode_NearestAndLesser_ByValue(TNumber value);
TNodeIndex SBT_FindNode_NearestAndGreater_ByValue(TNumber value);

TNodeIndex SBT_FindNextUsedNode(TNodeIndex s);

// Get

// to replace, TNode *GetPointerToNode(TNodeIndex t);

TNodeIndex GetRootIndex();
TNumber GetValueByIndex(TNodeIndex t);

#endif
