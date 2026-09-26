// ============================================================================
// SpcEngine.h - SPC700 + DSP を統合するエンジン (libspc.js の SPCEngine の移植)
// ============================================================================
#ifndef SNESAPU_SPCENGINE_H
#define SNESAPU_SPCENGINE_H

#include "Spc700.h"
#include "Dsp.h"
#include "SpcFile.h"
#include <memory>

namespace snesapu {

inline constexpr int CPU_CYCLES_PER_SAMPLE = 32; // 32000Hz出力時の基準値 (1024000Hz / 32000Hz)

class SpcEngine {
public:
    SpcEngine();

    std::unique_ptr<Dsp> dsp;
    std::unique_ptr<Spc700> cpu;

    bool loaded = false;
    int64_t cycleAccum = 0;

    // XRAM: IPL ROM読み込み時に退避される元メモリ内容 (アドレスFFC0-FFFF, 64byte)
    // SNESAPU.DLL の GetAPUData ppXRAM 相当。ここでは常にIPL ROM無効時の
    // 元のRAM内容を保持するバッファとして単純に確保する(実際のSPCファイルには
    // IPL ROM部分の元データは含まれないため、ロード時点のRAM内容をそのまま保持)。
    std::array<uint8_t, 128> xram{};

    void loadSpc(const ParsedSpc& parsed);

    // FixAPU相当: レジスタのみ設定 (RAM/DSPレジスタは別途 GetAPUData 経由で設定する運用を想定)
    void fixRegs(uint16_t pc, uint8_t a, uint8_t y, uint8_t x, uint8_t psw, uint8_t sp);

    // ResetAPU相当: SPC700/DSPを初期状態に戻す
    void resetApu();

    // 1サンプル分(32000Hz基準)生成する
    void renderSample(double& outL, double& outR);

    // 指定クロック数(1024000Hz基準)だけCPUを進める。DSPは生成しない(SeekAPU向け高速モード用)。
    void advanceCyclesNoDsp(int64_t clocks);
};

} // namespace snesapu

#endif // SNESAPU_SPCENGINE_H