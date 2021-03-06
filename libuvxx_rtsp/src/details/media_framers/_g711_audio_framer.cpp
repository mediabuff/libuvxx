#include "details/media_framers/_g711_audio_framer.hpp"
#include "sample_attributes.hpp"

using namespace uvxx::rtsp;
using namespace uvxx::rtsp::sample_attributes;
using namespace uvxx::rtsp::details::media_framers;

static const uint32_t BITS_PER_SAMPLE = 16;

static const uint32_t BITRATE = 64 * 1000;

_g711_audio_framer::_g711_audio_framer(const media_subsession& subsession) : _media_framer_base(subsession)
{
    auto sample = _media_framer_base::working_sample();

    sample.attribute_set(ATTRIBUTE_AUDIO_BITS_PER_SAMPLE, BITS_PER_SAMPLE);

    sample.attribute_set(ATTRIBUTE_AUDIO_CHANNEL_COUNT, _subsession.channel_count());

    sample.attribute_set(ATTRIBUTE_AUDIO_SAMPLES_PER_SECOND, _subsession.rtp_timestamp_frequency());

    sample.attribute_set(ATTRIBUTE_AUDIO_BITRATE, BITRATE);
}

_g711_audio_framer::~_g711_audio_framer()
{
}

void _g711_audio_framer::sample_receieved(bool packet_marker_bit)
{
    _media_framer_base::sample_receieved(packet_marker_bit);
}