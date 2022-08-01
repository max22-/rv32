#ifndef _ENV_PICORV32_TEST_H
#define _ENV_PICORV32_TEST_H

#ifndef TEST_FUNC_NAME
#  define TEST_FUNC_NAME mytest
#  define TEST_FUNC_TXT "mytest"
#  define TEST_FUNC_RET mytest_ret
#endif

#define RVTEST_RV32U
#define TESTNUM x28

#define RVTEST_CODE_BEGIN		\
	.text;				\
	.global TEST_FUNC_NAME;		\
	.global TEST_FUNC_RET;		\
TEST_FUNC_NAME:				\
	lui	a1,%hi(.test_name);	\
	addi	a1,a1,%lo(.test_name);	\
.prname_next:				\
	lb	a0,0(a1);		\
	beq	a0,zero,.prname_done;	\
	li      a7, 1;			\
	ecall;			        \
	addi	a1,a1,1;		\
	jal	zero,.prname_next;	\
.test_name:				\
	.ascii TEST_FUNC_TXT;		\
	.byte 0x00;			\
	.balign 4, 0;			\
.prname_done:				\
	addi	a0,zero,'.';		\
	li      a7, 1;			\
	ecall;				\
	ecall;
 

#define RVTEST_PASS			\
  li      a7, 1;			\
  li      a0, 'O';			\
  ecall;				\
  li	a0,'K';				\
  ecall;				\
  li	a0, '\n';			\
  ecall;				\
  jal	zero,TEST_FUNC_RET;

#define RVTEST_FAIL				\
  li    a7, 1;					\
  li	a0, 'E';				\
  ecall;					\
  li	a0, 'R';				\
  ecall;					\
  li	a0, 'R';				\
  ecall;					\
  li	a0, 'O';				\
  ecall;					\
  li	a0, 'R';				\
  ecall;					\
  li	a0,'\n';				\
  ecall;					\
  ebreak;

#define RVTEST_CODE_END
#define RVTEST_DATA_BEGIN .balign 4;
#define RVTEST_DATA_END

#endif
