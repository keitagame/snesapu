// ============================================================================
// SNESAPU.cpp - SNESAPU.DLL 互換 API 実装本体
//
// libspc.js (SPC700+S-DSP JSエミュレータ) をベースに、改良版 SNESAPU.DLL
// (https://dgrfactory.jp/spcplay/snesapu.html) の v2.x系 API仕様に準拠した
// エクスポート関数を提供する。
// ============================================================================
#include "SNESAPU.h"
#include "SpcEngine.h"
#include "SpcFile.h"
#include "Resampler.h"

#include <cstring>
#include <cstdlib>
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>

using namespace snesapu;

namespace {

// ---------------------------------------------------------------------------
// グローバル状態 (SNESAPU.DLLは元々スレッドセーフでない単一インスタンス設計のため、
// 本移植でも同様にプロセス内シングルトンとして実装する)
// ---------------------------------------------------------------------------
struct GlobalState {
    SpcEngine engine;
    Resampler resampler;

    // SetAPUOpt で設定される出力フォーマット
    uint32_t mixType = MIX_FLOAT;
    uint32_t numChn = 2;
    uint32_t bits = 16;
    uint32_t rate = 32000;
    uint32_t inter = INT_GAUSS;
    uint32_t opts = 0;

    // SetAPULength / SetDSPVol によるフェードアウト管理
    bool lengthSet = false;
    uint64_t playSamplesTotal = 0;   // time(1/64000秒)をサンプル数に換算した値(rate基準)
    uint64_t fadeSamplesTotal = 0;   // fade(1/64000秒)をサンプル数に換算した値
    uint64_t samplesEmitted = 0;     // これまでにEmuAPUで生成したサンプル数

    double dspVol = 1.0; // SetDSPVol (フェードアウト制御にも使う内部倍率)

    // SNESAPUCallback
    CBFUNC callbackProc = nullptr;
    uint32_t callbackMask = 0;

    // 直近ロードしたSPCファイルの内容 (SeekAPUの逆シーク非対応方針に合わせ、
    // 現状は特に使わないが、将来のTransmitSPC等拡張のために保持できるようにしておく)
    std::vector<uint8_t> lastLoadedFile;

    std::mutex apiMutex; // 「スレッドセーフではない」仕様だが、内部データ破壊だけは防ぐ

    GlobalState() {
        applyOpt();
        engine.resetApu();
        resampler.configure((double)rate);
    }

    void applyOpt() {
        // DSPオプションフラグをDspへ反映
        engine.dsp->optFlags = opts;
        engine.dsp->vuMaxAsFloat = (opts & DSP_FLOAT) != 0;
    }
};

GlobalState& state() {
    static GlobalState g;
    return g;
}

inline int8_t interToDspEnum(uint32_t inter) {
    // 本移植では常にJS版と同じ4点ガウス補間+cubic出力補間を用いるため、
    // inter値そのものはメタ情報として保持するのみ(音質差の実装は将来拡張)。
    (void)inter;
    return 0;
}

// EmuAPU出力フォーマットへ変換して書き込むヘルパー
template <typename SampleT>
inline SampleT convertSample(double v, uint32_t bitsVal) {
    if (bitsVal == 32 && false) { /* -32 (float) は別関数で処理 */ }
    double scaled = v;
    if (scaled > 1.0) scaled = 1.0;
    if (scaled < -1.0) scaled = -1.0;
    return (SampleT)std::lround(scaled * (double)((1u << (sizeof(SampleT) * 8 - 1)) - 1));
}

// EmuAPUで実際に波形を書き出す処理
// 戻り値: 書き込んだバイト数
size_t writeSamplesToBuffer(uint8_t* pBuf, const std::vector<double>& mixL,
                             const std::vector<double>& mixR, uint32_t numChn,
                             uint32_t bitsVal) {
    size_t count = mixL.size();
    uint8_t* p = pBuf;

    auto putSample = [&](double v) {
        if (bitsVal == 8) {
            uint8_t s = (uint8_t)(std::clamp(v, -1.0, 1.0) * 127.0 + 128.0);
            *p++ = s;
        } else if (bitsVal == 16) {
            int16_t s = (int16_t)std::lround(std::clamp(v, -1.0, 1.0) * 32767.0);
            std::memcpy(p, &s, 2); p += 2;
        } else if (bitsVal == 24) {
            int32_t full = (int32_t)std::lround(std::clamp(v, -1.0, 1.0) * 8388607.0);
            p[0] = (uint8_t)(full & 0xff);
            p[1] = (uint8_t)((full >> 8) & 0xff);
            p[2] = (uint8_t)((full >> 16) & 0xff);
            p += 3;
        } else if (bitsVal == 32) {
            int32_t s = (int32_t)std::lround(std::clamp(v, -1.0, 1.0) * 2147483647.0);
            std::memcpy(p, &s, 4); p += 4;
        } else { // -32 (float表現。u32のbitsフィールドはunsignedなのでAPI層でs32から変換済み)
            float f = (float)v;
            std::memcpy(p, &f, 4); p += 4;
        }
    };

    for (size_t i = 0; i < count; i++) {
        putSample(mixL[i]);
        if (numChn == 2) putSample(mixR[i]);
    }

    return (size_t)(p - pBuf);
}

} // namespace

