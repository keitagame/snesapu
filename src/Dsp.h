// ============================================================================
// Dsp.h - SNES S-DSP エミュレータ (libspc.js の DSP クラスの移植)
// ============================================================================
#ifndef SNESAPU_DSP_H
#define SNESAPU_DSP_H

#include <cstdint>
#include <array>
#include <string>

namespace snesapu {

struct SampleDirEntry {
    uint32_t start;
    uint32_t loop;
};

enum class EnvMode : uint8_t {
    Off = 0,
    Attack,
    Decay,
    Sustain,
    Release,
    KonDelay,
};

struct DspVoice {
    uint32_t brrAddr = 0;
    uint32_t brrOffset = 0;
    uint32_t pitchCounter = 0;
    std::array<int32_t, 16> decodedBlock{};
    std::array<double, 4> interp{};   // JS版 Float64Array 互換 (ガウス補間の履歴)
    int32_t history0 = 0, history1 = 0;
    bool keyOn = false;
    bool keyOff = false;
    EnvMode envMode = EnvMode::Off;
    int32_t envLevel = 0;
    bool loopFlag = false;
    bool endFlag = false;
    int32_t outSample = 0;
    int32_t konDelay = 0;
    bool konLatched = false;
};

class Dsp {
public:
    explicit Dsp(std::array<uint8_t, 0x10000>* ram);

    std::array<uint8_t, 0x10000>* ram; // Spc700のRAMを共有参照 (所有権は持たない)
    std::array<uint8_t, 128> regs{};
    uint8_t regAddr = 0;

    std::array<DspVoice, 8> voices{};

    uint32_t noiseLFSR = 0x4000;

    uint32_t echoOffset = 0;
    uint32_t echoLength = 4;
    std::array<int32_t, 8> firHistL{};
    std::array<int32_t, 8> firHistR{};
    int firPos = 0;

    double dcPrevInL = 0, dcPrevOutL = 0;
    double dcPrevInR = 0, dcPrevOutR = 0;
    double lpL = 0, lpR = 0;

    uint32_t globalCounter = 0x77ff;
    uint32_t pendingKon = 0;

    // --- 出力音量レベル (GetAPUData の ppVMMaxL/ppVMMaxR 相当、EmuAPU呼出前に0クリアしておくと最大値が積算される) ---
    uint32_t vuMaxL = 0;
    uint32_t vuMaxR = 0;
    bool vuMaxAsFloat = false; // DSP_FLOAT オプション

    // --- SetAPUOpt由来のオプション (opts) ---
    uint32_t optFlags = 0; // DSP_* のビットOR

    // --- SetDSPPitch/SetDSPStereo/SetDSPEFBCT/SetDSPAmp/SetDSPVol 用の外部パラメータ ---
    double pitchScale = 1.0;    // 32000基準で1倍
    double stereoSep = 1.0;     // 0=モノラル相当,1=標準,2=強調
    double efbLeak = 1.0;       // 32768基準、-1.0～1.0
    double ampScale = 1.0;      // 65536基準で1倍 (ResetAPU / SetDSPAmp)
    double volScale = 1.0;      // 65536基準 (SetDSPVol、フェードアウト用)

    void reset();

    uint8_t read(uint32_t addr) const;
    void    write(uint32_t addr, uint8_t val);

    // グローバルレジスタ アクセサ
    int8_t volL(int v) const;
    int8_t volR(int v) const;
    uint16_t pitch(int v) const;
    uint8_t srcn(int v) const;
    uint8_t adsr1(int v) const;
    uint8_t adsr2(int v) const;
    uint8_t gain(int v) const;

    uint8_t kon() const  { return regs[0x4c]; }
    uint8_t koff() const { return regs[0x5c]; }
    uint8_t flg() const  { return regs[0x6c]; }
    uint8_t pmon() const { return regs[0x2d]; }
    uint8_t non() const  { return regs[0x3d]; }
    uint8_t eon() const  { return regs[0x4d]; }
    uint8_t dir() const  { return regs[0x5d]; }
    int8_t  mvolL() const { return s8(regs[0x0c]); }
    int8_t  mvolR() const { return s8(regs[0x1c]); }
    int8_t  evolL() const { return s8(regs[0x2c]); }
    int8_t  evolR() const { return s8(regs[0x3c]); }
    int8_t  efb() const   { return s8(regs[0x0d]); }
    uint8_t esa() const   { return regs[0x6d]; }
    uint8_t edl() const   { return regs[0x7d] & 0x0f; }
    int8_t  fir(int i) const { return s8(regs[(i << 4) | 0x0f]); }

    SampleDirEntry getSampleDirEntry(uint8_t srcNum) const;
    bool decodeBrrBlock(DspVoice& voice, uint32_t addr, int voiceIdx);

    void clockNoise();
    int32_t noiseSample() const;

    int32_t stepEnvelope(DspVoice& voice, int vIdx);
    bool rateFires(int rateIndex) const;

    // 1サンプル分のエミュレーションを行い、フィルタ処理前の生(L,R)ペアを返す。
    // 戻り値は [-1.0, 1.0] 程度に正規化された浮動小数。
    void generateSample(double& outL, double& outR);

    // 外部からの直接書き込み(SetDSPRegやAPI経由)用。戻り値はSetDSPReg互換 (0以外=状態変化あり)。
    bool setRegExternal(uint8_t reg, uint8_t val);

private:
    static inline int8_t s8(uint8_t v) { return (int8_t)v; }
    void triggerKeyOn(DspVoice& voice, int i);
    void echoStep(int32_t eMixL, int32_t eMixR, int32_t& outEchoL, int32_t& outEchoR);
};

} // namespace snesapu

#endif // SNESAPU_DSP_H