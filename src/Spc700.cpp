// ============================================================================
// Spc700.cpp - SPC700 CPUエミュレータ コア実装
// ============================================================================
#include "Spc700.h"
#include "Dsp.h"

namespace snesapu {

const std::array<uint8_t, 64> Spc700::bootRom = { {
    0xcd, 0xef, 0xbd, 0xe8, 0x00, 0xc6, 0x1d, 0xd0, 0xfc, 0x8f, 0xaa, 0xf4, 0x8f, 0xbb, 0xf5, 0x78,
    0xcc, 0xf4, 0xd0, 0xfb, 0x2f, 0x19, 0xeb, 0xf4, 0xd0, 0xfc, 0x7e, 0xf4, 0xd0, 0x0b, 0xe4, 0xf5,
    0xcb, 0xf4, 0xd7, 0x00, 0xfc, 0xd0, 0xf3, 0xab, 0x01, 0x10, 0xef, 0x7e, 0xf4, 0x10, 0xeb, 0xba,
    0xf6, 0xda, 0x00, 0xba, 0xf4, 0xc4, 0xf4, 0xdd, 0x5d, 0xd0, 0xdb, 0x1f, 0x00, 0x00, 0xc0, 0xff
} };

Spc700::Spc700(Dsp* dsp) : dsp_(dsp) {
    buildOpTable();
}

void Spc700::reset() {
    // JS版に明示的なreset()は無いが、コンストラクタ相当の初期化をここでも提供する。
    ram.fill(0);
    ioIn.fill(0);
    ioOut.fill(0);
    timerEnable.fill(0);
    timerTarget.fill(0);
    timerCounter.fill(0);
    timerOut.fill(0);
    tAccum.fill(0);
    romEnable = true;
    cycles = 0;
    A = X = Y = SP = 0;
    PC = 0;
    flagN = flagV = flagP = flagB = flagH = flagI = flagZ = flagC = 0;
    halted = false;
    haltedEnv = false;
}

uint8_t Spc700::read(uint32_t addr) {
    addr &= 0xffffu;
    if (addr >= 0xf0u && addr <= 0xffu) {
        switch (addr) {
            case 0xf2: return dsp_->regAddr;
            case 0xf3: return dsp_->read(dsp_->regAddr & 0x7f);
            case 0xf4: case 0xf5: case 0xf6: case 0xf7:
                return ioIn[addr - 0xf4];
            case 0xf8: case 0xf9:
                return ram[addr];
            case 0xfd: return readTimerOut(0);
            case 0xfe: return readTimerOut(1);
            case 0xff: return readTimerOut(2);
            default:
                return 0;
        }
    }
    if (addr >= 0xffc0u && romEnable) {
        return bootRom[addr & 0x3fu];
    }
    return ram[addr];
}

void Spc700::write(uint32_t addr, uint8_t val) {
    addr &= 0xffffu;
    val &= 0xffu;
    switch (addr) {
        case 0xf0:
            break;
        case 0xf1: {
            for (int t = 0; t < 3; t++) {
                uint8_t en = (val >> t) & 1;
                if (en && !timerEnable[t]) {
                    timerCounter[t] = 0;
                    timerOut[t] = 0;
                    tAccum[t] = 0;
                }
                timerEnable[t] = en;
            }
            if (val & 0x10) { ioIn[0] = 0; ioIn[1] = 0; }
            if (val & 0x20) { ioIn[2] = 0; ioIn[3] = 0; }
            romEnable = (val & 0x80) != 0;
            break;
        }
        case 0xf2:
            dsp_->regAddr = val;
            break;
        case 0xf3:
            if (!(dsp_->regAddr & 0x80)) {
                uint8_t regIdx = dsp_->regAddr & 0x7f;
                uint8_t writeVal = val;
                if (onDspRegWrite) {
                    writeVal = onDspRegWrite(regIdx, val);
                }
                dsp_->write(regIdx, writeVal);
            }
            break;
        case 0xf4: case 0xf5: case 0xf6: case 0xf7:
            ioOut[addr - 0xf4] = val;
            break;
        case 0xfa: timerTarget[0] = val; break;
        case 0xfb: timerTarget[1] = val; break;
        case 0xfc: timerTarget[2] = val; break;
        default: break;
    }
    ram[addr] = val;
}

uint8_t Spc700::readTimerOut(int t) {
    uint8_t v = timerOut[t] & 0x0f;
    timerOut[t] = 0;
    return v;
}

void Spc700::tickTimers(int cyc) {
    static const int periods[3] = { 128, 128, 16 };
    for (int t = 0; t < 3; t++) {
        if (!timerEnable[t]) continue;
        tAccum[t] += cyc;
        while (tAccum[t] >= periods[t]) {
            tAccum[t] -= periods[t];
            timerCounter[t] = (timerCounter[t] + 1) & 0xff;
            if (timerCounter[t] == timerTarget[t]) {
                timerCounter[t] = 0;
                timerOut[t] = (timerOut[t] + 1) & 0x0f;
            }
        }
    }
}

uint8_t Spc700::getPSW() const {
    return (uint8_t)((flagN << 7) | (flagV << 6) | (flagP << 5) |
                      (flagB << 4) | (flagH << 3) | (flagI << 2) |
                      (flagZ << 1) | (flagC));
}

void Spc700::setPSW(uint8_t v) {
    flagN = (v >> 7) & 1;
    flagV = (v >> 6) & 1;
    flagP = (v >> 5) & 1;
    flagB = (v >> 4) & 1;
    flagH = (v >> 3) & 1;
    flagI = (v >> 2) & 1;
    flagZ = (v >> 1) & 1;
    flagC = v & 1;
}

uint8_t Spc700::setNZ8(int v) {
    v &= 0xff;
    flagZ = (v == 0) ? 1 : 0;
    flagN = (v & 0x80) ? 1 : 0;
    return (uint8_t)v;
}

void Spc700::push8(uint8_t v) {
    ram[0x100 + SP] = v & 0xff;
    SP = (SP - 1) & 0xff;
}
uint8_t Spc700::pop8() {
    SP = (SP + 1) & 0xff;
    return ram[0x100 + SP];
}
void Spc700::push16(uint16_t v) {
    push8((v >> 8) & 0xff);
    push8(v & 0xff);
}
uint16_t Spc700::pop16() {
    uint8_t lo = pop8();
    uint8_t hi = pop8();
    return (uint16_t)((hi << 8) | lo);
}

uint8_t Spc700::fetch8() {
    uint8_t v = read(PC);
    PC = (PC + 1) & 0xffff;
    return v;
}
uint16_t Spc700::fetch16() {
    uint8_t lo = fetch8();
    uint8_t hi = fetch8();
    return (uint16_t)((hi << 8) | lo);
}

uint16_t Spc700::rd16dp(uint32_t a) {
    return (uint16_t)(read(a) | (read(dpNext(a)) << 8));
}
void Spc700::wr16dp(uint32_t a, uint16_t w) {
    write(a, w & 0xff);
    write(dpNext(a), (w >> 8) & 0xff);
}

uint8_t Spc700::adc(uint8_t a, uint8_t b, uint8_t carryIn) {
    int result = a + b + carryIn;
    flagH = (((a & 0xf) + (b & 0xf) + carryIn) > 0xf) ? 1 : 0;
    flagC = (result > 0xff) ? 1 : 0;
    int r8 = result & 0xff;
    flagV = ((~(a ^ b) & (a ^ r8) & 0x80) != 0) ? 1 : 0;
    setNZ8(r8);
    return (uint8_t)r8;
}
uint8_t Spc700::sbc(uint8_t a, uint8_t b, uint8_t carryIn) {
    return adc(a, (uint8_t)((~b) & 0xff), carryIn);
}

int Spc700::branch(bool cond, uint8_t disp) {
    if (cond) {
        int s = (disp & 0x80) ? (int)disp - 256 : (int)disp;
        PC = (uint16_t)((PC + s) & 0xffff);
        return 2;
    }
    return 0;
}

int Spc700::exec(uint8_t op) {
    OpFn fn = opTable[op];
    if (!fn) return 2;
    return (this->*fn)();
}

int Spc700::step() {
    uint16_t pcBefore = PC;
    uint8_t op = fetch8();

    if (onFetch) {
        FetchAction action = onFetch(pcBefore, op);
        if (action == FetchAction::Halt) {
            halted = true;
            haltedEnv = false;
            // PCを戻し、フェッチをキャンセルした状態にする(次回EmuAPUまで進行しない)
            PC = pcBefore;
            return 0;
        } else if (action == FetchAction::HaltEnv) {
            halted = true;
            haltedEnv = true;
            PC = pcBefore;
            return 0;
        } else if (action == FetchAction::NopOut) {
            // PCは進めない(pcBeforeに戻す)がタイマーは進める。opをNOP(0x00)相当として2サイクル消費。
            PC = pcBefore;
            int cyc = 2;
            cycles += cyc;
            tickTimers(cyc);
            return cyc;
        }
    }

    int cyc = exec(op);
    cycles += cyc;
    tickTimers(cyc);
    return cyc;
}

} // namespace snesapu

#include "Spc700Ops.inc.cpp"