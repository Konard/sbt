#ifndef __SBT_H__
#define __SBT_H__

#include <stdint.h>

typedef int64_t TValue;
typedef int64_t TNodeIndex;
typedef uint64_t TNodeSize;

// Обработчики событий

typedef int8_t (*FuncOnRotate)(TNodeIndex nodeIndex1, TNodeIndex nodeIndex2, const char *stringAction); // LEFT_ROTATE, RIGHT_ROTATE
typedef int8_t (*FuncOnWalk)(TNodeIndex nodeIndex1, TNodeIndex nodeIndex2, const char *stringAction); // WALK_DOWN, WALK_UP, WALK_NODE
typedef int8_t (*FuncOnFind)(TNodeIndex nodeIndex, const char *stringAction); // FOUND

// Установка оповещений о событиях

int8_t SBT_SetCallback_OnRotate(FuncOnRotate func_);
int8_t SBT_SetCallback_OnWalk(FuncOnWalk func_);
int8_t SBT_SetCallback_OnFind(FuncOnFind func_);

// Объявление в .h для внутреннего использования и экспериментов
int8_t SBT_LeftRotate(TNodeIndex t);
int8_t SBT_RightRotate(TNodeIndex t);

// Основные функции

int8_t SBT_AddNode(TValue value); // для неуникального ключа
int8_t SBT_AddNodeUniq(TValue value);
int8_t SBT_DeleteNode(TValue value); // -1, если нет такого узла в дереве
int8_t SBT_DeleteAllNodes(TValue value); // для неуникального ключа

TNodeIndex SBT_AllocateNode();
int8_t SBT_FreeNode(TNodeIndex t); // -1, если не удается удалить ... (в данной реализации - всегда = 0)

// Print, Dump & Check

void SBT_CheckAllNodesBalance_At(int8_t depth, TNodeIndex t);
void SBT_CheckAllNodesBalance();

void SBT_CheckAllNodesSize_At(int8_t depth, TNodeIndex t);
void SBT_CheckAllNodesSize();

void SBT_CheckAllNodes(); // balance + size

void SBT_PrintAllNodes_At(int8_t depth, TNodeIndex t);
void SBT_PrintAllNodes();

void SBT_DumpAllNodes();

// Search & Walk

TNodeIndex SBT_FindNode_At(TValue value, TNodeIndex t);
TNodeIndex SBT_FindNode(TValue value);

void SBT_FindAllNodes_At(TValue value, TNodeIndex t);
void SBT_FindAllNodes(TValue value);

void SBT_WalkAllNodes_At(int8_t depth, TNodeIndex t);
void SBT_WalkAllNodes();

TNodeIndex SBT_FindNode_NearestAndLesser_ByIndex(TNodeIndex t);
TNodeIndex SBT_FindNode_NearestAndGreater_ByIndex(TNodeIndex t);

TNodeIndex SBT_FindNode_NearestAndLesser_ByValue(TValue value);
TNodeIndex SBT_FindNode_NearestAndGreater_ByValue(TValue value);

TNodeIndex SBT_FindNextUsedNode(TNodeIndex s);

TNodeIndex GetRootIndex();
TValue GetValueByIndex(TNodeIndex t);

void SBT_Initialise();
void SBT_Deinitialise();

#endif
