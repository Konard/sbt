	BITS 64
	SECTION .text

	global SBT_Initialise_Opt
	global SBT_LeftRotate_Opt
	global SBT_RightRotate_Opt

	extern NN
	extern ROOT
	extern aValue
	extern aUT
	extern aLT
	extern aRT
	extern aSize
	extern aFREE
	extern SBT_DumpAllNodes

%macro	PUSHALL 0
	PUSH RAX
	PUSH RBX
	PUSH RCX
	PUSH RDX

	PUSH R8
	PUSH R9
	PUSH R10
	PUSH R11
	PUSH R12
	PUSH R13
	PUSH R14
	PUSH R15
%endmacro

%macro	POPALL 0
	POP R15
	POP R14
	POP R13
	POP R12
	POP R11
	POP R10
	POP R9
	POP R8

	POP RDX
	POP RCX
	POP RBX
	POP RAX
%endmacro

@L3:
	MOV EAX, ECX
	ADD EAX, EDX
	ADD EAX, R8D
	ADD EAX, R9D
	RET

SBT_Initialise_Opt:
;	for (t = 0; t < NN; t++) {
;		aValue[t] = 0;	// значение, привязанное к ноде
;		aU[t] = -1;	// ссылка на уровень выше
;		aL[t] = -1;	// ссылка на левое поддерево, = -1, если нет дочерних вершин
;		aR[t] = -1;	// ссылка на правое поддерево
;		aSize[t] = 0;	// size в понимании SBT
;		aFREE[t] = 0x01;
;	}

	; *** aValue
	MOV RBX,[aValue]
	MOV RCX,[NN] ; сколько, 10 млн.
@LoopValue:
	MOV qword [RBX],0
	ADD RBX,8 ; +QWORD uint64_t
	LOOPNZ @LoopValue

	; *** aUT
	MOV RBX,[aUT]
	MOV RCX,[NN] ; сколько, 10 млн.
@LoopUT:
	MOV qword [RBX],-1
	ADD RBX,8 ; +QWORD uint64_t
	LOOPNZ @LoopUT

	; *** aLT
	MOV RBX,[aLT]
	MOV RCX,[NN] ; сколько, 10 млн.
@LoopLT:
	MOV qword [RBX],-1
	ADD RBX,8 ; +QWORD uint64_t
	LOOPNZ @LoopLT

	; *** aRT
	MOV RBX,[aRT]
	MOV RCX,[NN] ; сколько, 10 млн.
@LoopRT:
	MOV qword [RBX],-1
	ADD RBX,8 ; +QWORD uint64_t
	LOOPNZ @LoopRT

	; *** aSize
	MOV RBX,[aSize]
	MOV RCX,[NN] ; сколько, 10 млн.
@LoopSize:
	MOV qword [RBX],0
	ADD RBX,8 ; +QWORD uint64_t
	LOOPNZ @LoopSize

	; *** aFREE
	MOV RBX,[aFREE]
	MOV RCX,[NN] ; сколько, 10 млн.
@LoopFREE:
	MOV byte [RBX],1
	ADD RBX,1 ; +BYTE uint8_t
	LOOPNZ @LoopFREE

	RET

; цель оптимизации SBTAMD64 в том, чтобы
; достичь скорости вставки 10 млн. вершин за 1 сек,
; т.е. 300 тактов на 3-ГГц ядре процессора на вставку

; -------------------------------------------------------
SBT_LeftRotate_Opt:

	; TNodeIndex t
	CMP RCX,0
	JL @SBT_LeftRotateRet0 ; if (t < 0) return 0;
	MOV R8,[aRT]
	MOV RAX,qword [R8+RCX*8] ; TNodeIndex k = aRT[t];
	CMP RAX,0
	JL @SBT_LeftRotateRet0 ; if (k < 0) return 0;
	MOV R9,[aUT]
	MOV RBX,qword [R9+RCX*8] ; TNodeIndex p = aUT[t];
	; RAX = k;
	; RBX = p;
	; RCX = t;

	; RAX,RBX,RCX
	; R8 = [aRT]
	; R9 = [aUT]
	; R10 = [aLT]

	; поворачиваем ребро дерева
	MOV R10,[aLT]
	MOV RDX,qword [R10+RAX*8]
	MOV qword [R8+RCX*8],RDX ; aRT[t] = aLT[k];
	MOV qword [R10+RAX*8],RCX ; aLT[k] = t;

	; корректируем size
	; R11 = [aSize]
	MOV R11,[aSize]
	MOV RDX,qword [R11+RCX*8] ; ? меньше использовать один регистр
	MOV qword [R11+RAX*8],RDX ; aSize[k] = aSize[t];

	; R12 = n_l
	; R13 = n_r
	; R14 = s_l
	; R15 = s_r -> SUM
	MOV R12,qword [R10+RCX*8] ; TNodeIndex n_l = aLT[t];
	MOV R13,qword [R8+RCX*8] ; TNodeIndex n_r = aRT[t];

	CMP R12,-1
	JNE @SBT_LeftRotateSLN ; (n_l != -1)
	MOV R14,0
	JMP @SBT_LeftRotateSLN0
@SBT_LeftRotateSLN:
	MOV R14,qword [R11+R12*8] ; TNodeSize s_l = ((n_l != -1) ? aSize[n_l] : 0);
@SBT_LeftRotateSLN0:

	CMP R13,-1
	JNE @SBT_LeftRotateSRN ; (n_r != -1)
	MOV R15,0
	JMP @SBT_LeftRotateSRN0
@SBT_LeftRotateSRN:
	MOV R15,qword [R11+R13*8] ; TNodeSize s_r = ((n_r != -1) ? aSize[n_r] : 0);
