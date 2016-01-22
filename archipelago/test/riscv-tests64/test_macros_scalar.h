#ifndef __TEST_MACROS_SCALAR_H
#define __TEST_MACROS_SCALAR_H

#-----------------------------------------------------------------------
# Helper macros
#-----------------------------------------------------------------------

#define TEST_CASE( testnum, testreg, correctval, code... ) \
test_ ## testnum: \
    code; \
    li  x29, correctval; \
    li  x28, testnum; \
    bne testreg, x29, fail;

#define TEST_CASE_JUMP( testnum, testreg, correctval, code... ) \
test_ ## testnum: \
    code; \
    li  x29, correctval; \
    li  x28, testnum; \
    beq testreg, x29, pass_ ## testnum; \
    j fail; \
pass_ ## testnum: \

# We use a macro hack to simpify code generation for various numbers
# of bubble cycles.

#define TEST_INSERT_NOPS_0
#define TEST_INSERT_NOPS_1  nop; TEST_INSERT_NOPS_0
#define TEST_INSERT_NOPS_2  nop; TEST_INSERT_NOPS_1
#define TEST_INSERT_NOPS_3  nop; TEST_INSERT_NOPS_2
#define TEST_INSERT_NOPS_4  nop; TEST_INSERT_NOPS_3
#define TEST_INSERT_NOPS_5  nop; TEST_INSERT_NOPS_4
#define TEST_INSERT_NOPS_6  nop; TEST_INSERT_NOPS_5
#define TEST_INSERT_NOPS_7  nop; TEST_INSERT_NOPS_6
#define TEST_INSERT_NOPS_8  nop; TEST_INSERT_NOPS_7
#define TEST_INSERT_NOPS_9  nop; TEST_INSERT_NOPS_8
#define TEST_INSERT_NOPS_10 nop; TEST_INSERT_NOPS_9

#-----------------------------------------------------------------------
# Tests for instructions with immediate operand
#-----------------------------------------------------------------------

#define TEST_IMM_OP( testnum, inst, result, val1, imm ) \
    TEST_CASE( testnum, x3, result, \
      li  x1, val1; \
      inst x3, x1, imm; \
    )

#define TEST_IMM_SRC1_EQ_DEST( testnum, inst, result, val1, imm ) \
    TEST_CASE( testnum, x1, result, \
      li  x1, val1; \
      inst x1, x1, imm; \
    )

#define TEST_IMM_DEST_BYPASS( testnum, nop_cycles, inst, result, val1, imm ) \
    TEST_CASE( testnum, x6, result, \
      li  x4, 0; \
1:    li  x1, val1; \
      inst x3, x1, imm; \
      TEST_INSERT_NOPS_ ## nop_cycles \
      addi  x6, x3, 0; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_IMM_SRC1_BYPASS( testnum, nop_cycles, inst, result, val1, imm ) \
    TEST_CASE( testnum, x3, result, \
      li  x4, 0; \
1:    li  x1, val1; \
      TEST_INSERT_NOPS_ ## nop_cycles \
      inst x3, x1, imm; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_IMM_ZEROSRC1( testnum, inst, result, imm ) \
    TEST_CASE( testnum, x1, result, \
      inst x1, x0, imm; \
    )

#define TEST_IMM_ZERODEST( testnum, inst, val1, imm ) \
    TEST_CASE( testnum, x0, 0, \
      li  x1, val1; \
      inst x0, x1, imm; \
    )

#-----------------------------------------------------------------------
# Tests for vector config instructions
#-----------------------------------------------------------------------

#define TEST_VVCFGIVL( testnum, nxpr, nfpr, bank, vl, result ) \
    TEST_CASE_JUMP( testnum, x1, result, \
      li x2, bank; \
      mtpcr x2, cr18; \
      li x1, vl; \
      vvcfgivl x1,x1,nxpr,nfpr; \
    )

#define TEST_VVCFG( testnum, nxpr, nfpr, bank, vl, result ) \
    TEST_CASE_JUMP( testnum, x1, result, \
      li x2, bank; \
      mtpcr x2, cr18; \
      li x1, nxpr; \
      li x2, nfpr; \
      vvcfg x1,x2; \
      li x1, vl; \
      vsetvl x1,x1; \
    )

#define TEST_VSETVL( testnum, nxpr, nfpr, bank, vl, result ) \
    TEST_CASE_JUMP( testnum, x1, result, \
      li x2, bank; \
      mtpcr x2, cr18; \
      vvcfgivl x0,x0,nxpr,nfpr; \
      li x1, vl; \
      vsetvl x1, x1; \
    )

#-----------------------------------------------------------------------
# Tests for an instruction with register operands
#-----------------------------------------------------------------------

#define TEST_R_OP( testnum, inst, result, val1 ) \
    TEST_CASE( testnum, x3, result, \
      li  x1, val1; \
      inst x3, x1; \
    )

#define TEST_R_SRC1_EQ_DEST( testnum, inst, result, val1 ) \
    TEST_CASE( testnum, x1, result, \
      li  x1, val1; \
      inst x1, x1; \
    )

#define TEST_R_DEST_BYPASS( testnum, nop_cycles, inst, result, val1 ) \
    TEST_CASE( testnum, x6, result, \
      li  x4, 0; \
1:    li  x1, val1; \
      inst x3, x1; \
      TEST_INSERT_NOPS_ ## nop_cycles \
      addi  x6, x3, 0; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#-----------------------------------------------------------------------
# Tests for an instruction with register-register operands
#-----------------------------------------------------------------------

#define TEST_RR_OP( testnum, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x3, result, \
      li  x1, val1; \
      li  x2, val2; \
      inst x3, x1, x2; \
    )

#define TEST_RR_SRC1_EQ_DEST( testnum, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x1, result, \
      li  x1, val1; \
      li  x2, val2; \
      inst x1, x1, x2; \
    )

#define TEST_RR_SRC2_EQ_DEST( testnum, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x2, result, \
      li  x1, val1; \
      li  x2, val2; \
      inst x2, x1, x2; \
    )

