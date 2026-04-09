#pragma once

#include "picojson/picojson.h"
#include <Processing.NDI.Advanced.h>

#include "device.ndi2mxl.yaml.h"

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
    flowDef.media_type = "video/v210";
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

bool convertFrame(uint8_t* ndiData, uint8_t* mxlBuffer, int width, int height, int lineStride) {
    // v210 stride: 6 pixels per group, 16 bytes per group (4 × uint32_t)
    const size_t v210Stride = static_cast<size_t>((width + 5) / 6) * 16;

    for (int y = 0; y < height; y++) {
        const uint8_t* ndiRow = ndiData + y * lineStride;
        uint8_t* mxlRow = mxlBuffer + y * v210Stride;
        uyvy_to_v210_line(ndiRow, mxlRow, width);
    }

    return true;
}
