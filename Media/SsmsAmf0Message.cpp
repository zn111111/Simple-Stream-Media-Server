#include <string>
#include <unordered_map>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "SsmsMediaDefine.h"
#include "Base/SsmsLogStream.h"
#include "SsmsAmf0Message.h"

using namespace ssms::media;

int SsmsAmf0Number::Parse(char *data, uint32_t len)
{
    uint64_t val = __builtin_bswap64((*(uint64_t *)data));
    memcpy(&value_, &val, sizeof(val));\
    return 8;
}

int SsmsAmf0Number::Encode(char *data)
{
    *data = 0x00;
    uint64_t val = 0;
    memcpy(&val, &value_, sizeof(value_));
    val = __builtin_bswap64(val);
    memcpy(data + 1, &val, sizeof(val));
    return 9;
}

void SsmsAmf0Number::SetValue(double value)
{
    value_ = value;
}

int SsmsAmf0Number::EncodeSize()
{
    return 9;
}

std::string SsmsAmf0Number::SsmsAmf0Number::Value()
{
    return std::to_string((uint64_t)value_);
}

int SsmsAmf0Bool::Parse(char *data, uint32_t len)
{
    value_ = (*data);
    return 1;
}

int SsmsAmf0Bool::Encode(char *data)
{
    *data = 0x01;
    memcpy(data + 1, &value_, sizeof(value_));
    return 2;
}
void SsmsAmf0Bool::SetValue(bool value)
{
    value_ = value;
}

int SsmsAmf0Bool::EncodeSize()
{
    return 2;
}

std::string SsmsAmf0Bool::Value()
{
    return std::to_string(value_);
}

int SsmsAmf0String::Parse(char *data, uint32_t len)
{
    uint32_t str_len = ::ntohs(*(uint16_t *)data);
    value_.assign(data + 2, str_len);
    return str_len + 2;
}

int SsmsAmf0String::Encode(char *data)
{
    *data = 0x02;
    uint16_t len = value_.size();
    len = ::htons(len);
    memcpy(data + 1, &len, sizeof(len));
    memcpy(data + 3, value_.data(), value_.size());
    return value_.size() + 3;
}

void SsmsAmf0String::SetValue(const std::string &value)
{
    value_ = value;
}

int SsmsAmf0String::EncodeSize()
{
    return value_.size() + 3;
}

std::string SsmsAmf0String::Value()
{
    return value_;
}

int SsmsAmf0Object::Parse(char *data, uint32_t len)
{
    uint32_t offset = 0;
    int ret = 0;
    while (offset < len)
    {
        if (len - offset >= 3 && 0x00 == *(data + offset) && 0x00 == *(data + offset + 1) && 0x09 == *(data + offset + 2))
        {
            offset += 3;
            return offset;
        }

        uint16_t str_len = ::ntohs(*(uint16_t *)(data + offset));
        offset += 2;
        std::string key(data + offset, str_len);
        offset += str_len;
        uint8_t type = *(data + offset++);
        switch (type)
        {
            case Amf0Number:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0Number>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
                break;
            case Amf0Bool:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0Bool>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
                break;
            case Amf0String:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0String>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
                break;
            case Amf0Object:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0Object>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
                break;
            case Amf0Null:
                break;
            case Amf0EcmaArray:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0EcmaArray>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
            break;
            default:
                ret = -1;
                return ret;
        }
    }

    return offset;
}

int SsmsAmf0Object::Encode(char *data)
{
    *data = 0x03;
    uint32_t offset = 1;
    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        uint16_t len = it->first.size();
        len = ::htons(len);
        memcpy(data + offset, &len, sizeof(len));
        offset += 2;
        memcpy(data + offset, it->first.data(), it->first.size());
        offset += it->first.size();
        offset += it->second->Encode(data + offset);
    }
    *(data + offset++) = 0x00;
    *(data + offset++) = 0x00;
    *(data + offset++) = 0x09;

    return offset;
}

void SsmsAmf0Object::SetValue(const std::string &key, SsmsAmf0TypePtr value)
{
    properties[key] = value;
}

int SsmsAmf0Object::EncodeSize()
{
    uint32_t size = 1;
    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        size += it->first.size() + 2;
        size += it->second->EncodeSize();
    }
    size += 3;

    return size;
}

std::string SsmsAmf0Object::Value()
{
    if (value_.empty())
    {
        std::ostringstream stream;
        for (auto it = properties.begin(); it != properties.end();)
        {
            stream << it->first << ": " << it->second->Value();
            ++it;
            if (it != properties.end())
            {
                stream << ", ";
            }
        }
        value_.assign(stream.str().data(), stream.str().size());
    }

    return value_;
}

