#include "SsmsHttpHandler.h"
#include "Base/SsmsLogStream.h"
#include "Base/SsmsUtils.h"
#include "SsmsMediaDefine.h"

using namespace ssms::media::base;
using namespace ssms::base;
using namespace ssms::media;

int SsmsHttpHandler::Parse(const SsmsBufferPtr &data, std::string &method, std::string &url, std::unordered_map<std::string, std::string> &out_header, const char **out_payload, int &out_payload_len)
{
    const char *p = data->peek();
    const char *pos = nullptr;
    uint32_t offset = 0;
    if (!(pos = strstr(p + offset, " ")))
    {
        return 1;
    }
    method.assign(p + offset, pos - p - offset);
    bool parse_payload = false;
    if (method == "POST")
    {
        parse_payload = true;
    }
    offset += method.size() + 1;
    int url_end = 0;
    if (data->readableBytes() < offset || !(pos = strstr(p + offset, " ")))
    {
        return 1;
    }
    url.assign(p + offset, pos - p - offset);
    if (!(pos = strstr(p, "\r\n")))
    {
        return 1;
    }
    offset = pos - p + 2;

    std::vector<std::string> v(2);
    while ((pos = strstr(p + offset, "\r\n\r\n")))
    {
        if (!(pos = strstr(p + offset, "\r\n")))
        {
            return 1;
        }
        v = SsmsUtils::Split(std::string(p + offset, pos - p - offset), ": ");
        offset = pos - p + 2;
        if (v.size() != 2)
        {
            v.clear();
            continue;
        }
        out_header[v[0]] = v[1];
        v.clear();
    }

    offset += 2;
    if (parse_payload)
    {
        if (out_header.find("Content-Length") == out_header.end())
        {
            LOG_ERROR << "http data format error\n";
            return -1;
        }
        else if (data->readableBytes() < atoi(out_header["Content-Length"].c_str()) + offset)
        {
            return 1;
        }
        *out_payload = p + offset;
        out_payload_len = atoi(out_header["Content-Length"].c_str());
        offset += out_payload_len;
    }
    data->retrieve(offset);
    return 0;
}

int SsmsHttpHandler::OnResponse(const std::string &method, const std::string &url, SsmsPacketPtr &pkt, const char *out_payload, int out_payload_len)
{
    return 0;
}