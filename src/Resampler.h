// ============================================================================
// Resampler.h - 32000Hz -> 任意サンプリングレートへのcubic補間リサンプラー
// (libspc.js の SPCPlayer._advanceDspSample / _cubic / _process の移植)
// ============================================================================
#ifndef SNESAPU_RESAMPLER_H
#define SNESAPU_RESAMPLER_H

#include "SpcEngine.h"
#include "DspTables.h"
#include <array>

namespace snesapu {

class Resampler {
public:
    void configure(double outputRate) {
        resampleRatio = SDSP_RATE / outputRate;
        srcPos = 0.0;
        haveSample = false;
        hL.fill(0.0); hR.fill(0.0);
    }

    void reset() {
        srcPos = 0.0;
        haveSample = false;
        hL.fill(0.0); hR.fill(0.0);
    }

    static double cubic(double y0, double y1, double y2, double y3, double t) {
        double a = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
        double b =        y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
        double c = -0.5 * y0            + 0.5 * y2;
        return ((a * t + b) * t + c) * t + y1;
    }

    // outCount サンプルペアを生成する。amp(音量スケール)とfade(0-1)を適用。
    // volumeGain: SetDSPAmp/ResetAPU由来の追加音量(65536=1倍相当をdoubleで正規化した値)
    void render(SpcEngine& engine, double* outL, double* outR, int outCount, double volumeGain) {
        if (!haveSample) {
            for (int k = 0; k < 4; k++) advanceDspSample(engine);
            haveSample = true;
        }

        double ratio = resampleRatio;

        for (int i = 0; i < outCount; i++) {
            while (srcPos >= 1.0) {
                advanceDspSample(engine);
                srcPos -= 1.0;
            }
            double t = srcPos;
            double l = cubic(hL[0], hL[1], hL[2], hL[3], t);
            double r = cubic(hR[0], hR[1], hR[2], hR[3], t);

            // 元のJS実装は固定で x5 のヘッドルームゲインを掛けている (SNESAPU側のampはさらに別途乗算)
            l = l * 5.0 * volumeGain;
            r = r * 5.0 * volumeGain;

            outL[i] = l;
            outR[i] = r;

            srcPos += ratio;
        }
    }

private:
    void advanceDspSample(SpcEngine& engine) {
        hL[0] = hL[1]; hL[1] = hL[2]; hL[2] = hL[3];
        hR[0] = hR[1]; hR[1] = hR[2]; hR[2] = hR[3];
        double l, r;
        engine.renderSample(l, r);
        hL[3] = l; hR[3] = r;
    }

    double resampleRatio = 1.0;
    double srcPos = 0.0;
    bool haveSample = false;
    std::array<double, 4> hL{};
    std::array<double, 4> hR{};
};

} // namespace snesapu

#endif // SNESAPU_RESAMPLER_H