// ============================================================================
// EmuAPU
// ============================================================================
SNESAPU_API void* SNESAPU_CALL EmuAPU(void* pBuf, u32 len, u8 type) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    uint32_t sampleBytes = (g.bits == 8) ? 1u : (g.bits == 16 ? 2u : (g.bits == 24 ? 3u : 4u));
    uint32_t frameBytes = sampleBytes * g.numChn;

    // type: 0 = クロック数基準 (24576000で1秒), 1 = サンプル数基準
    uint32_t sampleCount;
    if (type == 1) {
        sampleCount = len;
    } else {
        // 24576000クロック/秒 を基準に rate へ変換
        double seconds = (double)len / 24576000.0;
        sampleCount = (uint32_t)std::llround(seconds * (double)g.rate);
    }

    if (sampleCount == 0) {
        return pBuf;
    }

    std::vector<double> outL(sampleCount), outR(sampleCount);

    // フェードアウト計算をサンプル単位で行う (SetAPULength由来)
    for (uint32_t i = 0; i < sampleCount; i++) {
        double vol = g.dspVol * g.engine.dsp->ampScale;

        if (g.lengthSet) {
            uint64_t emitted = g.samplesEmitted + i;
            if (emitted >= g.playSamplesTotal) {
                uint64_t fadePos = emitted - g.playSamplesTotal;
                if (g.fadeSamplesTotal == 0 || fadePos >= g.fadeSamplesTotal) {
                    vol = 0.0;
                } else {
                    double fadeT = 1.0 - (double)fadePos / (double)g.fadeSamplesTotal;
                    vol *= fadeT;
                }
            }
        }

        double l, r;
        g.resampler.render(g.engine, &l, &r, 1, vol);
        outL[i] = l;
        outR[i] = r;

        // VUメータ更新 (絶対値の最大)
        uint32_t al = (uint32_t)std::lround(std::fabs(l) * 32768.0);
        uint32_t ar = (uint32_t)std::lround(std::fabs(r) * 32768.0);
        if (al > g.engine.dsp->vuMaxL) g.engine.dsp->vuMaxL = al;
        if (ar > g.engine.dsp->vuMaxR) g.engine.dsp->vuMaxR = ar;
    }

    g.samplesEmitted += sampleCount;

    size_t bitsField = (g.bits == (uint32_t)(-32)) ? (size_t)-32 : (size_t)g.bits;
    (void)bitsField;
    (void)frameBytes;

    size_t written = writeSamplesToBuffer((uint8_t*)pBuf, outL, outR, g.numChn, g.bits);

    return (uint8_t*)pBuf + written;
}

