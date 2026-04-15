#pragma once

#include <stdexcept>
#include <string>

#include <picojson/picojson.h>

#include <mxl/mxl.h>
#include <mxl/flow.h>

#include "device.mxl2ndi_sink.yaml.h"

#include "Logger.h"

namespace mxlcpp {

bool fillFlowDef(mxlInstance instance, const std::string& flowId, mxl2ndi_sink::Flow_def& flowDef) {
    char buffer[1024];
    size_t bufferSize = sizeof(buffer);
    mxlStatus status = mxlGetFlowDef(instance, flowId.c_str(), buffer, &bufferSize);
    if (status != MXL_STATUS_OK) {
        LOG(ERROR) << "Failed to get MXL flow definition: " << flowId << " with error status \"" << status
                   << "\"";
        return false;
    }
    picojson::value picoVal;
    std::string err = picojson::parse(picoVal, buffer);
    if (!err.empty()) {
        LOG(ERROR) << "Failed to parse flow definition JSON: " << err;
        return false;
    }
    // we have json to parse now
    std::unique_ptr<st2138::Value> retValue = std::make_unique<st2138::Value>();
    google::protobuf::Map<std::string, st2138::Value>* rootFields =
      retValue->mutable_struct_value()->mutable_fields();
    st2138::Value value;
    picojson::object& obj = picoVal.get<picojson::object>();
    if (obj.contains("id") && obj["id"].is<std::string>()) {
        flowDef.id = obj["id"].get<std::string>();
    }
    if (obj.contains("description") && obj["description"].is<std::string>()) {
        flowDef.description = obj["description"].get<std::string>();
    }
    if (obj.contains("format") && obj["format"].is<std::string>()) {
        flowDef.format = obj["format"].get<std::string>();
    }
    if (obj.contains("label") && obj["label"].is<std::string>()) {
        flowDef.label = obj["label"].get<std::string>();
    }
    if (obj.contains("media_type") && obj["media_type"].is<std::string>()) {
        flowDef.media_type = obj["media_type"].get<std::string>();
    }
    if (obj.contains("interlace_mode") && obj["interlace_mode"].is<std::string>()) {
        flowDef.interlace_mode = obj["interlace_mode"].get<std::string>();
    }
    if (obj.contains("colorspace") && obj["colorspace"].is<std::string>()) {
        flowDef.colorspace = obj["colorspace"].get<std::string>();
    }
    if (obj.contains("frame_width") && obj["frame_width"].is<double>()) {
        flowDef.frame_width = static_cast<int32_t>(obj["frame_width"].get<double>());
    }
    if (obj.contains("frame_height") && obj["frame_height"].is<double>()) {
        flowDef.frame_height = static_cast<int32_t>(obj["frame_height"].get<double>());
    }
    if (obj.contains("tags") && obj["tags"].is<picojson::object>()) {
        flowDef.tags.clear();
        picojson::object& tags = obj["tags"].get<picojson::object>();
        for (const auto& [tagKey, tagValue] : tags) {
            if (tagValue.is<picojson::array>()) {
                picojson::array tagArray = tagValue.get<picojson::array>();
                std::vector<std::string> values;
                for (const auto& tagItem : tagArray) {
                    if (tagItem.is<std::string>()) {
                        values.push_back(tagItem.get<std::string>());
                    }
                }
                flowDef.tags.push_back({tagKey, values});
            }
        }
    }
    if (obj.contains("grain_rate") && obj["grain_rate"].is<picojson::object>()) {
        picojson::object& grainRate = obj["grain_rate"].get<picojson::object>();
        if (grainRate.contains("numerator") && grainRate["numerator"].is<double>() &&
            grainRate.contains("denominator") && grainRate["denominator"].is<double>()) {
            flowDef.grain_rate.numerator = static_cast<int32_t>(grainRate["numerator"].get<double>());
            flowDef.grain_rate.denominator = static_cast<int32_t>(grainRate["denominator"].get<double>());
        }
    }
    if (obj.contains("components") && obj["components"].is<picojson::array>()) {
        flowDef.components.clear();
        picojson::array components = obj["components"].get<picojson::array>();
        for (const auto& compItem : components) {
            picojson::object compObj = compItem.get<picojson::object>();
            mxl2ndi_sink::Flow_def::Components_elem compDef;
            if (compObj.contains("name") && compObj["name"].is<std::string>()) {
                compDef.name = compObj["name"].get<std::string>();
            }
            if (compObj.contains("width") && compObj["width"].is<double>()) {
                compDef.width = static_cast<int32_t>(compObj["width"].get<double>());
            }
            if (compObj.contains("height") && compObj["height"].is<double>()) {
                compDef.height = static_cast<int32_t>(compObj["height"].get<double>());
            }
            if (compObj.contains("bit_depth") && compObj["bit_depth"].is<double>()) {
                compDef.bit_depth = static_cast<int32_t>(compObj["bit_depth"].get<double>());
            }
            flowDef.components.push_back(compDef);
        }
    }
    if (obj.contains("parents") && obj["parents"].is<picojson::array>()) {
        flowDef.parents.clear();
        picojson::array parents = obj["parents"].get<picojson::array>();
        for (const auto& parentItem : parents) {
            if (parentItem.is<std::string>()) {
                flowDef.parents.push_back(parentItem.get<std::string>());
            }
        }
    }

    return true;
}

class MxlReader {
  public:
    MxlReader(std::string const& name, std::string const& domain, std::string flowId)
        : _name(name), _domain(domain), _flowId(flowId) {}

    ~MxlReader() {
        // have a helper function
        releaseReader();
        if (_instance != nullptr) {
            mxlDestroyInstance(_instance);
        }
    }

