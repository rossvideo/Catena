#pragma once

#include <stdexcept>
#include <string>

#include <picojson/picojson.h>

#include <mxl/mxl.h>
#include <mxl/flow.h>

namespace mxlcpp {
class MxlReader {
  public:
    MxlReader(std::string const& domain, std::string flowId) : _domain(domain), _flowId(flowId) {
        _instance = mxlCreateInstance(_domain.c_str(), "");
        if (_instance == nullptr) {
            throw std::runtime_error("Failed to create MXL instance");
        }

        auto ret = mxlCreateFlowReader(_instance, _flowId.c_str(), "", &_reader);
        if (ret != MXL_STATUS_OK) {
            throw std::runtime_error("Failed to create MXL Flow Reader with error status \"" +
                                     std::to_string(static_cast<int>(ret)) + "\"");
        }
        ret = mxlFlowReaderGetInfo(_reader, &_flowInfo);
        if (ret != MXL_STATUS_OK) {
            throw std::runtime_error("Failed to get MXL Flow Info with error status \"" +
                                     std::to_string(static_cast<int>(ret)) + "\"");
        }

        std::unique_ptr<st2138::Value> flowDef = getFlowDef();
        if (flowDef == nullptr) {
            throw std::runtime_error("Failed to get MXL Flow Definition");
        }

        _rate = _flowInfo.config.common.grainRate;
        _timeoutNs = static_cast<std::uint64_t>(
          1.0 * _rate.denominator * (1'000'000'000.0 / _rate.numerator) + 1'000'000ULL);

        _ndiFrame.xres = flowDef->struct_value().fields().at("frame_width").int32_value();
        _ndiFrame.yres = flowDef->struct_value().fields().at("frame_height").int32_value();
        _ndiFrame.FourCC = NDIlib_FourCC_video_type_UYVY;  // 8-bit 4:2:2
        _ndiFrame.frame_rate_N = static_cast<int>(_rate.numerator);
        _ndiFrame.frame_rate_D = static_cast<int>(_rate.denominator);
        _ndiFrame.picture_aspect_ratio = 16.0f / 9.0f;
        _ndiFrame.frame_format_type = NDIlib_frame_format_type_progressive;

        _ndiFrame.line_stride_in_bytes = _ndiFrame.xres * 2;

        _ndiBufferBytes =
          static_cast<size_t>(_ndiFrame.yres) * static_cast<size_t>(_ndiFrame.line_stride_in_bytes);

        _ndiFrame.p_data = new std::uint8_t[_ndiBufferBytes];
    }

    ~MxlReader() {
        if (_ndiFrame.p_data != nullptr) {
            delete[] _ndiFrame.p_data;
            _ndiFrame.p_data = nullptr;
        }
        if (_reader != nullptr) {
            mxlReleaseFlowReader(_instance, _reader);
        }
        if (_instance != nullptr) {
            mxlDestroyInstance(_instance);
        }
    }

    const mxlFlowReader& get() const { return _reader; }

    const mxlInstance& instance() const { return _instance; }

    std::unique_ptr<st2138::Value> getFlowDef() const {
        char buffer[1024];
        size_t bufferSize = sizeof(buffer);
        mxlStatus status = mxlGetFlowDef(_instance, _flowId.c_str(), buffer, &bufferSize);
        if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to get MXL flow definition: " << status;
            return nullptr;
        }
        picojson::value picoVal;
        std::string err = picojson::parse(picoVal, buffer);
        if (!err.empty()) {
            LOG(ERROR) << "Failed to parse flow definition JSON: " << err;
            return nullptr;
        }
        // we have json to parse now
        std::unique_ptr<st2138::Value> retValue = std::make_unique<st2138::Value>();
        google::protobuf::Map<std::string, st2138::Value>* rootFields =
          retValue->mutable_struct_value()->mutable_fields();
        st2138::Value value;
        picojson::object& obj = picoVal.get<picojson::object>();
        //strings
        std::vector<std::string> strings{"id",         "description",    "format",    "label",
                                         "media_type", "interlace_mode", "colorspace"};
        for (const std::string& key : strings) {
            if (obj.contains(key)) {
                value.set_string_value(obj[key].get<std::string>());
                rootFields->insert({key, value});
            }
        }
        // numbers
        std::vector<std::string> numbers{"frame_width", "frame_height"};
        for (const std::string& key : numbers) {
            if (obj.contains(key)) {
                value.set_int32_value(static_cast<int32_t>(obj[key].get<double>()));
                rootFields->insert({key, value});
            }
        }
        // tags
        if (obj.contains("tags")) {
            picojson::object& tags = obj["tags"].get<picojson::object>();
            /* making this structure:
                "string_array_values": {
                    "struct_values": [{
                        "fields": {
                            "name": { "string_value": "NAME" },
                            "values": { "string_array_values": { "strings": ["VALUE1", "VALUE23"] } }
                        }
                    }]
                }
                */
            google::protobuf::RepeatedPtrField<st2138::StructValue>* structValues =
              value.mutable_struct_array_values()->mutable_struct_values();
            for (const auto& [tagKey, tagValue] : tags) {
                picojson::array tagArray = tagValue.get<picojson::array>();
                st2138::Value nameValue;
                nameValue.set_string_value(tagKey);
                st2138::Value tagValues;
                google::protobuf::RepeatedPtrField<std::string>* strings =
                  tagValues.mutable_string_array_values()->mutable_strings();
                for (const auto& tagItem : tagArray) {
                    strings->Add()->assign(tagItem.get<std::string>());
                }
                google::protobuf::Map<std::string, st2138::Value>* tag =
                  structValues->Add()->mutable_fields();
                tag->insert({"name", nameValue});
                tag->insert({"values", tagValues});
            }
            rootFields->insert({"tags", value});
        }
        // grain rate
        if (obj.contains("grain_rate")) {
            picojson::object& grainRate = obj["grain_rate"].get<picojson::object>();
            st2138::Value grainRateValue;
            google::protobuf::Map<std::string, st2138::Value>* grainRateFields =
              grainRateValue.mutable_struct_value()->mutable_fields();
            if (grainRate.contains("numerator") && grainRate.contains("denominator")) {
                int32_t numerator = static_cast<int32_t>(grainRate["numerator"].get<double>());
                value.set_int32_value(numerator);
                grainRateFields->insert({"numerator", value});
                int32_t denominator = static_cast<int32_t>(grainRate["denominator"].get<double>());
                value.set_int32_value(denominator);
                grainRateFields->insert({"denominator", value});
            }
            rootFields->insert({"grain_rate", grainRateValue});
        }
        // components
        if (obj.contains("components")) {
            picojson::array components = obj["components"].get<picojson::array>();
            google::protobuf::RepeatedPtrField<st2138::StructValue>* structValues =
              value.mutable_struct_array_values()->mutable_struct_values();
            for (const auto& compItem : components) {
                picojson::object compObj = compItem.get<picojson::object>();
                st2138::Value nameValue;
                nameValue.set_string_value(compObj["name"].get<std::string>());
                st2138::Value widthValue;
                widthValue.set_int32_value(static_cast<int32_t>(compObj["width"].get<double>()));
                st2138::Value heightValue;
                heightValue.set_int32_value(static_cast<int32_t>(compObj["height"].get<double>()));
                st2138::Value bitDepthValue;
                bitDepthValue.set_int32_value(static_cast<int32_t>(compObj["bit_depth"].get<double>()));
                google::protobuf::Map<std::string, st2138::Value>* compMap =
                  structValues->Add()->mutable_fields();
                compMap->insert({"name", nameValue});
                compMap->insert({"width", widthValue});
                compMap->insert({"height", heightValue});
                compMap->insert({"bit_depth", bitDepthValue});
            }
            rootFields->insert({"components", value});
        }
        st2138::Value parents;
        parents.mutable_string_array_values();
        rootFields->insert({"parents", parents});  // empty for now
        return retValue;
    }

    const NDIlib_video_frame_v2_t* dumpNdiFrame(uint64_t& sleepNs) {
        uint64_t index = mxlGetCurrentIndex(&_rate);

        mxlGrainInfo grainInfo;
        uint8_t* payload = nullptr;

        mxlStatus status = mxlFlowReaderGetGrain(_reader, index, 1000000000, &grainInfo, &payload);
        if (status == MXL_ERR_OUT_OF_RANGE_TOO_EARLY) {
            // We are too early somehow, keep trying the same grain index
            mxlFlowReaderGetInfo(_reader, &_flowInfo);
            LOG(WARNING) << "Failed to get samples at index " << index << ": TOO EARLY. Last published "
                         << _flowInfo.runtime.headIndex;
            return nullptr;
        } else if (status == MXL_ERR_OUT_OF_RANGE_TOO_LATE) {
            mxlFlowReaderGetInfo(_reader, &_flowInfo);
            LOG(WARNING) << "Failed to get grain at index " << index << ": TOO LATE. Last published "
                         << _flowInfo.runtime.headIndex;
            // Grain expired. Realign to current index. GStreamer repeats the last valid frame for missing data; consuming applications
            // should do the same.
            index = mxlGetCurrentIndex(&_rate);
            return nullptr;
        } else if (status == MXL_ERR_TIMEOUT) {
            // Timeout waiting for grain
            LOG(WARNING) << "Timeout waiting for grain at index " << index;
            return nullptr;
        } else if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Unexpected error when reading the grain " << index << " with status "
                       << static_cast<int>(status) << ". Exiting.";
            return nullptr;
        }

        if (grainInfo.validSlices != grainInfo.totalSlices) {
            // Full frame not ready yet
            return nullptr;
        }

        if (!(grainInfo.flags & MXL_GRAIN_FLAG_INVALID)) {
            // valid grain, process payload here
            // copy payload to NDI frame
            // LOG(INFO) << grainInfo.grainSize << " bytes read for grain index " << grainInfo.index;


            // v210 is packed 10-bit 4:2:2
            const int width = _ndiFrame.xres;   // 1920
            const int height = _ndiFrame.yres;  // 1080

            // Stride of the v210 data in bytes (already includes padding)
            const size_t v210_stride_bytes = grainInfo.grainSize / height;

            const std::uint8_t* src = payload;

            for (int y = 0; y < height; ++y) {
                const std::uint8_t* src_line = src + y * v210_stride_bytes;
                std::uint8_t* dst_line = _ndiFrame.p_data + y * _ndiFrame.line_stride_in_bytes;

                v210_to_uyvy_line(src_line, dst_line, width);
            }
            sleepNs = mxlGetNsUntilIndex(index + 1, &_rate);
            return &_ndiFrame;
        }
        return nullptr;
    }

