// ============================================================================
// SpcFile.h - SPCファイルバイナリパーサ (libspc.js の parseSPC() の移植)
// ============================================================================
#ifndef SNESAPU_SPCFILE_H
#define SNESAPU_SPCFILE_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <stdexcept>

namespace snesapu {

struct SpcMeta {
    std::string title;
    std::string game;
    std::string dumper;
    std::string comment;
};

struct ParsedSpc {
    uint16_t pc;
    uint8_t a, x, y, psw, sp;

    const uint8_t* ram;      // 0x100 から始まる 0x10000 バイト (呼び出し元バッファ内を指す)
    const uint8_t* dspRegs;  // 0x10100 から始まる 0x80 バイト

    SpcMeta meta;
};

// SNESAPU.DLLのLoadSPCFile仕様に合わせ、最低66048(0x10180)バイトのバッファを要求する。
// バッファの内容はそのまま保持され続けるので、戻り値のram/dspRegsポインタは
// pFileBuffer が有効な間のみ有効。
inline ParsedSpc parseSpcBuffer(const uint8_t* bytes, size_t length) {
    if (length < 0x10180) {
        throw std::runtime_error("Invalid SPC file size.");
    }

    // ヘッダ文字列チェック ("SNES-SPC700 Sound File")
    static const char kMagic[] = "SNES-SPC700 Sound File";
    for (size_t i = 0; i < sizeof(kMagic) - 1; i++) {
        if (bytes[i] != (uint8_t)kMagic[i]) {
            throw std::runtime_error("Invalid SPC header format.");
        }
    }

    ParsedSpc out{};
    out.pc = (uint16_t)(bytes[0x25] | (bytes[0x26] << 8));
    out.a = bytes[0x27];
    out.x = bytes[0x28];
    out.y = bytes[0x29];
    out.psw = bytes[0x2a];
    out.sp = bytes[0x2b];

    auto readString = [&](size_t offset, size_t len) -> std::string {
        std::string s;
        s.reserve(len);
        for (size_t i = 0; i < len; i++) {
            uint8_t c = bytes[offset + i];
            if (c == 0) break;
            s.push_back((char)c);
        }
        // trim (前後の空白除去、JSの String.trim() 相当)
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    };

    out.meta.title   = readString(0x2e, 32);
    out.meta.game    = readString(0x4e, 32);
    out.meta.dumper  = readString(0x6e, 16);
    out.meta.comment = readString(0x7e, 32);

    out.ram = bytes + 0x100;
    out.dspRegs = bytes + 0x10100;

    return out;
}

} // namespace snesapu

#endif // SNESAPU_SPCFILE_H