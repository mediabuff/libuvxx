#include "BasicUsageEnvironment.hh"

#include "media_sample.hpp"
#include "details/_live_server_media_session.hpp"
#include "details/_server_media_session_impl.hpp"
#include "details/_uvxx_task_scheduler.hpp"
#include "details/_h264_media_subsession.hpp"

using namespace uvxx::rtsp::details;

_server_media_session_impl::_server_media_session_impl()
{
    __live_server_media_session = _live_server_media_session_ptr(new _live_server_media_session(),
    [](_live_server_media_session* session)
    {
        /* since the live555 server manages the lifetime of this once
           it gets its grubby paws on the pointer */
        if(!session->is_externally_owned())
        {
            Medium::close(session);
        }
    });

    __live_server_media_session->on_session_closed(std::bind(&_server_media_session_impl::on_session_closed, this));

    OutPacketBuffer::maxSize = 450 * 1024;
}

_server_media_session_impl::~_server_media_session_impl()
{
    if(__live_server_media_session)
    {
        __live_server_media_session->on_session_closed(nullptr);
    }
}

void _server_media_session_impl::on_session_closed()
{
    __live_server_media_session.reset(static_cast<_live_server_media_session*>(nullptr));
}


_server_media_session_impl::_server_media_session_impl(_server_media_session_impl&& rhs)
{
    *this = std::move(rhs);
}

_server_media_session_impl& _server_media_session_impl::operator=(_server_media_session_impl&& rhs)
{
    __live_server_media_session = std::move(rhs.__live_server_media_session);

    _descriptor = std::move(rhs._descriptor);

    return *this;
}

void _server_media_session_impl::set_media_descriptor(const uvxx::rtsp::media_descriptor& descriptor)
{
    _descriptor = descriptor;

    configure_session();
}

void _server_media_session_impl::deliver_sample(int stream_id, const uvxx::rtsp::media_sample& sample)
{
    auto it = _stream_sources.find(stream_id);

    if(it == _stream_sources.end())
    {
        throw std::exception();
    }

    auto& source = it->second;
  
    source->deliver_sample(sample);
}

void _server_media_session_impl::configure_session()
{
    auto& streams = _descriptor.get_streams();

    for(auto& stream : streams)
    {
        if(stream.codec_name() == "H264")
        {
            auto subsession = new _h264_media_subsession(stream.stream_id());

            subsession->source_factory_create_set(std::bind(&_server_media_session_impl::create_framed_source, this, std::placeholders::_1, std::placeholders::_2));

            __live_server_media_session->addSubsession(subsession);
        }
    }
}

void _server_media_session_impl::on_framed_source_closed(int stream_id)
{
    auto it = _stream_sources.find(stream_id);

    if (it == _stream_sources.end())
    {
        throw std::exception();
    }

    _stream_sources.erase(it);
}

FramedSource* _server_media_session_impl::create_framed_source(int stream_id, unsigned /*client_session_id*/)
{
    auto it = _stream_sources.find(stream_id);

    std::shared_ptr<_live_framed_source> source = std::shared_ptr<_live_framed_source>(new _live_framed_source(stream_id), [](_live_framed_source*)
    {
        /* todo add logic later in case live55 doesn't free*/
    });

    source->on_closed_set(std::bind(&_server_media_session_impl::on_framed_source_closed, this, std::placeholders::_1));
    
    _stream_sources[stream_id] = source;
    
    return source.get();
}