#pragma once

#include "picojson/picojson.h"
#include <Processing.NDI.Advanced.h>

#include "device.ndi2mxl.yaml.h"

#include <algorithm>
#include <cstdint>
#include <vector>

void createFlowDef(const ndi2mxl::Create_flow& createFlow, ndi2mxl::Flow_def& flowDef) {
    flowDef.colorspace = "BT709";
    flowDef.components = {ndi2mxl::Flow_def::Components_elem{"Y", createFlow.width, createFlow.height, 10},
                          ndi2mxl::Flow_def::Components_elem{"Cb", createFlow.width / 2, createFlow.height, 10},
                          ndi2mxl::Flow_def::Components_elem{"Cr", createFlow.width / 2, createFlow.height, 10}};
    flowDef.description = "NDI video flow";
    flowDef.format = "urn:x-nmos:format:video";
    flowDef.frame_width = createFlow.width;
    flowDef.frame_height = createFlow.height;
    flowDef.grain_rate = {createFlow.numerator, createFlow.denominator};
    flowDef.id = createFlow.id;
    flowDef.interlace_mode = "progressive";
    flowDef.label = createFlow.label;
    flowDef.media_type = createFlow.alpha == 0 ? "video/v210" : "video/v210a";
    flowDef.tags = {ndi2mxl::Flow_def::Tags_elem{"urn:x-nmos:tag:grouphint/v1.0", {"NDI Source:Video"}}};
}

std::string createVideoFlowJson(const ndi2mxl::Flow_def& flowDef) {
    auto root = picojson::object{};
    root["description"] = picojson::value(flowDef.description);

    root["id"] = picojson::value(flowDef.id);
    root["tags"] = picojson::value(picojson::object());
    root["format"] = picojson::value(flowDef.format);
    root["label"] = picojson::value(flowDef.label);
    root["parents"] = picojson::value(picojson::array());
    root["media_type"] = picojson::value(flowDef.media_type);

    auto tags = picojson::object{};
    auto groupHint = picojson::array{};
    groupHint.emplace_back(picojson::value{"Looping Source:Video"});
    tags["urn:x-nmos:tag:grouphint/v1.0"] = picojson::value(groupHint);
    root["tags"] = picojson::value(tags);

    auto grain_rate = picojson::object{};
    grain_rate["numerator"] = picojson::value(static_cast<double>(flowDef.grain_rate.numerator));
    grain_rate["denominator"] = picojson::value(static_cast<double>(flowDef.grain_rate.denominator));
    root["grain_rate"] = picojson::value(grain_rate);

    root["frame_width"] = picojson::value(static_cast<double>(flowDef.frame_width));
    root["frame_height"] = picojson::value(static_cast<double>(flowDef.frame_height));
    root["interlace_mode"] = picojson::value(flowDef.interlace_mode);
    root["colorspace"] = picojson::value(flowDef.colorspace);

    auto components = picojson::array{};
    auto add_component = [&](std::string const& name, int w, int h) {
        auto comp = picojson::object{};
        comp["name"] = picojson::value(name);
        comp["width"] = picojson::value(static_cast<double>(w));
        comp["height"] = picojson::value(static_cast<double>(h));
        comp["bit_depth"] = picojson::value(10.0);
        components.emplace_back(comp);
    };

    for (const auto& comp : flowDef.components) {
        add_component(comp.name, comp.width, comp.height);
    }

    root["components"] = picojson::value(components);

    return picojson::value(root).serialize(true);
}

inline std::uint16_t eight_to_ten(std::uint8_t v8) { return static_cast<std::uint16_t>(v8) << 2; }