std::string SsmsAmf0Object::GetProperty(const std::string &key)
{
    std::string ret;
    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        if (it->first == key)
        {
            ret.assign(it->second->Value());
            break;
        }

        std::shared_ptr<ssms::media::SsmsAmf0Object> s_obj = std::dynamic_pointer_cast<SsmsAmf0Object>(it->second);
        std::shared_ptr<ssms::media::SsmsAmf0EcmaArray> s_ecma = std::dynamic_pointer_cast<SsmsAmf0EcmaArray>(it->second);
        if (s_obj)
        {
            ret.assign(s_obj->GetProperty(key));
            if (!ret.empty())
            {
                break;
            }
        }
        else if (s_ecma)
        {
            ret.assign(s_ecma->GetProperty(key));
            if (!ret.empty())
            {
                break;
            }
        }
    }

    return ret;
}

int SsmsAmf0NULL::Encode(char *data)
{
    *data = Amf0Null;
    return 1;
}

int SsmsAmf0NULL::EncodeSize()
{
    return 1;
}

int SsmsAmf0Undefined::Encode(char *data)
{
    *data = Amf0Undefined;
    return 1;
}

int SsmsAmf0Undefined::EncodeSize()
{
    return 1;
}

int SsmsAmf0EcmaArray::Parse(char *data, uint32_t len)
{
    uint32_t offset = 0;
    uint32_t ecma_len = ::ntohl(*(uint32_t *)(data + offset));
    offset += 4;
    int ret = 0;
    for (int i = 0; i < ecma_len; i++)
    {
        uint16_t str_len = ::ntohs(*(uint16_t *)(data + offset));
        offset += 2;
        std::string key(data + offset, str_len);
        offset += str_len;
        uint8_t type = *(data + offset++);
        switch (type)
        {
            case Amf0Number:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0Number>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
                break;
            case Amf0Bool:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0Bool>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
                break;
            case Amf0String:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0String>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
                break;
            case Amf0Object:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0Object>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
                break;
            case Amf0Null:
                break;
            case Amf0EcmaArray:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0EcmaArray>();
                ret = amf0_data->Parse(data + offset, len - offset);
                if (ret <= 0)
                {
                    ret = -1;
                    return ret;
                }
                properties[key] = amf0_data;
                offset += ret;
            }
                break;
            default:
                ret = -1;
                break;
        }
    }

    if (len - offset < 3 || 0x00 != *(data + offset) || 0x00 != *(data + offset + 1) || 0x09 != *(data + offset + 2))
    {
        ret = -1;
        return ret;
    }
    offset += 3;

    return offset;
}

int SsmsAmf0EcmaArray::Encode(char *data)
{
    *data = 0x08;
    uint32_t offset = 1;
    uint32_t array_len = properties.size();
    array_len = ::htonl(array_len);
    memcpy(data + offset, &array_len, sizeof(array_len));
    offset += sizeof(array_len);
    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        uint16_t len = it->first.size();
        len = ::htons(len);
        memcpy(data + offset, &len, sizeof(len));
        offset += 2;
        memcpy(data + offset, it->first.data(), it->first.size());
        offset += it->first.size();
        offset += it->second->Encode(data + offset);
    }
    *(data + offset++) = 0x00;
    *(data + offset++) = 0x00;
    *(data + offset++) = 0x09;

    return offset;
}

void SsmsAmf0EcmaArray::SetValue(const std::string &key, SsmsAmf0TypePtr value)
{
    properties[key] = value;
}

int SsmsAmf0EcmaArray::EncodeSize()
{
    uint32_t size = 5;
    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        size += it->first.size() + 2;
        size += it->second->EncodeSize();
    }
    size += 3;

    return size;
}

std::string SsmsAmf0EcmaArray::Value()
{
    if (value_.empty())
    {
        std::ostringstream stream;
        for (auto it = properties.begin(); it != properties.end(); ++it)
        {
            stream << it->first << ": " << it->second->Value();
            if (it != properties.end())
            {
                stream << ", ";
            }
        }
        value_.assign(stream.str().data(), stream.str().size());
    }

    return value_;
}

std::string SsmsAmf0EcmaArray::GetProperty(const std::string &key)
{
    std::string ret;
    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        if (it->first == key)
        {
            ret.assign(it->second->Value());
            break;
        }

        std::shared_ptr<ssms::media::SsmsAmf0Object> s_obj = std::dynamic_pointer_cast<SsmsAmf0Object>(it->second);
        std::shared_ptr<ssms::media::SsmsAmf0EcmaArray> s_ecma = std::dynamic_pointer_cast<SsmsAmf0EcmaArray>(it->second);
        if (s_obj)
        {
            ret.assign(s_obj->GetProperty(key));
            if (!ret.empty())
            {
                break;
            }
        }
        else if (s_ecma)
        {
            ret.assign(s_ecma->GetProperty(key));
            if (!ret.empty())
            {
                break;
            }
        }
    }

    return ret;
}