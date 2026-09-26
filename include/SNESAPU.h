// ============================================================================
// SNESAPU.h - SNESAPU.DLL 互換 公開ヘッダ
//
// libspc.js (SPC700 + S-DSP JS エミュレータ) を C++ に移植し、
// 改良版 SNESAPU.DLL (v2.x系) のAPI仕様に準拠したインターフェースを提供する。
//
// 参考: https://dgrfactory.jp/spcplay/snesapu.html
// ============================================================================
#ifndef SNESAPU_PORT_H
#define SNESAPU_PORT_H

#include <cstdint>

// ---------------------------------------------------------------------------
// 呼び出し規約 / エクスポートマクロ
// ---------------------------------------------------------------------------
#if defined(_WIN32)
  #define SNESAPU_CALL __stdcall
  #ifdef SNESAPU_BUILD_DLL
    #define SNESAPU_API extern "C" __declspec(dllexport)
  #else
    #define SNESAPU_API extern "C" __declspec(dllimport)
  #endif
#else
  #define SNESAPU_CALL
  #ifdef SNESAPU_BUILD_DLL
    #define SNESAPU_API extern "C" __attribute__((visibility("default")))
  #else
    #define SNESAPU_API extern "C"
  #endif
#endif

// ---------------------------------------------------------------------------
// 基本型 (SNESAPU.DLL の u8/u16/u32/s8/s16/s32/b8 に対応)
// ---------------------------------------------------------------------------
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef uint8_t  b8; // bool として扱うが 0/非0 の8bit値

// ---------------------------------------------------------------------------
// SetAPUOpt の mixType
// ---------------------------------------------------------------------------
enum {
    MIX_NONE  = 0,
    MIX_INT   = 1, // 互換のため残置 (内部的には MIX_FLOAT と同一)
    MIX_MMX   = 2, // 互換のため残置 (内部的には MIX_FLOAT と同一)
    MIX_FLOAT = 3,
};

// ---------------------------------------------------------------------------
// SetAPUOpt の inter (補間方式)
// ---------------------------------------------------------------------------
enum {
    INT_NONE   = 0,
    INT_LINEAR = 1,
    INT_CUBIC  = 2,
    INT_GAUSS  = 3,
    INT_SINC   = 4,
    INT_GAUSS4 = 7,
};

// ---------------------------------------------------------------------------
// SetAPUOpt の opts (DSPエミュレーションオプション、ビットOR指定)
// ---------------------------------------------------------------------------
enum {
    DSP_ANALOG   = 0x00000001,
    DSP_OLDSMP   = 0x00000002,
    DSP_SURND    = 0x00000004,
    DSP_REVERSE  = 0x00000008,
    DSP_NOECHO   = 0x00000010,
    DSP_NOPMOD   = 0x00000020,
    DSP_NOPREAD  = 0x00000040,
    DSP_NOFIR    = 0x00000080,
    DSP_BASS     = 0x00000100,
    DSP_NOENV    = 0x00000200,
    DSP_NONOISE  = 0x00000400,
    DSP_ECHOFIR  = 0x00000800,
    DSP_NOSURND  = 0x00001000,
    DSP_ENVSPD   = 0x00002000,
    DSP_NOPLMT   = 0x00004000,
    DSP_FLOAT    = 0x40000000,
    DSP_NOSAFE   = 0x80000000,
};

// ---------------------------------------------------------------------------
// GetAPUData で取得できる構造体
// ---------------------------------------------------------------------------
#pragma pack(push, 1)

// 1ボイス分の128byte中16byte区画に対応するDSPレジスタ定義
struct DSPReg {
    s8  volL, volR;      // 00,01 音量 (符号あり)
    u16 pitch;            // 02,03 ピッチ (下位14bit有効)
    u8  srcn;             // 04    音源番号
    u8  adsr1, adsr2;     // 05,06 ADSR設定
    u8  gain;             // 07    GAIN設定
    u8  envx;             // 08    エンベロープ値 (読取専用)
    s8  outx;             // 09    出力値 (読取専用)
    u8  reserved[6];      // 0A-0F (未使用/グローバルレジスタ領域と重複)
};

// 発音状態 (SNESAPU独自の内部Voice構造体相当)
struct Voice {
    u32 brrAddr;      // 現在のBRRブロックアドレス
    u32 brrOffset;    // ブロック内オフセット (0-15)
    u32 pitchCounter; // ピッチアキュムレータ
    s32 history1;     // BRR予測用履歴1
    s32 history2;     // BRR予測用履歴2
    s32 envLevel;      // 現在のエンベロープレベル (0-2047)
    u32 envMode;       // 0=off,1=attack,2=decay,3=sustain,4=release,5=kon-delay
    u32 loopFlag;
    u32 endFlag;
    s32 outSample;     // 直近の出力サンプル
    u32 konLatched;
};