#define TEST_RR_SRC12_EQ_DEST( testnum, inst, result, val1 ) \
    TEST_CASE( testnum, x1, result, \
      li  x1, val1; \
      inst x1, x1, x1; \
    )

#define TEST_RR_DEST_BYPASS( testnum, nop_cycles, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x6, result, \
      li  x4, 0; \
1:    li  x1, val1; \
      li  x2, val2; \
      inst x3, x1, x2; \
      TEST_INSERT_NOPS_ ## nop_cycles \
      addi  x6, x3, 0; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_RR_SRC12_BYPASS( testnum, src1_nops, src2_nops, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x3, result, \
      li  x4, 0; \
1:    li  x1, val1; \
      TEST_INSERT_NOPS_ ## src1_nops \
      li  x2, val2; \
      TEST_INSERT_NOPS_ ## src2_nops \
      inst x3, x1, x2; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_RR_SRC21_BYPASS( testnum, src1_nops, src2_nops, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x3, result, \
      li  x4, 0; \
1:    li  x2, val2; \
      TEST_INSERT_NOPS_ ## src1_nops \
      li  x1, val1; \
      TEST_INSERT_NOPS_ ## src2_nops \
      inst x3, x1, x2; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_RR_ZEROSRC1( testnum, inst, result, val ) \
    TEST_CASE( testnum, x2, result, \
      li x1, val; \
      inst x2, x0, x1; \
    )

#define TEST_RR_ZEROSRC2( testnum, inst, result, val ) \
    TEST_CASE( testnum, x2, result, \
      li x1, val; \
      inst x2, x1, x0; \
    )

#define TEST_RR_ZEROSRC12( testnum, inst, result ) \
    TEST_CASE( testnum, x1, result, \
      inst x1, x0, x0; \
    )

#define TEST_RR_ZERODEST( testnum, inst, val1, val2 ) \
    TEST_CASE( testnum, x0, 0, \
      li x1, val1; \
      li x2, val2; \
      inst x0, x1, x2; \
    )

#-----------------------------------------------------------------------
# Test memory instructions
#-----------------------------------------------------------------------

#define TEST_LD_OP( testnum, inst, result, offset, base ) \
    TEST_CASE( testnum, x3, result, \
      la  x1, base; \
      inst x3, offset(x1); \
    )

#define TEST_ST_OP( testnum, load_inst, store_inst, result, offset, base ) \
    TEST_CASE( testnum, x3, result, \
      la  x1, base; \
      li  x2, result; \
      store_inst x2, offset(x1); \
      load_inst x3, offset(x1); \
    )

#define TEST_LD_DEST_BYPASS( testnum, nop_cycles, inst, result, offset, base ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x4, 0; \
1:  la  x1, base; \
    inst x3, offset(x1); \
    TEST_INSERT_NOPS_ ## nop_cycles \
    addi  x6, x3, 0; \
    li  x29, result; \
    bne x6, x29, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b; \

#define TEST_LD_SRC1_BYPASS( testnum, nop_cycles, inst, result, offset, base ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x4, 0; \
1:  la  x1, base; \
    TEST_INSERT_NOPS_ ## nop_cycles \
    inst x3, offset(x1); \
    li  x29, result; \
    bne x3, x29, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_ST_SRC12_BYPASS( testnum, src1_nops, src2_nops, load_inst, store_inst, result, offset, base ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x4, 0; \
1:  la  x1, result; \
    TEST_INSERT_NOPS_ ## src1_nops \
    la  x2, base; \
    TEST_INSERT_NOPS_ ## src2_nops \
    store_inst x1, offset(x2); \
    load_inst x3, offset(x2); \
    li  x29, result; \
    bne x3, x29, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_ST_SRC21_BYPASS( testnum, src1_nops, src2_nops, load_inst, store_inst, result, offset, base ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x4, 0; \
1:  la  x2, base; \
    TEST_INSERT_NOPS_ ## src1_nops \
    la  x1, result; \
    TEST_INSERT_NOPS_ ## src2_nops \
    store_inst x1, offset(x2); \
    load_inst x3, offset(x2); \
    li  x29, result; \
    bne x3, x29, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#-----------------------------------------------------------------------
# Test branch instructions
#-----------------------------------------------------------------------

#define TEST_BR1_OP_TAKEN( testnum, inst, val1 ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x1, val1; \
    inst x1, 2f; \
    bne x0, x28, fail; \
1:  bne x0, x28, 3f; \
2:  inst x1, 1b; \
    bne x0, x28, fail; \
3:

#define TEST_BR1_OP_NOTTAKEN( testnum, inst, val1 ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x1, val1; \
    inst x1, 1f; \
    bne x0, x28, 2f; \
1:  bne x0, x28, fail; \
2:  inst x1, 1b; \
3:

#define TEST_BR1_SRC1_BYPASS( testnum, nop_cycles, inst, val1 ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x4, 0; \
1:  li  x1, val1; \
    TEST_INSERT_NOPS_ ## nop_cycles \
    inst x1, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_BR2_OP_TAKEN( testnum, inst, val1, val2 ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x1, val1; \
    li  x2, val2; \
    inst x1, x2, 2f; \
    bne x0, x28, fail; \
1:  bne x0, x28, 3f; \
2:  inst x1, x2, 1b; \
    bne x0, x28, fail; \
3:

#define TEST_BR2_OP_NOTTAKEN( testnum, inst, val1, val2 ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x1, val1; \
    li  x2, val2; \
    inst x1, x2, 1f; \
    bne x0, x28, 2f; \
1:  bne x0, x28, fail; \
2:  inst x1, x2, 1b; \
3:

#define TEST_BR2_SRC12_BYPASS( testnum, src1_nops, src2_nops, inst, val1, val2 ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x4, 0; \
1:  li  x1, val1; \
    TEST_INSERT_NOPS_ ## src1_nops \
    li  x2, val2; \
    TEST_INSERT_NOPS_ ## src2_nops \
    inst x1, x2, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_BR2_SRC21_BYPASS( testnum, src1_nops, src2_nops, inst, val1, val2 ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x4, 0; \
1:  li  x2, val2; \
    TEST_INSERT_NOPS_ ## src1_nops \
    li  x1, val1; \
    TEST_INSERT_NOPS_ ## src2_nops \
    inst x1, x2, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#-----------------------------------------------------------------------
# Test jump instructions
#-----------------------------------------------------------------------

#define TEST_JR_SRC1_BYPASS( testnum, nop_cycles, inst ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x4, 0; \
1:  la  x6, 2f; \
    TEST_INSERT_NOPS_ ## nop_cycles \
    inst x6; \
    bne x0, x28, fail; \
2:  addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_JALR_SRC1_BYPASS( testnum, nop_cycles, inst ) \
test_ ## testnum: \
    li  x28, testnum; \
    li  x4, 0; \
1:  la  x6, 2f; \
    TEST_INSERT_NOPS_ ## nop_cycles \
    inst x19, x6, 0; \
    bne x0, x28, fail; \
2:  addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#-----------------------------------------------------------------------
# Tests floating-point instructions
#-----------------------------------------------------------------------

#define TEST_FP_OP_S_INTERNAL( testnum, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  x28, testnum; \
  la  t0, test_ ## testnum ## _data ;\
  flw f0, 0(t0); \
  flw f1, 4(t0); \
  flw f2, 8(t0); \
  lw  t3, 12(t0); \
  code; \
  bne t0, t3, fail; \
  b 1f; \
  .align 2; \
  test_ ## testnum ## _data: \
  .float val1; \
  .float val2; \
  .float val3; \
  .result; \
1:

#define TEST_FP_OP_D_INTERNAL( testnum, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  x28, testnum; \
  la  t0, test_ ## testnum ## _data ;\
  fld f0, 0(t0); \
  fld f1, 8(t0); \
  fld f2, 16(t0); \
  ld  t3, 24(t0); \
  code; \
  bne t0, t3, fail; \
  b 1f; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double val1; \
  .double val2; \
  .double val3; \
  .result; \
1:

#define TEST_FCVT_S_D( testnum, result, val1 ) \
  TEST_FP_OP_D_INTERNAL( testnum, double result, val1, 0.0, 0.0, \
                    fcvt.s.d f3, f0; fcvt.d.s f3, f3; mftx.d t0, f3)

#define TEST_FCVT_D_S( testnum, result, val1 ) \
  TEST_FP_OP_S_INTERNAL( testnum, float result, val1, 0.0, 0.0, \
                    fcvt.d.s f3, f0; fcvt.s.d f3, f3; mftx.s t0, f3)

#define TEST_FP_OP1_S( testnum, inst, result, val1 ) \
  TEST_FP_OP_S_INTERNAL( testnum, float result, val1, 0.0, 0.0, \
                    inst f3, f0; mftx.s t0, f3)

#define TEST_FP_OP1_D( testnum, inst, result, val1 ) \
  TEST_FP_OP_D_INTERNAL( testnum, double result, val1, 0.0, 0.0, \
                    inst f3, f0; mftx.d t0, f3)

#define TEST_FP_OP2_S( testnum, inst, result, val1, val2 ) \
  TEST_FP_OP_S_INTERNAL( testnum, float result, val1, val2, 0.0, \
                    inst f3, f0, f1; mftx.s t0, f3)

#define TEST_FP_OP2_D( testnum, inst, result, val1, val2 ) \
  TEST_FP_OP_D_INTERNAL( testnum, double result, val1, val2, 0.0, \
                    inst f3, f0, f1; mftx.d t0, f3)

#define TEST_FP_OP3_S( testnum, inst, result, val1, val2, val3 ) \
  TEST_FP_OP_S_INTERNAL( testnum, float result, val1, val2, val3, \
                    inst f3, f0, f1, f2; mftx.s t0, f3)

#define TEST_FP_OP3_D( testnum, inst, result, val1, val2, val3 ) \
  TEST_FP_OP_D_INTERNAL( testnum, double result, val1, val2, val3, \
                    inst f3, f0, f1, f2; mftx.d t0, f3)

#define TEST_FP_INT_OP_S( testnum, inst, result, val1, rm ) \
  TEST_FP_OP_S_INTERNAL( testnum, word result, val1, 0.0, 0.0, \
                    inst t0, f0, rm)

#define TEST_FP_INT_OP_D( testnum, inst, result, val1, rm ) \
  TEST_FP_OP_D_INTERNAL( testnum, dword result, val1, 0.0, 0.0, \
                    inst t0, f0, rm)

#define TEST_FP_CMP_OP_S( testnum, inst, result, val1, val2 ) \
  TEST_FP_OP_S_INTERNAL( testnum, word result, val1, val2, 0.0, \
                    inst t0, f0, f1)

#define TEST_FP_CMP_OP_D( testnum, inst, result, val1, val2 ) \
  TEST_FP_OP_D_INTERNAL( testnum, dword result, val1, val2, 0.0, \
                    inst t0, f0, f1)

#define TEST_INT_FP_OP_S( testnum, inst, result, val1 ) \
test_ ## testnum: \
  li  x28, testnum; \
  la  t0, test_ ## testnum ## _data ;\
  lw  t3, 0(t0); \
  li  t0, val1; \
  inst f0, t0; \
  mftx.s t0, f0; \
  bne t0, t3, fail; \
  b 1f; \
  .align 2; \
  test_ ## testnum ## _data: \
  .float result; \
1:

#define TEST_INT_FP_OP_D( testnum, inst, result, val1 ) \
test_ ## testnum: \
  li  x28, testnum; \
  la  t0, test_ ## testnum ## _data ;\
  ld  t3, 0(t0); \
  li  t0, val1; \
  inst f0, t0; \
  mftx.d t0, f0; \
  bne t0, t3, fail; \
  b 1f; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double result; \
1:

#define TEST_ILLEGAL_VT_REGID( testnum, nxreg, nfreg, inst, reg1, reg2, reg3) \
  mfpcr t0,cr0; \
  li t1,1; \
  slli t2,t1,8; \
  or t0,t0,t1; \
  mtpcr t0,cr0; \
  la t0, handler ## testnum; \
  mtpcr t0, cr3; \
  li t0, 4; \
  vvcfgivl t0, t0, nxreg, nfreg; \
  la t0, src1; \
  la t1, src2; \
  vld vx2, t0; \
  vld vx3, t1; \
  lui a0,%hi(vtcode1 ## testnum); \
  vf %lo(vtcode1 ## testnum)(a0); \
  la t2, dest; \
  vsd vx2, t2; \
  fence.v.l; \
vtcode1 ## testnum: \
  add v0, v0, v1; \
illegal ## testnum: \
  inst reg1, reg2, reg3; \
  stop; \
vtcode2 ## testnum: \
  add v0, v0, v1; \
  stop; \
handler ## testnum: \
  vxcptkill; \
  li s8,2; \
  mfpcr t0,cr6; \
  li t1,26; \
  bne t0,t1,fail; \
  mfpcr t0,cr2; \
  la t1,illegal ## testnum; \
  bne t0,t1,fail; \
  li t0,4; \
  vvcfgivl t0,t0,32,0; \
  la t0,src1; \
  la t1,src2; \
  vld vx2,t0; \
  vld vx3,t1; \
  lui a0,%hi(vtcode2 ## testnum); \
  vf %lo(vtcode2 ## testnum)(a0); \
  la t2,dest; \
  vsd vx2,t2; \
  fence.v.l; \
  ld a1,0(t2); \
  li a2,5; \
  li s8,2; \
  bne a1,a2,fail; \
  ld a1,8(t2); \
  li s8,3; \
  bne a1,a2,fail; \
  ld a1,16(t2); \
  li s8,4; \
  bne a1,a2,fail; \
  ld a1,24(t2); \
  li s8,5; \
  bne a1,a2,fail; \

#define TEST_ILLEGAL_TVEC_REGID( testnum, nxreg, nfreg, inst, reg1, reg2, aux) \
  mfpcr t0,cr0; \
  li t1,1; \
  slli t2,t1,8; \
  or t0,t0,t1; \
  mtpcr t0,cr0; \
  la t0, handler ## testnum; \
  mtpcr t0, cr3; \
  li t0, 4; \
  vvcfgivl t0, t0, nxreg, nfreg; \
  la t0, src1; \
  la t1, src2; \
  vld vx2, t0; \
  vld vx3, t1; \
  lui a0,%hi(vtcode1 ## testnum); \
  vf %lo(vtcode1 ## testnum)(a0); \
  la reg2, dest; \
illegal ## testnum: \
  inst reg1, reg2; \
  la t2, dest; \
  vsd vx2, t2; \
  fence.v.l; \
vtcode1 ## testnum: \
  add v0, v0, v1; \
  stop; \
vtcode2 ## testnum: \
  add v0, v0, v1; \
  stop; \
handler ## testnum: \
  vxcptkill; \
  li s8,2; \
  mfpcr t0,cr6; \
  li t1,27; \
  bne t0,t1,fail; \
  mfpcr t0, cr2; \
  li t1, aux; \
  bne t0, t1, fail; \
  li t0,4; \
  vvcfgivl t0,t0,32,0; \
  la t0,src1; \
  la t1,src2; \
  vld vx2,t0; \
  vld vx3,t1; \
  lui a0,%hi(vtcode2 ## testnum); \
  vf %lo(vtcode2 ## testnum)(a0); \
  la t2,dest; \
  vsd vx2,t2; \
  fence.v.l; \
  ld a1,0(t2); \
  li a2,5; \
  li s8,2; \
  bne a1,a2,fail; \
  ld a1,8(t2); \
  li s8,3; \
  bne a1,a2,fail; \
  ld a1,16(t2); \
  li s8,4; \
  bne a1,a2,fail; \
  ld a1,24(t2); \
  li s8,5; \
  bne a1,a2,fail; \

#-----------------------------------------------------------------------
# Pass and fail code (assumes test num is in x28)
#-----------------------------------------------------------------------

#ifdef __USER_PHYSICAL_SCALAR

#undef TEST_FAIL

#define TEST_FAIL \
        mtpcr x28, cr30; \
1:      beq x0, x0, 1b; \
        nop; \

#endif


#define TEST_PASSFAIL \
        bne x0, x28, pass; \
fail: \
        TEST_FAIL \
pass: \
        TEST_PASS \

#endif
