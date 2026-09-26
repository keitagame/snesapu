// ============================================================================
// Spc700Ops.inc.h - SPC700 オペコード実装のメンバ関数プロトタイプ
// Spc700.h の class Spc700 { ... } 内で #include される。
// 実装は Spc700Ops.inc.cpp (Spc700.cpp から #include) にある。
// ============================================================================

// 転送・ロード系
int op_MOV_A_imm(); int op_MOV_X_imm(); int op_MOV_Y_imm();
int op_MOV_A_X(); int op_MOV_A_Y(); int op_MOV_X_A(); int op_MOV_Y_A();
int op_MOV_X_SP(); int op_MOV_SP_X();

int op_MOV_dp_A(); int op_MOV_A_dp(); int op_MOV_dp_X(); int op_MOV_X_dp();
int op_MOV_dp_Y(); int op_MOV_Y_dp();

int op_MOV_dpX_A(); int op_MOV_A_dpX(); int op_MOV_dpY_X(); int op_MOV_X_dpY();
int op_MOV_dpX_Y(); int op_MOV_Y_dpX();

int op_MOV_abs_A(); int op_MOV_A_abs(); int op_MOV_abs_X(); int op_MOV_X_abs();
int op_MOV_abs_Y(); int op_MOV_Y_abs();

int op_MOV_absX_A(); int op_MOV_absY_A(); int op_MOV_A_absX(); int op_MOV_A_absY();

int op_MOV_indX_A(); int op_MOV_A_indX(); int op_MOV_indXinc_A(); int op_MOV_A_indXinc();

int op_MOV_dpXind_A(); int op_MOV_A_dpXind();
int op_MOV_dpindY_A(); int op_MOV_A_dpindY();

int op_MOV_dp_dp(); int op_MOV_imm_dp();

int op_MOVW_YA_dp(); int op_MOVW_dp_YA();
int op_INCW_dp(); int op_DECW_dp();
int op_ADDW_YA_dp(); int op_SUBW_YA_dp(); int op_CMPW_YA_dp();

// 論理・算術 (A, imm/dp/dpX/abs/absX/absY/indX/indXinc/dpXind/dpindY)
int op_OR_A_imm();  int op_AND_A_imm();  int op_EOR_A_imm();  int op_CMP_A_imm();
int op_ADC_A_imm(); int op_SBC_A_imm();

int op_OR_A_dp();   int op_AND_A_dp();   int op_EOR_A_dp();   int op_CMP_A_dp();
int op_ADC_A_dp();  int op_SBC_A_dp();

int op_OR_A_dpX();  int op_AND_A_dpX();  int op_EOR_A_dpX();  int op_CMP_A_dpX();
int op_ADC_A_dpX(); int op_SBC_A_dpX();

int op_OR_A_abs();  int op_AND_A_abs();  int op_EOR_A_abs();  int op_CMP_A_abs();
int op_ADC_A_abs(); int op_SBC_A_abs();

int op_OR_A_absX(); int op_OR_A_absY();
int op_AND_A_absX();int op_AND_A_absY();
int op_EOR_A_absX();int op_EOR_A_absY();
int op_CMP_A_absX();int op_CMP_A_absY();
int op_ADC_A_absX();int op_ADC_A_absY();
int op_SBC_A_absX();int op_SBC_A_absY();

int op_OR_A_indX(); int op_AND_A_indX(); int op_EOR_A_indX(); int op_CMP_A_indX();
int op_ADC_A_indX();int op_SBC_A_indX();

int op_OR_A_dpXind(); int op_AND_A_dpXind(); int op_EOR_A_dpXind(); int op_CMP_A_dpXind();
int op_ADC_A_dpXind();int op_SBC_A_dpXind();

int op_OR_A_dpindY(); int op_AND_A_dpindY(); int op_EOR_A_dpindY(); int op_CMP_A_dpindY();
int op_ADC_A_dpindY();int op_SBC_A_dpindY();

int op_OR_dp_dp();  int op_AND_dp_dp();  int op_EOR_dp_dp();  int op_CMP_dp_dp();
int op_ADC_dp_dp();  int op_SBC_dp_dp();

int op_OR_dp_imm(); int op_AND_dp_imm(); int op_EOR_dp_imm(); int op_CMP_dp_imm();
int op_ADC_dp_imm();int op_SBC_dp_imm();

