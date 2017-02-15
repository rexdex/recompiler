#include <xtl.h>
#include <math.h>

#define TEST_REG1(x)		\
	x vr0;		\
	x vr1;		\
	x vr2;		\
	x vr4;		\
	x vr8;		\
	x vr16;		\
	x vr31;

#define TEST_VREG1_SIMM(x)	\
	x vr0, -16;	\
	x vr0, -15;	\
	x vr0, -7;	\
	x vr0, -3;	\
	x vr0, -2;	\
	x vr0, -1;	\
	x vr0, 0;	\
	x vr0, 1;	\
	x vr0, 2;	\
	x vr0, 4;	\
	x vr0, 8;	\
	x vr0, 15;	\
	x vr0, 0;	\
	x vr1, 0;	\
	x vr2, 0;	\
	x vr4, 0;	\
	x vr8, 0;	\
	x vr16, 0;	\
	x vr31, 0;

#define TEST_VREG2_IMM(x)	\
	x vr0, vr0, 0;			\
	x vr1, vr0, 0;			\
	x vr2, vr0, 0;			\
	x vr4, vr0, 0;			\
	x vr8, vr0, 0;			\
	x vr16, vr0, 0;		\
	x vr31, vr0, 0;		\
	x vr0, vr0, 0;			\
	x vr0, vr1, 0;			\
	x vr0, vr2, 0;			\
	x vr0, vr4, 0;			\
	x vr0, vr8, 0;			\
	x vr0, vr16, 0;			\
	x vr0, vr31, 0;			\
	x vr0, vr0, 0;			\
	x vr0, vr0, 1;			\
	x vr0, vr0, 2;			\
	x vr0, vr0, 4;			\
	x vr0, vr0, 8;			\
	x vr0, vr0, 16;		\
	x vr0, vr0, 31;

#define TEST_VREG2_IMM_7(x)	\
	x vr0, vr0, 0;			\
	x vr1, vr0, 0;			\
	x vr2, vr0, 0;			\
	x vr4, vr0, 0;			\
	x vr8, vr0, 0;			\
	x vr16, vr0, 0;		\
	x vr31, vr0, 0;		\
	x vr0, vr0, 0;			\
	x vr0, vr1, 0;			\
	x vr0, vr2, 0;			\
	x vr0, vr4, 0;			\
	x vr0, vr8, 0;			\
	x vr0, vr16, 0;			\
	x vr0, vr31, 0;			\
	x vr0, vr0, 0;			\
	x vr0, vr0, 1;			\
	x vr0, vr0, 2;			\
	x vr0, vr0, 3;			\
	x vr0, vr0, 4;			\
	x vr0, vr0, 5;			\
	x vr0, vr0, 6;

#define TEST_VREG2_IMM_B(x)	\
	x vr0, vr0, 0;			\
	x vr1, vr0, 0;			\
	x vr2, vr0, 0;			\
	x vr4, vr0, 0;			\
	x vr8, vr0, 0;			\
	x vr16, vr0, 0;		\
	x vr31, vr0, 0;		\
	x vr0, vr0, 0;			\
	x vr0, vr1, 0;			\
	x vr0, vr2, 0;			\
	x vr0, vr4, 0;			\
	x vr0, vr8, 0;			\
	x vr0, vr16, 0;			\
	x vr0, vr31, 0;			\
	x vr0, vr0, 0;			\
	x vr0, vr0, 1;			\
	x vr0, vr0, 2;			\
	x vr0, vr0, 4;			\
	x vr0, vr0, 8;			\
	x vr0, vr0, 15;

#define TEST_VREG2_IMM_H(x)	\
	x vr0, vr0, 0;			\
	x vr1, vr0, 0;			\
	x vr2, vr0, 0;			\
	x vr4, vr0, 0;			\
	x vr8, vr0, 0;			\
	x vr16, vr0, 0;		\
	x vr31, vr0, 0;		\
	x vr0, vr0, 0;			\
	x vr0, vr1, 0;			\
	x vr0, vr2, 0;			\
	x vr0, vr4, 0;			\
	x vr0, vr8, 0;			\
	x vr0, vr16, 0;			\
	x vr0, vr31, 0;			\
	x vr0, vr0, 0;			\
	x vr0, vr0, 1;			\
	x vr0, vr0, 2;			\
	x vr0, vr0, 4;			\
	x vr0, vr0, 7;

#define TEST_VREG2_IMM_W(x)	\
	x vr0, vr0, 0;			\
	x vr1, vr0, 0;			\
	x vr2, vr0, 0;			\
	x vr4, vr0, 0;			\
	x vr8, vr0, 0;			\
	x vr16, vr0, 0;		\
	x vr31, vr0, 0;		\
	x vr0, vr0, 0;			\
	x vr0, vr1, 0;			\
	x vr0, vr2, 0;			\
	x vr0, vr4, 0;			\
	x vr0, vr8, 0;			\
	x vr0, vr16, 0;			\
	x vr0, vr31, 0;			\
	x vr0, vr0, 0;			\
	x vr0, vr0, 1;			\
	x vr0, vr0, 2;			\
	x vr0, vr0, 3;

#define TEST_VREG1_IMM_B(x)	\
	x vr0, 0;			\
	x vr1, 0;			\
	x vr2, 0;			\
	x vr4, 0;			\
	x vr8, 0;			\
	x vr16, 0;			\
	x vr31, 0;			\
	x vr0, 0;			\
	x vr0, 1;			\
	x vr0, 2;			\
	x vr0, 4;			\
	x vr0, 8;			\
	x vr0, 15;

#define TEST_VREG1_IMM_H(x)	\
	x vr0, 0;			\
	x vr1, 0;			\
	x vr2, 0;			\
	x vr4, 0;			\
	x vr8, 0;			\
	x vr16, 0;			\
	x vr31, 0;			\
	x vr0, 0;			\
	x vr0, 1;			\
	x vr0, 2;			\
	x vr0, 4;			\
	x vr0, 7;

#define TEST_VREG1_IMM_W(x)	\
	x vr0, 0;			\
	x vr1, 0;			\
	x vr2, 0;			\
	x vr4, 0;			\
	x vr8, 0;			\
	x vr16, 0;			\
	x vr31, 0;			\
	x vr0, 0;			\
	x vr0, 1;			\
	x vr0, 2;			\
	x vr0, 3

#define TEST_VREG_REG2(x)	\
	x vr0, r15, r7;			\
	x vr1, r15, r7;			\
	x vr2, r15, r7;			\
	x vr4, r15, r7;			\
	x vr8, r15, r7;			\
	x vr16, r15, r7;		\
	x vr31, r15, r7;		\
	x vr0, r0, r15;			\
	x vr0, r1, r15;			\
	x vr0, r2, r15;			\
	x vr0, r4, r15;			\
	x vr0, r8, r15;			\
	x vr0, r16, r15;		\
	x vr0, r31, r15;		\
	x vr0, r7, r0;			\
	x vr0, r7, r1;			\
	x vr0, r7, r2;			\
	x vr0, r7, r4;			\
	x vr0, r7, r8;			\
	x vr0, r7, r16;		\
	x vr0, r7, r31;

#define TEST_VREG2(x)	\
	x vr0, vr0;			\
	x vr1, vr0;			\
	x vr2, vr0;			\
	x vr4, vr0;			\
	x vr8, vr0;			\
	x vr16, vr0;		\
	x vr31, vr0;		\
	x vr0, vr0;			\
	x vr0, vr1;			\
	x vr0, vr2;			\
	x vr0, vr4;			\
	x vr0, vr8;			\
	x vr0, vr16;			\
	x vr0, vr31;