#pragma pack(pop)

// SNESAPUCallbackProc 用エフェクトマスク
enum {
    CBE_DSPREG   = 0x00000001,
    CBE_S700FCH  = 0x00000002,
    CBE_REQBP    = 0x10000000,
    CBE_INCDATA  = 0x20000000,
    CBE_INCS700  = 0x40000000,
};

typedef u32 (SNESAPU_CALL *CBFUNC)(u32 effect, u32 addr, u32 value, void *pData);

// ---------------------------------------------------------------------------
// API 一覧 (改良版 SNESAPU.DLL v2.x系 互換)
// ---------------------------------------------------------------------------

// SPC700, DSPのエミュレーションを行い波形データを生成する
SNESAPU_API void* SNESAPU_CALL EmuAPU(void *pBuf, u32 len, u8 type);

// SPC700, DSPのレジスタを初期化する
SNESAPU_API void SNESAPU_CALL FixAPU(u16 pc, u8 a, u8 y, u8 x, u8 psw, u8 sp);

// 内部の演奏に関するメモリへのポインタを取得する
SNESAPU_API void SNESAPU_CALL GetAPUData(u8 **ppRAM, u8 **ppXRAM, u8 **ppOutPort,
                                          u32 **ppT64Cnt, DSPReg **ppDSP, Voice **ppVoice,
                                          u32 **ppVMMaxL, u32 **ppVMMaxR);

// 現在のSPC700レジスタ値を取得する
SNESAPU_API void SNESAPU_CALL GetSPCRegs(u16 *pPC, u8 *pA, u8 *pY, u8 *pX, u8 *pPSW, u8 *pSP);

// SPC700の入力ポートに値を書き込む
SNESAPU_API void SNESAPU_CALL InPort(u8 port, u8 val);

// SPCファイルのバッファを読み取り、新しいSPCを演奏する準備を行う
// pFile は最低 66048 バイトのバッファ (0x2E + 0x10000 + 0x80 = 0x10180)
SNESAPU_API void SNESAPU_CALL LoadSPCFile(void *pFile);

// SNESAPUの演奏に関するメモリを初期化する
SNESAPU_API void SNESAPU_CALL ResetAPU(u32 amp);

// 指定時間だけシークして演奏をスキップする
SNESAPU_API void SNESAPU_CALL SeekAPU(u32 time, u8 fast);

// 演奏時間とフェードアウト時間を設定する
SNESAPU_API void SNESAPU_CALL SetAPULength(u32 time, u32 fade);

// 波形データ生成の基本オプションを設定する
SNESAPU_API void SNESAPU_CALL SetAPUOpt(u32 mixType, u32 numChn, u32 bits, u32 rate,
                                         u32 inter, u32 opts);

// SPC700の64KB RAMに1byte書き込む
SNESAPU_API void SNESAPU_CALL SetAPURAM(u32 addr, u8 val);

// 演奏速度を設定する (65536 = 100%)
SNESAPU_API void SNESAPU_CALL SetAPUSmpClk(u32 speed);

// 音量を設定する (65536 = 100%)
SNESAPU_API void SNESAPU_CALL SetDSPAmp(u32 amp);

// エコーフィードバック反転度を設定する
SNESAPU_API void SNESAPU_CALL SetDSPEFBCT(s32 leak);

// ピッチ(音程)を設定する (32000 = 1倍)
SNESAPU_API void SNESAPU_CALL SetDSPPitch(u32 pitch);

// DSPレジスタに1byte書き込む
SNESAPU_API b8 SNESAPU_CALL SetDSPReg(u8 reg, u8 val);

// 左右拡散度を設定する (32768 = 標準)
SNESAPU_API void SNESAPU_CALL SetDSPStereo(u32 sep);

// マスタ音量比を設定する (フェードアウト用、65536 = 100%)
SNESAPU_API void SNESAPU_CALL SetDSPVol(u32 vol);

// SNESAPUのバージョン情報を取得する
SNESAPU_API void SNESAPU_CALL SNESAPUInfo(u32 *pVer, u32 *pMin, u32 *pOpt);

// --- コンテキスト(スナップショット)保存/復元 ---
SNESAPU_API u32 SNESAPU_CALL GetSNESAPUContextSize();
SNESAPU_API u32 SNESAPU_CALL GetSNESAPUContext(void *pCtxOut);
SNESAPU_API u32 SNESAPU_CALL SetSNESAPUContext(void *pCtxIn);

// --- コールバック(本移植では簡易実装、CBE_DSPREGのみ有効) ---
SNESAPU_API CBFUNC SNESAPU_CALL SNESAPUCallback(CBFUNC pCbFunc, u32 cbMask);

#endif // SNESAPU_PORT_H
