// ============================================================================
// Spc700.h - SPC700 CPUエミュレータ (libspc.js の SPC700 クラスの移植)
// ============================================================================
#ifndef SNESAPU_SPC700_H
#define SNESAPU_SPC700_H

#include <cstdint>
#include <cstring>
#include <array>
#include <functional>

namespace snesapu {

class Dsp; // 前方宣言

// フェッチ・コールバック用の戻り値 (SNESAPUCallbackProc の CBE_S700FCH 相当)
enum class FetchAction : uint8_t {
    Normal = 0x00,
    Halt   = 0x01, // 命令実行を中断し、次のEmuAPUまで何もしない
    NopOut = 0x02, // 命令をNOPに置換 (PCは進めないがタイマーは進める)
    HaltEnv= 0x03, // 命令実行を中断し、DSPエンベロープ処理も停止
};

class Spc700 {
public:
    explicit Spc700(Dsp* dsp);

    // --- レジスタ ---
    uint8_t  A = 0, X = 0, Y = 0, SP = 0;
    uint16_t PC = 0;

    // --- フラグ (PSW) ---
    uint8_t flagN = 0, flagV = 0, flagP = 0, flagB = 0;
    uint8_t flagH = 0, flagI = 0, flagZ = 0, flagC = 0;

    // --- メモリ ---
    std::array<uint8_t, 0x10000> ram{};

    // --- I/Oポート ---
    std::array<uint8_t, 4> ioIn{};
    std::array<uint8_t, 4> ioOut{};

    // --- タイマー ---
    std::array<uint8_t, 3> timerEnable{};
    std::array<uint8_t, 3> timerTarget{};
    std::array<uint8_t, 3> timerCounter{};
    std::array<uint8_t, 3> timerOut{};
    std::array<int32_t, 3> tAccum{};

    bool romEnable = true;
    uint64_t cycles = 0;

    // IPL ROM (0xFFC0-0xFFFF)
    static const std::array<uint8_t, 64> bootRom;

    // --- コールバック (SNESAPUCallbackProc 互換) ---
    // CBE_DSPREG: DSPレジスタ書き込みフック。戻り値が実際に書き込む値。
    std::function<uint8_t(uint8_t addr, uint8_t value)> onDspRegWrite;
    // CBE_S700FCH: フェッチフック。戻り値でエミュレーション継続方法を制御。
    std::function<FetchAction(uint16_t pc, uint8_t opcode)> onFetch;

    void reset();

    uint8_t read(uint32_t addr);
    void    write(uint32_t addr, uint8_t val);

    uint8_t readTimerOut(int t);
    void    tickTimers(int cyc);

    uint8_t getPSW() const;
    void    setPSW(uint8_t v);

    inline uint32_t dpBase() const { return flagP ? 0x100u : 0x000u; }

    uint8_t setNZ8(int v);

    void    push8(uint8_t v);
    uint8_t pop8();
    void    push16(uint16_t v);
    uint16_t pop16();

    uint8_t  fetch8();
    uint16_t fetch16();

    inline uint32_t dp(uint32_t off) const { return dpBase() | (off & 0xffu); }
    inline uint32_t dpNext(uint32_t a) const { return (a & 0x100u) | ((a + 1u) & 0xffu); }
    uint16_t rd16dp(uint32_t a);
    void     wr16dp(uint32_t a, uint16_t w);

    uint8_t adc(uint8_t a, uint8_t b, uint8_t carryIn);
    uint8_t sbc(uint8_t a, uint8_t b, uint8_t carryIn);

    // 1命令実行し消費サイクル数を返す。suppressTimers=trueの場合はタイマーを進めない(Halt系フック用)。
    int step();

    // "halted" 状態: フェッチフックが Halt/HaltEnv を返した場合にセットされる。
    // EmuAPU側はこのフラグを見て、残りサイクルの消費のみ行い実行はスキップする。
    bool halted = false;
    bool haltedEnv = false; // true の場合DSPエンベロープ処理も止める(Dsp側で参照)

    // SpcEngine構築時、Spc700とDspが相互参照するためコンストラクタ後にDSPを紐付ける。
    void attachDsp(Dsp* dsp) { dsp_ = dsp; }

private:
    Dsp* dsp_ = nullptr;

    int branch(bool cond, uint8_t disp);
    int exec(uint8_t op);

    // オペコードテーブル: 各関数は消費サイクル数を返す
    using OpFn = int (Spc700::*)();
    std::array<OpFn, 256> opTable{};
    void buildOpTable();

    // --- オペコード実装 (opcodes.inc に分割実装、本クラスのメンバ関数として定義) ---
    #include "Spc700Ops.inc.h"
};

} // namespace snesapu

#endif // SNESAPU_SPC700_H