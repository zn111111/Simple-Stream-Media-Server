#pragma once

#include <string>

namespace ssms
{
    namespace media
    {
        class SsmsSdp
        {
        public:
            SsmsSdp(std::string server_addr, std::string server_port);
            ~SsmsSdp() = default;

            int Decode(const std::string &sdp);
            std::string Encode();
            void SetStream(const std::string stream_name);
            void SetFingerprint(std::string fingerprint);
            std::string GetLocalUfrag() const;
            std::string GetRemoteUfrag() const;
            std::string GetLocalPassword() const;
        private:
            std::string stream_name_;
            int video_payload_type_{-1};
            int audio_payload_type_{-1};
            //服务端生成的用户名
            std::string local_ufrag_;
            //服务端生成的密码
            std::string local_pwd_;
            //客户端发来的用户名
            std::string remote_ufrag_;
            //客户端发来的密码
            std::string remote_pwd_;
            std::string fingerprint_;
            uint32_t audio_ssrc_{0};
            uint32_t video_ssrc_{0};
            std::string server_addr_;
            std::string server_port_;
        };
    }
}