#define TEST_VREG3_IMM(x)	\
	x vr0, vr0, vr0, 0;	\
	x vr1, vr0, vr0, 0;	\
	x vr2, vr0, vr0, 0;	\
	x vr4, vr0, vr0, 0;	\
	x vr8, vr0, vr0, 0;	\
	x vr16, vr0, vr0, 0;	\
	x vr31, vr0, vr0, 0;	\
	x vr0, vr0, vr0, 0;	\
	x vr0, vr1, vr0, 0;	\
	x vr0, vr2, vr0, 0;	\
	x vr0, vr4, vr0, 0;	\
	x vr0, vr8, vr0, 0;	\
	x vr0, vr16, vr0, 0;	\
	x vr0, vr31, vr0, 0;	\
	x vr0, vr0, vr0, 0;	\
	x vr0, vr0, vr1, 0;	\
	x vr0, vr0, vr2, 0;	\
	x vr0, vr0, vr4, 0;	\
	x vr0, vr0, vr8, 0;	\
	x vr0, vr0, vr16, 0;	\
	x vr0, vr0, vr31, 0;	\
	x vr0, vr0, vr0, 0;	\
	x vr0, vr0, vr0, 1;	\
	x vr0, vr0, vr0, 2;	\
	x vr0, vr0, vr0, 4;	\
	x vr0, vr0, vr0, 8;	\
	x vr0, vr0, vr0, 15;

#define TEST_VREG4(x)			\
	x vr0, vr0, vr0, vr0;	\
	x vr1, vr0, vr0, vr0;	\
	x vr2, vr0, vr0, vr0;	\
	x vr4, vr0, vr0, vr0;	\
	x vr8, vr0, vr0, vr0;	\
	x vr16, vr0, vr0, vr0;	\
	x vr31, vr0, vr0, vr0;	\
	x vr0, vr0, vr0, vr0;	\
	x vr0, vr1, vr0, vr0;	\
	x vr0, vr2, vr0, vr0;	\
	x vr0, vr4, vr0, vr0;	\
	x vr0, vr8, vr0, vr0;	\
	x vr0, vr16, vr0, vr0;	\
	x vr0, vr31, vr0, vr0;	\
	x vr0, vr0, vr0, vr0;	\
	x vr0, vr0, vr1, vr0;	\
	x vr0, vr0, vr2, vr0;	\
	x vr0, vr0, vr4, vr0;	\
	x vr0, vr0, vr8, vr0;	\
	x vr0, vr0, vr16, vr0;\
	x vr0, vr0, vr31, vr0;\
	x vr0, vr0, vr0, vr0;\
	x vr0, vr0, vr0, vr1;\
	x vr0, vr0, vr0, vr2;\
	x vr0, vr0, vr0, vr4;\
	x vr0, vr0, vr0, vr8;\
	x vr0, vr0, vr0, vr16;\
	x vr0, vr0, vr0, vr31;

#define TEST_VREG42(x)			\
	x vr0, vr0, vr0, vr0;	\
	x vr1, vr0, vr0, vr1;	\
	x vr2, vr0, vr0, vr2;	\
	x vr4, vr0, vr0, vr4;	\
	x vr8, vr0, vr0, vr8;	\
	x vr16, vr0, vr0, vr16;	\
	x vr31, vr0, vr0, vr31;	\
	x vr0, vr0, vr0, vr0;	\
	x vr0, vr1, vr0, vr0;	\
	x vr0, vr2, vr0, vr0;	\
	x vr0, vr4, vr0, vr0;	\
	x vr0, vr8, vr0, vr0;	\
	x vr0, vr16, vr0, vr0;	\
	x vr0, vr31, vr0, vr0;	\
	x vr0, vr0, vr0, vr0;	\
	x vr0, vr0, vr1, vr0;	\
	x vr0, vr0, vr2, vr0;	\
	x vr0, vr0, vr4, vr0;	\
	x vr0, vr0, vr8, vr0;	\
	x vr0, vr0, vr16, vr0;\
	x vr0, vr0, vr31, vr0;\
	x vr0, vr0, vr0, vr0;\
	x vr1, vr0, vr0, vr1;\
	x vr2, vr0, vr0, vr2;\
	x vr4, vr0, vr0, vr4;\
	x vr8, vr0, vr0, vr8;\
	x vr16, vr0, vr0, vr16;\
	x vr31, vr0, vr0, vr31;

#define TEST_VREG42_ABAC(x)			\
	x vr0, vr0, vr0, vr0;	\
	x vr1, vr0, vr1, vr0;	\
	x vr2, vr0, vr2, vr0;	\
	x vr4, vr0, vr4, vr0;	\
	x vr8, vr0, vr8, vr0;	\
	x vr16, vr0, vr16, vr0;	\
	x vr31, vr0, vr31, vr0;	\
	x vr0, vr0, vr0, vr0;	\
	x vr0, vr1, vr0, vr0;	\
	x vr0, vr2, vr0, vr0;	\
	x vr0, vr4, vr0, vr0;	\
	x vr0, vr8, vr0, vr0;	\
	x vr0, vr16, vr0, vr0;	\
	x vr0, vr31, vr0, vr0;	\
	x vr0, vr0, vr0, vr0;	\
	x vr0, vr0, vr0, vr1;	\
	x vr0, vr0, vr0, vr2;	\
	x vr0, vr0, vr0, vr4;	\
	x vr0, vr0, vr0, vr8;	\
	x vr0, vr0, vr0, vr16;\
	x vr0, vr0, vr0, vr31;\
	x vr0, vr0, vr0, vr0;\
	x vr1, vr0, vr1, vr0;\
	x vr2, vr0, vr2, vr0;\
	x vr4, vr0, vr4, vr0;\
	x vr8, vr0, vr8, vr0;\
	x vr16, vr0, vr16, vr0;\
	x vr31, vr0, vr31, vr0;

#define TEST_VREG3(x)\
	x vr0, vr0, vr0; \
	x vr1, vr0, vr0; \
	x vr2, vr0, vr0; \
	x vr4, vr0, vr0; \
	x vr8, vr0, vr0; \
	x vr16, vr0, vr0; \
	x vr31, vr0, vr0; \
	x vr0, vr0, vr0; \
	x vr0, vr1, vr0; \
	x vr0, vr2, vr0; \
	x vr0, vr4, vr0; \
	x vr0, vr8, vr0; \
	x vr0, vr16, vr0; \
	x vr0, vr31, vr0; \
	x vr0, vr0, vr0; \
	x vr0, vr0, vr1; \
	x vr0, vr0, vr2; \
	x vr0, vr0, vr4; \
	x vr0, vr0, vr8; \
	x vr0, vr0, vr16; \
	x vr0, vr0, vr31;

#define TEST128_VREG1_SIMM(x,imm)	\
	x vr0,imm;				\
	x vr1,imm;				\
	x vr2,imm;				\
	x vr4,imm;				\
	x vr8,imm;				\
	x vr16,imm;				\
	x vr32,imm;			\
	x vr64,imm;				\
	x vr127,imm;

#define TEST128_VREG2(x)	\
	x vr1, vr1;				\
	x vr1, vr33;			\
	x vr1, vr65;			\
	x vr1, vr97;			\
	x vr33, vr1;			\
	x vr33, vr33;			\
	x vr33, vr65;			\
	x vr33, vr97;			\
	x vr65, vr1;			\
	x vr65, vr33;			\
	x vr65, vr65;			\
	x vr65, vr97;			\
	x vr97, vr1;			\
	x vr97, vr33;			\
	x vr97, vr65;			\
	x vr97, vr97;

#define TEST128_VREG2_IMM(x,imm)	\
	x vr0, vr15,imm;				\
	x vr1, vr15,imm;			\
	x vr2, vr15,imm;			\
	x vr4, vr15,imm;			\
	x vr8, vr15,imm;			\
	x vr16, vr15,imm;			\
	x vr31, vr15,imm;			\
	x vr15, vr0,imm;				\
	x vr15, vr1,imm;			\
	x vr15, vr2,imm;			\
	x vr15, vr4,imm;			\
	x vr15, vr8,imm;			\
	x vr15, vr16,imm;			\
	x vr15, vr31,imm;			\
	x vr1, vr1,imm;				\
	x vr1, vr33,imm;			\
	x vr1, vr65,imm;			\
	x vr1, vr97,imm;			\
	x vr33, vr1,imm;			\
	x vr33, vr33,imm;			\
	x vr33, vr65,imm;			\
	x vr33, vr97,imm;			\
	x vr65, vr1,imm;			\
	x vr65, vr33,imm;			\
	x vr65, vr65,imm;			\
	x vr65, vr97,imm;			\
	x vr97, vr1,imm;			\
	x vr97, vr33,imm;			\
	x vr97, vr65,imm;			\
	x vr97, vr97,imm;

