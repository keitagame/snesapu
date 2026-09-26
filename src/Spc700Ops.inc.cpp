// ============================================================================
// Spc700Ops.inc.cpp - SPC700 オペコード実装本体
// Spc700.cpp の末尾で #include される (namespace snesapu 内)。
// libspc.js の T[opcode] = function(){...} を1対1でメンバ関数に移植。
// ============================================================================
namespace snesapu {

// ---- 転送・ロード ----
int Spc700::op_MOV_A_imm() { uint8_t v = fetch8(); A = setNZ8(v); return 2; }
int Spc700::op_MOV_X_imm() { uint8_t v = fetch8(); X = setNZ8(v); return 2; }
int Spc700::op_MOV_Y_imm() { uint8_t v = fetch8(); Y = setNZ8(v); return 2; }

int Spc700::op_MOV_A_X() { A = setNZ8(X); return 2; }
int Spc700::op_MOV_A_Y() { A = setNZ8(Y); return 2; }
int Spc700::op_MOV_X_A() { X = setNZ8(A); return 2; }
int Spc700::op_MOV_Y_A() { Y = setNZ8(A); return 2; }
int Spc700::op_MOV_X_SP() { X = setNZ8(SP); return 2; }
int Spc700::op_MOV_SP_X() { SP = X; return 2; }

int Spc700::op_MOV_dp_A() { uint32_t a = dp(fetch8()); write(a, A); return 4; }
int Spc700::op_MOV_A_dp() { uint32_t a = dp(fetch8()); A = setNZ8(read(a)); return 3; }
int Spc700::op_MOV_dp_X() { uint32_t a = dp(fetch8()); write(a, X); return 4; }
int Spc700::op_MOV_X_dp() { uint32_t a = dp(fetch8()); X = setNZ8(read(a)); return 3; }
int Spc700::op_MOV_dp_Y() { uint32_t a = dp(fetch8()); write(a, Y); return 4; }
int Spc700::op_MOV_Y_dp() { uint32_t a = dp(fetch8()); Y = setNZ8(read(a)); return 3; }

int Spc700::op_MOV_dpX_A() { uint32_t a = dp((fetch8() + X) & 0xff); write(a, A); return 5; }
int Spc700::op_MOV_A_dpX() { uint32_t a = dp((fetch8() + X) & 0xff); A = setNZ8(read(a)); return 4; }
int Spc700::op_MOV_dpY_X() { uint32_t a = dp((fetch8() + Y) & 0xff); write(a, X); return 5; }
int Spc700::op_MOV_X_dpY() { uint32_t a = dp((fetch8() + Y) & 0xff); X = setNZ8(read(a)); return 4; }
int Spc700::op_MOV_dpX_Y() { uint32_t a = dp((fetch8() + X) & 0xff); write(a, Y); return 5; }
int Spc700::op_MOV_Y_dpX() { uint32_t a = dp((fetch8() + X) & 0xff); Y = setNZ8(read(a)); return 4; }

int Spc700::op_MOV_abs_A() { uint32_t a = fetch16(); write(a, A); return 5; }
int Spc700::op_MOV_A_abs() { uint32_t a = fetch16(); A = setNZ8(read(a)); return 4; }
int Spc700::op_MOV_abs_X() { uint32_t a = fetch16(); write(a, X); return 5; }
int Spc700::op_MOV_X_abs() { uint32_t a = fetch16(); X = setNZ8(read(a)); return 4; }
int Spc700::op_MOV_abs_Y() { uint32_t a = fetch16(); write(a, Y); return 5; }
int Spc700::op_MOV_Y_abs() { uint32_t a = fetch16(); Y = setNZ8(read(a)); return 4; }

int Spc700::op_MOV_absX_A() { uint32_t a = (fetch16() + X) & 0xffff; write(a, A); return 6; }
int Spc700::op_MOV_absY_A() { uint32_t a = (fetch16() + Y) & 0xffff; write(a, A); return 6; }
int Spc700::op_MOV_A_absX() { uint32_t a = (fetch16() + X) & 0xffff; A = setNZ8(read(a)); return 5; }
int Spc700::op_MOV_A_absY() { uint32_t a = (fetch16() + Y) & 0xffff; A = setNZ8(read(a)); return 5; }

int Spc700::op_MOV_indX_A() { write(dp(X), A); return 4; }
int Spc700::op_MOV_A_indX() { A = setNZ8(read(dp(X))); return 3; }
int Spc700::op_MOV_indXinc_A() { write(dp(X), A); X = (X + 1) & 0xff; return 4; }
int Spc700::op_MOV_A_indXinc() { A = setNZ8(read(dp(X))); X = (X + 1) & 0xff; return 4; }

int Spc700::op_MOV_dpXind_A() {
    uint32_t ptr = dp((fetch8() + X) & 0xff);
    uint32_t a = rd16dp(ptr);
    write(a, A); return 7;
}
int Spc700::op_MOV_A_dpXind() {
    uint32_t ptr = dp((fetch8() + X) & 0xff);
    uint32_t a = rd16dp(ptr);
    A = setNZ8(read(a)); return 6;
}
int Spc700::op_MOV_dpindY_A() {
    uint32_t ptr = dp(fetch8());
    uint32_t base = rd16dp(ptr);
    uint32_t a = (base + Y) & 0xffff;
    write(a, A); return 7;
}
int Spc700::op_MOV_A_dpindY() {
    uint32_t ptr = dp(fetch8());
    uint32_t base = rd16dp(ptr);
    uint32_t a = (base + Y) & 0xffff;
    A = setNZ8(read(a)); return 6;
}

int Spc700::op_MOV_dp_dp() { uint32_t src = dp(fetch8()); uint32_t dst = dp(fetch8()); write(dst, read(src)); return 5; }
int Spc700::op_MOV_imm_dp() { uint8_t v = fetch8(); uint32_t a = dp(fetch8()); write(a, v); return 5; }

int Spc700::op_MOVW_YA_dp() {
    uint32_t a = dp(fetch8());
    uint8_t lo = read(a); uint8_t hi = read(dpNext(a));
    A = lo; Y = hi;
    flagZ = ((lo | hi) == 0) ? 1 : 0;
    flagN = (hi & 0x80) ? 1 : 0;
    return 5;
}
int Spc700::op_MOVW_dp_YA() {
    uint32_t a = dp(fetch8());
    read(a); // JS版に合わせ読み捨てを再現(副作用なし)
    write(a, A); write(dpNext(a), Y);
    return 5;
}
int Spc700::op_INCW_dp() {
    uint32_t a = dp(fetch8());
    uint16_t w = (uint16_t)((rd16dp(a) + 1) & 0xffff);
    wr16dp(a, w);
    flagZ = (w == 0) ? 1 : 0; flagN = (w & 0x8000) ? 1 : 0;
    return 6;
}
int Spc700::op_DECW_dp() {
    uint32_t a = dp(fetch8());
    uint16_t w = (uint16_t)((rd16dp(a) - 1) & 0xffff);
    wr16dp(a, w);
    flagZ = (w == 0) ? 1 : 0; flagN = (w & 0x8000) ? 1 : 0;
    return 6;
}
int Spc700::op_ADDW_YA_dp() {
    uint32_t a = dp(fetch8());
    int ya = (Y << 8) | A;
    int m = rd16dp(a);
    int result = ya + m;
    int r16 = result & 0xffff;
    flagC = (result > 0xffff) ? 1 : 0;
    flagV = ((~(ya ^ m) & (ya ^ r16) & 0x8000) != 0) ? 1 : 0;
    flagH = ((((ya & 0xfff) + (m & 0xfff)) > 0xfff)) ? 1 : 0;
    Y = (r16 >> 8) & 0xff; A = r16 & 0xff;
    flagZ = (r16 == 0) ? 1 : 0; flagN = (r16 & 0x8000) ? 1 : 0;
    return 5;
}
int Spc700::op_SUBW_YA_dp() {
    uint32_t a = dp(fetch8());
    int ya = (Y << 8) | A;
    int m = rd16dp(a);
    int mInv = (~m) & 0xffff;
    int result = ya + mInv + 1;
    int r16 = result & 0xffff;
    flagC = (result > 0xffff) ? 1 : 0;
    flagV = ((~(ya ^ mInv) & (ya ^ r16) & 0x8000) != 0) ? 1 : 0;
    flagH = ((((ya & 0xfff) + (mInv & 0xfff) + 1) > 0xfff)) ? 1 : 0;
    Y = (r16 >> 8) & 0xff; A = r16 & 0xff;
    flagZ = (r16 == 0) ? 1 : 0; flagN = (r16 & 0x8000) ? 1 : 0;
    return 5;
}
int Spc700::op_CMPW_YA_dp() {
    uint32_t a = dp(fetch8());
    int ya = (Y << 8) | A;
    int m = rd16dp(a);
    int result = (ya - m) & 0xffff;
    flagC = (ya >= m) ? 1 : 0;
    flagZ = (result == 0) ? 1 : 0;
    flagN = (result & 0x8000) ? 1 : 0;
    return 4;
}

// ---- 論理・算術 (imm) ----
int Spc700::op_OR_A_imm()  { uint8_t v = fetch8(); A = setNZ8(A | v); return 2; }
int Spc700::op_AND_A_imm() { uint8_t v = fetch8(); A = setNZ8(A & v); return 2; }
int Spc700::op_EOR_A_imm() { uint8_t v = fetch8(); A = setNZ8(A ^ v); return 2; }
int Spc700::op_CMP_A_imm() { uint8_t v = fetch8(); int r = (A - v) & 0x1ff; flagC = (A >= v) ? 1 : 0; setNZ8(r); return 2; }
int Spc700::op_ADC_A_imm() { uint8_t v = fetch8(); A = adc(A, v, flagC); return 2; }
int Spc700::op_SBC_A_imm() { uint8_t v = fetch8(); A = sbc(A, v, flagC); return 2; }

// ---- 論理・算術 (dp) ----
int Spc700::op_OR_A_dp()  { uint8_t v = read(dp(fetch8())); A = setNZ8(A | v); return 3; }
int Spc700::op_AND_A_dp() { uint8_t v = read(dp(fetch8())); A = setNZ8(A & v); return 3; }
int Spc700::op_EOR_A_dp() { uint8_t v = read(dp(fetch8())); A = setNZ8(A ^ v); return 3; }
int Spc700::op_CMP_A_dp() { uint8_t v = read(dp(fetch8())); flagC = (A >= v) ? 1 : 0; setNZ8((A - v) & 0x1ff); return 3; }
int Spc700::op_ADC_A_dp() { uint8_t v = read(dp(fetch8())); A = adc(A, v, flagC); return 3; }
int Spc700::op_SBC_A_dp() { uint8_t v = read(dp(fetch8())); A = sbc(A, v, flagC); return 3; }

// ---- 論理・算術 (dp+X) ----
int Spc700::op_OR_A_dpX()  { uint8_t v = read(dp((fetch8() + X) & 0xff)); A = setNZ8(A | v); return 4; }
int Spc700::op_AND_A_dpX() { uint8_t v = read(dp((fetch8() + X) & 0xff)); A = setNZ8(A & v); return 4; }
int Spc700::op_EOR_A_dpX() { uint8_t v = read(dp((fetch8() + X) & 0xff)); A = setNZ8(A ^ v); return 4; }
int Spc700::op_CMP_A_dpX() { uint8_t v = read(dp((fetch8() + X) & 0xff)); flagC = (A >= v) ? 1 : 0; setNZ8((A - v) & 0x1ff); return 4; }
int Spc700::op_ADC_A_dpX() { uint8_t v = read(dp((fetch8() + X) & 0xff)); A = adc(A, v, flagC); return 4; }
int Spc700::op_SBC_A_dpX() { uint8_t v = read(dp((fetch8() + X) & 0xff)); A = sbc(A, v, flagC); return 4; }

// ---- 論理・算術 (abs) ----
int Spc700::op_OR_A_abs()  { uint8_t v = read(fetch16()); A = setNZ8(A | v); return 4; }
int Spc700::op_AND_A_abs() { uint8_t v = read(fetch16()); A = setNZ8(A & v); return 4; }
int Spc700::op_EOR_A_abs() { uint8_t v = read(fetch16()); A = setNZ8(A ^ v); return 4; }
int Spc700::op_CMP_A_abs() { uint8_t v = read(fetch16()); flagC = (A >= v) ? 1 : 0; setNZ8((A - v) & 0x1ff); return 4; }
int Spc700::op_ADC_A_abs() { uint8_t v = read(fetch16()); A = adc(A, v, flagC); return 4; }
int Spc700::op_SBC_A_abs() { uint8_t v = read(fetch16()); A = sbc(A, v, flagC); return 4; }

// ---- 論理・算術 (abs+X/Y) ----
int Spc700::op_OR_A_absX()  { uint8_t v = read((fetch16() + X) & 0xffff); A = setNZ8(A | v); return 5; }
int Spc700::op_OR_A_absY()  { uint8_t v = read((fetch16() + Y) & 0xffff); A = setNZ8(A | v); return 5; }
int Spc700::op_AND_A_absX() { uint8_t v = read((fetch16() + X) & 0xffff); A = setNZ8(A & v); return 5; }
int Spc700::op_AND_A_absY() { uint8_t v = read((fetch16() + Y) & 0xffff); A = setNZ8(A & v); return 5; }
int Spc700::op_EOR_A_absX() { uint8_t v = read((fetch16() + X) & 0xffff); A = setNZ8(A ^ v); return 5; }
int Spc700::op_EOR_A_absY() { uint8_t v = read((fetch16() + Y) & 0xffff); A = setNZ8(A ^ v); return 5; }
int Spc700::op_CMP_A_absX() { uint8_t v = read((fetch16() + X) & 0xffff); flagC = (A >= v) ? 1 : 0; setNZ8((A - v) & 0x1ff); return 5; }
int Spc700::op_CMP_A_absY() { uint8_t v = read((fetch16() + Y) & 0xffff); flagC = (A >= v) ? 1 : 0; setNZ8((A - v) & 0x1ff); return 5; }
int Spc700::op_ADC_A_absX() { uint8_t v = read((fetch16() + X) & 0xffff); A = adc(A, v, flagC); return 5; }
int Spc700::op_ADC_A_absY() { uint8_t v = read((fetch16() + Y) & 0xffff); A = adc(A, v, flagC); return 5; }
int Spc700::op_SBC_A_absX() { uint8_t v = read((fetch16() + X) & 0xffff); A = sbc(A, v, flagC); return 5; }
int Spc700::op_SBC_A_absY() { uint8_t v = read((fetch16() + Y) & 0xffff); A = sbc(A, v, flagC); return 5; }

// ---- 論理・算術 (dp,X) ----
int Spc700::op_OR_A_indX()  { uint8_t v = read(dp(X)); A = setNZ8(A | v); return 3; }
int Spc700::op_AND_A_indX() { uint8_t v = read(dp(X)); A = setNZ8(A & v); return 3; }
int Spc700::op_EOR_A_indX() { uint8_t v = read(dp(X)); A = setNZ8(A ^ v); return 3; }
int Spc700::op_CMP_A_indX() { uint8_t v = read(dp(X)); flagC = (A >= v) ? 1 : 0; setNZ8((A - v) & 0x1ff); return 3; }
int Spc700::op_ADC_A_indX() { uint8_t v = read(dp(X)); A = adc(A, v, flagC); return 3; }
int Spc700::op_SBC_A_indX() { uint8_t v = read(dp(X)); A = sbc(A, v, flagC); return 3; }

// ---- 論理・算術 ([dp+X]) ----
int Spc700::op_OR_A_dpXind() {
    uint32_t ptr = dp((fetch8() + X) & 0xff); uint32_t a = rd16dp(ptr);
    uint8_t v = read(a); A = setNZ8(A | v); return 6;
}
int Spc700::op_AND_A_dpXind() {
    uint32_t ptr = dp((fetch8() + X) & 0xff); uint32_t a = rd16dp(ptr);
    uint8_t v = read(a); A = setNZ8(A & v); return 6;
}
int Spc700::op_EOR_A_dpXind() {
    uint32_t ptr = dp((fetch8() + X) & 0xff); uint32_t a = rd16dp(ptr);
    uint8_t v = read(a); A = setNZ8(A ^ v); return 6;
}
int Spc700::op_CMP_A_dpXind() {
    uint32_t ptr = dp((fetch8() + X) & 0xff); uint32_t a = rd16dp(ptr);
    uint8_t v = read(a); flagC = (A >= v) ? 1 : 0; setNZ8((A - v) & 0x1ff); return 6;
}
int Spc700::op_ADC_A_dpXind() {
    uint32_t ptr = dp((fetch8() + X) & 0xff); uint32_t a = rd16dp(ptr);
    uint8_t v = read(a); A = adc(A, v, flagC); return 6;
}
int Spc700::op_SBC_A_dpXind() {
    uint32_t ptr = dp((fetch8() + X) & 0xff); uint32_t a = rd16dp(ptr);
    uint8_t v = read(a); A = sbc(A, v, flagC); return 6;
}

// ---- 論理・算術 ([dp]+Y) ----
int Spc700::op_OR_A_dpindY() {
    uint32_t ptr = dp(fetch8()); uint32_t base = rd16dp(ptr);
    uint8_t v = read((base + Y) & 0xffff); A = setNZ8(A | v); return 6;
}
int Spc700::op_AND_A_dpindY() {
    uint32_t ptr = dp(fetch8()); uint32_t base = rd16dp(ptr);
    uint8_t v = read((base + Y) & 0xffff); A = setNZ8(A & v); return 6;
}
int Spc700::op_EOR_A_dpindY() {
    uint32_t ptr = dp(fetch8()); uint32_t base = rd16dp(ptr);
    uint8_t v = read((base + Y) & 0xffff); A = setNZ8(A ^ v); return 6;
}
int Spc700::op_CMP_A_dpindY() {
    uint32_t ptr = dp(fetch8()); uint32_t base = rd16dp(ptr);
    uint8_t v = read((base + Y) & 0xffff); flagC = (A >= v) ? 1 : 0; setNZ8((A - v) & 0x1ff); return 6;
}
int Spc700::op_ADC_A_dpindY() {
    uint32_t ptr = dp(fetch8()); uint32_t base = rd16dp(ptr);
    uint8_t v = read((base + Y) & 0xffff); A = adc(A, v, flagC); return 6;
}
int Spc700::op_SBC_A_dpindY() {
    uint32_t ptr = dp(fetch8()); uint32_t base = rd16dp(ptr);
    uint8_t v = read((base + Y) & 0xffff); A = sbc(A, v, flagC); return 6;
}

// ---- dp, dp ----
int Spc700::op_OR_dp_dp()  { uint32_t src = dp(fetch8()); uint32_t dst = dp(fetch8()); write(dst, setNZ8(read(dst) | read(src))); return 6; }
int Spc700::op_AND_dp_dp() { uint32_t src = dp(fetch8()); uint32_t dst = dp(fetch8()); write(dst, setNZ8(read(dst) & read(src))); return 6; }
int Spc700::op_EOR_dp_dp() { uint32_t src = dp(fetch8()); uint32_t dst = dp(fetch8()); write(dst, setNZ8(read(dst) ^ read(src))); return 6; }
int Spc700::op_CMP_dp_dp() {
    uint32_t src = dp(fetch8()); uint32_t dst = dp(fetch8());
    int a = read(dst), b = read(src); flagC = (a >= b) ? 1 : 0; setNZ8((a - b) & 0x1ff); return 6;
}
int Spc700::op_ADC_dp_dp() { uint32_t src = dp(fetch8()); uint32_t dst = dp(fetch8()); write(dst, adc(read(dst), read(src), flagC)); return 6; }
int Spc700::op_SBC_dp_dp() { uint32_t src = dp(fetch8()); uint32_t dst = dp(fetch8()); write(dst, sbc(read(dst), read(src), flagC)); return 6; }

// ---- dp, imm ----
int Spc700::op_OR_dp_imm()  { uint8_t v = fetch8(); uint32_t a = dp(fetch8()); write(a, setNZ8(read(a) | v)); return 5; }
int Spc700::op_AND_dp_imm() { uint8_t v = fetch8(); uint32_t a = dp(fetch8()); write(a, setNZ8(read(a) & v)); return 5; }
int Spc700::op_EOR_dp_imm() { uint8_t v = fetch8(); uint32_t a = dp(fetch8()); write(a, setNZ8(read(a) ^ v)); return 5; }
int Spc700::op_CMP_dp_imm() {
    uint8_t v = fetch8(); uint32_t a = dp(fetch8());
    int m = read(a); flagC = (m >= v) ? 1 : 0; setNZ8((m - v) & 0x1ff); return 5;
}
int Spc700::op_ADC_dp_imm() { uint8_t v = fetch8(); uint32_t a = dp(fetch8()); write(a, adc(read(a), v, flagC)); return 5; }
int Spc700::op_SBC_dp_imm() { uint8_t v = fetch8(); uint32_t a = dp(fetch8()); write(a, sbc(read(a), v, flagC)); return 5; }

// ---- (X), (Y) ----
int Spc700::op_OR_indX_indY()  { uint32_t dstA = dp(X); uint32_t srcA = dp(Y); write(dstA, setNZ8(read(dstA) | read(srcA))); return 5; }
int Spc700::op_AND_indX_indY() { uint32_t dstA = dp(X); uint32_t srcA = dp(Y); write(dstA, setNZ8(read(dstA) & read(srcA))); return 5; }
int Spc700::op_EOR_indX_indY() { uint32_t dstA = dp(X); uint32_t srcA = dp(Y); write(dstA, setNZ8(read(dstA) ^ read(srcA))); return 5; }
int Spc700::op_CMP_indX_indY() {
    uint32_t dstA = dp(X); uint32_t srcA = dp(Y);
    int a = read(dstA), b = read(srcA); flagC = (a >= b) ? 1 : 0; setNZ8((a - b) & 0x1ff); return 5;
}
int Spc700::op_ADC_indX_indY() { uint32_t dstA = dp(X); uint32_t srcA = dp(Y); write(dstA, adc(read(dstA), read(srcA), flagC)); return 5; }
int Spc700::op_SBC_indX_indY() { uint32_t dstA = dp(X); uint32_t srcA = dp(Y); write(dstA, sbc(read(dstA), read(srcA), flagC)); return 5; }

// ---- CMPX/CMPY ----
int Spc700::op_CMPX_imm() { uint8_t v = fetch8(); flagC = (X >= v) ? 1 : 0; setNZ8((X - v) & 0x1ff); return 2; }
int Spc700::op_CMPY_imm() { uint8_t v = fetch8(); flagC = (Y >= v) ? 1 : 0; setNZ8((Y - v) & 0x1ff); return 2; }
int Spc700::op_CMPX_dp()  { uint8_t v = read(dp(fetch8())); flagC = (X >= v) ? 1 : 0; setNZ8((X - v) & 0x1ff); return 3; }
int Spc700::op_CMPY_dp()  { uint8_t v = read(dp(fetch8())); flagC = (Y >= v) ? 1 : 0; setNZ8((Y - v) & 0x1ff); return 3; }
int Spc700::op_CMPX_abs() { uint8_t v = read(fetch16()); flagC = (X >= v) ? 1 : 0; setNZ8((X - v) & 0x1ff); return 4; }
int Spc700::op_CMPY_abs() { uint8_t v = read(fetch16()); flagC = (Y >= v) ? 1 : 0; setNZ8((Y - v) & 0x1ff); return 4; }

// ---- INC/DEC ----
int Spc700::op_INC_A() { A = setNZ8(A + 1); return 2; }
int Spc700::op_DEC_A() { A = setNZ8(A - 1); return 2; }
int Spc700::op_INC_X() { X = setNZ8(X + 1); return 2; }
int Spc700::op_DEC_X() { X = setNZ8(X - 1); return 2; }
int Spc700::op_INC_Y() { Y = setNZ8(Y + 1); return 2; }
int Spc700::op_DEC_Y() { Y = setNZ8(Y - 1); return 2; }

int Spc700::op_INC_dp() { uint32_t a = dp(fetch8()); write(a, setNZ8(read(a) + 1)); return 4; }
int Spc700::op_DEC_dp() { uint32_t a = dp(fetch8()); write(a, setNZ8(read(a) - 1)); return 4; }
int Spc700::op_INC_dpX() { uint32_t a = dp((fetch8() + X) & 0xff); write(a, setNZ8(read(a) + 1)); return 5; }
int Spc700::op_DEC_dpX() { uint32_t a = dp((fetch8() + X) & 0xff); write(a, setNZ8(read(a) - 1)); return 5; }
int Spc700::op_INC_abs() { uint32_t a = fetch16(); write(a, setNZ8(read(a) + 1)); return 5; }
int Spc700::op_DEC_abs() { uint32_t a = fetch16(); write(a, setNZ8(read(a) - 1)); return 5; }

// ---- シフト・ローテート ----
namespace {
    // 以下のヘルパーは Spc700インスタンスに依存するため、各opにインライン展開している。
}

int Spc700::op_ASL_A() {
    uint8_t v = A;
    uint8_t c = (v & 0x80) ? 1 : 0; uint8_t r = (uint8_t)((v << 1) & 0xff);
    flagC = c; A = setNZ8(r); return 2;
}
int Spc700::op_ASL_dp() {
    uint32_t a = dp(fetch8()); uint8_t v = read(a);
    uint8_t c = (v & 0x80) ? 1 : 0; uint8_t r = (uint8_t)((v << 1) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 4;
}
int Spc700::op_ASL_dpX() {
    uint32_t a = dp((fetch8() + X) & 0xff); uint8_t v = read(a);
    uint8_t c = (v & 0x80) ? 1 : 0; uint8_t r = (uint8_t)((v << 1) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 5;
}
int Spc700::op_ASL_abs() {
    uint32_t a = fetch16(); uint8_t v = read(a);
    uint8_t c = (v & 0x80) ? 1 : 0; uint8_t r = (uint8_t)((v << 1) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 5;
}

int Spc700::op_LSR_A() {
    uint8_t v = A;
    uint8_t c = v & 1; uint8_t r = (uint8_t)((v >> 1) & 0xff);
    flagC = c; A = setNZ8(r); return 2;
}
int Spc700::op_LSR_dp() {
    uint32_t a = dp(fetch8()); uint8_t v = read(a);
    uint8_t c = v & 1; uint8_t r = (uint8_t)((v >> 1) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 4;
}
int Spc700::op_LSR_dpX() {
    uint32_t a = dp((fetch8() + X) & 0xff); uint8_t v = read(a);
    uint8_t c = v & 1; uint8_t r = (uint8_t)((v >> 1) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 5;
}
int Spc700::op_LSR_abs() {
    uint32_t a = fetch16(); uint8_t v = read(a);
    uint8_t c = v & 1; uint8_t r = (uint8_t)((v >> 1) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 5;
}

int Spc700::op_ROL_A() {
    uint8_t v = A;
    uint8_t c = (v & 0x80) ? 1 : 0; uint8_t r = (uint8_t)(((v << 1) | flagC) & 0xff);
    flagC = c; A = setNZ8(r); return 2;
}
int Spc700::op_ROL_dp() {
    uint32_t a = dp(fetch8()); uint8_t v = read(a);
    uint8_t c = (v & 0x80) ? 1 : 0; uint8_t r = (uint8_t)(((v << 1) | flagC) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 4;
}
int Spc700::op_ROL_dpX() {
    uint32_t a = dp((fetch8() + X) & 0xff); uint8_t v = read(a);
    uint8_t c = (v & 0x80) ? 1 : 0; uint8_t r = (uint8_t)(((v << 1) | flagC) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 5;
}
int Spc700::op_ROL_abs() {
    uint32_t a = fetch16(); uint8_t v = read(a);
    uint8_t c = (v & 0x80) ? 1 : 0; uint8_t r = (uint8_t)(((v << 1) | flagC) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 5;
}

int Spc700::op_ROR_A() {
    uint8_t v = A;
    uint8_t c = v & 1; uint8_t r = (uint8_t)(((v >> 1) | (flagC << 7)) & 0xff);
    flagC = c; A = setNZ8(r); return 2;
}
int Spc700::op_ROR_dp() {
    uint32_t a = dp(fetch8()); uint8_t v = read(a);
    uint8_t c = v & 1; uint8_t r = (uint8_t)(((v >> 1) | (flagC << 7)) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 4;
}
int Spc700::op_ROR_dpX() {
    uint32_t a = dp((fetch8() + X) & 0xff); uint8_t v = read(a);
    uint8_t c = v & 1; uint8_t r = (uint8_t)(((v >> 1) | (flagC << 7)) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 5;
}
int Spc700::op_ROR_abs() {
    uint32_t a = fetch16(); uint8_t v = read(a);
    uint8_t c = v & 1; uint8_t r = (uint8_t)(((v >> 1) | (flagC << 7)) & 0xff);
    flagC = c; write(a, setNZ8(r)); return 5;
}

int Spc700::op_XCN_A() { A = setNZ8(((A << 4) | (A >> 4)) & 0xff); return 5; }

int Spc700::op_MUL_YA() {
    int r = (Y & 0xff) * (A & 0xff);
    A = r & 0xff; Y = (r >> 8) & 0xff;
    setNZ8(Y);
    return 9;
}

int Spc700::op_DIV_YA_X() {
    int ya = (Y << 8) | A;
    int x = X;
    flagH = (((Y & 0xf) >= (x & 0xf))) ? 1 : 0;
    flagV = (Y >= x) ? 1 : 0;
    if (x == 0) {
        // ゼロ除算: JS版は Math.floor(ya/0) = Infinity となり不定動作。
        // 実機同様の挙動(Y値がそのまま、Aは0xFF相当)に近い安全策を取る。
        A = 0xff;
        Y = Y; // 変化なし
    } else if (Y < (x << 1)) {
        A = (ya / x) & 0xff;
        Y = (ya % x) & 0xff;
    } else {
        int d = 256 - x;
        A = (255 - ((ya - (x << 9)) / d)) & 0xff;
        Y = (x + ((ya - (x << 9)) % d)) & 0xff;
    }
    setNZ8(A);
    return 12;
}

int Spc700::op_DAA_A() {
    int a = A;
    if (flagC || a > 0x99) { a = (a + 0x60) & 0xff; flagC = 1; }
    if (flagH || (a & 0x0f) > 9) { a = (a + 0x06) & 0xff; }
    A = setNZ8(a);
    return 3;
}
int Spc700::op_DAS_A() {
    int a = A;
    if (!flagC || a > 0x99) { a = (a - 0x60) & 0xff; flagC = 0; }
    if (!flagH || (a & 0x0f) > 9) { a = (a - 0x06) & 0xff; }
    A = setNZ8(a);
    return 3;
}

// ---- フラグ操作 ----
int Spc700::op_CLRC() { flagC = 0; return 2; }
int Spc700::op_SETC() { flagC = 1; return 2; }
int Spc700::op_NOTC() { flagC = flagC ^ 1; return 3; }
int Spc700::op_CLRP() { flagP = 0; return 2; }
int Spc700::op_SETP() { flagP = 1; return 2; }
int Spc700::op_CLRV() { flagV = 0; flagH = 0; return 2; }
int Spc700::op_EI()   { flagI = 1; return 2; }
int Spc700::op_DI()   { flagI = 0; return 2; }

// ---- スタック ----
int Spc700::op_PUSH_A() { push8(A); return 4; }
int Spc700::op_PUSH_X() { push8(X); return 4; }
int Spc700::op_PUSH_Y() { push8(Y); return 4; }
int Spc700::op_PUSH_PSW() { push8(getPSW()); return 4; }
int Spc700::op_POP_A() { A = pop8(); return 4; }
int Spc700::op_POP_X() { X = pop8(); return 4; }
int Spc700::op_POP_Y() { Y = pop8(); return 4; }
int Spc700::op_POP_PSW() { setPSW(pop8()); return 4; }

// ---- 分岐 ----
int Spc700::op_BRA() { uint8_t d = fetch8(); int s = (d & 0x80) ? (int)d - 256 : (int)d; PC = (uint16_t)((PC + s) & 0xffff); return 4; }
int Spc700::op_BEQ() { uint8_t d = fetch8(); return 2 + branch(flagZ == 1, d); }
int Spc700::op_BNE() { uint8_t d = fetch8(); return 2 + branch(flagZ == 0, d); }
int Spc700::op_BCS() { uint8_t d = fetch8(); return 2 + branch(flagC == 1, d); }
int Spc700::op_BCC() { uint8_t d = fetch8(); return 2 + branch(flagC == 0, d); }
int Spc700::op_BVS() { uint8_t d = fetch8(); return 2 + branch(flagV == 1, d); }
int Spc700::op_BVC() { uint8_t d = fetch8(); return 2 + branch(flagV == 0, d); }
int Spc700::op_BMI() { uint8_t d = fetch8(); return 2 + branch(flagN == 1, d); }
int Spc700::op_BPL() { uint8_t d = fetch8(); return 2 + branch(flagN == 0, d); }

// ---- ビットset/clr (dp内) ----
int Spc700::op_SET1(int bit) { uint32_t a = dp(fetch8()); int v = read(a); v |= (1 << bit); write(a, v & 0xff); return 4; }
int Spc700::op_CLR1(int bit) { uint32_t a = dp(fetch8()); int v = read(a); v &= ~(1 << bit); write(a, v & 0xff); return 4; }

// ---- ビット演算 (mem.bit, abs13+bit3) ----
int Spc700::op_MOV1_C_mem() {
    uint16_t w = fetch16(); uint32_t addr = w & 0x1fff; int bit = (w >> 13) & 7;
    int v = read(addr);
    flagC = (v >> bit) & 1;
    return 4;
}
int Spc700::op_MOV1_mem_C() {
    uint16_t w = fetch16(); uint32_t addr = w & 0x1fff; int bit = (w >> 13) & 7;
    int v = read(addr);
    if (flagC) v |= (1 << bit); else v &= ~(1 << bit);
    write(addr, v & 0xff);
    return 6;
}
int Spc700::op_AND1_C_mem() {
    uint16_t w = fetch16(); uint32_t addr = w & 0x1fff; int bit = (w >> 13) & 7;
    int v = (read(addr) >> bit) & 1; flagC = flagC & v; return 4;
}
int Spc700::op_AND1_C_notmem() {
    uint16_t w = fetch16(); uint32_t addr = w & 0x1fff; int bit = (w >> 13) & 7;
    int v = (read(addr) >> bit) & 1; flagC = flagC & (v ^ 1); return 4;
}
int Spc700::op_OR1_C_mem() {
    uint16_t w = fetch16(); uint32_t addr = w & 0x1fff; int bit = (w >> 13) & 7;
    int v = (read(addr) >> bit) & 1; flagC = flagC | v; return 5;
}
int Spc700::op_OR1_C_notmem() {
    uint16_t w = fetch16(); uint32_t addr = w & 0x1fff; int bit = (w >> 13) & 7;
    int v = (read(addr) >> bit) & 1; flagC = flagC | (v ^ 1); return 5;
}
int Spc700::op_EOR1_C_mem() {
    uint16_t w = fetch16(); uint32_t addr = w & 0x1fff; int bit = (w >> 13) & 7;
    int v = (read(addr) >> bit) & 1; flagC = flagC ^ v; return 5;
}
int Spc700::op_NOT1_mem() {
    uint16_t w = fetch16(); uint32_t addr = w & 0x1fff; int bit = (w >> 13) & 7;
    int v = read(addr); v ^= (1 << bit); write(addr, v & 0xff); return 5;
}

// ---- TSET1/TCLR1 ----
int Spc700::op_TSET1() {
    uint32_t a = fetch16(); int v = read(a);
    setNZ8((A - v) & 0x1ff);
    write(a, v | A); return 6;
}
int Spc700::op_TCLR1() {
    uint32_t a = fetch16(); int v = read(a);
    setNZ8((A - v) & 0x1ff);
    write(a, v & (~A & 0xff)); return 6;
}

// ---- BBS/BBC ----
int Spc700::op_BBS(int bit) {
    uint32_t a = dp(fetch8()); uint8_t d = fetch8();
    int v = read(a);
    return 5 + branch(((v >> bit) & 1) == 1, d);
}
int Spc700::op_BBC(int bit) {
    uint32_t a = dp(fetch8()); uint8_t d = fetch8();
    int v = read(a);
    return 5 + branch(((v >> bit) & 1) == 0, d);
}

int Spc700::op_CBNE_dp() {
    uint32_t a = dp(fetch8()); uint8_t d = fetch8(); int v = read(a);
    return 5 + branch(A != v, d);
}
int Spc700::op_CBNE_dpX() {
    uint32_t a = dp((fetch8() + X) & 0xff); uint8_t d = fetch8(); int v = read(a);
    return 6 + branch(A != v, d);
}

int Spc700::op_DBNZ_Y() {
    uint8_t d = fetch8(); Y = (Y - 1) & 0xff;
    return 4 + branch(Y != 0, d);
}
int Spc700::op_DBNZ_dp() {
    uint32_t a = dp(fetch8()); uint8_t d = fetch8();
    int v = read(a); v = (v - 1) & 0xff; write(a, v);
    return 5 + branch(v != 0, d);
}

// ---- ジャンプ・コール ----
int Spc700::op_JMP_abs() { PC = fetch16(); return 3; }
int Spc700::op_JMP_absXind() {
    uint32_t base = fetch16(); uint32_t ptr = (base + X) & 0xffff;
    PC = (uint16_t)(read(ptr) | (read((ptr + 1) & 0xffff) << 8));
    return 6;
}

int Spc700::op_CALL() { uint32_t a = fetch16(); push16(PC); PC = (uint16_t)a; return 8; }
int Spc700::op_PCALL() { uint32_t a = 0xFF00u | fetch8(); push16(PC); PC = (uint16_t)a; return 6; }

int Spc700::op_TCALL(int n) {
    uint32_t vecAddr = 0xFFDEu - n * 2;
    uint32_t target = read(vecAddr) | (read((vecAddr + 1) & 0xffff) << 8);
    push16(PC);
    PC = (uint16_t)target;
    return 8;
}

int Spc700::op_RET() { PC = pop16(); return 5; }
int Spc700::op_RETI() { setPSW(pop8()); PC = pop16(); return 6; }

int Spc700::op_BRK() {
    push16(PC);
    push8(getPSW());
    flagB = 1; flagI = 0;
    PC = (uint16_t)(read(0xFFDE) | (read(0xFFDF) << 8));
    return 8;
}

int Spc700::op_NOP() { return 2; }

int Spc700::op_SLEEP() { PC = (PC - 1) & 0xffff; return 3; }
int Spc700::op_STOP()  { PC = (PC - 1) & 0xffff; return 3; }

int Spc700::op_ILLEGAL() { return 2; }

// ---- bit値を静的束縛するためのトランポリン ----
int Spc700::op_SET1_b0() { return op_SET1(0); } int Spc700::op_CLR1_b0() { return op_CLR1(0); }
int Spc700::op_SET1_b1() { return op_SET1(1); } int Spc700::op_CLR1_b1() { return op_CLR1(1); }
int Spc700::op_SET1_b2() { return op_SET1(2); } int Spc700::op_CLR1_b2() { return op_CLR1(2); }
int Spc700::op_SET1_b3() { return op_SET1(3); } int Spc700::op_CLR1_b3() { return op_CLR1(3); }
int Spc700::op_SET1_b4() { return op_SET1(4); } int Spc700::op_CLR1_b4() { return op_CLR1(4); }
int Spc700::op_SET1_b5() { return op_SET1(5); } int Spc700::op_CLR1_b5() { return op_CLR1(5); }
int Spc700::op_SET1_b6() { return op_SET1(6); } int Spc700::op_CLR1_b6() { return op_CLR1(6); }
int Spc700::op_SET1_b7() { return op_SET1(7); } int Spc700::op_CLR1_b7() { return op_CLR1(7); }

int Spc700::op_BBS_b0() { return op_BBS(0); } int Spc700::op_BBC_b0() { return op_BBC(0); }
int Spc700::op_BBS_b1() { return op_BBS(1); } int Spc700::op_BBC_b1() { return op_BBC(1); }
int Spc700::op_BBS_b2() { return op_BBS(2); } int Spc700::op_BBC_b2() { return op_BBC(2); }
int Spc700::op_BBS_b3() { return op_BBS(3); } int Spc700::op_BBC_b3() { return op_BBC(3); }
int Spc700::op_BBS_b4() { return op_BBS(4); } int Spc700::op_BBC_b4() { return op_BBC(4); }
int Spc700::op_BBS_b5() { return op_BBS(5); } int Spc700::op_BBC_b5() { return op_BBC(5); }
int Spc700::op_BBS_b6() { return op_BBS(6); } int Spc700::op_BBC_b6() { return op_BBC(6); }
int Spc700::op_BBS_b7() { return op_BBS(7); } int Spc700::op_BBC_b7() { return op_BBC(7); }

int Spc700::op_TCALL_0()  { return op_TCALL(0); }  int Spc700::op_TCALL_1()  { return op_TCALL(1); }
int Spc700::op_TCALL_2()  { return op_TCALL(2); }  int Spc700::op_TCALL_3()  { return op_TCALL(3); }
int Spc700::op_TCALL_4()  { return op_TCALL(4); }  int Spc700::op_TCALL_5()  { return op_TCALL(5); }
int Spc700::op_TCALL_6()  { return op_TCALL(6); }  int Spc700::op_TCALL_7()  { return op_TCALL(7); }
int Spc700::op_TCALL_8()  { return op_TCALL(8); }  int Spc700::op_TCALL_9()  { return op_TCALL(9); }
int Spc700::op_TCALL_10() { return op_TCALL(10); } int Spc700::op_TCALL_11() { return op_TCALL(11); }
int Spc700::op_TCALL_12() { return op_TCALL(12); } int Spc700::op_TCALL_13() { return op_TCALL(13); }
int Spc700::op_TCALL_14() { return op_TCALL(14); } int Spc700::op_TCALL_15() { return op_TCALL(15); }

// ============================================================================
// オペコードテーブル構築
// ============================================================================
void Spc700::buildOpTable() {
    opTable.fill(nullptr);

    opTable[0x00] = &Spc700::op_NOP;
    opTable[0xE8] = &Spc700::op_MOV_A_imm;
    opTable[0xCD] = &Spc700::op_MOV_X_imm;
    opTable[0x8D] = &Spc700::op_MOV_Y_imm;

    opTable[0x7D] = &Spc700::op_MOV_A_X;
    opTable[0xDD] = &Spc700::op_MOV_A_Y;
    opTable[0x5D] = &Spc700::op_MOV_X_A;
    opTable[0xFD] = &Spc700::op_MOV_Y_A;
    opTable[0x9D] = &Spc700::op_MOV_X_SP;
    opTable[0xBD] = &Spc700::op_MOV_SP_X;

    opTable[0xC4] = &Spc700::op_MOV_dp_A;
    opTable[0xE4] = &Spc700::op_MOV_A_dp;
    opTable[0xD8] = &Spc700::op_MOV_dp_X;
    opTable[0xF8] = &Spc700::op_MOV_X_dp;
    opTable[0xCB] = &Spc700::op_MOV_dp_Y;
    opTable[0xEB] = &Spc700::op_MOV_Y_dp;

    opTable[0xD4] = &Spc700::op_MOV_dpX_A;
    opTable[0xF4] = &Spc700::op_MOV_A_dpX;
    opTable[0xD9] = &Spc700::op_MOV_dpY_X;
    opTable[0xF9] = &Spc700::op_MOV_X_dpY;
    opTable[0xDB] = &Spc700::op_MOV_dpX_Y;
    opTable[0xFB] = &Spc700::op_MOV_Y_dpX;

    opTable[0xC5] = &Spc700::op_MOV_abs_A;
    opTable[0xE5] = &Spc700::op_MOV_A_abs;
    opTable[0xC9] = &Spc700::op_MOV_abs_X;
    opTable[0xE9] = &Spc700::op_MOV_X_abs;
    opTable[0xCC] = &Spc700::op_MOV_abs_Y;
    opTable[0xEC] = &Spc700::op_MOV_Y_abs;

    opTable[0xD5] = &Spc700::op_MOV_absX_A;
    opTable[0xD6] = &Spc700::op_MOV_absY_A;
    opTable[0xF5] = &Spc700::op_MOV_A_absX;
    opTable[0xF6] = &Spc700::op_MOV_A_absY;

    opTable[0xC6] = &Spc700::op_MOV_indX_A;
    opTable[0xE6] = &Spc700::op_MOV_A_indX;
    opTable[0xAF] = &Spc700::op_MOV_indXinc_A;
    opTable[0xBF] = &Spc700::op_MOV_A_indXinc;

    opTable[0xC7] = &Spc700::op_MOV_dpXind_A;
    opTable[0xE7] = &Spc700::op_MOV_A_dpXind;
    opTable[0xD7] = &Spc700::op_MOV_dpindY_A;
    opTable[0xF7] = &Spc700::op_MOV_A_dpindY;

    opTable[0xFA] = &Spc700::op_MOV_dp_dp;
    opTable[0x8F] = &Spc700::op_MOV_imm_dp;

    opTable[0xBA] = &Spc700::op_MOVW_YA_dp;
    opTable[0xDA] = &Spc700::op_MOVW_dp_YA;
    opTable[0x3A] = &Spc700::op_INCW_dp;
    opTable[0x1A] = &Spc700::op_DECW_dp;
    opTable[0x7A] = &Spc700::op_ADDW_YA_dp;
    opTable[0x9A] = &Spc700::op_SUBW_YA_dp;
    opTable[0x5A] = &Spc700::op_CMPW_YA_dp;

    opTable[0x08] = &Spc700::op_OR_A_imm;
    opTable[0x28] = &Spc700::op_AND_A_imm;
    opTable[0x48] = &Spc700::op_EOR_A_imm;
    opTable[0x68] = &Spc700::op_CMP_A_imm;
    opTable[0x88] = &Spc700::op_ADC_A_imm;
    opTable[0xA8] = &Spc700::op_SBC_A_imm;

    opTable[0x04] = &Spc700::op_OR_A_dp;
    opTable[0x24] = &Spc700::op_AND_A_dp;
    opTable[0x44] = &Spc700::op_EOR_A_dp;
    opTable[0x64] = &Spc700::op_CMP_A_dp;
    opTable[0x84] = &Spc700::op_ADC_A_dp;
    opTable[0xA4] = &Spc700::op_SBC_A_dp;

    opTable[0x14] = &Spc700::op_OR_A_dpX;
    opTable[0x34] = &Spc700::op_AND_A_dpX;
    opTable[0x54] = &Spc700::op_EOR_A_dpX;
    opTable[0x74] = &Spc700::op_CMP_A_dpX;
    opTable[0x94] = &Spc700::op_ADC_A_dpX;
    opTable[0xB4] = &Spc700::op_SBC_A_dpX;

    opTable[0x05] = &Spc700::op_OR_A_abs;
    opTable[0x25] = &Spc700::op_AND_A_abs;
    opTable[0x45] = &Spc700::op_EOR_A_abs;
    opTable[0x65] = &Spc700::op_CMP_A_abs;
    opTable[0x85] = &Spc700::op_ADC_A_abs;
    opTable[0xA5] = &Spc700::op_SBC_A_abs;

    opTable[0x15] = &Spc700::op_OR_A_absX;
    opTable[0x16] = &Spc700::op_OR_A_absY;
    opTable[0x35] = &Spc700::op_AND_A_absX;
    opTable[0x36] = &Spc700::op_AND_A_absY;
    opTable[0x55] = &Spc700::op_EOR_A_absX;
    opTable[0x56] = &Spc700::op_EOR_A_absY;
    opTable[0x75] = &Spc700::op_CMP_A_absX;
    opTable[0x76] = &Spc700::op_CMP_A_absY;
    opTable[0x95] = &Spc700::op_ADC_A_absX;
    opTable[0x96] = &Spc700::op_ADC_A_absY;
    opTable[0xB5] = &Spc700::op_SBC_A_absX;
    opTable[0xB6] = &Spc700::op_SBC_A_absY;

    opTable[0x06] = &Spc700::op_OR_A_indX;
    opTable[0x26] = &Spc700::op_AND_A_indX;
    opTable[0x46] = &Spc700::op_EOR_A_indX;
    opTable[0x66] = &Spc700::op_CMP_A_indX;
    opTable[0x86] = &Spc700::op_ADC_A_indX;
    opTable[0xA6] = &Spc700::op_SBC_A_indX;

    opTable[0x07] = &Spc700::op_OR_A_dpXind;
    opTable[0x27] = &Spc700::op_AND_A_dpXind;
    opTable[0x47] = &Spc700::op_EOR_A_dpXind;
    opTable[0x67] = &Spc700::op_CMP_A_dpXind;
    opTable[0x87] = &Spc700::op_ADC_A_dpXind;
    opTable[0xA7] = &Spc700::op_SBC_A_dpXind;

    opTable[0x17] = &Spc700::op_OR_A_dpindY;
    opTable[0x37] = &Spc700::op_AND_A_dpindY;
    opTable[0x57] = &Spc700::op_EOR_A_dpindY;
    opTable[0x77] = &Spc700::op_CMP_A_dpindY;
    opTable[0x97] = &Spc700::op_ADC_A_dpindY;
    opTable[0xB7] = &Spc700::op_SBC_A_dpindY;

    opTable[0x09] = &Spc700::op_OR_dp_dp;
    opTable[0x29] = &Spc700::op_AND_dp_dp;
    opTable[0x49] = &Spc700::op_EOR_dp_dp;
    opTable[0x69] = &Spc700::op_CMP_dp_dp;
    opTable[0x89] = &Spc700::op_ADC_dp_dp;
    opTable[0xA9] = &Spc700::op_SBC_dp_dp;

    opTable[0x18] = &Spc700::op_OR_dp_imm;
    opTable[0x38] = &Spc700::op_AND_dp_imm;
    opTable[0x58] = &Spc700::op_EOR_dp_imm;
    opTable[0x78] = &Spc700::op_CMP_dp_imm;
    opTable[0x98] = &Spc700::op_ADC_dp_imm;
    opTable[0xB8] = &Spc700::op_SBC_dp_imm;

    opTable[0x19] = &Spc700::op_OR_indX_indY;
    opTable[0x39] = &Spc700::op_AND_indX_indY;
    opTable[0x59] = &Spc700::op_EOR_indX_indY;
    opTable[0x79] = &Spc700::op_CMP_indX_indY;
    opTable[0x99] = &Spc700::op_ADC_indX_indY;
    opTable[0xB9] = &Spc700::op_SBC_indX_indY;

    opTable[0xC8] = &Spc700::op_CMPX_imm;
    opTable[0xAD] = &Spc700::op_CMPY_imm;
    opTable[0x3E] = &Spc700::op_CMPX_dp;
    opTable[0x7E] = &Spc700::op_CMPY_dp;
    opTable[0x1E] = &Spc700::op_CMPX_abs;
    opTable[0x5E] = &Spc700::op_CMPY_abs;

    opTable[0xBC] = &Spc700::op_INC_A;
    opTable[0x9C] = &Spc700::op_DEC_A;
    opTable[0x3D] = &Spc700::op_INC_X;
    opTable[0x1D] = &Spc700::op_DEC_X;
    opTable[0xFC] = &Spc700::op_INC_Y;
    opTable[0xDC] = &Spc700::op_DEC_Y;

    opTable[0xAB] = &Spc700::op_INC_dp;
    opTable[0x8B] = &Spc700::op_DEC_dp;
    opTable[0xBB] = &Spc700::op_INC_dpX;
    opTable[0x9B] = &Spc700::op_DEC_dpX;
    opTable[0xAC] = &Spc700::op_INC_abs;
    opTable[0x8C] = &Spc700::op_DEC_abs;

    opTable[0x1C] = &Spc700::op_ASL_A;
    opTable[0x0B] = &Spc700::op_ASL_dp;
    opTable[0x1B] = &Spc700::op_ASL_dpX;
    opTable[0x0C] = &Spc700::op_ASL_abs;

    opTable[0x5C] = &Spc700::op_LSR_A;
    opTable[0x4B] = &Spc700::op_LSR_dp;
    opTable[0x5B] = &Spc700::op_LSR_dpX;
    opTable[0x4C] = &Spc700::op_LSR_abs;

    opTable[0x3C] = &Spc700::op_ROL_A;
    opTable[0x2B] = &Spc700::op_ROL_dp;
    opTable[0x3B] = &Spc700::op_ROL_dpX;
    opTable[0x2C] = &Spc700::op_ROL_abs;

    opTable[0x7C] = &Spc700::op_ROR_A;
    opTable[0x6B] = &Spc700::op_ROR_dp;
    opTable[0x7B] = &Spc700::op_ROR_dpX;
    opTable[0x6C] = &Spc700::op_ROR_abs;

    opTable[0x9F] = &Spc700::op_XCN_A;
    opTable[0xCF] = &Spc700::op_MUL_YA;
    opTable[0x9E] = &Spc700::op_DIV_YA_X;
    opTable[0xDF] = &Spc700::op_DAA_A;
    opTable[0xBE] = &Spc700::op_DAS_A;

    opTable[0x60] = &Spc700::op_CLRC;
    opTable[0x80] = &Spc700::op_SETC;
    opTable[0xED] = &Spc700::op_NOTC;
    opTable[0x20] = &Spc700::op_CLRP;
    opTable[0x40] = &Spc700::op_SETP;
    opTable[0xE0] = &Spc700::op_CLRV;
    opTable[0xA0] = &Spc700::op_EI;
    opTable[0xC0] = &Spc700::op_DI;

    opTable[0x2D] = &Spc700::op_PUSH_A;
    opTable[0x4D] = &Spc700::op_PUSH_X;
    opTable[0x6D] = &Spc700::op_PUSH_Y;
    opTable[0x0D] = &Spc700::op_PUSH_PSW;
    opTable[0xAE] = &Spc700::op_POP_A;
    opTable[0xCE] = &Spc700::op_POP_X;
    opTable[0xEE] = &Spc700::op_POP_Y;
    opTable[0x8E] = &Spc700::op_POP_PSW;

    opTable[0x2F] = &Spc700::op_BRA;
    opTable[0xF0] = &Spc700::op_BEQ;
    opTable[0xD0] = &Spc700::op_BNE;
    opTable[0xB0] = &Spc700::op_BCS;
    opTable[0x90] = &Spc700::op_BCC;
    opTable[0x70] = &Spc700::op_BVS;
    opTable[0x50] = &Spc700::op_BVC;
    opTable[0x30] = &Spc700::op_BMI;
    opTable[0x10] = &Spc700::op_BPL;

    // SET1/CLR1 (8種、bit0-7)
    // opSet = 0x02 | (bit<<5), opClr = 0x12 | (bit<<5)
    // 注: CLR1 bit7 は opClr = 0x12 | (7<<5) = 0xF2 となるが、
    // 元のJS実装 (_buildOpTable) では for(bit=0..7) の opSet/opClr 登録ループ後に
    // 別の命令テーブルは上書きしていないため、JS版でも T[0xF2] は実際には
    // "CLR1 d.7" として登録される (JS版ソースの617-622行目のループを参照)。
    // ただし本ポートでは SPC700::write() 内で 0xF2 (DSPレジスタアドレス指定) は
    // CPUのメモリマップドI/Oとして扱われるため、オペコードとしての0xF2とは独立している。
    // JS実装との忠実な互換性を保つため、CLR1 bit7 をそのまま0xF2に登録する。
    opTable[0x02] = &Spc700::op_SET1_b0; opTable[0x12] = &Spc700::op_CLR1_b0;
    opTable[0x22] = &Spc700::op_SET1_b1; opTable[0x32] = &Spc700::op_CLR1_b1;
    opTable[0x42] = &Spc700::op_SET1_b2; opTable[0x52] = &Spc700::op_CLR1_b2;
    opTable[0x62] = &Spc700::op_SET1_b3; opTable[0x72] = &Spc700::op_CLR1_b3;
    opTable[0x82] = &Spc700::op_SET1_b4; opTable[0x92] = &Spc700::op_CLR1_b4;
    opTable[0xA2] = &Spc700::op_SET1_b5; opTable[0xB2] = &Spc700::op_CLR1_b5;
    opTable[0xC2] = &Spc700::op_SET1_b6; opTable[0xD2] = &Spc700::op_CLR1_b6;
    opTable[0xE2] = &Spc700::op_SET1_b7; opTable[0xF2] = &Spc700::op_CLR1_b7;

    opTable[0xAA] = &Spc700::op_MOV1_C_mem;
    opTable[0xCA] = &Spc700::op_MOV1_mem_C;
    opTable[0x4A] = &Spc700::op_AND1_C_mem;
    opTable[0x6A] = &Spc700::op_AND1_C_notmem;
    opTable[0x0A] = &Spc700::op_OR1_C_mem;
    opTable[0x2A] = &Spc700::op_OR1_C_notmem;
    opTable[0x8A] = &Spc700::op_EOR1_C_mem;
    opTable[0xEA] = &Spc700::op_NOT1_mem;

    opTable[0x0E] = &Spc700::op_TSET1;
    opTable[0x4E] = &Spc700::op_TCLR1;

    // BBS/BBC (0x03|(bit<<5), 0x13|(bit<<5))
    opTable[0x03] = &Spc700::op_BBS_b0; opTable[0x13] = &Spc700::op_BBC_b0;
    opTable[0x23] = &Spc700::op_BBS_b1; opTable[0x33] = &Spc700::op_BBC_b1;
    opTable[0x43] = &Spc700::op_BBS_b2; opTable[0x53] = &Spc700::op_BBC_b2;
    opTable[0x63] = &Spc700::op_BBS_b3; opTable[0x73] = &Spc700::op_BBC_b3;
    opTable[0x83] = &Spc700::op_BBS_b4; opTable[0x93] = &Spc700::op_BBC_b4;
    opTable[0xA3] = &Spc700::op_BBS_b5; opTable[0xB3] = &Spc700::op_BBC_b5;
    opTable[0xC3] = &Spc700::op_BBS_b6; opTable[0xD3] = &Spc700::op_BBC_b6;
    opTable[0xE3] = &Spc700::op_BBS_b7; opTable[0xF3] = &Spc700::op_BBC_b7;

    opTable[0x2E] = &Spc700::op_CBNE_dp;
    opTable[0xDE] = &Spc700::op_CBNE_dpX;

    opTable[0xFE] = &Spc700::op_DBNZ_Y;
    opTable[0x6E] = &Spc700::op_DBNZ_dp;

    opTable[0x5F] = &Spc700::op_JMP_abs;
    opTable[0x1F] = &Spc700::op_JMP_absXind;

    opTable[0x3F] = &Spc700::op_CALL;
    opTable[0x4F] = &Spc700::op_PCALL;

    // TCALL 0-15 (opcode = 0x01 | (n<<4))
    opTable[0x01] = &Spc700::op_TCALL_0;  opTable[0x11] = &Spc700::op_TCALL_1;
    opTable[0x21] = &Spc700::op_TCALL_2;  opTable[0x31] = &Spc700::op_TCALL_3;
    opTable[0x41] = &Spc700::op_TCALL_4;  opTable[0x51] = &Spc700::op_TCALL_5;
    opTable[0x61] = &Spc700::op_TCALL_6;  opTable[0x71] = &Spc700::op_TCALL_7;
    opTable[0x81] = &Spc700::op_TCALL_8;  opTable[0x91] = &Spc700::op_TCALL_9;
    opTable[0xA1] = &Spc700::op_TCALL_10; opTable[0xB1] = &Spc700::op_TCALL_11;
    opTable[0xC1] = &Spc700::op_TCALL_12; opTable[0xD1] = &Spc700::op_TCALL_13;
    opTable[0xE1] = &Spc700::op_TCALL_14; opTable[0xF1] = &Spc700::op_TCALL_15;

    opTable[0x6F] = &Spc700::op_RET;
    opTable[0x7F] = &Spc700::op_RETI;
    opTable[0x0F] = &Spc700::op_BRK;

    opTable[0xEF] = &Spc700::op_SLEEP;
    opTable[0xFF] = &Spc700::op_STOP;
}

} // namespace snesapu