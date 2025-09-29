// Copyright 2025 Ross Video Ltd
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

/**
 * @brief Visual Audio Deck - Catena gRPC service example
 * @file visual_audio_deck.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

// device model
#include "device.visual_audio_deck.yaml.h"

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <ParamDescriptor.h>

// connections/gRPC
#include <ServiceImpl.h>
#include <ServiceCredentials.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>
#include <signal.h>
#include <functional>
#include <map>
#include <cmath>
#include <Logger.h>

#include "httplib.h"  // from cpp-httplib

using grpc::Server;
using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;

using namespace catena::common;


Server *globalServer = nullptr;
std::atomic<bool> running = true;

// Signal generation variables
std::atomic<double> currentTime = 0.0;
std::atomic<bool> signalGenerationRunning = false;
constexpr double PI = 3.14159265358979323846;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        running = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

// Frequency mapping for sliders when channel is selected
std::map<int, int> sliderToFrequency = {
    {1, 10}, {2, 21}, {3, 42}, {4, 83}, 
    {5, 166}, {6, 333}, {7, 577}, {8, 1000}
};

void updateDisplayParameters(const std::string& imgOid, const std::string& imgValue,
                           const std::string& textOid, const std::string& textValue,
                           const std::string& modeOid, const std::string& modeValue) {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    
    // Update display image
    std::unique_ptr<IParam> imgParam = dm.getParam(imgOid, err);
    if (imgParam) {
        auto* img = dynamic_cast<ParamWithValue<std::string>*>(imgParam.get());
        if (img) {
            std::lock_guard lg(dm.mutex());
            img->get() = imgValue;
            dm.getValueSetByServer().emit(imgOid, img);
        }
    }
    
    // Update display text
    std::unique_ptr<IParam> textParam = dm.getParam(textOid, err);
    if (textParam) {
        auto* text = dynamic_cast<ParamWithValue<std::string>*>(textParam.get());
        if (text) {
            std::lock_guard lg(dm.mutex());
            text->get() = textValue;
            dm.getValueSetByServer().emit(textOid, text);
        }
    }
    
    // Update display mode
    std::unique_ptr<IParam> modeParam = dm.getParam(modeOid, err);
    if (modeParam) {
        auto* mode = dynamic_cast<ParamWithValue<std::string>*>(modeParam.get());
        if (mode) {
            std::lock_guard lg(dm.mutex());
            mode->get() = modeValue;
            dm.getValueSetByServer().emit(modeOid, mode);
        }
    }
}

void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};

    // Define clear_all command
    std::unique_ptr<IParam> clearCommand = dm.getCommand("/clear_all", err);
    assert(clearCommand != nullptr);

    clearCommand->defineCommand([](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            st2138::CommandResponse response;
            
            // Clear all channel solos (INT32 as boolean)
            for (int i = 1; i <= 8; i++) {
                std::string soloOid = "/ch" + std::to_string(i) + "_solo";
                std::unique_ptr<IParam> soloParam = dm.getParam(soloOid, err);
                if (soloParam) {
                    auto* solo = dynamic_cast<ParamWithValue<int32_t>*>(soloParam.get());
                    if (solo) {
                        std::lock_guard lg(dm.mutex());
                        solo->get() = 0; // false
                        dm.getValueSetByServer().emit(soloOid, solo);
                    }
                }
            }
            
            // Clear main solo (INT32 as boolean)
            std::unique_ptr<IParam> mainSoloParam = dm.getParam("/main_solo", err);
            if (mainSoloParam) {
                auto* mainSolo = dynamic_cast<ParamWithValue<int32_t>*>(mainSoloParam.get());
                if (mainSolo) {
                    std::lock_guard lg(dm.mutex());
                    mainSolo->get() = 0; // false
                    dm.getValueSetByServer().emit("/main_solo", mainSolo);
                }
            }
            
            response.mutable_no_response();
            co_return response;
        }(value, respond));
    });

    // Define main_select_cmd
    std::unique_ptr<IParam> mainSelectCommand = dm.getCommand("/main_select_cmd", err);
    assert(mainSelectCommand != nullptr);

    mainSelectCommand->defineCommand([](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            st2138::CommandResponse response;
            
            // Turn on main select
            std::unique_ptr<IParam> mainSelectParam = dm.getParam("/main_select", err);
            if (mainSelectParam) {
                auto* mainSelect = dynamic_cast<ParamWithValue<int32_t>*>(mainSelectParam.get());
                if (mainSelect) {
                    std::lock_guard lg(dm.mutex());
                    mainSelect->get() = 1; // true
                    dm.getValueSetByServer().emit("/main_select", mainSelect);
                }
            }
            
            // Turn off all channel selects
            for (int i = 1; i <= 8; i++) {
                std::string selectOid = "/ch" + std::to_string(i) + "_select";
                std::unique_ptr<IParam> selectParam = dm.getParam(selectOid, err);
                if (selectParam) {
                    auto* select = dynamic_cast<ParamWithValue<int32_t>*>(selectParam.get());
                    if (select) {
                        std::lock_guard lg(dm.mutex());
                        select->get() = 0; // false
                        dm.getValueSetByServer().emit(selectOid, select);
                    }
                }
            }
            
            // Set main display: mode="LR", text="main", img="ross_video_icon.png"
            updateDisplayParameters("/main_display_img", "eo://ross_video_icon.png",
                                  "/main_display_text", "main",
                                  "/main_display_mode", "LR");
            
            response.mutable_no_response();
            co_return response;
        }(value, respond));
    });

    // Define channel select commands
    for (int channelNum = 1; channelNum <= 8; channelNum++) {
        std::string cmdOid = "/ch" + std::to_string(channelNum) + "_select_cmd";
        std::unique_ptr<IParam> channelSelectCommand = dm.getCommand(cmdOid, err);
        assert(channelSelectCommand != nullptr);

        channelSelectCommand->defineCommand([channelNum](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
            return std::make_unique<ParamDescriptor::CommandResponder>([channelNum](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
                catena::exception_with_status err{"", catena::StatusCode::OK};
                st2138::CommandResponse response;
                
                // Turn on this channel's select
                std::string selectOid = "/ch" + std::to_string(channelNum) + "_select";
                std::unique_ptr<IParam> selectParam = dm.getParam(selectOid, err);
                if (selectParam) {
                    auto* select = dynamic_cast<ParamWithValue<int32_t>*>(selectParam.get());
                    if (select) {
                        std::lock_guard lg(dm.mutex());
                        select->get() = 1; // true
                        dm.getValueSetByServer().emit(selectOid, select);
                    }
                }
                
                // Turn off main select
                std::unique_ptr<IParam> mainSelectParam = dm.getParam("/main_select", err);
                if (mainSelectParam) {
                    auto* mainSelect = dynamic_cast<ParamWithValue<int32_t>*>(mainSelectParam.get());
                    if (mainSelect) {
                        std::lock_guard lg(dm.mutex());
                        mainSelect->get() = 0; // false
                        dm.getValueSetByServer().emit("/main_select", mainSelect);
                    }
                }
                
                // Turn off other channel selects
                for (int i = 1; i <= 8; i++) {
                    if (i != channelNum) {
                        std::string otherSelectOid = "/ch" + std::to_string(i) + "_select";
                        std::unique_ptr<IParam> otherSelectParam = dm.getParam(otherSelectOid, err);
                        if (otherSelectParam) {
                            auto* otherSelect = dynamic_cast<ParamWithValue<int32_t>*>(otherSelectParam.get());
                            if (otherSelect) {
                                std::lock_guard lg(dm.mutex());
                                otherSelect->get() = 0; // false
                                dm.getValueSetByServer().emit(otherSelectOid, otherSelect);
                            }
                        }
                    }
                }
                
                // Set frequency mapping for sliders
                for (int sliderNum = 1; sliderNum <= 8; sliderNum++) {
                    std::string sliderOid = "/ch" + std::to_string(channelNum) + "_slider";  // Use the selected channel's slider
                    std::unique_ptr<IParam> sliderParam = dm.getParam(sliderOid, err);
                    if (sliderParam) {
                        auto* slider = dynamic_cast<ParamWithValue<float>*>(sliderParam.get());
                        if (slider) {
                            // Set slider to frequency level (simplified mapping)
                            int frequency = sliderToFrequency[sliderNum];
                            // Convert frequency to dB level (simplified)
                            float dbLevel = -80.0f + (frequency / 1000.0f) * 90.0f; // Map 0-1000 Hz to -80 to +10 dB
                            dbLevel = std::max(-80.0f, std::min(10.0f, dbLevel));
                            
                            std::lock_guard lg(dm.mutex());
                            slider->get() = dbLevel;
                            dm.getValueSetByServer().emit(sliderOid, slider);
                        }
                    }
                }
                
                // Update display parameters
                std::string channelName = "Channel " + std::to_string(channelNum);
                std::string displayImgOid = "/ch" + std::to_string(channelNum) + "_display_img";
                std::string displayTextOid = "/ch" + std::to_string(channelNum) + "_display_text";
                std::string displayModeOid = "/ch" + std::to_string(channelNum) + "_display_mode";
                
                updateDisplayParameters(displayImgOid, "eo://signal_wave_icon.png",
                                      displayTextOid, channelName,
                                      displayModeOid, "Freq");
                
                response.mutable_no_response();
                co_return response;
            }(value, respond));
        });
    }

    // Define main solo command
    std::unique_ptr<IParam> mainSoloCommand = dm.getCommand("/main_solo_cmd", err);
    assert(mainSoloCommand != nullptr);

    mainSoloCommand->defineCommand([](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            st2138::CommandResponse response;
            
            // Toggle main solo
            std::unique_ptr<IParam> mainSoloParam = dm.getParam("/main_solo", err);
            if (mainSoloParam) {
                auto* mainSolo = dynamic_cast<ParamWithValue<int32_t>*>(mainSoloParam.get());
                if (mainSolo) {
                    std::lock_guard lg(dm.mutex());
                    mainSolo->get() = (mainSolo->get() == 0) ? 1 : 0; // toggle
                    dm.getValueSetByServer().emit("/main_solo", mainSolo);
                }
            }
            
            response.mutable_no_response();
            co_return response;
        }(value, respond));
    });

    // Define main mute command
    std::unique_ptr<IParam> mainMuteCommand = dm.getCommand("/main_mute_cmd", err);
    assert(mainMuteCommand != nullptr);

    mainMuteCommand->defineCommand([](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            st2138::CommandResponse response;
            
            // Toggle main mute
            std::unique_ptr<IParam> mainMuteParam = dm.getParam("/main_mute", err);
            if (mainMuteParam) {
                auto* mainMute = dynamic_cast<ParamWithValue<int32_t>*>(mainMuteParam.get());
                if (mainMute) {
                    std::lock_guard lg(dm.mutex());
                    mainMute->get() = (mainMute->get() == 0) ? 1 : 0; // toggle
                    dm.getValueSetByServer().emit("/main_mute", mainMute);
                }
            }
            
            response.mutable_no_response();
            co_return response;
        }(value, respond));
    });

    // Define channel solo commands
    for (int channelNum = 1; channelNum <= 8; channelNum++) {
        std::string cmdOid = "/ch" + std::to_string(channelNum) + "_solo_cmd";
        std::unique_ptr<IParam> channelSoloCommand = dm.getCommand(cmdOid, err);
        assert(channelSoloCommand != nullptr);

        channelSoloCommand->defineCommand([channelNum](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
            return std::make_unique<ParamDescriptor::CommandResponder>([channelNum](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
                catena::exception_with_status err{"", catena::StatusCode::OK};
                st2138::CommandResponse response;
                
                // Check if this channel is selected
                std::string selectOid = "/ch" + std::to_string(channelNum) + "_select";
                std::unique_ptr<IParam> selectParam = dm.getParam(selectOid, err);
                bool isSelected = false;
                if (selectParam) {
                    auto* select = dynamic_cast<ParamWithValue<int32_t>*>(selectParam.get());
                    if (select) {
                        isSelected = (select->get() != 0);
                    }
                }
                
                std::string soloOid = "/ch" + std::to_string(channelNum) + "_solo";
                std::unique_ptr<IParam> soloParam = dm.getParam(soloOid, err);
                
                if (isSelected && soloParam) {
                    auto* solo = dynamic_cast<ParamWithValue<int32_t>*>(soloParam.get());
                    if (solo) {
                        if (solo->get() == 0) {
                            // Enter frequency editing mode
                            std::lock_guard lg(dm.mutex());
                            solo->get() = 1; // Set solo to true
                            dm.getValueSetByServer().emit(soloOid, solo);
                            
                            // Update display for frequency editing mode
                            std::string displayImgOid = "/ch" + std::to_string(channelNum) + "_display_img";
                            std::string displayTextOid = "/ch" + std::to_string(channelNum) + "_display_text";
                            std::string displayModeOid = "/ch" + std::to_string(channelNum) + "_display_mode";
                            
                            // Get current frequency for display
                            std::string freqOid = "/ch" + std::to_string(channelNum) + "_frequency";
                            std::unique_ptr<IParam> freqParam = dm.getParam(freqOid, err);
                            std::string freqText = "440.0"; // default
                            if (freqParam) {
                                auto* freq = dynamic_cast<ParamWithValue<float>*>(freqParam.get());
                                if (freq) {
                                    freqText = std::to_string(freq->get());
                                }
                            }
                            
                            updateDisplayParameters(displayImgOid, "eo://nice_icon.png",
                                                  displayTextOid, freqText,
                                                  displayModeOid, "SET FREQ");
                        } else {
                            // Exit frequency editing mode, save frequency
                            std::lock_guard lg(dm.mutex());
                            solo->get() = 0; // Set solo to false
                            dm.getValueSetByServer().emit(soloOid, solo);
                        }
                    }
                } else {
                    // Just toggle solo
                    if (soloParam) {
                        auto* solo = dynamic_cast<ParamWithValue<int32_t>*>(soloParam.get());
                        if (solo) {
                            std::lock_guard lg(dm.mutex());
                            solo->get() = (solo->get() == 0) ? 1 : 0; // toggle
                            dm.getValueSetByServer().emit(soloOid, solo);
                        }
                    }
                }
                
                response.mutable_no_response();
                co_return response;
            }(value, respond));
        });
    }

    // Define channel mute commands
    for (int channelNum = 1; channelNum <= 8; channelNum++) {
        std::string cmdOid = "/ch" + std::to_string(channelNum) + "_mute_cmd";
        std::unique_ptr<IParam> channelMuteCommand = dm.getCommand(cmdOid, err);
        assert(channelMuteCommand != nullptr);

        channelMuteCommand->defineCommand([channelNum](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
            return std::make_unique<ParamDescriptor::CommandResponder>([channelNum](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
                catena::exception_with_status err{"", catena::StatusCode::OK};
                st2138::CommandResponse response;
                
                // Toggle channel mute
                std::string muteOid = "/ch" + std::to_string(channelNum) + "_mute";
                std::unique_ptr<IParam> muteParam = dm.getParam(muteOid, err);
                if (muteParam) {
                    auto* mute = dynamic_cast<ParamWithValue<int32_t>*>(muteParam.get());
                    if (mute) {
                        std::lock_guard lg(dm.mutex());
                        mute->get() = (mute->get() == 0) ? 1 : 0; // toggle
                        dm.getValueSetByServer().emit(muteOid, mute);
                    }
                }
                
                response.mutable_no_response();
                co_return response;
            }(value, respond));
        });
    }
}

std::string getExecutableDir(const char* argv0) {
    std::string path(argv0);
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return ".";
}

// Store original frequencies for comparison
std::map<int, float> originalFrequencies = {
    {1, 261.63f}, {2, 329.63f}, {3, 392.00f}, {4, 783.99f},
    {5, 880.00f}, {6, 987.77f}, {7, 1108.73f}, {8, 1244.51f}
};

void generateChannelSignal(int channelNum) {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    
    // Get channel frequency (f0)
    std::string freqOid = "/ch" + std::to_string(channelNum) + "_frequency";
    std::unique_ptr<IParam> freqParam = dm.getParam(freqOid, err);
    float f0 = originalFrequencies[channelNum]; // Default frequency
    if (freqParam) {
        auto* freq = dynamic_cast<ParamWithValue<float>*>(freqParam.get());
        if (freq) {
            f0 = freq->get();
        }
    }
    
    
    double t = currentTime.load();
    
    // Calculate signal using the formula:
    // signal = 0
    // for n from 1 to 6:
    //     signal += (1/n) * sin(2 * PI * n * (f0+n*10) * t)
    
    double signal = 0.0;
    for (int n = 1; n <= 6; n++) {
        double frequency = f0 + n * 10.0;
        signal += (1.0 / n) * std::sin(2.0 * PI * n * frequency * t);
    }
    
    // Update the channel signal parameter
    std::string signalOid = "/ch" + std::to_string(channelNum) + "_signal";
    std::unique_ptr<IParam> signalParam = dm.getParam(signalOid, err);
    if (signalParam) {
        auto* signalVal = dynamic_cast<ParamWithValue<float>*>(signalParam.get());
        if (signalVal) {
            std::lock_guard lg(dm.mutex());
            signalVal->get() = static_cast<float>(signal);
            dm.getValueSetByServer().emit(signalOid, signalVal);
        }
    }
}

void updateMainOutput() {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    
    // Calculate main output as sum of channel signal * channel volume
    double mainOutput = 0.0;
    
    for (int i = 1; i <= 8; i++) {
        // Get channel signal
        std::string signalOid = "/ch" + std::to_string(i) + "_signal";
        std::unique_ptr<IParam> signalParam = dm.getParam(signalOid, err);
        float channelSignal = 0.0f;
        if (signalParam) {
            auto* signal = dynamic_cast<ParamWithValue<float>*>(signalParam.get());
            if (signal) {
                channelSignal = signal->get();
            }
        }
        
        // Get channel slider value (use as volume)
        std::string sliderOid = "/ch" + std::to_string(i) + "_slider";
        std::unique_ptr<IParam> sliderParam = dm.getParam(sliderOid, err);
        float channelSlider = 0.0f;
        if (sliderParam) {
            auto* slider = dynamic_cast<ParamWithValue<float>*>(sliderParam.get());
            if (slider) {
                channelSlider = slider->get();
            }
        }
        
        // Convert dB slider value to linear scale for mixing
        // slider is in dB (-80 to +10), convert to linear (0.0001 to ~3.16)
        float linearGain = std::pow(10.0f, channelSlider / 20.0f);
        
        mainOutput += channelSignal * linearGain;
    }
    
    // Update main output parameter
    std::unique_ptr<IParam> mainOutputParam = dm.getParam("/main_output", err);
    if (mainOutputParam) {
        auto* output = dynamic_cast<ParamWithValue<float>*>(mainOutputParam.get());
        if (output) {
            std::lock_guard lg(dm.mutex());
            output->get() = static_cast<float>(mainOutput);
            dm.getValueSetByServer().emit("/main_output", output);
        }
    }
}

void updateWarnings() {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    
    for (int i = 1; i <= 8; i++) {
        // Get signal value for CLIP calculation
        std::string signalOid = "/ch" + std::to_string(i) + "_signal";
        std::unique_ptr<IParam> signalParam = dm.getParam(signalOid, err);
        float signalValue = 0.0f;
        if (signalParam) {
            auto* signal = dynamic_cast<ParamWithValue<float>*>(signalParam.get());
            if (signal) {
                signalValue = signal->get();
            }
        }
        
        // Get frequency value for Freq warning
        std::string freqOid = "/ch" + std::to_string(i) + "_frequency";
        std::unique_ptr<IParam> freqParam = dm.getParam(freqOid, err);
        float freqValue = originalFrequencies[i];
        if (freqParam) {
            auto* freq = dynamic_cast<ParamWithValue<float>*>(freqParam.get());
            if (freq) {
                freqValue = freq->get();
            }
        }
        
        // Update COMP warning (always true as per requirement)
        std::string compOid = "/ch" + std::to_string(i) + "_warning_comp";
        std::unique_ptr<IParam> compParam = dm.getParam(compOid, err);
        if (compParam) {
            auto* comp = dynamic_cast<ParamWithValue<int32_t>*>(compParam.get());
            if (comp) {
                std::lock_guard lg(dm.mutex());
                comp->get() = 1; // Always true
                dm.getValueSetByServer().emit(compOid, comp);
            }
        }
        
        // Update CLIP warning
        std::string clipOid = "/ch" + std::to_string(i) + "_warning_clip";
        std::unique_ptr<IParam> clipParam = dm.getParam(clipOid, err);
        if (clipParam) {
            auto* clip = dynamic_cast<ParamWithValue<int32_t>*>(clipParam.get());
            if (clip) {
                std::lock_guard lg(dm.mutex());
                if (signalValue > 1.0f) {
                    float remainder = signalValue - 1.0f;
                    int clipValue = static_cast<int>(std::round(remainder * 100.0f));
                    clip->get() = clipValue;
                } else {
                    clip->get() = 0;
                }
                dm.getValueSetByServer().emit(clipOid, clip);
            }
        }
        
        // Update Freq warning
        std::string freqWarningOid = "/ch" + std::to_string(i) + "_warning_freq";
        std::unique_ptr<IParam> freqWarningParam = dm.getParam(freqWarningOid, err);
        if (freqWarningParam) {
            auto* freqWarning = dynamic_cast<ParamWithValue<int32_t>*>(freqWarningParam.get());
            if (freqWarning) {
                std::lock_guard lg(dm.mutex());
                // Check if frequency has been modified (with small tolerance for floating point comparison)
                bool isModified = std::abs(freqValue - originalFrequencies[i]) > 0.01f;
                freqWarning->get() = isModified ? 1 : 0;
                dm.getValueSetByServer().emit(freqWarningOid, freqWarning);
            }
        }
    }
}

void signalGenerationLoop() {
    DEBUG_LOG << "Signal generation loop started at 60Hz";
    signalGenerationRunning = true;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    const auto targetInterval = std::chrono::microseconds(16667); // 1/60th of a second in microseconds
    
    while (running.load() && signalGenerationRunning.load()) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Update current time in seconds
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameStart - startTime);
        currentTime.store(elapsed.count() / 1000000.0); // Convert to seconds
        
        // Generate signals for all 8 channels
        for (int i = 1; i <= 8; i++) {
            generateChannelSignal(i);
        }
        
        // Update main output
        updateMainOutput();
        
        // Update warnings
        updateWarnings();
        
        // Sleep to maintain 60Hz rate
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = frameEnd - frameStart;
        auto sleepTime = targetInterval - frameDuration;
        
        if (sleepTime > std::chrono::microseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
    
    signalGenerationRunning = false;
    DEBUG_LOG << "Signal generation loop stopped";
}

void RunRPCServer(std::string addr)
{
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, catena::gRPC::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        ServiceConfig config = ServiceConfig()
            .set_EOPath(absl::GetFlag(FLAGS_static_root))
            .set_authz(absl::GetFlag(FLAGS_authz))
            .set_maxConnections(absl::GetFlag(FLAGS_max_connections))
            .set_cq(cq.get())
            .add_dm(&dm);
        ServiceImpl service(config);

        // Updating device's default max array length.
        dm.set_default_max_length(absl::GetFlag(FLAGS_default_max_array_size));

        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        DEBUG_LOG << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms);

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        // wait for the server to shutdown and tidy up
        server->Wait();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

// Global pointer to web server for control
std::unique_ptr<httplib::Server> globalWebServer;
std::atomic<bool> webServerRunning = false;

void RunWebServer(const std::string& exeDir, int port) {
    globalWebServer = std::make_unique<httplib::Server>();
    
    std::string webpage_dir = exeDir + "/webpage/";
    
    // Serve index.html for root
    globalWebServer->Get(R"(/)", [webpage_dir](const httplib::Request&, httplib::Response& res) {
        std::string file_path = webpage_dir + "index.html";
        std::ifstream file(file_path, std::ios::in | std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            res.set_header("Content-Type", "text/html");
            res.set_content(content, "text/html");
        } else {
            res.status = 404;
            res.set_content("404 Not Found", "text/plain");
        }
    });

    // Serve static files from webpage/
    globalWebServer->Get(R"(/(?!api/)(.*))", [webpage_dir](const httplib::Request& req, httplib::Response& res) {
        std::string rel_path = req.matches[1];
        if (rel_path.empty()) return;
        std::string file_path = webpage_dir + rel_path;
        std::ifstream file(file_path, std::ios::in | std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            // Basic content type detection
            if (rel_path.size() >= 5 && rel_path.substr(rel_path.size()-5) == ".html") {
                res.set_header("Content-Type", "text/html");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".css") {
                res.set_header("Content-Type", "text/css");
            } else if (rel_path.size() >= 3 && rel_path.substr(rel_path.size()-3) == ".js") {
                res.set_header("Content-Type", "application/javascript");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".ico") {
                res.set_header("Content-Type", "image/x-icon");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".png") {
                res.set_header("Content-Type", "image/png");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".jpg") {
                res.set_header("Content-Type", "image/jpeg");
            } else if (rel_path.size() >= 5 && rel_path.substr(rel_path.size()-5) == ".jpeg") {
                res.set_header("Content-Type", "image/jpeg");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".svg") {
                res.set_header("Content-Type", "image/svg+xml");
            } else {
                res.set_header("Content-Type", "application/octet-stream");
            }
            res.set_content(content, res.get_header_value("Content-Type"));
        } else {
            res.status = 404;
            res.set_content("404 Not Found", "text/plain");
        }
    });
    
    // API endpoint to get all parameters
    globalWebServer->Get("/api/params", [](const httplib::Request&, httplib::Response& res) {
        catena::exception_with_status err{"", catena::StatusCode::OK};
        
        std::stringstream json;
        json << std::fixed << std::setprecision(6);
        json << "{\n";
        json << "  \"main\": {\n";
        
        // Main output
        std::unique_ptr<IParam> mainOutputParam = dm.getParam("/main_output", err);
        if (mainOutputParam) {
            auto* mainOutput = dynamic_cast<ParamWithValue<float>*>(mainOutputParam.get());
            if (mainOutput) {
                json << "    \"output\": " << mainOutput->get() << ",\n";
            }
        }
        
        // Main slider
        std::unique_ptr<IParam> mainSliderParam = dm.getParam("/main_slider", err);
        if (mainSliderParam) {
            auto* mainSlider = dynamic_cast<ParamWithValue<float>*>(mainSliderParam.get());
            if (mainSlider) {
                json << "    \"slider\": " << mainSlider->get() << ",\n";
            }
        }
        
        // Main select (INT32 as boolean)
        std::unique_ptr<IParam> mainSelectParam = dm.getParam("/main_select", err);
        if (mainSelectParam) {
            auto* mainSelect = dynamic_cast<ParamWithValue<int32_t>*>(mainSelectParam.get());
            if (mainSelect) {
                json << "    \"select\": " << (mainSelect->get() != 0 ? "true" : "false") << ",\n";
            }
        }
        
        // Main solo (INT32 as boolean)
        std::unique_ptr<IParam> mainSoloParam = dm.getParam("/main_solo", err);
        if (mainSoloParam) {
            auto* mainSolo = dynamic_cast<ParamWithValue<int32_t>*>(mainSoloParam.get());
            if (mainSolo) {
                json << "    \"solo\": " << (mainSolo->get() != 0 ? "true" : "false") << ",\n";
            }
        }
        
        // Main mute (INT32 as boolean)
        std::unique_ptr<IParam> mainMuteParam = dm.getParam("/main_mute", err);
        if (mainMuteParam) {
            auto* mainMute = dynamic_cast<ParamWithValue<int32_t>*>(mainMuteParam.get());
            if (mainMute) {
                json << "    \"mute\": " << (mainMute->get() != 0 ? "true" : "false") << ",\n";
            }
        }
        
        // Main display fields
        std::unique_ptr<IParam> mainDisplayImgParam = dm.getParam("/main_display_img", err);
        if (mainDisplayImgParam) {
            auto* mainDisplayImg = dynamic_cast<ParamWithValue<std::string>*>(mainDisplayImgParam.get());
            if (mainDisplayImg) {
                json << "    \"display_img\": \"" << mainDisplayImg->get() << "\",\n";
            }
        }
        
        std::unique_ptr<IParam> mainDisplayTextParam = dm.getParam("/main_display_text", err);
        if (mainDisplayTextParam) {
            auto* mainDisplayText = dynamic_cast<ParamWithValue<std::string>*>(mainDisplayTextParam.get());
            if (mainDisplayText) {
                json << "    \"display_text\": \"" << mainDisplayText->get() << "\",\n";
            }
        }
        
        std::unique_ptr<IParam> mainDisplayModeParam = dm.getParam("/main_display_mode", err);
        if (mainDisplayModeParam) {
            auto* mainDisplayMode = dynamic_cast<ParamWithValue<std::string>*>(mainDisplayModeParam.get());
            if (mainDisplayMode) {
                json << "    \"display_mode\": \"" << mainDisplayMode->get() << "\"\n";
            }
        }
        
        json << "  },\n";
        json << "  \"channels\": [\n";
        
        for (int i = 1; i <= 8; i++) {
            json << "    {\n";
            json << "      \"id\": " << i << ",\n";
            
            // Channel signal
            std::string signalOid = "/ch" + std::to_string(i) + "_signal";
            std::unique_ptr<IParam> signalParam = dm.getParam(signalOid, err);
            if (signalParam) {
                auto* signal = dynamic_cast<ParamWithValue<float>*>(signalParam.get());
                if (signal) {
                    json << "      \"signal\": " << signal->get() << ",\n";
                }
            }
            
            
            // Channel frequency
            std::string freqOid = "/ch" + std::to_string(i) + "_frequency";
            std::unique_ptr<IParam> freqParam = dm.getParam(freqOid, err);
            if (freqParam) {
                auto* freq = dynamic_cast<ParamWithValue<float>*>(freqParam.get());
                if (freq) {
                    json << "      \"frequency\": " << freq->get() << ",\n";
                }
            }
            
            
            // Channel select (INT32 as boolean)
            std::string selectOid = "/ch" + std::to_string(i) + "_select";
            std::unique_ptr<IParam> selectParam = dm.getParam(selectOid, err);
            if (selectParam) {
                auto* select = dynamic_cast<ParamWithValue<int32_t>*>(selectParam.get());
                if (select) {
                    json << "      \"select\": " << (select->get() != 0 ? "true" : "false") << ",\n";
                }
            }
            
            // Channel solo (INT32 as boolean)
            std::string soloOid = "/ch" + std::to_string(i) + "_solo";
            std::unique_ptr<IParam> soloParam = dm.getParam(soloOid, err);
            if (soloParam) {
                auto* solo = dynamic_cast<ParamWithValue<int32_t>*>(soloParam.get());
                if (solo) {
                    json << "      \"solo\": " << (solo->get() != 0 ? "true" : "false") << ",\n";
                }
            }
            
            // Channel mute (INT32 as boolean)
            std::string muteOid = "/ch" + std::to_string(i) + "_mute";
            std::unique_ptr<IParam> muteParam = dm.getParam(muteOid, err);
            if (muteParam) {
                auto* mute = dynamic_cast<ParamWithValue<int32_t>*>(muteParam.get());
                if (mute) {
                    json << "      \"mute\": " << (mute->get() != 0 ? "true" : "false") << ",\n";
                }
            }
            
            // Channel slider
            std::string sliderOid = "/ch" + std::to_string(i) + "_slider";
            std::unique_ptr<IParam> sliderParam = dm.getParam(sliderOid, err);
            if (sliderParam) {
                auto* slider = dynamic_cast<ParamWithValue<float>*>(sliderParam.get());
                if (slider) {
                    json << "      \"slider\": " << slider->get() << ",\n";
                }
            }
            
            // Warning COMP (INT32 as boolean)
            std::string warningCompOid = "/ch" + std::to_string(i) + "_warning_comp";
            std::unique_ptr<IParam> warningCompParam = dm.getParam(warningCompOid, err);
            if (warningCompParam) {
                auto* warningComp = dynamic_cast<ParamWithValue<int32_t>*>(warningCompParam.get());
                if (warningComp) {
                    json << "      \"warning_comp\": " << (warningComp->get() != 0 ? "true" : "false") << ",\n";
                }
            }
            
            // Warning CLIP (INT32)
            std::string warningClipOid = "/ch" + std::to_string(i) + "_warning_clip";
            std::unique_ptr<IParam> warningClipParam = dm.getParam(warningClipOid, err);
            if (warningClipParam) {
                auto* warningClip = dynamic_cast<ParamWithValue<int32_t>*>(warningClipParam.get());
                if (warningClip) {
                    json << "      \"warning_clip\": " << warningClip->get() << ",\n";
                }
            }
            
            // Warning Freq (INT32 as boolean)
            std::string warningFreqOid = "/ch" + std::to_string(i) + "_warning_freq";
            std::unique_ptr<IParam> warningFreqParam = dm.getParam(warningFreqOid, err);
            if (warningFreqParam) {
                auto* warningFreq = dynamic_cast<ParamWithValue<int32_t>*>(warningFreqParam.get());
                if (warningFreq) {
                    json << "      \"warning_freq\": " << (warningFreq->get() != 0 ? "true" : "false") << ",\n";
                }
            }
            
            // Channel display fields
            std::string displayImgOid = "/ch" + std::to_string(i) + "_display_img";
            std::unique_ptr<IParam> displayImgParam = dm.getParam(displayImgOid, err);
            if (displayImgParam) {
                auto* displayImg = dynamic_cast<ParamWithValue<std::string>*>(displayImgParam.get());
                if (displayImg) {
                    json << "      \"display_img\": \"" << displayImg->get() << "\",\n";
                }
            }
            
            std::string displayTextOid = "/ch" + std::to_string(i) + "_display_text";
            std::unique_ptr<IParam> displayTextParam = dm.getParam(displayTextOid, err);
            if (displayTextParam) {
                auto* displayText = dynamic_cast<ParamWithValue<std::string>*>(displayTextParam.get());
                if (displayText) {
                    json << "      \"display_text\": \"" << displayText->get() << "\",\n";
                }
            }
            
            std::string displayModeOid = "/ch" + std::to_string(i) + "_display_mode";
            std::unique_ptr<IParam> displayModeParam = dm.getParam(displayModeOid, err);
            if (displayModeParam) {
                auto* displayMode = dynamic_cast<ParamWithValue<std::string>*>(displayModeParam.get());
                if (displayMode) {
                    json << "      \"display_mode\": \"" << displayMode->get() << "\"\n";
                }
            }
            
            json << "    }";
            if (i < 8) json << ",";
            json << "\n";
        }
        
        json << "  ]\n";
        json << "}";
        
        res.set_header("Content-Type", "application/json");
        res.set_content(json.str(), "application/json");
    });
    
    webServerRunning = true;
    globalWebServer->listen("0.0.0.0", port);
    webServerRunning = false;
}

int main(int argc, char* argv[])
{
    Logger::StartLogging(argc, argv);

    std::string addr;
    absl::SetProgramUsageMessage("Runs the Visual Audio Deck Catena Service");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));

    // Get executable directory for static file serving
    std::string exeDir = getExecutableDir(argv[0]);
    defineCommands();

    DEBUG_LOG << "Starting Visual Audio Deck Catena gRPC service with web interface...";
    DEBUG_LOG << "gRPC server will run on: " << addr;
    DEBUG_LOG << "Web server will run on: http://localhost:" << (absl::GetFlag(FLAGS_port) + 1);
    
    // Start signal generation thread
    std::thread signalThread(signalGenerationLoop);
    
    // Start web server in its own thread
    std::thread webServerThread(RunWebServer, exeDir, absl::GetFlag(FLAGS_port) + 1);

    // Start Catena RPC server in its own thread
    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();

    // Shut down signal generation
    signalGenerationRunning = false;
    signalThread.join();
    
    // Shut down web server after Catena RPC thread ends
    if (webServerRunning && globalWebServer) {
        globalWebServer->stop();
    }
    webServerThread.join();

    DEBUG_LOG << "All services shut down.";

    // Shutdown Google Logging
    google::ShutdownGoogleLogging();
    return 0;
}