#define TEST128_VREG2_MB(x,y,z)	\
	x vr0, vr15,y,z;	\
	x vr1, vr15,y,z;	\
	x vr2, vr15,y,z;	\
	x vr4, vr15,y,z;	\
	x vr8, vr15,y,z;	\
	x vr16, vr15,y,z;	\
	x vr31, vr15,y,z;	\
	x vr15, vr0,y,z;	\
	x vr15, vr1,y,z;	\
	x vr15, vr2,y,z;	\
	x vr15, vr4,y,z;	\
	x vr15, vr8,y,z;	\
	x vr15, vr16,y,z;	\
	x vr15, vr31,y,z;	\
	x vr1, vr1,y,z;		\
	x vr1, vr33,y,z;	\
	x vr1, vr65,y,z;	\
	x vr1, vr97,y,z;	\
	x vr33, vr1,y,z;	\
	x vr33, vr33,y,z;	\
	x vr33, vr65,y,z;	\
	x vr33, vr97,y,z;	\
	x vr65, vr1,y,z;	\
	x vr65, vr33,y,z;	\
	x vr65, vr65,y,z;	\
	x vr65, vr97,y,z;	\
	x vr97, vr1,y,z;	\
	x vr97, vr33,y,z;	\
	x vr97, vr65,y,z;	\
	x vr97, vr97,y,z;

#define TEST128_VREG2_MBX(x,y,z,w)	\
	x vr0, vr15,y,z,w;	\
	x vr1, vr15,y,z,w;	\
	x vr2, vr15,y,z,w;	\
	x vr4, vr15,y,z,w;	\
	x vr8, vr15,y,z,w;	\
	x vr16, vr15,y,z,w;	\
	x vr31, vr15,y,z,w;	\
	x vr15, vr0,y,z,w;	\
	x vr15, vr1,y,z,w;	\
	x vr15, vr2,y,z,w;	\
	x vr15, vr4,y,z,w;	\
	x vr15, vr8,y,z,w;	\
	x vr15, vr16,y,z,w;	\
	x vr15, vr31,y,z,w;	\
	x vr1, vr1,y,z,w;		\
	x vr1, vr33,y,z,w;	\
	x vr1, vr65,y,z,w;	\
	x vr1, vr97,y,z,w;	\
	x vr33, vr1,y,z,w;	\
	x vr33, vr33,y,z,w;	\
	x vr33, vr65,y,z,w;	\
	x vr33, vr97,y,z,w;	\
	x vr65, vr1,y,z,w;	\
	x vr65, vr33,y,z,w;	\
	x vr65, vr65,y,z,w;	\
	x vr65, vr97,y,z,w;	\
	x vr97, vr1,y,z,w;	\
	x vr97, vr33,y,z,w;	\
	x vr97, vr65,y,z,w;	\
	x vr97, vr97,y,z,w;

#define TEST128_VREG3_1(x)	\
	x vr1, vr1, vr1;		\
	x vr1, vr1, vr33;		\
	x vr1, vr1, vr65;		\
	x vr1, vr1, vr97;		\
	x vr1, vr33, vr1;		\
	x vr1, vr33, vr33;		\
	x vr1, vr33, vr65;		\
	x vr1, vr33, vr97;		\
	x vr1, vr65, vr1;		\
	x vr1, vr65, vr33;		\
	x vr1, vr65, vr65;		\
	x vr1, vr65, vr97;		\
	x vr1, vr97, vr1;		\
	x vr1, vr97, vr33;		\
	x vr1, vr97, vr65;		\
	x vr1, vr97, vr97;		\
	x vr33, vr1, vr1;		\
	x vr33, vr1, vr33;		\
	x vr33, vr1, vr65;		\
	x vr33, vr1, vr97;		\
	x vr33, vr33, vr1;		\
	x vr33, vr33, vr33;		\
	x vr33, vr33, vr65;		\
	x vr33, vr33, vr97;		\
	x vr33, vr65, vr1;		\
	x vr33, vr65, vr33;		\
	x vr33, vr65, vr65;		\
	x vr33, vr65, vr97;		\
	x vr33, vr97, vr1;		\
	x vr33, vr97, vr33;		\
	x vr33, vr97, vr65;		\
	x vr33, vr97, vr97;

#define TEST128_VREG3_1_IMM(x,imm)	\
	x vr1, vr1, vr1,imm;		\
	x vr1, vr1, vr33,imm;		\
	x vr1, vr1, vr65,imm;		\
	x vr1, vr1, vr97,imm;		\
	x vr1, vr33, vr1,imm;		\
	x vr1, vr33, vr33,imm;		\
	x vr1, vr33, vr65,imm;		\
	x vr1, vr33, vr97,imm;		\
	x vr1, vr65, vr1,imm;		\
	x vr1, vr65, vr33,imm;		\
	x vr1, vr65, vr65,imm;		\
	x vr1, vr65, vr97,imm;		\
	x vr1, vr97, vr1,imm;		\
	x vr1, vr97, vr33,imm;		\
	x vr1, vr97, vr65,imm;		\
	x vr1, vr97, vr97,imm;		\
	x vr33, vr1, vr1,imm;		\
	x vr33, vr1, vr33,imm;		\
	x vr33, vr1, vr65,imm;		\
	x vr33, vr1, vr97,imm;		\
	x vr33, vr33, vr1,imm;		\
	x vr33, vr33, vr33,imm;		\
	x vr33, vr33, vr65,imm;		\
	x vr33, vr33, vr97,imm;		\
	x vr33, vr65, vr1,imm;		\
	x vr33, vr65, vr33,imm;		\
	x vr33, vr65, vr65,imm;		\
	x vr33, vr65, vr97,imm;		\
	x vr33, vr97, vr1,imm;		\
	x vr33, vr97, vr33,imm;		\
	x vr33, vr97, vr65,imm;		\
	x vr33, vr97, vr97,imm;

#define TEST128_VREG3_2(x)	\
	x vr65, vr1, vr1;		\
	x vr65, vr1, vr33;		\
	x vr65, vr1, vr65;		\
	x vr65, vr1, vr97;		\
	x vr65, vr33, vr1;		\
	x vr65, vr33, vr33;		\
	x vr65, vr33, vr65;		\
	x vr65, vr33, vr97;		\
	x vr65, vr65, vr1;		\
	x vr65, vr65, vr33;		\
	x vr65, vr65, vr65;		\
	x vr65, vr65, vr97;		\
	x vr65, vr97, vr1;		\
	x vr65, vr97, vr33;		\
	x vr65, vr97, vr65;		\
	x vr65, vr97, vr97;		\
	x vr97, vr1, vr1;		\
	x vr97, vr1, vr33;		\
	x vr97, vr1, vr65;		\
	x vr97, vr1, vr97;		\
	x vr97, vr33, vr1;		\
	x vr97, vr33, vr33;		\
	x vr97, vr33, vr65;		\
	x vr97, vr33, vr97;		\
	x vr97, vr65, vr1;		\
	x vr97, vr65, vr33;		\
	x vr97, vr65, vr65;		\
	x vr97, vr65, vr97;		\
	x vr97, vr97, vr1;		\
	x vr97, vr97, vr33;		\
	x vr97, vr97, vr65;		\
	x vr97, vr97, vr97