// ============================================================================
// FixAPU
// ============================================================================
SNESAPU_API void SNESAPU_CALL FixAPU(u16 pc, u8 a, u8 y, u8 x, u8 psw, u8 sp) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    g.engine.fixRegs(pc, a, y, x, psw, sp);
}

// ============================================================================
// GetAPUData
// ============================================================================
SNESAPU_API void SNESAPU_CALL GetAPUData(u8** ppRAM, u8** ppXRAM, u8** ppOutPort,
                                          u32** ppT64Cnt, DSPReg** ppDSP, Voice** ppVoice,
                                          u32** ppVMMaxL, u32** ppVMMaxR) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    if (ppRAM) *ppRAM = g.engine.cpu->ram.data();
    if (ppXRAM) *ppXRAM = g.engine.xram.data();
    if (ppOutPort) *ppOutPort = g.engine.cpu->ioOut.data();

    // T64Cnt (タイマー64000カウンタ) は本移植ではEmuAPUで生成したサンプル数から近似計算する。
    // 32000Hz基準のサンプル数を64000カウンタへ変換した値を都度更新するstaticバッファを用意。
    static thread_local uint32_t t64CntValue = 0;
    t64CntValue = (uint32_t)((g.samplesEmitted * 64000ull) / (g.rate ? g.rate : 32000ull));
    if (ppT64Cnt) *ppT64Cnt = &t64CntValue;

    // DSPRegの型はSNESAPUのグローバル128byteレジスタ配列とレイアウトが異なるため、
    // ここでは互換のためにDspReg配列へ変換したバッファを都度生成して返す。
    static thread_local std::array<DSPReg, 8> dspRegBuf;
    for (int i = 0; i < 8; i++) {
        auto& d = dspRegBuf[i];
        d.volL = g.engine.dsp->volL(i);
        d.volR = g.engine.dsp->volR(i);
        d.pitch = g.engine.dsp->pitch(i);
        d.srcn = g.engine.dsp->srcn(i);
        d.adsr1 = g.engine.dsp->adsr1(i);
        d.adsr2 = g.engine.dsp->adsr2(i);
        d.gain = g.engine.dsp->gain(i);
        d.envx = (uint8_t)(g.engine.dsp->voices[i].envLevel >> 4);
        d.outx = (int8_t)(g.engine.dsp->voices[i].outSample >> 8);
    }
    if (ppDSP) *ppDSP = dspRegBuf.data();

    static thread_local std::array<Voice, 8> voiceBuf;
    for (int i = 0; i < 8; i++) {
        auto& src = g.engine.dsp->voices[i];
        auto& dst = voiceBuf[i];
        dst.brrAddr = src.brrAddr;
        dst.brrOffset = src.brrOffset;
        dst.pitchCounter = src.pitchCounter;
        dst.history1 = src.history0;
        dst.history2 = src.history1;
        dst.envLevel = src.envLevel;
        dst.envMode = (uint32_t)src.envMode;
        dst.loopFlag = src.loopFlag ? 1u : 0u;
        dst.endFlag = src.endFlag ? 1u : 0u;
        dst.outSample = src.outSample;
        dst.konLatched = src.konLatched ? 1u : 0u;
    }
    if (ppVoice) *ppVoice = voiceBuf.data();

    if (ppVMMaxL) *ppVMMaxL = &g.engine.dsp->vuMaxL;
    if (ppVMMaxR) *ppVMMaxR = &g.engine.dsp->vuMaxR;
}

// ============================================================================
// GetSPCRegs
// ============================================================================
SNESAPU_API void SNESAPU_CALL GetSPCRegs(u16* pPC, u8* pA, u8* pY, u8* pX, u8* pPSW, u8* pSP) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    if (pPC) *pPC = g.engine.cpu->PC;
    if (pA) *pA = g.engine.cpu->A;
    if (pY) *pY = g.engine.cpu->Y;
    if (pX) *pX = g.engine.cpu->X;
    if (pPSW) *pPSW = g.engine.cpu->getPSW();
    if (pSP) *pSP = g.engine.cpu->SP;
}

