	BITS 64
	SECTION .text

	global SBT_Initialise_Opt

	extern NN
	extern aValue
	extern aUT
	extern aLT
	extern aRT
	extern aSize
	extern aFREE

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