// Convert one line of UYVY 8-bit into v210 packed 10-bit 4:2:2.
// This is the inverse of v210_to_uyvy_line in mxl_reader.hpp.
//
// UYVY layout (per 2 pixels): U Y0 V Y1
// v210 layout (per 6 pixels = 4 x uint32_t):
//   w0: Cb0[9:0]  | Y0[19:10]  | Cr0[29:20]
//   w1: Y1[9:0]   | Cb2[19:10] | Y2[29:20]
//   w2: Cr2[9:0]  | Y3[19:10]  | Cb4[29:20]
//   w3: Y4[9:0]   | Cr4[19:10] | Y5[29:20]
inline void uyvy_to_v210_line(const std::uint8_t* src_uyvy, std::uint8_t* dst_v210, int width_pixels) {
    const std::uint8_t* s = src_uyvy;
    std::uint32_t* d = reinterpret_cast<std::uint32_t*>(dst_v210);

    int x = 0;
    while (x < width_pixels) {
        // Read 6 pixels from UYVY (3 macro-pixels of U Y V Y = 12 bytes)
        std::uint16_t Cb0 = eight_to_ten(s[0]);
        std::uint16_t Y0 = eight_to_ten(s[1]);
        std::uint16_t Cr0 = eight_to_ten(s[2]);
        std::uint16_t Y1 = eight_to_ten(s[3]);

        std::uint16_t Cb2 = eight_to_ten(s[4]);
        std::uint16_t Y2 = eight_to_ten(s[5]);
        std::uint16_t Cr2 = eight_to_ten(s[6]);
        std::uint16_t Y3 = eight_to_ten(s[7]);

        std::uint16_t Cb4 = eight_to_ten(s[8]);
        std::uint16_t Y4 = eight_to_ten(s[9]);
        std::uint16_t Cr4 = eight_to_ten(s[10]);
        std::uint16_t Y5 = eight_to_ten(s[11]);

        // Pack into 4 x uint32_t words
        *d++ = (static_cast<std::uint32_t>(Cb0)) | (static_cast<std::uint32_t>(Y0) << 10) |
               (static_cast<std::uint32_t>(Cr0) << 20);

        *d++ = (static_cast<std::uint32_t>(Y1)) | (static_cast<std::uint32_t>(Cb2) << 10) |
               (static_cast<std::uint32_t>(Y2) << 20);

        *d++ = (static_cast<std::uint32_t>(Cr2)) | (static_cast<std::uint32_t>(Y3) << 10) |
               (static_cast<std::uint32_t>(Cb4) << 20);

        *d++ = (static_cast<std::uint32_t>(Y4)) | (static_cast<std::uint32_t>(Cr4) << 10) |
               (static_cast<std::uint32_t>(Y5) << 20);

        s += 12;  // 6 pixels × 2 bytes per pixel in UYVY
        x += 6;
    }
}

// Convert full-range 8-bit RGB into full-range 8-bit BT.709 Y'CbCr.
inline void rgb_to_ycbcr709(std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t& Y,
                            std::uint8_t& Cb, std::uint8_t& Cr) {
    const float y = 0.2126f * R + 0.7152f * G + 0.0722f * B;
    const float cb = 128.0f - 0.1146f * R - 0.3854f * G + 0.5f * B;
    const float cr = 128.0f + 0.5f * R - 0.4542f * G - 0.0458f * B;
    auto clamp8 = [](float v) -> std::uint8_t {
        v = std::min(std::max(v, 0.0f), 255.0f);
        return static_cast<std::uint8_t>(v + 0.5f);
    };
    Y = clamp8(y);
    Cb = clamp8(cb);
    Cr = clamp8(cr);
}

// Convert one line of NDI BGRA 8-bit into v210 packed 10-bit 4:2:2 fill data.
// If alpha10_out is non-null it is filled with the per-pixel 10-bit alpha for
// use when writing the v210a key buffer. Chroma is 4:2:2 subsampled by
// averaging adjacent pixel pairs.
inline void bgra_to_v210_line(const std::uint8_t* src_bgra, std::uint8_t* dst_v210, int width_pixels,
                              std::uint16_t* alpha10_out) {
    std::uint32_t* d = reinterpret_cast<std::uint32_t*>(dst_v210);

    int x = 0;
    while (x < width_pixels) {
        std::uint16_t Y[6] = {0}, Cb[6] = {0}, Cr[6] = {0};
        for (int i = 0; i < 6; i++) {
            int px = x + i;
            std::uint8_t B, G, R, A;
            if (px < width_pixels) {
                // NDI BGRA byte order: B, G, R, A
                B = src_bgra[px * 4 + 0];
                G = src_bgra[px * 4 + 1];
                R = src_bgra[px * 4 + 2];
                A = src_bgra[px * 4 + 3];
            } else {
                B = G = R = 0;
                A = 255;
            }
            std::uint8_t y8, cb8, cr8;
            rgb_to_ycbcr709(R, G, B, y8, cb8, cr8);
            Y[i] = eight_to_ten(y8);
            Cb[i] = eight_to_ten(cb8);
            Cr[i] = eight_to_ten(cr8);
            if (alpha10_out && px < width_pixels) {
                alpha10_out[px] = eight_to_ten(A);
            }
        }

        // 4:2:2 chroma: one sample per pixel pair (0-1, 2-3, 4-5)
        std::uint16_t Cb0 = (Cb[0] + Cb[1]) / 2;
        std::uint16_t Cr0 = (Cr[0] + Cr[1]) / 2;
        std::uint16_t Cb2 = (Cb[2] + Cb[3]) / 2;
        std::uint16_t Cr2 = (Cr[2] + Cr[3]) / 2;
        std::uint16_t Cb4 = (Cb[4] + Cb[5]) / 2;
        std::uint16_t Cr4 = (Cr[4] + Cr[5]) / 2;

        *d++ = (static_cast<std::uint32_t>(Cb0)) | (static_cast<std::uint32_t>(Y[0]) << 10) |
               (static_cast<std::uint32_t>(Cr0) << 20);
        *d++ = (static_cast<std::uint32_t>(Y[1])) | (static_cast<std::uint32_t>(Cb2) << 10) |
               (static_cast<std::uint32_t>(Y[2]) << 20);
        *d++ = (static_cast<std::uint32_t>(Cr2)) | (static_cast<std::uint32_t>(Y[3]) << 10) |
               (static_cast<std::uint32_t>(Cb4) << 20);
        *d++ = (static_cast<std::uint32_t>(Y[4])) | (static_cast<std::uint32_t>(Cr4) << 10) |
               (static_cast<std::uint32_t>(Y[5]) << 20);

        x += 6;
    }
}

