#include <sstream>
#include "SsmsWebrtcPlayClient.h"
#include "Base/SsmsHttpHandler.h"
#include "Base/SsmsLogStream.h"
#include "SsmsPacket.h"
#include "SsmsWebrtcServer.h"

using namespace ssms::media::base;

SsmsWebrtcPlayClient::SsmsWebrtcPlayClient(const SsmsLiveManagmentPtr &live_manage,
                                        SsmsEventLoop *loop,
                                        const SsmsNetAddressPtr &server_addr,
                                        SsmsWebrtcServerPtr rtc_server)
: handler_(server_addr, rtc_server)
{
    
}

int SsmsWebrtcPlayClient::Init()
{
    handler_.SetWebrtcPlayClient(shared_from_this());
    return handler_.Init();
}

void SsmsWebrtcPlayClient::Active()
{
    
}

bool SsmsWebrtcPlayClient::Alive() const
{
    return connection_alive_;
}

void SsmsWebrtcPlayClient::Reset()
{
    connection_alive_ = false;
}

std::string SsmsWebrtcPlayClient::GetSessionName()
{
    return handler_.GetSessionName();
}

std::string SsmsWebrtcPlayClient::GetLocalPassword()
{
    return handler_.sdp_.GetLocalPassword();
}

void SsmsWebrtcPlayClient::SetClientAddr(const SsmsNetAddressPtr &client_addr)
{
    client_addr_ = client_addr;
}

SsmsNetAddressPtr SsmsWebrtcPlayClient::GetClientAddr() const
{
    return client_addr_;
}

int SsmsWebrtcPlayClient::ProcessSdp(const std::string &method, const char *out_payload, int out_payload_len, SsmsPacketPtr &pkt)
{
    connection_alive_ = true;
    return handler_.ProcessSdp(method, out_payload, out_payload_len, pkt);
}

int SsmsWebrtcPlayClient::OnUdpMessage(const SsmsUdpSocketPtr &udp_socket, UdpPacketType type, const SsmsUdpPktPtr &pkt)
{
    connection_alive_ = true;
    return handler_.OnMessage(udp_socket, type, pkt, client_addr_);
}