int op_OR_indX_indY(); int op_AND_indX_indY(); int op_EOR_indX_indY(); int op_CMP_indX_indY();
int op_ADC_indX_indY();int op_SBC_indX_indY();

int op_CMPX_imm(); int op_CMPY_imm();
int op_CMPX_dp();  int op_CMPY_dp();
int op_CMPX_abs(); int op_CMPY_abs();

// INC/DEC
int op_INC_A(); int op_DEC_A(); int op_INC_X(); int op_DEC_X(); int op_INC_Y(); int op_DEC_Y();
int op_INC_dp(); int op_DEC_dp(); int op_INC_dpX(); int op_DEC_dpX();
int op_INC_abs(); int op_DEC_abs();

// シフト・ローテート
int op_ASL_A(); int op_ASL_dp(); int op_ASL_dpX(); int op_ASL_abs();
int op_LSR_A(); int op_LSR_dp(); int op_LSR_dpX(); int op_LSR_abs();
int op_ROL_A(); int op_ROL_dp(); int op_ROL_dpX(); int op_ROL_abs();
int op_ROR_A(); int op_ROR_dp(); int op_ROR_dpX(); int op_ROR_abs();

int op_XCN_A();
int op_MUL_YA();
int op_DIV_YA_X();
int op_DAA_A(); int op_DAS_A();

// フラグ操作
int op_CLRC(); int op_SETC(); int op_NOTC();
int op_CLRP(); int op_SETP();
int op_CLRV();
int op_EI(); int op_DI();

// スタック
int op_PUSH_A(); int op_PUSH_X(); int op_PUSH_Y(); int op_PUSH_PSW();
int op_POP_A();  int op_POP_X();  int op_POP_Y();  int op_POP_PSW();

// 分岐
int op_BRA(); int op_BEQ(); int op_BNE(); int op_BCS(); int op_BCC();
int op_BVS(); int op_BVC(); int op_BMI(); int op_BPL();

// ビット set/clr (dp内, 8種)
int op_SET1(int bit); int op_CLR1(int bit);
// メンバ関数ポインタにbit値を静的束縛できないためのトランポリン (0x02,0x12系)
int op_SET1_b0(); int op_CLR1_b0(); int op_SET1_b1(); int op_CLR1_b1();
int op_SET1_b2(); int op_CLR1_b2(); int op_SET1_b3(); int op_CLR1_b3();
int op_SET1_b4(); int op_CLR1_b4(); int op_SET1_b5(); int op_CLR1_b5();
int op_SET1_b6(); int op_CLR1_b6(); int op_SET1_b7(); int op_CLR1_b7();

// ビットテスト/演算 (mem.bit)
int op_MOV1_C_mem(); int op_MOV1_mem_C();
int op_AND1_C_mem(); int op_AND1_C_notmem();
int op_OR1_C_mem();  int op_OR1_C_notmem();
int op_EOR1_C_mem();
int op_NOT1_mem();

// TSET1/TCLR1
int op_TSET1(); int op_TCLR1();

// 条件分岐系 (BBS/BBC, CBNE, DBNZ)
int op_BBS(int bit); int op_BBC(int bit);
int op_BBS_b0(); int op_BBC_b0(); int op_BBS_b1(); int op_BBC_b1();
int op_BBS_b2(); int op_BBC_b2(); int op_BBS_b3(); int op_BBC_b3();
int op_BBS_b4(); int op_BBC_b4(); int op_BBS_b5(); int op_BBC_b5();
int op_BBS_b6(); int op_BBC_b6(); int op_BBS_b7(); int op_BBC_b7();
int op_CBNE_dp(); int op_CBNE_dpX();
int op_DBNZ_Y(); int op_DBNZ_dp();

// ジャンプ・コール・割り込み
int op_JMP_abs(); int op_JMP_absXind();
int op_CALL(); int op_PCALL();
int op_TCALL(int n);
int op_TCALL_0();  int op_TCALL_1();  int op_TCALL_2();  int op_TCALL_3();
int op_TCALL_4();  int op_TCALL_5();  int op_TCALL_6();  int op_TCALL_7();
int op_TCALL_8();  int op_TCALL_9();  int op_TCALL_10(); int op_TCALL_11();
int op_TCALL_12(); int op_TCALL_13(); int op_TCALL_14(); int op_TCALL_15();
int op_RET(); int op_RETI();
int op_BRK();
int op_NOP();
int op_SLEEP(); int op_STOP();

int op_ILLEGAL();