// v210 fill stride in bytes: 6 pixels per group, 16 bytes per group (4 x uint32_t).
inline size_t v210FillStride(int width_pixels) {
    return static_cast<size_t>((width_pixels + 5) / 6) * 16;
}

// v210a key (alpha) stride in bytes: 3 luma samples per 32-bit block, lines are
// 4-byte aligned. See the mxl video/v210a specification.
inline size_t v210aKeyStride(int width_pixels) {
    return static_cast<size_t>((width_pixels + 2) / 3) * 4;
}

// Write one line of the v210a key (alpha) buffer. Each 32-bit little-endian
// block packs 3 alpha (luma) samples in bits 0-9, 10-19 and 20-29. Unused
// samples in the trailing block are zero padded.
inline void write_v210a_key_line(const std::uint16_t* alpha10, std::uint8_t* dst_key, int width_pixels) {
    std::uint32_t* d = reinterpret_cast<std::uint32_t*>(dst_key);
    int x = 0;
    while (x < width_pixels) {
        std::uint32_t block = 0;
        for (int i = 0; i < 3; i++) {
            std::uint16_t a = (x + i < width_pixels) ? (alpha10[x + i] & 0x3FF) : 0;
            block |= static_cast<std::uint32_t>(a) << (10 * i);
        }
        *d++ = block;
        x += 3;
    }
}

// Convert an NDI video frame into an mxl grain buffer.
//
// ndiHasAlpha : true when the NDI frame is BGRA (carries an alpha channel),
//               false when it is UYVY.
// mxlAlpha    : true when the mxl flow is video/v210a (fill + key), false for
//               plain video/v210 (fill only).
//
// Alpha is stripped when the NDI source has it but the mxl flow does not, and a
// fully opaque key is written when the mxl flow has alpha but the NDI source
// does not.
bool convertFrame(uint8_t* ndiData, uint8_t* mxlBuffer, int width, int height, int lineStride,
                  bool ndiHasAlpha, bool mxlAlpha) {
    const size_t v210Stride = v210FillStride(width);
    const size_t fillSize = v210Stride * static_cast<size_t>(height);
    const size_t keyStride = v210aKeyStride(width);
    uint8_t* keyBase = mxlBuffer + fillSize;

    // Per-line alpha scratch, defaulting to fully opaque (used as-is for UYVY).
    std::vector<std::uint16_t> alphaLine;
    if (mxlAlpha) {
        alphaLine.assign(static_cast<size_t>(width), eight_to_ten(255));
    }

    for (int y = 0; y < height; y++) {
        const uint8_t* ndiRow = ndiData + y * lineStride;
        uint8_t* fillRow = mxlBuffer + y * v210Stride;

        if (ndiHasAlpha) {
            bgra_to_v210_line(ndiRow, fillRow, width, mxlAlpha ? alphaLine.data() : nullptr);
        } else {
            uyvy_to_v210_line(ndiRow, fillRow, width);
            // alphaLine stays fully opaque
        }

        if (mxlAlpha) {
            write_v210a_key_line(alphaLine.data(), keyBase + y * keyStride, width);
        }
    }

    return true;
}