// ============================================================================
// InPort
// ============================================================================
SNESAPU_API void SNESAPU_CALL InPort(u8 port, u8 val) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    if (port < 4) {
        g.engine.cpu->ioIn[port] = val;
    }
}

// ============================================================================
// LoadSPCFile
// ============================================================================
SNESAPU_API void SNESAPU_CALL LoadSPCFile(void* pFile) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    const uint8_t* bytes = (const uint8_t*)pFile;
    // 仕様上は最低66048バイト保証されている前提 (呼び出し側の責務)
    ParsedSpc parsed = parseSpcBuffer(bytes, 0x10180);

    // ResetAPU相当の初期化 (amp維持)
    g.engine.resetApu();
    g.engine.dsp->ampScale = g.engine.dsp->ampScale; // 明示的に維持 (resetApu内ではampScaleを変更しない)

    g.engine.loadSpc(parsed);

    // SetAPULength/SetDSPVol はLoadSPCFileで解除される
    g.lengthSet = false;
    g.samplesEmitted = 0;
    g.dspVol = 1.0;

    g.resampler.reset();
    g.applyOpt();
}

// ============================================================================
// ResetAPU
// ============================================================================
SNESAPU_API void SNESAPU_CALL ResetAPU(u32 amp) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    g.engine.resetApu();

    // SetAPULength/SetDSPVol はResetAPUで解除される
    g.lengthSet = false;
    g.samplesEmitted = 0;
    g.dspVol = 1.0;

    if (amp != 0xffffffffu /* -1 */) {
        g.engine.dsp->ampScale = (double)amp / 65536.0;
    }

    g.resampler.reset();
    g.applyOpt();
}

// ============================================================================
// SeekAPU
// ============================================================================
SNESAPU_API void SNESAPU_CALL SeekAPU(u32 time, u8 fast) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    // time: 1/64000秒単位。まずサンプル数(32000Hz基準の内部エンジン)へ変換。
    uint64_t internalSamples = ((uint64_t)time * 32000ull) / 64000ull;

    if (fast) {
        // DSPエミュレーションを行わない高速シーク: CPUクロックのみ進める。
        int64_t clocks = (int64_t)internalSamples * CPU_CYCLES_PER_SAMPLE;
        g.engine.advanceCyclesNoDsp(clocks);
    } else {
        double dl, dr;
        for (uint64_t i = 0; i < internalSamples; i++) {
            g.engine.renderSample(dl, dr);
        }
    }

    g.samplesEmitted += (internalSamples * (uint64_t)g.rate) / 32000ull;
}

// ============================================================================
// SetAPULength
// ============================================================================
SNESAPU_API void SNESAPU_CALL SetAPULength(u32 time, u32 fade) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    g.playSamplesTotal = ((uint64_t)time * (uint64_t)g.rate) / 64000ull;
    g.fadeSamplesTotal = ((uint64_t)fade * (uint64_t)g.rate) / 64000ull;
    g.lengthSet = true;
}

// ============================================================================
// SetAPUOpt
// ============================================================================
SNESAPU_API void SNESAPU_CALL SetAPUOpt(u32 mixType, u32 numChn, u32 bits, u32 rate,
                                         u32 inter, u32 opts) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    if (mixType != 0xffffffffu) g.mixType = mixType;
    if (numChn != 0xffffffffu) g.numChn = numChn;
    if (bits != 0xffffffffu) g.bits = bits;
    if (rate != 0xffffffffu) {
        g.rate = rate;
        g.resampler.configure((double)rate);
    }
    if (inter != 0xffffffffu) g.inter = inter;
    if (opts != 0xffffffffu) g.opts = opts;

    g.applyOpt();
}

// ============================================================================
// SetAPURAM
// ============================================================================
SNESAPU_API void SNESAPU_CALL SetAPURAM(u32 addr, u8 val) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    g.engine.cpu->ram[addr & 0xffff] = val;
}

