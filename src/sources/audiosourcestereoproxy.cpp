#include "sources/audiosourcestereoproxy.h"

#include "util/sample.h"
#include "util/logger.h"


namespace mixxx {

namespace {

const Logger kLogger("AudioSourceStereoProxy");

} // anonymous namespace

AudioSourceStereoProxy::AudioSourceStereoProxy(
        AudioSourcePointer pAudioSource,
        SINT maxReadableFrames)
    : AudioSource(*pAudioSource),
      m_pAudioSource(std::move(pAudioSource)),
      m_tempSampleBuffer(
              (m_pAudioSource->channelCount() != 2) ?
                      m_pAudioSource->frames2samples(maxReadableFrames) : 0),
      m_tempOutputBuffer(m_tempSampleBuffer) {
    setChannelCount(2);
}

AudioSourceStereoProxy::AudioSourceStereoProxy(
        AudioSourcePointer pAudioSource,
        SampleBuffer::WritableSlice m_tempOutputBuffer)
    : AudioSource(*pAudioSource),
      m_pAudioSource(std::move(pAudioSource)),
      m_tempOutputBuffer(m_tempOutputBuffer) {
    setChannelCount(2);
}

namespace {

inline
bool isDisjunct(
        const SampleBuffer::WritableSlice& slice1,
        const SampleBuffer::WritableSlice& slice2) {
    if (slice1.data() == slice2.data()) {
        return false;
    }
    if ((slice1.size() == 0) || (slice2.size() == 0)) {
        return true;
    }
    if (slice1.data() < slice2.data()) {
        return slice1.data(slice1.size()) <= slice2.data();
    } else {
        return slice2.data(slice2.size()) <= slice1.data();
    }
}

}

ReadableSampleFrames AudioSourceStereoProxy::readSampleFramesClamped(
        WritableSampleFrames sampleFrames) {
    if (m_pAudioSource->channelCount() == 2) {
        return readSampleFramesClampedOn(*m_pAudioSource, sampleFrames);
    }

    // Check location and capacity of temporary buffer
    VERIFY_OR_DEBUG_ASSERT(isDisjunct(
            m_tempOutputBuffer,
            SampleBuffer::WritableSlice(sampleFrames.writableSlice()))) {
        kLogger.warning()
                << "Overlap between output and temporary sample buffer detected";
        return ReadableSampleFrames();
    }
    {
        const SINT numberOfSamplesToRead =
                m_pAudioSource->frames2samples(sampleFrames.frameIndexRange().length());
        VERIFY_OR_DEBUG_ASSERT(m_tempOutputBuffer.size() >= numberOfSamplesToRead) {
            kLogger.warning()
                    << "Insufficient temporary sample buffer capacity"
                    << m_tempOutputBuffer.size()
                    << "<"
                    << numberOfSamplesToRead
                    << "for reading frames"
                    << sampleFrames.frameIndexRange();
            return ReadableSampleFrames();
        }
    }

    const auto readableSampleFrames =
            readSampleFramesClampedOn(
                    *m_pAudioSource,
                    WritableSampleFrames(
                            sampleFrames.frameIndexRange(),
                            m_tempOutputBuffer));
    if (readableSampleFrames.frameIndexRange().empty()) {
        return readableSampleFrames;
    }
    DEBUG_ASSERT(readableSampleFrames.frameIndexRange() <= sampleFrames.frameIndexRange());
    DEBUG_ASSERT(readableSampleFrames.frameIndexRange().start() >= sampleFrames.frameIndexRange().start());
    const SINT frameOffset =
            readableSampleFrames.frameIndexRange().start() - sampleFrames.frameIndexRange().start();
    SampleBuffer::WritableSlice writableSlice(
            sampleFrames.writableData(frames2samples(frameOffset)),
            frames2samples(readableSampleFrames.frameIndexRange().length()));
    if (m_pAudioSource->channelCount() == 1) {
        SampleUtil::copyMonoToDualMono(
                writableSlice.data(),
                readableSampleFrames.readableData(),
                readableSampleFrames.frameIndexRange().length());
    } else {
        SampleUtil::copyMultiToStereo(
                writableSlice.data(),
                readableSampleFrames.readableData(),
                readableSampleFrames.frameIndexRange().length(),
                m_pAudioSource->channelCount());
    }
    return ReadableSampleFrames(
            readableSampleFrames.frameIndexRange(),
            SampleBuffer::ReadableSlice(
                    writableSlice.data(),
                    writableSlice.size()));
}

}
