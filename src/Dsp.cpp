// ============================================================================
// Dsp.cpp - S-DSP エミュレータ実装
// ============================================================================
#include "Dsp.h"
#include "DspTables.h"
#include <cmath>

#ifndef SNESAPU_PI
#define SNESAPU_PI 3.14159265358979323846
#endif

namespace snesapu {

namespace {
    // 実行時に1度だけ計算するLPF係数 (constexprにできないためstatic初期化)
    double lpAlpha() {
        static const double alpha = 1.0 - std::exp(-2.0 * SNESAPU_PI * LP_CUTOFF_HZ / SDSP_RATE);
        return alpha;
    }

    inline int32_t clamp16(int32_t v) {
        if (v > 32767) return 32767;
        if (v < -32768) return -32768;
        return v;
    }
    inline int32_t signExtend16(int32_t v) {
        // (v << 16) >> 16 相当 (JSの符号拡張と同じ)
        return (int32_t)(int16_t)(v & 0xffff);
    }
}

Dsp::Dsp(std::array<uint8_t, 0x10000>* ramPtr) : ram(ramPtr) {
    for (auto& v : voices) {
        v.decodedBlock.fill(0);
        v.interp.fill(0.0);
    }
}

void Dsp::reset() {
    regs.fill(0);
    regAddr = 0;
    globalCounter = 0x77ff;
    pendingKon = 0;
    echoOffset = 0;
    echoLength = 4;
    firHistL.fill(0); firHistR.fill(0); firPos = 0;
    dcPrevInL = dcPrevOutL = dcPrevInR = dcPrevOutR = 0;
    lpL = lpR = 0;
    for (auto& v : voices) {
        v.interp.fill(0.0);
        v.pitchCounter = 0;
        v.envLevel = 0;
        v.keyOn = false;
        v.keyOff = false;
        v.envMode = EnvMode::Off;
        v.history0 = 0; v.history1 = 0;
        v.brrOffset = 16;
        v.endFlag = false;
        v.konLatched = false;
    }
}

uint8_t Dsp::read(uint32_t addr) const {
    return regs[addr & 0x7f];
}

void Dsp::write(uint32_t addr, uint8_t val) {
    addr &= 0x7f;
    val &= 0xff;
    if (addr == 0x7c) {
        regs[0x7c] = 0;
        return;
    }
    if (addr == 0x4c) {
        pendingKon = pendingKon | val;
    }
    regs[addr] = val;
}

bool Dsp::setRegExternal(uint8_t reg, uint8_t val) {
    uint8_t before = regs[reg & 0x7f];
    write(reg, val);
    uint8_t after = regs[reg & 0x7f];
    // SetDSPRegの戻り値仕様: DSPの状態に影響があった場合は0以外
    return before != after || reg == 0x4c; // KONは常に状態影響ありとみなす
}

int8_t Dsp::volL(int v) const { return s8(regs[v * 0x10 + 0x00]); }
int8_t Dsp::volR(int v) const { return s8(regs[v * 0x10 + 0x01]); }
uint16_t Dsp::pitch(int v) const {
    return (uint16_t)(regs[v * 0x10 + 0x02] | ((regs[v * 0x10 + 0x03] & 0x3f) << 8));
}
uint8_t Dsp::srcn(int v) const { return regs[v * 0x10 + 0x04]; }
uint8_t Dsp::adsr1(int v) const { return regs[v * 0x10 + 0x05]; }
uint8_t Dsp::adsr2(int v) const { return regs[v * 0x10 + 0x06]; }
uint8_t Dsp::gain(int v) const { return regs[v * 0x10 + 0x07]; }

SampleDirEntry Dsp::getSampleDirEntry(uint8_t srcNum) const {
    uint32_t base = (uint32_t)(dir() << 8) + (uint32_t)srcNum * 4;
    auto& r = *ram;
    uint32_t start = r[base] | (r[base + 1] << 8);
    uint32_t loop  = r[base + 2] | (r[base + 3] << 8);
    return { start, loop };
}

bool Dsp::decodeBrrBlock(DspVoice& voice, uint32_t addr, int voiceIdx) {
    auto& r = *ram;
    uint8_t header = r[addr & 0xffff];
    int range = (header >> 4) & 0x0f;
    int filter = (header >> 2) & 0x03;
    int loopBit = (header >> 1) & 1;
    int endBit = header & 1;

    auto& out = voice.decodedBlock;
    int32_t h1 = voice.history0;
    int32_t h2 = voice.history1;

    for (int i = 0; i < 16; i++) {
        int byteIdx = 1 + (i >> 1);
        uint8_t byte = r[(addr + byteIdx) & 0xffff];
        int nibble = (i & 1) == 0 ? (byte >> 4) : (byte & 0x0f);
        if (nibble >= 8) nibble -= 16;

        int32_t sample;
        if (range <= 12) {
            sample = (nibble << range) >> 1;
        } else {
            sample = (nibble < 0) ? -2048 : 0;
        }

        switch (filter) {
            case 1: sample += h1 + ((-h1) >> 4); break;
            case 2: sample += h1 * 2 + ((-(h1 * 3)) >> 5) - h2 + (h2 >> 4); break;
            case 3: sample += h1 * 2 + ((-(h1 * 13)) >> 6) - h2 + ((h2 * 3) >> 4); break;
            default: break;
        }

        if (sample > 32767) sample = 32767;
        else if (sample < -32768) sample = -32768;
        sample = signExtend16(sample); // (sample << 17) >> 17 相当 (17bit精度→16bit風符号拡張)
        // 備考: 上記のシフト量17は元JS実装の (sample << 17) >> 17 と同じ効果(下位16bit符号拡張+1bit余分)
        // を再現する必要があるため、下記で明示的に計算し直す。
        {
            int32_t tmp = sample;
            tmp = (int32_t)(((uint32_t)tmp << 17)) >> 17;
            sample = tmp;
        }

        out[i] = sample;
        h2 = h1;
        h1 = sample;
    }

    voice.history0 = h1;
    voice.history1 = h2;
    voice.loopFlag = (loopBit == 1);
    voice.endFlag = (endBit == 1);
    if (endBit == 1) {
        regs[0x7c] |= (uint8_t)(1 << voiceIdx);
    }
    return endBit == 1;
}

void Dsp::clockNoise() {
    int rate = flg() & 0x1f;
    if (rateFires(rate)) {
        uint32_t lfsr = noiseLFSR;
        uint32_t fb = (lfsr ^ (lfsr >> 1)) & 1;
        noiseLFSR = ((lfsr >> 1) & 0x3fff) | (fb << 14);
    }
}

int32_t Dsp::noiseSample() const {
    return signExtend16((int32_t)((noiseLFSR << 17) >> 17));
    // 備考: JSの ((this.noiseLFSR << 17) >> 17) は 32bit演算後に符号拡張される。
    // noiseLFSRは15bit値なので << 17 は32bit内に収まり、その後の >> 17 (算術シフト)
    // でビット14が符号ビットとして拡張される。下記で明示的に再現する。
}

int32_t Dsp::stepEnvelope(DspVoice& voice, int vIdx) {
    uint8_t a1 = adsr1(vIdx);
    uint8_t a2 = adsr2(vIdx);
    bool useADSR = (a1 & 0x80) != 0;

    if (voice.keyOff) {
        voice.envMode = EnvMode::Release;
    }

    if (voice.envMode == EnvMode::Release) {
        voice.envLevel -= 8;
        if (voice.envLevel <= 0) {
            voice.envLevel = 0;
            voice.envMode = EnvMode::Off;
        }
        return voice.envLevel;
    }

    if (useADSR) {
        int attackRate = (a1 & 0x0f) * 2 + 1;
        int decayRate = ((a1 >> 4) & 0x07) * 2 + 16;
        int sustainRate = a2 & 0x1f;
        int sustainLvl = (((a2 >> 5) & 0x07) + 1) * 256;

        if (voice.envMode == EnvMode::Attack) {
            int rate = attackRate;
            if (rateFires(rate)) {
                voice.envLevel += (rate == 31) ? 1024 : 32;
                if (voice.envLevel >= 0x7e0) voice.envMode = EnvMode::Decay;
                if (voice.envLevel > 0x7ff) voice.envLevel = 0x7ff;
            }
        } else if (voice.envMode == EnvMode::Decay) {
            if (rateFires(decayRate)) {
                voice.envLevel -= (((voice.envLevel - 1) >> 8) + 1);
                if (voice.envLevel < 0) voice.envLevel = 0;
                if (voice.envLevel < sustainLvl) voice.envMode = EnvMode::Sustain;
            }
        } else if (voice.envMode == EnvMode::Sustain) {
            if (sustainRate > 0 && rateFires(sustainRate)) {
                voice.envLevel -= (((voice.envLevel - 1) >> 8) + 1);
                if (voice.envLevel < 0) voice.envLevel = 0;
            }
        }
    } else {
        uint8_t gainVal = gain(vIdx);
        if ((gainVal & 0x80) == 0) {
            voice.envLevel = (gainVal & 0x7f) * 16;
        } else {
            int mode = (gainVal >> 5) & 0x03;
            int rate = gainVal & 0x1f;
            if (rateFires(rate)) {
                if (mode == 0) {
                    voice.envLevel -= 32;
                } else if (mode == 1) {
                    voice.envLevel -= (((voice.envLevel - 1) >> 8) + 1);
                } else if (mode == 2) {
                    voice.envLevel += 32;
                } else {
                    voice.envLevel += (voice.envLevel < 0x600) ? 32 : 8;
                }
                if (voice.envLevel < 0) voice.envLevel = 0;
                if (voice.envLevel > 2047) voice.envLevel = 2047;
            }
        }
    }

    if (voice.envLevel < 0) voice.envLevel = 0;
    if (voice.envLevel > 2047) voice.envLevel = 2047;
    return voice.envLevel;
}

bool Dsp::rateFires(int rateIndex) const {
    int32_t period = COUNTER_RATES[rateIndex];
    if (period == 0) return false;
    return ((int64_t)(globalCounter + COUNTER_OFFSETS[rateIndex]) % period) == 0;
}

void Dsp::echoStep(int32_t eMixL, int32_t eMixR, int32_t& outEchoL, int32_t& outEchoR) {
    auto& r = *ram;
    uint32_t base = ((uint32_t)(esa()) << 8) + echoOffset;
    base &= 0xffff;

    int32_t inL = r[base] | (r[(base + 1) & 0xffff] << 8); inL = signExtend16(inL);
    int32_t inR = r[(base + 2) & 0xffff] | (r[(base + 3) & 0xffff] << 8); inR = signExtend16(inR);

    firHistL[firPos] = inL >> 1; firHistR[firPos] = inR >> 1;
    firPos = (firPos + 1) & 7;

    int32_t fl = 0, fr = 0;
    for (int t = 0; t < 7; t++) {
        int idx = (firPos + t) & 7;
        int32_t c = fir(t);
        fl += (firHistL[idx] * c) >> 6;
        fr += (firHistR[idx] * c) >> 6;
    }
    {
        int idx = (firPos + 7) & 7;
        int32_t c = fir(7);
        fl = signExtend16(fl); fr = signExtend16(fr);
        fl += (firHistL[idx] * c) >> 6;
        fr += (firHistR[idx] * c) >> 6;
    }
    fl = clamp16(fl); fr = clamp16(fr);
    fl &= ~1; fr &= ~1;

    if (!(flg() & 0x20)) {
        int32_t wl = eMixL + ((fl * efb()) >> 7);
        int32_t wr = eMixR + ((fr * efb()) >> 7);
        wl = clamp16(wl); wr = clamp16(wr);
        wl &= ~1; wr &= ~1;
        r[base]                = wl & 0xff;
        r[(base + 1) & 0xffff] = (wl >> 8) & 0xff;
        r[(base + 2) & 0xffff] = wr & 0xff;
        r[(base + 3) & 0xffff] = (wr >> 8) & 0xff;
    }

    echoOffset += 4;
    if (echoOffset >= echoLength) {
        echoOffset = 0;
        echoLength = (uint32_t)edl() * 2048;
        if (echoLength == 0) echoLength = 4;
    }

    outEchoL = fl;
    outEchoR = fr;
}

void Dsp::triggerKeyOn(DspVoice& voice, int i) {
    regs[0x7c] &= (uint8_t)~(1 << i);
    SampleDirEntry dirEntry = getSampleDirEntry(srcn(i));
    voice.brrAddr = dirEntry.start;
    voice.brrOffset = 16;
    voice.pitchCounter = 0;
    voice.history0 = 0; voice.history1 = 0;
    voice.envLevel = 0;
    voice.envMode = EnvMode::KonDelay;
    voice.konDelay = 5;
    voice.keyOff = false;
    voice.endFlag = false;
    voice.loopFlag = false;
    voice.outSample = 0;
}

void Dsp::generateSample(double& outLRef, double& outRRef) {
    globalCounter = (globalCounter == 0) ? 0x77ff : globalCounter - 1;
    clockNoise();

    int32_t mixL = 0, mixR = 0;
    int32_t eMixL = 0, eMixR = 0;
    uint32_t konReg = kon() | pendingKon;
    uint32_t koffReg = koff();
    bool resetFlag = (flg() & 0x80) != 0;
    pendingKon = 0;

    for (int i = 0; i < 8; i++) {
        DspVoice& voice = voices[i];
        uint32_t bit = 1u << i;

        if (konReg & bit) {
            if (!voice.konLatched) {
                triggerKeyOn(voice, i);
                voice.konLatched = true;
            }
        } else {
            voice.konLatched = false;
        }
        voice.keyOff = ((koffReg & bit) != 0) || resetFlag;
        if (resetFlag) {
            voice.envLevel = 0;
            voice.envMode = EnvMode::Off;
        }

        if (voice.envMode == EnvMode::Off) {
            voice.outSample = 0;
            continue;
        }

        if (voice.envMode == EnvMode::KonDelay) {
            if (voice.brrOffset >= 16) {
                decodeBrrBlock(voice, voice.brrAddr, i);
                voice.interp[0] = 0; voice.interp[1] = 0; voice.interp[2] = 0;
                voice.interp[3] = voice.decodedBlock[0];
                voice.brrOffset = 1;
            }
            voice.outSample = 0;
            voice.konDelay--;
            if (voice.konDelay <= 0) {
                voice.envMode = EnvMode::Attack;
            }
            continue;
        }

        int32_t p = pitch(i) & 0x3fff;
        if (i > 0 && (pmon() & bit)) {
            int32_t prevOut = voices[i - 1].outSample;
            p = (p * ((prevOut >> 4) + 0x400)) >> 10;
        }
        if (p > 0x3fff) p = 0x3fff;

        int gi = (int)((voice.pitchCounter >> 4) & 0xff);
        auto& ip = voice.interp;
        double gs = (GAUSS_TABLE[255 - gi] * ip[0]);
        gs = (double)((int32_t)gs >> 10);
        gs += (double)(((int32_t)(GAUSS_TABLE[511 - gi] * ip[1])) >> 10);
        gs += (double)(((int32_t)(GAUSS_TABLE[256 + gi] * ip[2])) >> 10);
        {
            int32_t gsInt = (int32_t)gs;
            gsInt = signExtend16(gsInt);
            gs = (double)gsInt;
        }
        gs += (double)(((int32_t)(GAUSS_TABLE[gi] * ip[3])) >> 10);
        int32_t gsFinal = clamp16((int32_t)gs);
        int32_t sample = gsFinal >> 1;

        if (non() & bit) {
            sample = noiseSample();
        }

        int32_t env = stepEnvelope(voice, i);
        sample = (sample * env) >> 11;

        voice.outSample = sample;

        int32_t vl = (sample * volL(i)) >> 7;
        int32_t vr = (sample * volR(i)) >> 7;
        mixL += vl;
        mixR += vr;
        mixL = clamp16(mixL);
        mixR = clamp16(mixR);
        if (eon() & bit) {
            eMixL += vl;
            eMixR += vr;
            eMixL = clamp16(eMixL);
            eMixR = clamp16(eMixR);
        }

        voice.pitchCounter += (uint32_t)p;
        int advance = (int)(voice.pitchCounter >> 12);
        voice.pitchCounter &= 0xfff;

        while (advance-- > 0) {
            if (voice.brrOffset >= 16) {
                if (voice.endFlag) {
                    if (voice.loopFlag) {
                        SampleDirEntry dirEntry = getSampleDirEntry(srcn(i));
                        voice.brrAddr = dirEntry.loop;
                    } else {
                        voice.envMode = EnvMode::Off;
                        voice.envLevel = 0;
                        break;
                    }
                } else {
                    voice.brrAddr = (voice.brrAddr + 9) & 0xffff;
                }
                decodeBrrBlock(voice, voice.brrAddr, i);
                voice.brrOffset = 0;
            }
            auto& ip2 = voice.interp;
            ip2[0] = ip2[1]; ip2[1] = ip2[2]; ip2[2] = ip2[3];
            ip2[3] = voice.decodedBlock[voice.brrOffset];
            voice.brrOffset++;
        }
    }

    int32_t echoOutL = 0, echoOutR = 0;
    echoStep(eMixL, eMixR, echoOutL, echoOutR);

    int32_t dl = ((mixL * mvolL()) >> 7) + ((echoOutL * evolL()) >> 7);
    int32_t dr = ((mixR * mvolR()) >> 7) + ((echoOutR * evolR()) >> 7);
    dl = clamp16(dl); dr = clamp16(dr);
    double outL = dl / (32768.0 * OUTPUT_HEADROOM);
    double outR = dr / (32768.0 * OUTPUT_HEADROOM);

    if (flg() & 0x40) { outL = 0; outR = 0; }

    outL = softClip(outL);
    outR = softClip(outR);

    {
        const double R_DC = 0.99843;
        double yl = outL - dcPrevInL + R_DC * dcPrevOutL;
        dcPrevInL = outL; dcPrevOutL = yl; outL = yl;
        double yr = outR - dcPrevInR + R_DC * dcPrevOutR;
        dcPrevInR = outR; dcPrevOutR = yr; outR = yr;
    }

    {
        double alpha = lpAlpha();
        lpL += alpha * (outL - lpL); outL = lpL;
        lpR += alpha * (outR - lpR); outR = lpR;
    }

    outLRef = outL;
    outRRef = outR;
}

} // namespace snesapu