// ============================================================================
// SetAPUSmpClk
// ============================================================================
SNESAPU_API void SNESAPU_CALL SetAPUSmpClk(u32 speed) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    // speed: 65536=100%。CPU_CYCLES_PER_SAMPLE相当の消費量をスケールすることで実現。
    // 本移植ではエンジンのrenderSample内部で固定値32サイクル/サンプルを使うため、
    // ここでは簡易的にDSP側のpitchScaleとは独立した「演奏速度」を保持し、
    // EmuAPU呼び出し側でサンプル数計算時に加味する設計にはしていない。
    // (将来拡張: SpcEngineにcyclesPerSampleOverrideを持たせて speed/65536.0 * 32 を設定する)
    double factor = (double)speed / 65536.0;
    if (factor <= 0.0) factor = 1.0;
    // SpcEngineのCPU_CYCLES_PER_SAMPLEは定数のため、ここでは代替として
    // resamplerのレート比を微調整することで速度可変を近似する。
    double effectiveOutRate = (double)g.rate * factor;
    g.resampler.configure(effectiveOutRate > 0 ? (double)g.rate * (32000.0 / effectiveOutRate) * (g.rate / 32000.0) : (double)g.rate);
    // 上記は近似実装であることに注意 (正確な実装には CPU クロック配分の可変化が必要)
}

// ============================================================================
// SetDSPAmp
// ============================================================================
SNESAPU_API void SNESAPU_CALL SetDSPAmp(u32 amp) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    g.engine.dsp->ampScale = (double)amp / 65536.0;
}

// ============================================================================
// SetDSPEFBCT
// ============================================================================
SNESAPU_API void SNESAPU_CALL SetDSPEFBCT(s32 leak) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    g.engine.dsp->efbLeak = (double)leak / 32768.0;
}

// ============================================================================
// SetDSPPitch
// ============================================================================
SNESAPU_API void SNESAPU_CALL SetDSPPitch(u32 pitch) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    g.engine.dsp->pitchScale = (double)pitch / 32000.0;
}

// ============================================================================
// SetDSPReg
// ============================================================================
SNESAPU_API b8 SNESAPU_CALL SetDSPReg(u8 reg, u8 val) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    return g.engine.dsp->setRegExternal(reg, val) ? 1 : 0;
}

// ============================================================================
// SetDSPStereo
// ============================================================================
SNESAPU_API void SNESAPU_CALL SetDSPStereo(u32 sep) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    g.engine.dsp->stereoSep = (double)sep / 32768.0;
}

// ============================================================================
// SetDSPVol
// ============================================================================
SNESAPU_API void SNESAPU_CALL SetDSPVol(u32 vol) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);
    g.dspVol = (double)vol / 65536.0;
}

// ============================================================================
// SNESAPUInfo
// ============================================================================
SNESAPU_API void SNESAPU_CALL SNESAPUInfo(u32* pVer, u32* pMin, u32* pOpt) {
    // 本移植のバージョンを 9.9.9z 相当の識別できる値として返す。
    // フォーマット: 上位16bit=メジャー(4bit区切りBCD風), 次8bit=マイナー, 下位8bit=リビジョン(ASCII)
    if (pVer) *pVer = 0x00099900u; // 便宜上の識別バージョン (v9.9.9)
    if (pMin) *pMin = 0x00011000u; // 仕様通りの固定値
    if (pOpt) *pOpt = 0u;
}

// ============================================================================
// GetSNESAPUContextSize / GetSNESAPUContext / SetSNESAPUContext
// ============================================================================
namespace {
struct ContextBlob {
    Spc700 cpuSnapshot;
    Dsp dspSnapshot;
    uint32_t rate;
    uint64_t samplesEmitted;
    bool lengthSet;
    uint64_t playSamplesTotal;
    uint64_t fadeSamplesTotal;
    double dspVol;

    ContextBlob() : cpuSnapshot(nullptr), dspSnapshot(nullptr) {}
};
}

