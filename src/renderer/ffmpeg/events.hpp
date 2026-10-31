#pragma once

#include "render_settings.hpp"

#include <Geode/loader/Event.hpp>

namespace ffmpeg::events {
namespace impl {
    struct Dummy {};

    using writeFrame_t = geode::Result<>(Dummy::*)(std::vector<uint8_t> const&);

    // Geode 5 uses Event V2. Keep the FFmpeg API bridge synchronous and pass
    // mutable outputs by reference so the receiving FFmpeg API mod can fill them.
    struct CreateRecorderEvent
        : geode::Event<CreateRecorderEvent, bool(void*&)> {};

    struct DeleteRecorderEvent
        : geode::Event<DeleteRecorderEvent, bool(void*)> {};

    struct InitRecorderEvent
        : geode::Event<InitRecorderEvent, bool(void*, RenderSettings const&, geode::Result<>&)> {};

    struct StopRecorderEvent
        : geode::Event<StopRecorderEvent, bool(void*)> {};

    struct GetWriteFrameFunctionEvent
        : geode::Event<GetWriteFrameFunctionEvent, bool(writeFrame_t&)> {};

    struct CodecRecorderEvent
        : geode::Event<CodecRecorderEvent, bool(std::vector<std::string>&)> {};

    struct MixVideoAudioEvent
        : geode::Event<MixVideoAudioEvent, bool(
            std::filesystem::path const&,
            std::filesystem::path const&,
            std::filesystem::path const&,
            geode::Result<>&
        )> {};

    struct MixVideoRawEvent
        : geode::Event<MixVideoRawEvent, bool(
            std::filesystem::path const&,
            std::vector<float> const&,
            std::filesystem::path const&,
            geode::Result<>&
        )> {};
}

class Recorder {
public:
    Recorder() {
        void* ptr = nullptr;
        impl::CreateRecorderEvent().send(ptr);
        m_ptr = static_cast<impl::Dummy*>(ptr);
    }

    ~Recorder() {
        impl::DeleteRecorderEvent().send(m_ptr);
    }

    bool isValid() const { return m_ptr != nullptr; }

    geode::Result<> init(RenderSettings const& settings) {
        geode::Result<> result = geode::Err("Event was not handled");
        impl::InitRecorderEvent().send(m_ptr, settings, result);
        return result;
    }

    void stop() {
        impl::StopRecorderEvent().send(m_ptr);
    }

    geode::Result<> writeFrame(const std::vector<uint8_t>& frameData) {
        static auto writeFrame = [] {
            impl::writeFrame_t function = nullptr;
            impl::GetWriteFrameFunctionEvent().send(function);
            return function;
        }();

        if (!writeFrame)
            return geode::Err("Failed to call writeFrame function.");

        return std::invoke(writeFrame, m_ptr, frameData);
    }

    static std::vector<std::string> getAvailableCodecs() {
        std::vector<std::string> codecs;
        impl::CodecRecorderEvent().send(codecs);
        return codecs;
    }

private:
    impl::Dummy* m_ptr = nullptr;
};

class AudioMixer {
public:
    AudioMixer() = delete;

    static geode::Result<> mixVideoAudio(
        std::filesystem::path const& videoFile,
        std::filesystem::path const& audioFile,
        std::filesystem::path const& outputMp4File
    ) {
        geode::Result<> result = geode::Err("Event was not handled");
        impl::MixVideoAudioEvent().send(videoFile, audioFile, outputMp4File, result);
        return result;
    }

    static geode::Result<> mixVideoRaw(
        std::filesystem::path const& videoFile,
        const std::vector<float>& raw,
        std::filesystem::path const& outputMp4File
    ) {
        geode::Result<> result = geode::Err("Event was not handled");
        impl::MixVideoRawEvent().send(videoFile, raw, outputMp4File, result);
        return result;
    }
};

}
