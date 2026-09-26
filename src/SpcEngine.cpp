// ============================================================================
// SpcEngine.cpp - SPC700 + DSP 統合エンジン実装
// ============================================================================
#include "SpcEngine.h"
#include <algorithm>

namespace snesapu {

SpcEngine::SpcEngine() {
    // Spc700はコンストラクタでDsp*を要求するが、DspもSpc700::ramへの参照を要求するため
    // 循環依存がある。ここではSpc700を先にnullptrで構築し、Dsp構築後にattachDspで紐付ける。
    cpu = std::make_unique<Spc700>(nullptr);
    dsp = std::make_unique<Dsp>(&cpu->ram);
    cpu->attachDsp(dsp.get());
    loaded = false;
    cycleAccum = 0;
}

void SpcEngine::loadSpc(const ParsedSpc& parsed) {
    auto& c = *cpu;
    auto& d = *dsp;

    std::copy(parsed.ram, parsed.ram + 0x10000, c.ram.begin());
    c.A = parsed.a;
    c.X = parsed.x;
    c.Y = parsed.y;
    c.SP = parsed.sp;
    c.PC = parsed.pc;
    c.setPSW(parsed.psw);
    c.cycles = 0;

    uint8_t f1 = c.ram[0xf1];
    c.romEnable = (f1 & 0x80) != 0;
    for (int t = 0; t < 3; t++) {
        c.timerEnable[t] = (f1 >> t) & 1;
        c.timerTarget[t] = c.ram[0xfa + t];
        c.timerCounter[t] = 0;
        c.timerOut[t] = 0;
        c.tAccum[t] = 0;
    }
    for (int i = 0; i < 4; i++) {
        c.ioIn[i] = c.ram[0xf4 + i];
        c.ioOut[i] = c.ram[0xf4 + i];
    }
    d.regAddr = c.ram[0xf2];

    d.reset();
    std::copy(parsed.dspRegs, parsed.dspRegs + 128, d.regs.begin());
    d.regs[0x7c] = parsed.dspRegs[0x7c];

    uint8_t konSnapshot = parsed.dspRegs[0x4c];
    for (int i = 0; i < 8; i++) {
        DspVoice& v = d.voices[i];
        v.konLatched = ((konSnapshot >> i) & 1) == 1;
        uint8_t envx = parsed.dspRegs[i * 0x10 + 0x08];
        if (envx > 0) {
            v.envLevel = std::min(2047, (int)envx << 4);
            v.envMode = EnvMode::Sustain;
            SampleDirEntry dirEntry = d.getSampleDirEntry(d.srcn(i));
            v.brrAddr = dirEntry.start;
            v.pitchCounter = 0;
            v.history0 = 0; v.history1 = 0;
            v.endFlag = false;
            v.loopFlag = false;
            d.decodeBrrBlock(v, v.brrAddr, i);
            d.regs[0x7c] = parsed.dspRegs[0x7c];
            v.interp[0] = 0; v.interp[1] = 0; v.interp[2] = 0;
            v.interp[3] = v.decodedBlock[0];
            v.brrOffset = 1;
        }
    }

    loaded = true;
    cycleAccum = 0;
}

void SpcEngine::fixRegs(uint16_t pc, uint8_t a, uint8_t y, uint8_t x, uint8_t psw, uint8_t sp) {
    cpu->PC = pc;
    cpu->A = a;
    cpu->Y = y;
    cpu->X = x;
    cpu->setPSW(psw);
    cpu->SP = sp;
}

void SpcEngine::resetApu() {
    cpu->reset();
    dsp->reset();
    loaded = false;
    cycleAccum = 0;
}

void SpcEngine::renderSample(double& outL, double& outR) {
    if (!loaded) { outL = 0; outR = 0; return; }

    int64_t budget = CPU_CYCLES_PER_SAMPLE + cycleAccum;
    int guard = 0;
    while (budget > 0 && guard < 64) {
        if (cpu->halted) {
            // フェッチフックによりCPU停止中: サイクルのみ消費し実行はしない。
            break;
        }
        int used = cpu->step();
        budget -= used;
        guard++;
    }
    cycleAccum = budget;

    dsp->generateSample(outL, outR);
}

void SpcEngine::advanceCyclesNoDsp(int64_t clocks) {
    if (!loaded) return;
    int64_t remaining = clocks;
    int guard = 0;
    const int guardMax = 1 << 20;
    while (remaining > 0 && guard < guardMax) {
        if (cpu->halted) break;
        int used = cpu->step();
        remaining -= used;
        guard++;
    }
}

} // namespace snesapu