SNESAPU_API u32 SNESAPU_CALL GetSNESAPUContextSize() {
    return (u32)sizeof(ContextBlob);
}

SNESAPU_API u32 SNESAPU_CALL GetSNESAPUContext(void* pCtxOut) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    auto* blob = new (pCtxOut) ContextBlob(); // プレースメントnew (呼び出し側バッファは十分なサイズを確保済み前提)
    blob->cpuSnapshot = *g.engine.cpu; // Spc700はデフォルトコピー可能 (dsp_ポインタも含めコピーされる)
    blob->dspSnapshot = *g.engine.dsp;
    blob->dspSnapshot.ram = &blob->cpuSnapshot.ram; // コピー後、自身のram参照に貼り替え
    blob->cpuSnapshot.attachDsp(&blob->dspSnapshot);
    blob->rate = g.rate;
    blob->samplesEmitted = g.samplesEmitted;
    blob->lengthSet = g.lengthSet;
    blob->playSamplesTotal = g.playSamplesTotal;
    blob->fadeSamplesTotal = g.fadeSamplesTotal;
    blob->dspVol = g.dspVol;

    return 0; // 成功
}

SNESAPU_API u32 SNESAPU_CALL SetSNESAPUContext(void* pCtxIn) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    auto* blob = (ContextBlob*)pCtxIn;

    *g.engine.cpu = blob->cpuSnapshot;
    *g.engine.dsp = blob->dspSnapshot;
    g.engine.dsp->ram = &g.engine.cpu->ram;
    g.engine.cpu->attachDsp(g.engine.dsp.get());

    g.rate = blob->rate;
    g.samplesEmitted = blob->samplesEmitted;
    g.lengthSet = blob->lengthSet;
    g.playSamplesTotal = blob->playSamplesTotal;
    g.fadeSamplesTotal = blob->fadeSamplesTotal;
    g.dspVol = blob->dspVol;

    g.engine.loaded = true;
    g.resampler.reset();

    return 0; // 成功
}

// ============================================================================
// SNESAPUCallback
// ============================================================================
SNESAPU_API CBFUNC SNESAPU_CALL SNESAPUCallback(CBFUNC pCbFunc, u32 cbMask) {
    auto& g = state();
    std::lock_guard<std::mutex> lock(g.apiMutex);

    CBFUNC prev = g.callbackProc;
    g.callbackProc = pCbFunc;
    g.callbackMask = cbMask;

    // CBE_DSPREG (0x01) のみ本移植でサポート。Spc700::onDspRegWrite に橋渡しする。
    if (cbMask & CBE_DSPREG) {
        CBFUNC fn = pCbFunc;
        g.engine.cpu->onDspRegWrite = [fn](uint8_t addr, uint8_t value) -> uint8_t {
            if (!fn) return value;
            uint32_t result = fn(CBE_DSPREG, addr, value, nullptr);
            return (uint8_t)(result & 0xff);
        };
    } else {
        g.engine.cpu->onDspRegWrite = nullptr;
    }

    // CBE_S700FCH (0x02) もサポート。
    if (cbMask & CBE_S700FCH) {
        CBFUNC fn = pCbFunc;
        g.engine.cpu->onFetch = [fn](uint16_t pc, uint8_t opcode) -> FetchAction {
            if (!fn) return FetchAction::Normal;
            uint32_t addrPacked = (uint32_t)opcode | ((uint32_t)pc << 8); // 下位8bitが命令コード
            uint32_t result = fn(CBE_S700FCH, addrPacked, 0, nullptr);
            uint8_t lowByte = (uint8_t)(result & 0xff);
            switch (lowByte) {
                case 0x01: return FetchAction::Halt;
                case 0x02: return FetchAction::NopOut;
                case 0x03: return FetchAction::HaltEnv;
                default: return FetchAction::Normal;
            }
        };
    } else {
        g.engine.cpu->onFetch = nullptr;
    }

    return prev;
}