#define TEST128_VREG3_2_IMM(x,imm)	\
	x vr65, vr1, vr1,imm;		\
	x vr65, vr1, vr33,imm;		\
	x vr65, vr1, vr65,imm;		\
	x vr65, vr1, vr97,imm;		\
	x vr65, vr33, vr1,imm;		\
	x vr65, vr33, vr33,imm;		\
	x vr65, vr33, vr65,imm;		\
	x vr65, vr33, vr97,imm;		\
	x vr65, vr65, vr1,imm;		\
	x vr65, vr65, vr33,imm;		\
	x vr65, vr65, vr65,imm;		\
	x vr65, vr65, vr97,imm;		\
	x vr65, vr97, vr1,imm;		\
	x vr65, vr97, vr33,imm;		\
	x vr65, vr97, vr65,imm;		\
	x vr65, vr97, vr97,imm;		\
	x vr97, vr1, vr1,imm;		\
	x vr97, vr1, vr33,imm;		\
	x vr97, vr1, vr65,imm;		\
	x vr97, vr1, vr97,imm;		\
	x vr97, vr33, vr1,imm;		\
	x vr97, vr33, vr33,imm;		\
	x vr97, vr33, vr65,imm;		\
	x vr97, vr33, vr97,imm;		\
	x vr97, vr65, vr1,imm;		\
	x vr97, vr65, vr33,imm;		\
	x vr97, vr65, vr65,imm;		\
	x vr97, vr65, vr97,imm;		\
	x vr97, vr97, vr1,imm;		\
	x vr97, vr97, vr33,imm;		\
	x vr97, vr97, vr65,imm;		\
	x vr97, vr97, vr97,imm

#define TEST128_VREG4_ABCA_1(x)	\
	x vr1, vr1, vr1, vr1;		\
	x vr1, vr1, vr33, vr1;		\
	x vr1, vr1, vr65, vr1;		\
	x vr1, vr1, vr97, vr1;		\
	x vr1, vr33, vr1, vr1;		\
	x vr1, vr33, vr33, vr1;		\
	x vr1, vr33, vr65, vr1;		\
	x vr1, vr33, vr97, vr1;		\
	x vr1, vr65, vr1, vr1;		\
	x vr1, vr65, vr33, vr1;		\
	x vr1, vr65, vr65, vr1;		\
	x vr1, vr65, vr97, vr1;		\
	x vr1, vr97, vr1, vr1;		\
	x vr1, vr97, vr33, vr1;		\
	x vr1, vr97, vr65, vr1;		\
	x vr1, vr97, vr97, vr1;		\
	x vr33, vr1, vr1, vr33;		\
	x vr33, vr1, vr33, vr33;	\
	x vr33, vr1, vr65, vr33;	\
	x vr33, vr1, vr97, vr33;	\
	x vr33, vr33, vr1, vr33;	\
	x vr33, vr33, vr33, vr33;	\
	x vr33, vr33, vr65, vr33;	\
	x vr33, vr33, vr97, vr33;	\
	x vr33, vr65, vr1, vr33;	\
	x vr33, vr65, vr33, vr33;	\
	x vr33, vr65, vr65, vr33;	\
	x vr33, vr65, vr97, vr33;	\
	x vr33, vr97, vr1, vr33;	\
	x vr33, vr97, vr33, vr33;	\
	x vr33, vr97, vr65, vr33;	\
	x vr33, vr97, vr97, vr33;

#define TEST128_VREG4_ABCA_2(x)	\
	x vr65, vr1, vr1, vr65;		\
	x vr65, vr1, vr33, vr65;	\
	x vr65, vr1, vr65, vr65;	\
	x vr65, vr1, vr97, vr65;	\
	x vr65, vr33, vr1, vr65;	\
	x vr65, vr33, vr33, vr65;	\
	x vr65, vr33, vr65, vr65;	\
	x vr65, vr33, vr97, vr65;	\
	x vr65, vr65, vr1, vr65;	\
	x vr65, vr65, vr33, vr65;	\
	x vr65, vr65, vr65, vr65;	\
	x vr65, vr65, vr97, vr65;	\
	x vr65, vr97, vr1, vr65;	\
	x vr65, vr97, vr33, vr65;	\
	x vr65, vr97, vr65, vr65;	\
	x vr65, vr97, vr97, vr65;	\
	x vr97, vr1, vr1, vr97;		\
	x vr97, vr1, vr33, vr97;	\
	x vr97, vr1, vr65, vr97;	\
	x vr97, vr1, vr97, vr97;	\
	x vr97, vr33, vr1, vr97;	\
	x vr97, vr33, vr33, vr97;	\
	x vr97, vr33, vr65, vr97;	\
	x vr97, vr33, vr97, vr97;	\
	x vr97, vr65, vr1, vr97;	\
	x vr97, vr65, vr33, vr97;	\
	x vr97, vr65, vr65, vr97;	\
	x vr97, vr65, vr97, vr97;	\
	x vr97, vr97, vr1, vr97;	\
	x vr97, vr97, vr33, vr97;	\
	x vr97, vr97, vr65, vr97;	\
	x vr97, vr97, vr97, vr97

#define TEST128_VREG4_ABAC_1(x)	\
	x vr1, vr1, vr1, vr1;		\
	x vr1, vr1, vr1, vr33;		\
	x vr1, vr1, vr1, vr65;		\
	x vr1, vr1, vr1, vr97;		\
	x vr1, vr33, vr1, vr1;		\
	x vr1, vr33, vr1, vr33;		\
	x vr1, vr33, vr1, vr65;		\
	x vr1, vr33, vr1, vr97;		\
	x vr1, vr65, vr1, vr1;		\
	x vr1, vr65, vr1, vr33;		\
	x vr1, vr65, vr1, vr65;		\
	x vr1, vr65, vr1, vr97;		\
	x vr1, vr97, vr1, vr1;		\
	x vr1, vr97, vr1, vr33;		\
	x vr1, vr97, vr1, vr65;		\
	x vr1, vr97, vr1, vr97;		\
	x vr33, vr1, vr33, vr1;		\
	x vr33, vr1, vr33, vr33;	\
	x vr33, vr1, vr33, vr65;	\
	x vr33, vr1, vr33, vr97;	\
	x vr33, vr33, vr33, vr1;	\
	x vr33, vr33, vr33, vr33;	\
	x vr33, vr33, vr33, vr65;	\
	x vr33, vr33, vr33, vr97;	\
	x vr33, vr65, vr33, vr1;	\
	x vr33, vr65, vr33, vr33;	\
	x vr33, vr65, vr33, vr65;	\
	x vr33, vr65, vr33, vr97;	\
	x vr33, vr97, vr33, vr1;	\
	x vr33, vr97, vr33, vr33;	\
	x vr33, vr97, vr33, vr65;	\
	x vr33, vr97, vr33, vr97;

#define TEST128_VREG4_PERM_0(x,y)	\
	x vr0, vr0, vr0, y;		\
	x vr1, vr0, vr0, y;		\
	x vr2, vr0, vr1, y;		\
	x vr4, vr1, vr1, y;		\
	x vr8, vr1, vr2, y;		\
	x vr16, vr1, vr2, y;		\
	x vr31, vr2, vr4, y;		\
	x vr0, vr2, vr4, y;		\
	x vr1, vr2, vr8, y;		\
	x vr2, vr4, vr8, y;		\
	x vr4, vr4, vr16, y;		\
	x vr8, vr4, vr16, y;		\
	x vr16, vr8, vr31, y;		\
	x vr31, vr8, vr0, y;		\
	x vr0, vr8, vr0, y;		\
	x vr1, vr16, vr1, y;		\
	x vr2, vr16, vr1, y;		\
	x vr4, vr16, vr2, y;	\
	x vr8, vr16, vr2, y;	\
	x vr16, vr31, vr4, y;	\
	x vr31, vr31, vr4, y;	\
	x vr0, vr31, vr8, y;	\
	x vr1, vr0, vr8, y;	\
	x vr2, vr0, vr16, y;	\
	x vr4, vr0, vr16, y;	\
	x vr8, vr1, vr31, y;	\
	x vr16, vr1, vr0, y;	\
	x vr31, vr1, vr0, y;	\
	x vr0, vr2, vr1, y;	\
	x vr1, vr2, vr1, y;	\
	x vr2, vr2, vr2, y;	\
	x vr4, vr7, vr2, y;