  private:
    std::string _domain;
    std::string _flowId;
    mxlInstance _instance;
    mxlFlowReader _reader;
    mxlFlowInfo _flowInfo;
    mxlRational _rate;
    uint64_t _timeoutNs;
    NDIlib_video_frame_v2_t _ndiFrame{};
    size_t _ndiBufferBytes = 0;


    // Map 10-bit (0..1023) to 8-bit (0..255)
    inline std::uint8_t ten_to_eight(std::uint16_t v10) {
        // Fast: drop 2 LSBs
        return static_cast<std::uint8_t>(v10 >> 2);
    }

    // Convert one line of MXL 10-bit 4:2:2 into UYVY 8-bit.
    //
    // Assumes MXL samples for 2 pixels are in the order: Y0, Cb0, Y1, Cr0
    // and each sample is stored in a 16-bit word with only the lower 10 bits used.
    void v210_to_uyvy_line(const std::uint8_t* src_v210, std::uint8_t* dst_uyvy,
                           int width_pixels  // e.g. 1920
    ) {
        const std::uint32_t* s = reinterpret_cast<const std::uint32_t*>(src_v210);
        std::uint8_t* d = dst_uyvy;

        auto put_pair = [&](std::uint16_t U10, std::uint16_t Y0_10, std::uint16_t V10, std::uint16_t Y1_10) {
            const std::uint8_t U8 = ten_to_eight(U10);
            const std::uint8_t V8 = ten_to_eight(V10);
            const std::uint8_t Y0 = ten_to_eight(Y0_10);
            const std::uint8_t Y1 = ten_to_eight(Y1_10);

            *d++ = U8;
            *d++ = Y0;
            *d++ = V8;
            *d++ = Y1;
        };

        int x = 0;
        while (x < width_pixels) {
            std::uint32_t w0 = *s++;
            std::uint32_t w1 = *s++;
            std::uint32_t w2 = *s++;
            std::uint32_t w3 = *s++;

            std::uint16_t Cb0 = (w0) & 0x3FF;
            std::uint16_t Y0 = (w0 >> 10) & 0x3FF;
            std::uint16_t Cr0 = (w0 >> 20) & 0x3FF;

            std::uint16_t Y1 = (w1) & 0x3FF;
            std::uint16_t Cb2 = (w1 >> 10) & 0x3FF;
            std::uint16_t Y2 = (w1 >> 20) & 0x3FF;

            std::uint16_t Cr2 = (w2) & 0x3FF;
            std::uint16_t Y3 = (w2 >> 10) & 0x3FF;
            std::uint16_t Cb4 = (w2 >> 20) & 0x3FF;

            std::uint16_t Y4 = (w3) & 0x3FF;
            std::uint16_t Cr4 = (w3 >> 10) & 0x3FF;
            std::uint16_t Y5 = (w3 >> 20) & 0x3FF;

            // Pixels 0–1
            put_pair(Cb0, Y0, Cr0, Y1);
            // Pixels 2–3
            put_pair(Cb2, Y2, Cr2, Y3);
            // Pixels 4–5
            put_pair(Cb4, Y4, Cr4, Y5);

            x += 6;
        }
    }
};
}  // namespace mxlcpp