@SBT_LeftRotateSRN0:

	ADD R15,R14 ; aSize[t] = s_l + s_r + 1;
	INC R15
	MOV qword [R11+RCX*8],R15 ; RDX

	; меняем трёх предков
	; 1. t.right.parent = t
	; 2. k.parent = t.parent
	; 3. t.parent = k

	MOV RDX,qword [R8+RCX*8] ; aRT[t]
	CMP RDX,-1
	JE @SBT_LeftRotateRT1
	MOV qword [R9+RDX*8],RCX ; if (aRT[t] != -1) aUT[aRT[t]] = t;
@SBT_LeftRotateRT1:
	MOV qword [R9+RAX*8],RBX ; aUT[k] = p;
	MOV qword [R9+RCX*8],RAX ; aUT[t] = k;

	; меняем корень, parent -> t, k

	CMP RBX,-1
	JE @SBT_LeftRotateRBX0

	MOV RDX,qword [R10+RBX*8] ; aLT[p]
	CMP RDX,RCX
	JNE @SBT_LeftRotateRTPK

	MOV qword [R10+RBX*8],RAX ; if (aLT[p] == t) aLT[p] = k;
	MOV RAX,1
	RET
@SBT_LeftRotateRTPK:
	MOV qword [R8+RBX*8],RAX ; else aRT[p] = k;
	MOV RAX,1
	RET

@SBT_LeftRotateRBX0:
	MOV RDX,[ROOT]
	MOV qword [RDX],RAX ; if (p == -1) ROOT = k;
	MOV RAX,1
	RET

@SBT_LeftRotateRet0:
	MOV RAX,0
	RET

; -------------------------------------------------------
SBT_RightRotate_Opt:
	; TNodeIndex t
	CMP RCX,0
	JL @SBT_RightRotateRet0 ; if (t < 0) return 0;
	MOV R8,[aLT]
	MOV RAX,qword [R8+RCX*8] ; TNodeIndex k = aLT[t];
	CMP RAX,0
	JL @SBT_RightRotateRet0 ; if (k < 0) return 0;
	MOV R9,[aUT]
	MOV RBX,qword [R9+RCX*8] ; TNodeIndex p = aUT[t];
	; RAX = k;
	; RBX = p;
	; RCX = t;

	; RAX,RBX,RCX
	; R8 = [aRT]  -> [aLT]
	; R9 = [aUT]
	; R10 = [aLT] -> [aRT]

	; поворачиваем ребро дерева
	MOV R10,[aRT]
	MOV RDX,qword [R10+RAX*8]
	MOV qword [R8+RCX*8],RDX ; aLT[t] = aRT[k];
	MOV qword [R10+RAX*8],RCX ; aRT[k] = t;

	; корректируем size
	; R11 = [aSize]
	MOV R11,[aSize]
	MOV RDX,qword [R11+RCX*8] ; ? меньше использовать один регистр
	MOV qword [R11+RAX*8],RDX ; aSize[k] = aSize[t];

	; R12 = n_l
	; R13 = n_r
	; R14 = s_l
	; R15 = s_r -> SUM
	MOV R12,qword [R8+RCX*8] ; TNodeIndex n_l = aLT[t];
	MOV R13,qword [R10+RCX*8] ; TNodeIndex n_r = aRT[t];

	CMP R12,-1
	JNE @SBT_RightRotateSLN ; (n_l != -1)
	MOV R14,0
	JMP @SBT_RightRotateSLN0
@SBT_RightRotateSLN:
	MOV R14,qword [R11+R12*8] ; TNodeSize s_l = ((n_l != -1) ? aSize[n_l] : 0);
@SBT_RightRotateSLN0:

	CMP R13,-1
	JNE @SBT_RightRotateSRN ; (n_r != -1)
	MOV R15,0
	JMP @SBT_RightRotateSRN0
@SBT_RightRotateSRN:
	MOV R15,qword [R11+R13*8] ; TNodeSize s_r = ((n_r != -1) ? aSize[n_r] : 0);
@SBT_RightRotateSRN0:

	ADD R15,R14 ; aSize[t] = s_l + s_r + 1;
	INC R15
	MOV qword [R11+RCX*8],R15 ; RDX

	; меняем трёх предков
	; 1. t.right.parent = t
	; 2. k.parent = t.parent
	; 3. t.parent = k

	MOV RDX,qword [R8+RCX*8] ; aRT[t] -> aLT[t]
	CMP RDX,-1
	JE @SBT_RightRotateRT1
	MOV qword [R9+RDX*8],RCX ; if (aLT[t] != -1) aUT[aLT[t]] = t;
@SBT_RightRotateRT1:
	MOV qword [R9+RAX*8],RBX ; aUT[k] = p;
	MOV qword [R9+RCX*8],RAX ; aUT[t] = k;

	; меняем корень, parent -> t, k

	CMP RBX,-1
	JE @SBT_RightRotateRBX0

	MOV RDX,qword [R8+RBX*8] ; aLT[p]
	CMP RDX,RCX
	JNE @SBT_RightRotateRTPK

	MOV qword [R8+RBX*8],RAX ; if (aLT[p] == t) aLT[p] = k;
	MOV RAX,1
	RET
@SBT_RightRotateRTPK:
	MOV qword [R10+RBX*8],RAX ; else aRT[p] = k;
	MOV RAX,1
	RET

@SBT_RightRotateRBX0:
	MOV RDX,[ROOT]
	MOV qword [RDX],RAX ; if (p == -1) ROOT = k;
	MOV RAX,1
	RET

@SBT_RightRotateRet0:
	MOV RAX,0
	RET
