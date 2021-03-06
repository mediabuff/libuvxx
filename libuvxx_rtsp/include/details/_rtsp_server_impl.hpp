#pragma once
#include <memory>

#include "_live_environment_base.hpp"
#include "_live_server_media_session.hpp"

class UsageEnvironment;

class ServerMediaSession;

namespace uvxx { namespace rtsp { namespace details 
{
    class _live_rtsp_server;

    using _live_rtsp_server_ptr = std::shared_ptr<_live_rtsp_server>;

    using _usage_environment_ptr = std::shared_ptr<UsageEnvironment>;
   
    class _rtsp_server_impl : public event_dispatcher_object, protected _live_environment_base
    {
    public:
        explicit _rtsp_server_impl(uint16_t port);

        _rtsp_server_impl(const _rtsp_server_impl&) = delete;

        _rtsp_server_impl& operator=(const _rtsp_server_impl&) = delete;

    public:
        uint16_t port();

        void on_session_request_set(on_session_request_delegate callback);

    private:
        uvxx::pplx::task<_live_server_media_session*> on_live_media_session_lookup(const std::string& stream_name);

    private:
        _live_rtsp_server_ptr _live_server;

        uint16_t _port;

        on_session_request_delegate _on_session_request_delegate;
    };
}}}