    bool open() {
        mxlStatus status;
        if (_instance == nullptr) {
            _instance = mxlCreateInstance(_domain.c_str(), "");
            if (_instance == nullptr) {
                LOG(ERROR) << "Failed to create MXL instance: " << _domain;
                return false;
            }
        }

        if (_reader != nullptr) {
            return true;
        }
        status = mxlCreateFlowReader(_instance, _flowId.c_str(), "", &_reader);
        if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to create MXL Flow Reader: " << _flowId << " with error status \"" << status
                       << "\"";
            return false;
        }

        // just always update the flow reader
        status = mxlFlowReaderGetInfo(_reader, &_flowInfo);
        if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to get MXL Flow Info: " << _flowId << " with error status \"" << status
                       << "\"";
            // doesn't fail the whole reader
        }

        _index = _flowInfo.runtime.headIndex;
        LOG(INFO) << "Opened MXL Flow Reader for flow " << _flowId << " with head index " << _index;

        mxl2ndi_sink::Flow_def flowDef;
        mxlcpp::fillFlowDef(_instance, _flowId, flowDef);

        _rate = _flowInfo.config.common.grainRate;
        _timeoutNs = static_cast<std::uint64_t>(
          1.0 * _rate.denominator * (1'000'000'000.0 / _rate.numerator) + 1'000'000ULL);

        _ndiFrame.xres = flowDef.frame_width;
        _ndiFrame.yres = flowDef.frame_height;
        _ndiFrame.FourCC = NDIlib_FourCC_video_type_UYVY;  // 8-bit 4:2:2
        _ndiFrame.frame_rate_N = static_cast<int>(_rate.numerator);
        _ndiFrame.frame_rate_D = static_cast<int>(_rate.denominator);
        _ndiFrame.picture_aspect_ratio = 16.0f / 9.0f;
        _ndiFrame.frame_format_type = NDIlib_frame_format_type_progressive;

        _ndiFrame.line_stride_in_bytes = _ndiFrame.xres * 2;

        _ndiBufferBytes =
          static_cast<size_t>(_ndiFrame.yres) * static_cast<size_t>(_ndiFrame.line_stride_in_bytes);

        _ndiFrame.p_data = new std::uint8_t[_ndiBufferBytes];

        return true;
    }

    const std::string& getName() const { return _name; }
    void setName(std::string const& name) { _name = name; }

    const std::string& getDomain() const { return _domain; }

    const std::string& getFlowId() const { return _flowId; }

    const mxlFlowReader& get() const { return _reader; }

    const mxlInstance& instance() const { return _instance; }

    void fillFlowDef(mxl2ndi_sink::Flow_def& flowDef) {
        open();  // ensure reader is open to get flow info
        mxlcpp::fillFlowDef(_instance, _flowId, flowDef);
    }

    const NDIlib_video_frame_v2_t* dumpNdiFrame(uint64_t& sleepNs) {
        if (_reader == nullptr) {
            LOG(WARNING) << "Cannot dump NDI frame for reader " << _name
                         << " because MXL Flow Reader is null";
            return nullptr;
        }

        mxlGrainInfo grainInfo;
        uint8_t* payload = nullptr;

        mxlStatus status = mxlFlowReaderGetGrain(_reader, _index, 1000000000, &grainInfo, &payload);
        if (status == MXL_ERR_OUT_OF_RANGE_TOO_EARLY) {
            // We are too early somehow, keep trying the same grain index
            mxlFlowReaderGetInfo(_reader, &_flowInfo);
            LOG(WARNING) << "Failed to get samples at index " << _index << ": TOO EARLY. Last published "
                         << _flowInfo.runtime.headIndex;
            return nullptr;
        } else if (status == MXL_ERR_OUT_OF_RANGE_TOO_LATE) {
            mxlFlowReaderGetInfo(_reader, &_flowInfo);
            LOG(WARNING) << "Failed to get grain at index " << _index << ": TOO LATE. Last published "
                         << _flowInfo.runtime.headIndex;
            // Grain expired. Realign to current index. GStreamer repeats the last valid frame for missing data; consuming applications
            // should do the same.
            _index = mxlGetCurrentIndex(&_rate);
            return nullptr;
        } else if (status == MXL_ERR_TIMEOUT) {
            // Timeout waiting for grain
            LOG(WARNING) << "Timeout waiting for grain at index " << _index;
            return nullptr;
        } else if (status == MXL_ERR_FLOW_INVALID) {
            LOG(WARNING) << "Flow became invalid when trying to read grain at index " << _index;
            releaseReader();
            return nullptr;
        } else if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Unexpected error when reading the grain " << _index << " with status "
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

            ++_index;
            sleepNs = mxlGetNsUntilIndex(_index, &_rate);
            return &_ndiFrame;
        }
        return nullptr;
    }

  private:
    std::string _name;
    std::string _domain;
    std::string _flowId;
    mxlInstance _instance = nullptr;
    mxlFlowReader _reader = nullptr;
    mxlFlowInfo _flowInfo;
    uint64_t _index;
    mxlRational _rate;
    uint64_t _timeoutNs;
    NDIlib_video_frame_v2_t _ndiFrame{};
    size_t _ndiBufferBytes = 0;

    void releaseReader() {
        if (_ndiFrame.p_data != nullptr) {
            delete[] _ndiFrame.p_data;
            _ndiFrame.p_data = nullptr;
        }
        if (_reader != nullptr) {
            mxlReleaseFlowReader(_instance, _reader);
            _reader = nullptr;
        }
    }

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
