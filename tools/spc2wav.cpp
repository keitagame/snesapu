// ============================================================================
// spc2wav.cpp - SNESAPU移植の動作確認用CLIツール
// 使い方: spc2wav input.spc output.wav [seconds]
// ============================================================================
#include "SNESAPU.h"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>

#pragma pack(push, 1)
struct WavHeader {
    char riff[4] = {'R','I','F','F'};
    uint32_t chunkSize;
    char wave[4] = {'W','A','V','E'};
    char fmt[4] = {'f','m','t',' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 2;
    uint32_t sampleRate = 44100;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d','a','t','a'};
    uint32_t dataSize;
};
#pragma pack(pop)

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s input.spc output.wav [seconds=60]\n", argv[0]);
        return 1;
    }

    const char* inPath = argv[1];
    const char* outPath = argv[2];
    double seconds = (argc >= 4) ? std::atof(argv[3]) : 60.0;

    FILE* f = std::fopen(inPath, "rb");
    if (!f) {
        std::fprintf(stderr, "cannot open %s\n", inPath);
        return 1;
    }
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size < 0x10180) {
        std::fprintf(stderr, "file too small (%ld bytes)\n", size);
        std::fclose(f);
        return 1;
    }
    std::vector<uint8_t> buf(size);
    std::fread(buf.data(), 1, size, f);
    std::fclose(f);

    const uint32_t rate = 44100;
    const uint32_t numChn = 2;
    const uint32_t bits = 16;

    SetAPUOpt(MIX_FLOAT, numChn, bits, rate, INT_GAUSS, 0);
    LoadSPCFile(buf.data());

    uint32_t totalSamples = (uint32_t)(seconds * rate);
    std::vector<int16_t> pcm((size_t)totalSamples * numChn);

    void* endPtr = EmuAPU(pcm.data(), totalSamples, /*type=*/1);
    size_t writtenBytes = (uint8_t*)endPtr - (uint8_t*)pcm.data();
    size_t writtenSamples = writtenBytes / (sizeof(int16_t) * numChn);

    WavHeader hdr;
    hdr.sampleRate = rate;
    hdr.numChannels = (uint16_t)numChn;
    hdr.bitsPerSample = (uint16_t)bits;
    hdr.blockAlign = (uint16_t)(numChn * bits / 8);
    hdr.byteRate = hdr.sampleRate * hdr.blockAlign;
    hdr.dataSize = (uint32_t)(writtenSamples * hdr.blockAlign);
    hdr.chunkSize = 36 + hdr.dataSize;

    FILE* out = std::fopen(outPath, "wb");
    if (!out) {
        std::fprintf(stderr, "cannot open %s for writing\n", outPath);
        return 1;
    }
    std::fwrite(&hdr, sizeof(hdr), 1, out);
    std::fwrite(pcm.data(), 1, hdr.dataSize, out);
    std::fclose(out);

    std::printf("Wrote %s: %zu samples (%.2f sec)\n", outPath, writtenSamples,
                (double)writtenSamples / rate);

    return 0;
}