#define TEST128_VREG4_PERM_1(x,y)	\
	x vr1, vr1, vr1, y;		\
	x vr1, vr1, vr33, y;		\
	x vr1, vr1, vr65, y;		\
	x vr1, vr1, vr97, y;		\
	x vr1, vr33, vr1, y;		\
	x vr1, vr33, vr33, y;		\
	x vr1, vr33, vr65, y;		\
	x vr1, vr33, vr97, y;		\
	x vr1, vr65, vr1, y;		\
	x vr1, vr65, vr33, y;		\
	x vr1, vr65, vr65, y;		\
	x vr1, vr65, vr97, y;		\
	x vr1, vr97, vr1, y;		\
	x vr1, vr97, vr33, y;		\
	x vr1, vr97, vr65, y;		\
	x vr1, vr97, vr97, y;		\
	x vr33, vr1, vr1, y;		\
	x vr33, vr1, vr33, y;	\
	x vr33, vr1, vr65, y;	\
	x vr33, vr1, vr97, y;	\
	x vr33, vr33, vr1, y;	\
	x vr33, vr33, vr33, y;	\
	x vr33, vr33, vr65, y;	\
	x vr33, vr33, vr97, y;	\
	x vr33, vr65, vr1, y;	\
	x vr33, vr65, vr33, y;	\
	x vr33, vr65, vr65, y;	\
	x vr33, vr65, vr97, y;	\
	x vr33, vr97, vr1, y;	\
	x vr33, vr97, vr33, y;	\
	x vr33, vr97, vr65, y;	\
	x vr33, vr97, vr97, y;

#define TEST128_VREG4_PERM_2(x,y)	\
	x vr65, vr1, vr1, y;		\
	x vr65, vr1, vr33, y;	\
	x vr65, vr1, vr65, y;	\
	x vr65, vr1, vr97, y;	\
	x vr65, vr33, vr1, y;	\
	x vr65, vr33, vr33, y;	\
	x vr65, vr33, vr65, y;	\
	x vr65, vr33, vr97, y;	\
	x vr65, vr65, vr1, y;	\
	x vr65, vr65, vr33, y;	\
	x vr65, vr65, vr65, y;	\
	x vr65, vr65, vr97, y;	\
	x vr65, vr97, vr1, y;	\
	x vr65, vr97, vr33, y;	\
	x vr65, vr97, vr65, y;	\
	x vr65, vr97, vr97, y;	\
	x vr97, vr1, vr1, y;		\
	x vr97, vr1, vr33, y;	\
	x vr97, vr1, vr65, y;	\
	x vr97, vr1, vr97, y;	\
	x vr97, vr33, vr1, y;	\
	x vr97, vr33, vr33, y;	\
	x vr97, vr33, vr65, y;	\
	x vr97, vr33, vr97, y;	\
	x vr97, vr65, vr1, y;	\
	x vr97, vr65, vr33, y;	\
	x vr97, vr65, vr65, y;	\
	x vr97, vr65, vr97, y;	\
	x vr97, vr97, vr1, y;	\
	x vr97, vr97, vr33, y;	\
	x vr97, vr97, vr65, y;	\
	x vr97, vr97, vr97, y

#define TEST128_VREG3(x)	\
	TEST128_VREG3_1(x)		\
	TEST128_VREG3_2(x)

#define TEST128_VREG_REG2(x)	\
	x vr1, r3, r4;				\
	x vr1, r10, r12;			\
	x vr1, r1, r15;				\
	x vr1, r7, r31;				\
	x vr1, r31, r16;			\
	x vr31, r3, r4;				\
	x vr31, r10, r12;			\
	x vr31, r1, r15;			\
	x vr31, r7, r31;			\
	x vr31, r31, r16;			\
	x vr32, r3, r4;				\
	x vr32, r10, r12;			\
	x vr32, r1, r15;			\
	x vr32, r7, r31;			\
	x vr32, r31, r16;			\
	x vr63, r3, r4;				\
	x vr63, r10, r12;			\
	x vr63, r1, r15;			\
	x vr63, r7, r31;			\
	x vr63, r31, r16;			\
	x vr64, r3, r4;				\
	x vr64, r10, r12;			\
	x vr64, r1, r15;			\
	x vr64, r7, r31;			\
	x vr64, r31, r16;			\
	x vr95, r3, r4;				\
	x vr95, r10, r12;			\
	x vr95, r1, r15;			\
	x vr95, r7, r31;			\
	x vr95, r31, r16;			\
	x vr96, r3, r4;				\
	x vr96, r10, r12;			\
	x vr96, r1, r15;			\
	x vr96, r7, r31;			\
	x vr96, r31, r16;			\
	x vr127, r3, r4;			\
	x vr127, r10, r12;			\
	x vr127, r1, r15;			\
	x vr127, r7, r31;			\
	x vr127, r31, r16;			\
	x vr127, r3, r4;			\
	x vr127, r10, r12;			\
	x vr127, r1, r15;			\
	x vr127, r7, r31;			\
	x vr127, r31, r16;

__declspec(noinline) __declspec(naked) void test(int a, int b)
{
	__asm
	{
		TEST_REG1(mfvscr)
		TEST_REG1(mtvscr)

		TEST128_VREG_REG2(lvlxl128)
		TEST128_VREG_REG2(lvrx128)
		TEST128_VREG_REG2(lvrxl128)
		TEST128_VREG_REG2(lvx128)
		TEST128_VREG_REG2(lvxl128)

		TEST128_VREG_REG2(stvewx128)
		TEST128_VREG_REG2(stvlx128)
		TEST128_VREG_REG2(stvlxl128)
		TEST128_VREG_REG2(stvrx128)
		TEST128_VREG_REG2(stvrxl128)
		TEST128_VREG_REG2(stvx128)
		TEST128_VREG_REG2(stvxl128)

		TEST_VREG_REG2(lvlx)
		TEST_VREG_REG2(lvlxl)
		TEST_VREG_REG2(lvrx)
		TEST_VREG_REG2(lvrxl)
		TEST_VREG_REG2(lvsl)
		TEST_VREG_REG2(lvsr)
		TEST_VREG_REG2(lvx)
		TEST_VREG_REG2(lvxl)

		TEST_VREG_REG2(stvebx)
		TEST_VREG_REG2(stvehx)
		TEST_VREG_REG2(stvewx)
		TEST_VREG_REG2(stvlx)
		TEST_VREG_REG2(stvlxl)
		TEST_VREG_REG2(stvrx)
		TEST_VREG_REG2(stvrxl)
		TEST_VREG_REG2(stvx)
		TEST_VREG_REG2(stvxl)

		TEST_VREG3(vaddcuw)
		TEST_VREG3(vaddfp)
		TEST_VREG3(vaddfp)

		TEST_VREG3(vaddsbs)
		TEST_VREG3(vaddshs)
		TEST_VREG3(vaddsws)
		TEST_VREG3(vaddubm)
		TEST_VREG3(vaddubs)
		TEST_VREG3(vadduhm)
		TEST_VREG3(vadduhs)
		TEST_VREG3(vadduwm)
		TEST_VREG3(vadduws)
		TEST_VREG3(vand)
		TEST_VREG3(vandc)
		TEST_VREG3(vavgsb)
		TEST_VREG3(vavgsh)
		TEST_VREG3(vavgsw)
		TEST_VREG3(vavgub)
		TEST_VREG3(vavguh)
		TEST_VREG3(vavguw)

		TEST_VREG2_IMM(vcfpsxws)
		TEST_VREG2_IMM(vcfpuxws)
		TEST_VREG2_IMM(vcfsx)
		TEST_VREG2_IMM(vcfux)

		TEST_VREG3(vcmpbfp)
		TEST_VREG3(vcmpbfp.)
		TEST_VREG3(vcmpeqfp)
		TEST_VREG3(vcmpeqfp.)
		TEST_VREG3(vcmpequb)
		TEST_VREG3(vcmpequb.)
		TEST_VREG3(vcmpequh)
		TEST_VREG3(vcmpequh.)
		TEST_VREG3(vcmpequw)
		TEST_VREG3(vcmpequw.)
		TEST_VREG3(vcmpgefp)
		TEST_VREG3(vcmpgefp.)
		TEST_VREG3(vcmpgtfp)
		TEST_VREG3(vcmpgtfp.)
		TEST_VREG3(vcmpgtsb)
		TEST_VREG3(vcmpgtsb.)
		TEST_VREG3(vcmpgtsh)
		TEST_VREG3(vcmpgtsh.)
		TEST_VREG3(vcmpgtsw)
		TEST_VREG3(vcmpgtsw.)
		TEST_VREG3(vcmpgtub)
		TEST_VREG3(vcmpgtub.)
		TEST_VREG3(vcmpgtuh)
		TEST_VREG3(vcmpgtuh.)
		TEST_VREG3(vcmpgtuw)
		TEST_VREG3(vcmpgtuw.)

		TEST_VREG2_IMM(vcsxwfp)
		TEST_VREG2_IMM(vctsxs)
		TEST_VREG2_IMM(vctuxs)
		TEST_VREG2_IMM(vcuxwfp)

		TEST_VREG2(vexptefp)
		TEST_VREG2(vlogefp)

		TEST_VREG3(vmsum3fp128)
		TEST_VREG3(vmsum3fp128)
		TEST128_VREG3(vmsum3fp128)
		TEST128_VREG3(vmsum4fp128)

		TEST_VREG4(vmaddfp)
		TEST_VREG4(vnmsubfp)
		TEST_VREG4(vperm)

		TEST_VREG3(vmaxfp)
		TEST_VREG3(vmaxsb)
		TEST_VREG3(vmaxsh)
		TEST_VREG3(vmaxsw)
		TEST_VREG3(vmaxub)
		TEST_VREG3(vmaxuh)
		TEST_VREG3(vmaxuw)

		TEST_VREG3(vminfp)
		TEST_VREG3(vminsb)
		TEST_VREG3(vminsh)
		TEST_VREG3(vminsw)
		TEST_VREG3(vminub)
		TEST_VREG3(vminuh)
		TEST_VREG3(vminuw)

		TEST_VREG3(vmrghb)
		TEST_VREG3(vmrghh)
		TEST_VREG3(vmrghw)
		TEST_VREG3(vmrglb)
		TEST_VREG3(vmrglh)
		TEST_VREG3(vmrglw)

		TEST_VREG3(vnor)
		TEST_VREG3(vor)
		TEST_VREG3(vxor)
		TEST_VREG3(vpkpx)
		TEST_VREG3(vpkshss)
		TEST_VREG3(vpkshus)
		TEST_VREG3(vpkswss)
		TEST_VREG3(vpkswus)

		TEST_VREG3(vpkuhum)
		TEST_VREG3(vpkuhus)
		TEST_VREG3(vpkuwum)
		TEST_VREG3(vpkuwus)

		TEST_VREG2(vrefp)
		TEST_VREG2(vrfim)
		TEST_VREG2(vrfin)
		TEST_VREG2(vrfip)
		TEST_VREG2(vrfiz)

		TEST_VREG3(vrlb)
		TEST_VREG3(vrlh)
		TEST_VREG3(vrlw)

		TEST_VREG2(vrsqrtefp)

		TEST_VREG4(vsel)

		TEST_VREG3(vsl)
		TEST_VREG3(vslb)
		TEST_VREG3(vslh)
		TEST_VREG3(vslo)
		TEST_VREG3(vslw)
		TEST_VREG3_IMM(vsldoi)

		TEST_VREG2_IMM_W(vspltb)
		TEST_VREG2_IMM_W(vsplth)
		TEST_VREG2_IMM_W(vspltw)

		TEST_VREG1_SIMM(vspltisb)
		TEST_VREG1_SIMM(vspltish)
		TEST_VREG1_SIMM(vspltisw)

		TEST_VREG3(vsr)
		TEST_VREG3(vsrab)
		TEST_VREG3(vsrah)
		TEST_VREG3(vsraw)
		TEST_VREG3(vsrb)
		TEST_VREG3(vsrh)
		TEST_VREG3(vsrw)
		TEST_VREG3(vsro)

		TEST_VREG3(vsubcuw)
		TEST_VREG3(vsubfp)
		TEST_VREG3(vsubsbs)
		TEST_VREG3(vsubshs)
		TEST_VREG3(vsubsws)

		TEST_VREG3(vsububs)
		TEST_VREG3(vsubuhs)
		TEST_VREG3(vsubuws)
		TEST_VREG3(vsububm)
		TEST_VREG3(vsubuhm)
		TEST_VREG3(vsubuwm)

		TEST_VREG2(vupkhpx)
		TEST_VREG2(vupkhsb)
		TEST_VREG2(vupkhsh)
		TEST_VREG2(vupklpx)
		TEST_VREG2(vupklsb)
		TEST_VREG2(vupklsb)

		TEST128_VREG3(vaddfp128)
		TEST128_VREG3(vand128)
		TEST128_VREG3(vandc128)
		TEST128_VREG3(vor128)
		TEST128_VREG3(vxor128)
		TEST_VREG3(vaddfp128)
		TEST_VREG3(vand128)
		TEST_VREG3(vandc128)
		TEST_VREG3(vor128)
		TEST_VREG3(vxor128)

		TEST128_VREG3_1(vcmpbfp128)
		TEST128_VREG3_2(vcmpbfp128)
		TEST128_VREG3_1(vcmpbfp128.)
		TEST128_VREG3_2(vcmpbfp128.)
		TEST128_VREG3_1(vcmpeqfp128)
		TEST128_VREG3_2(vcmpeqfp128)
		TEST128_VREG3_1(vcmpeqfp128.)
		TEST128_VREG3_2(vcmpeqfp128.)
		TEST128_VREG3_1(vcmpequw128)
		TEST128_VREG3_2(vcmpequw128)
		TEST128_VREG3_1(vcmpequw128.)
		TEST128_VREG3_2(vcmpequw128.)
		TEST128_VREG3_1(vcmpequw128)
		TEST128_VREG3_2(vcmpequw128)
		TEST128_VREG3_1(vcmpequw128.)
		TEST128_VREG3_2(vcmpequw128.)
		TEST128_VREG3_1(vcmpgefp128)
		TEST128_VREG3_2(vcmpgefp128)
		TEST128_VREG3_1(vcmpgefp128.)
		TEST128_VREG3_2(vcmpgefp128.)
		TEST128_VREG3_1(vcmpgtfp128)
		TEST128_VREG3_2(vcmpgtfp128)
		TEST128_VREG3_1(vcmpgtfp128.)
		TEST128_VREG3_2(vcmpgtfp128.)
		TEST_VREG3(vcmpbfp128)
		TEST_VREG3(vcmpbfp128.)
		TEST_VREG3(vcmpeqfp128)
		TEST_VREG3(vcmpeqfp128.)
		TEST_VREG3(vcmpequw128)
		TEST_VREG3(vcmpequw128.)
		TEST_VREG3(vcmpequw128)
		TEST_VREG3(vcmpequw128.)
		TEST_VREG3(vcmpgefp128)
		TEST_VREG3(vcmpgefp128.)
		TEST_VREG3(vcmpgtfp128)
		TEST_VREG3(vcmpgtfp128.)

		TEST128_VREG3(vmaxfp128)
		TEST128_VREG3(vminfp128)
		TEST_VREG3(vmaxfp128)
		TEST_VREG3(vminfp128)

		TEST128_VREG3(vmrghw128)
		TEST128_VREG3(vmrglw128)
		TEST_VREG3(vmrghw128)
		TEST_VREG3(vmrglw128)

		TEST128_VREG3(vmulfp128)
		TEST128_VREG3(vnor128)
		TEST128_VREG3(vpkshss128)
		TEST128_VREG3(vpkshus128)
		TEST128_VREG3(vpkuhum128)
		TEST128_VREG3(vpkuhus128)
		TEST128_VREG3(vpkswss128)
		TEST128_VREG3(vpkswus128)
		TEST128_VREG3(vpkuwum128)
		TEST128_VREG3(vpkuwus128)
		TEST_VREG3(vmulfp128)
		TEST_VREG3(vnor128)
		TEST_VREG3(vpkshss128)
		TEST_VREG3(vpkshus128)
		TEST_VREG3(vpkuhum128)
		TEST_VREG3(vpkuhus128)
		TEST_VREG3(vpkswss128)
		TEST_VREG3(vpkswus128)
		TEST_VREG3(vpkuwum128)
		TEST_VREG3(vpkuwus128)

		TEST128_VREG2(vrefp128)
		TEST128_VREG2(vrfim128)
		TEST128_VREG2(vrfin128)
		TEST128_VREG2(vrfip128)
		TEST128_VREG2(vrfiz128)
		TEST128_VREG2(vrsqrtefp128)
		TEST_VREG2(vrefp128)
		TEST_VREG2(vrfim128)
		TEST_VREG2(vrfin128)
		TEST_VREG2(vrfip128)
		TEST_VREG2(vrfiz128)
		TEST_VREG2(vrsqrtefp128)

		TEST128_VREG2_MB(vrlimi128,0,0)
		TEST128_VREG2_MB(vrlimi128,0,1)
		TEST128_VREG2_MB(vrlimi128,0,2)
		TEST128_VREG2_MB(vrlimi128,0,3)
		TEST128_VREG2_MB(vrlimi128,1,0)
		TEST128_VREG2_MB(vrlimi128,1,1)
		TEST128_VREG2_MB(vrlimi128,1,2)
		TEST128_VREG2_MB(vrlimi128,1,3)
		TEST128_VREG2_MB(vrlimi128,2,0)
		TEST128_VREG2_MB(vrlimi128,2,1)
		TEST128_VREG2_MB(vrlimi128,2,2)
		TEST128_VREG2_MB(vrlimi128,2,3)
		TEST128_VREG2_MB(vrlimi128,4,0)
		TEST128_VREG2_MB(vrlimi128,4,1)
		TEST128_VREG2_MB(vrlimi128,4,2)
		TEST128_VREG2_MB(vrlimi128,4,3)
		TEST128_VREG2_MB(vrlimi128,8,0)
		TEST128_VREG2_MB(vrlimi128,8,1)
		TEST128_VREG2_MB(vrlimi128,8,2)
		TEST128_VREG2_MB(vrlimi128,8,3)
		TEST128_VREG2_MB(vrlimi128,15,0)
		TEST128_VREG2_MB(vrlimi128,15,1)
		TEST128_VREG2_MB(vrlimi128,15,2)
		TEST128_VREG2_MB(vrlimi128,15,3)

		TEST128_VREG2_IMM(vpermwi128,0)
		TEST128_VREG2_IMM(vpermwi128,1)
		TEST128_VREG2_IMM(vpermwi128,2)
		TEST128_VREG2_IMM(vpermwi128,4)
		TEST128_VREG2_IMM(vpermwi128,8)
		TEST128_VREG2_IMM(vpermwi128,16)
		TEST128_VREG2_IMM(vpermwi128,32)
		TEST128_VREG2_IMM(vpermwi128,64)
		TEST128_VREG2_IMM(vpermwi128,128)
		TEST128_VREG2_IMM(vpermwi128,255)

		TEST128_VREG3(vrlw128)
		TEST_VREG3(vrlw128)

		TEST128_VREG4_ABCA_1(vsel128)
		TEST128_VREG4_ABCA_2(vsel128)
		TEST_VREG42(vsel128)

		TEST128_VREG4_PERM_0(vperm128,vr0)
		TEST128_VREG4_PERM_1(vperm128,vr0)
		TEST128_VREG4_PERM_2(vperm128,vr0)
		TEST128_VREG4_PERM_0(vperm128,vr1)
		TEST128_VREG4_PERM_1(vperm128,vr1)
		TEST128_VREG4_PERM_2(vperm128,vr1)
		TEST128_VREG4_PERM_0(vperm128,vr2)
		TEST128_VREG4_PERM_1(vperm128,vr2)
		TEST128_VREG4_PERM_2(vperm128,vr2)
		TEST128_VREG4_PERM_0(vperm128,vr4)
		TEST128_VREG4_PERM_1(vperm128,vr4)
		TEST128_VREG4_PERM_2(vperm128,vr4)
		TEST128_VREG4_PERM_0(vperm128,vr7)
		TEST128_VREG4_PERM_1(vperm128,vr7)
		TEST128_VREG4_PERM_2(vperm128,vr7)
		
		TEST128_VREG3_1_IMM(vsldoi128,0)
		TEST128_VREG3_1_IMM(vsldoi128,1)
		TEST128_VREG3_1_IMM(vsldoi128,2)
		TEST128_VREG3_1_IMM(vsldoi128,4)
		TEST128_VREG3_1_IMM(vsldoi128,8)
		TEST128_VREG3_2_IMM(vsldoi128,0)
		TEST128_VREG3_2_IMM(vsldoi128,1)
		TEST128_VREG3_2_IMM(vsldoi128,2)
		TEST128_VREG3_2_IMM(vsldoi128,4)
		TEST128_VREG3_2_IMM(vsldoi128,8)
		TEST_VREG3_IMM(vsldoi128)

		TEST128_VREG3(vslo128)
		TEST128_VREG3(vslw128)
		TEST128_VREG3(vsraw128)
		TEST128_VREG3(vsro128)
		TEST128_VREG3(vsrw128)
		TEST128_VREG3(vsubfp128)
		TEST_VREG3(vslo128)
		TEST_VREG3(vslw128)
		TEST_VREG3(vsraw128)
		TEST_VREG3(vsro128)
		TEST_VREG3(vsrw128)
		TEST_VREG3(vsubfp128)

		TEST128_VREG1_SIMM(vspltisw128,-16)
		TEST128_VREG1_SIMM(vspltisw128,-1)
		TEST128_VREG1_SIMM(vspltisw128,0)
		TEST128_VREG1_SIMM(vspltisw128,1)
		TEST128_VREG1_SIMM(vspltisw128,2)
		TEST128_VREG1_SIMM(vspltisw128,4)
		TEST128_VREG1_SIMM(vspltisw128,8)
		TEST_VREG1_SIMM(vspltisw128)

		TEST128_VREG2_IMM(vspltw128,0)
		TEST128_VREG2_IMM(vspltw128,1)
		TEST128_VREG2_IMM(vspltw128,2)
		TEST128_VREG2_IMM(vspltw128,3)
		TEST_VREG2_IMM(vspltw128)

		TEST128_VREG2_IMM(vupkd3d128,0)
		TEST128_VREG2_IMM(vupkd3d128,1)
		TEST128_VREG2_IMM(vupkd3d128,2)
		TEST128_VREG2_IMM(vupkd3d128,3)
		TEST128_VREG2_IMM(vupkd3d128,4)
		TEST128_VREG2_IMM(vupkd3d128,5)
		TEST128_VREG2_IMM(vupkd3d128,6)
		TEST_VREG2_IMM_7(vupkd3d128)

		TEST128_VREG2_MBX(vpkd3d128,0,0,0)
		TEST128_VREG2_MBX(vpkd3d128,0,0,1)
		TEST128_VREG2_MBX(vpkd3d128,0,0,2)
		TEST128_VREG2_MBX(vpkd3d128,0,0,3)
		TEST128_VREG2_MBX(vpkd3d128,0,1,0)
		TEST128_VREG2_MBX(vpkd3d128,0,1,1)
		TEST128_VREG2_MBX(vpkd3d128,0,1,2)
		TEST128_VREG2_MBX(vpkd3d128,0,1,3)
		TEST128_VREG2_MBX(vpkd3d128,0,2,0)
		TEST128_VREG2_MBX(vpkd3d128,0,2,1)
		TEST128_VREG2_MBX(vpkd3d128,0,2,2)
		TEST128_VREG2_MBX(vpkd3d128,0,2,3)
		TEST128_VREG2_MBX(vpkd3d128,0,3,0)
		TEST128_VREG2_MBX(vpkd3d128,0,3,1)
		TEST128_VREG2_MBX(vpkd3d128,0,3,2)
		TEST128_VREG2_MBX(vpkd3d128,0,3,3)

		TEST128_VREG2_MBX(vpkd3d128,1,0,0)
		TEST128_VREG2_MBX(vpkd3d128,1,0,1)
		TEST128_VREG2_MBX(vpkd3d128,1,0,2)
		TEST128_VREG2_MBX(vpkd3d128,1,0,3)
		TEST128_VREG2_MBX(vpkd3d128,1,1,0)
		TEST128_VREG2_MBX(vpkd3d128,1,1,1)
		TEST128_VREG2_MBX(vpkd3d128,1,1,2)
		TEST128_VREG2_MBX(vpkd3d128,1,1,3)
		TEST128_VREG2_MBX(vpkd3d128,1,2,0)
		TEST128_VREG2_MBX(vpkd3d128,1,2,1)
		TEST128_VREG2_MBX(vpkd3d128,1,2,2)
		TEST128_VREG2_MBX(vpkd3d128,1,2,3)
		TEST128_VREG2_MBX(vpkd3d128,1,3,0)
		TEST128_VREG2_MBX(vpkd3d128,1,3,1)
		TEST128_VREG2_MBX(vpkd3d128,1,3,2)
		TEST128_VREG2_MBX(vpkd3d128,1,3,3)

		TEST128_VREG2_MBX(vpkd3d128,2,0,0)
		TEST128_VREG2_MBX(vpkd3d128,2,0,1)
		TEST128_VREG2_MBX(vpkd3d128,2,0,2)
		TEST128_VREG2_MBX(vpkd3d128,2,0,3)
		TEST128_VREG2_MBX(vpkd3d128,2,1,0)
		TEST128_VREG2_MBX(vpkd3d128,2,1,1)
		TEST128_VREG2_MBX(vpkd3d128,2,1,2)
		TEST128_VREG2_MBX(vpkd3d128,2,1,3)
		TEST128_VREG2_MBX(vpkd3d128,2,2,0)
		TEST128_VREG2_MBX(vpkd3d128,2,2,1)
		TEST128_VREG2_MBX(vpkd3d128,2,2,2)
		TEST128_VREG2_MBX(vpkd3d128,2,2,3)
		TEST128_VREG2_MBX(vpkd3d128,2,3,0)
		TEST128_VREG2_MBX(vpkd3d128,2,3,1)
		TEST128_VREG2_MBX(vpkd3d128,2,3,2)
		TEST128_VREG2_MBX(vpkd3d128,2,3,3)

		TEST128_VREG2_MBX(vpkd3d128,4,0,0)
		TEST128_VREG2_MBX(vpkd3d128,4,0,1)
		TEST128_VREG2_MBX(vpkd3d128,4,0,2)
		TEST128_VREG2_MBX(vpkd3d128,4,0,3)
		TEST128_VREG2_MBX(vpkd3d128,4,1,0)
		TEST128_VREG2_MBX(vpkd3d128,4,1,1)
		TEST128_VREG2_MBX(vpkd3d128,4,1,2)
		TEST128_VREG2_MBX(vpkd3d128,4,1,3)
		TEST128_VREG2_MBX(vpkd3d128,4,2,0)
		TEST128_VREG2_MBX(vpkd3d128,4,2,1)
		TEST128_VREG2_MBX(vpkd3d128,4,2,2)
		TEST128_VREG2_MBX(vpkd3d128,4,2,3)
		TEST128_VREG2_MBX(vpkd3d128,4,3,0)
		TEST128_VREG2_MBX(vpkd3d128,4,3,1)
		TEST128_VREG2_MBX(vpkd3d128,4,3,2)
		TEST128_VREG2_MBX(vpkd3d128,4,3,3)


		TEST128_VREG2(vupkhsb128)
		TEST128_VREG2(vupkhsh128)
		TEST128_VREG2(vupklsb128)
		TEST128_VREG2(vupklsh128)
		TEST_VREG2(vupkhsb128)
		TEST_VREG2(vupkhsh128)
		TEST_VREG2(vupklsb128)
		TEST_VREG2(vupklsh128)

		TEST_VREG2_IMM(vcfpuxws)
		TEST_VREG2_IMM(vcfpsxws)

		TEST128_VREG2_IMM(vcfpuxws128,0)
		TEST128_VREG2_IMM(vcfpuxws128,1)
		TEST128_VREG2_IMM(vcfpuxws128,2)
		TEST128_VREG2_IMM(vcfpuxws128,4)
		TEST128_VREG2_IMM(vcfpuxws128,8)
		TEST128_VREG2_IMM(vcfpuxws128,16)
		TEST128_VREG2_IMM(vcfpuxws128,31)

		TEST128_VREG2_IMM(vcfpsxws128,0)
		TEST128_VREG2_IMM(vcfpsxws128,1)
		TEST128_VREG2_IMM(vcfpsxws128,2)
		TEST128_VREG2_IMM(vcfpsxws128,4)
		TEST128_VREG2_IMM(vcfpsxws128,8)
		TEST128_VREG2_IMM(vcfpsxws128,16)
		TEST128_VREG2_IMM(vcfpsxws128,31)		

		TEST128_VREG2_IMM(vcsxwfp128,0)
		TEST128_VREG2_IMM(vcsxwfp128,1)
		TEST128_VREG2_IMM(vcsxwfp128,2)
		TEST128_VREG2_IMM(vcsxwfp128,4)
		TEST128_VREG2_IMM(vcsxwfp128,8)
		TEST128_VREG2_IMM(vcsxwfp128,16)
		TEST128_VREG2_IMM(vcsxwfp128,31)		

		TEST128_VREG2_IMM(vcuxwfp128,0)
		TEST128_VREG2_IMM(vcuxwfp128,1)
		TEST128_VREG2_IMM(vcuxwfp128,2)
		TEST128_VREG2_IMM(vcuxwfp128,4)
		TEST128_VREG2_IMM(vcuxwfp128,8)
		TEST128_VREG2_IMM(vcuxwfp128,16)
		TEST128_VREG2_IMM(vcuxwfp128,31)		

		TEST128_VREG4_ABCA_1(vmaddfp128)
		TEST128_VREG4_ABCA_2(vmaddfp128)
		TEST_VREG42(vmaddfp128)

		TEST128_VREG4_ABAC_1(vmaddcfp128)
		TEST_VREG42_ABAC(vmaddcfp128)

		TEST128_VREG4_ABCA_1(vnmsubfp128)
		TEST128_VREG4_ABCA_2(vnmsubfp128)
		TEST_VREG42(vnmsubfp128)

		TEST_VREG2_IMM(vcsxwfp)
		TEST_VREG2_IMM(vcsxwfp)
		TEST_VREG2_IMM(vcsxwfp)
		TEST_VREG2_IMM(vcsxwfp)
		TEST_VREG2_IMM(vcsxwfp)
		TEST_VREG2_IMM(vcsxwfp)
		TEST_VREG2_IMM(vcsxwfp)		

		TEST_VREG2_IMM(vcuxwfp)
		TEST_VREG2_IMM(vcuxwfp)
		TEST_VREG2_IMM(vcuxwfp)
		TEST_VREG2_IMM(vcuxwfp)
		TEST_VREG2_IMM(vcuxwfp)
		TEST_VREG2_IMM(vcuxwfp)
		TEST_VREG2_IMM(vcuxwfp)		

		TEST128_VREG2(vlogefp128)
		TEST128_VREG2(vexptefp128)
		TEST_VREG2(vlogefp128)
		TEST_VREG2(vexptefp128)
		
		
	}
}

void main()
{
	test(10,1000);
}
