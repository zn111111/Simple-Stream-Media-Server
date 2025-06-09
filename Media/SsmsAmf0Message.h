#pragma once

#include <string>
#include <unordered_map>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "SsmsMediaDefine.h"
#include "Base/SsmsLogStream.h"

namespace ssms
{
    namespace media
    {
        class SsmsAmf0Type
        {
        public:
            SsmsAmf0Type() = default;
            virtual ~SsmsAmf0Type() = default;

            //返回值是解析了多少个字节
            virtual int Parse(char *data, uint32_t len)
            {
                return 0;
            };
            //返回值是写入了多少个字节
            virtual int Encode(char *data) = 0;
            //返回值是value_编码之后的字节大小
            virtual int EncodeSize() = 0;
            virtual std::string Value()
            {
                return std::string();
            };
            virtual std::string GetProperty(const std::string &key)
            {
                return std::string();
            }
        };

        class SsmsAmf0Number : public SsmsAmf0Type
        {
        public:
            int Parse(char *data, uint32_t len) override;
            int Encode(char *data) override;
            void SetValue(double value);
            int EncodeSize() override;
            std::string Value() override;
        private:
            double value_{0.0};
        };

        class SsmsAmf0Bool : public SsmsAmf0Type
        {
        public:
            int Parse(char *data, uint32_t len) override;
            int Encode(char *data) override;
            void SetValue(bool value);
            int EncodeSize() override;
            std::string Value() override;
        private:
            bool value_{false};
        };

        class SsmsAmf0String : public SsmsAmf0Type
        {
        public:
            int Parse(char *data, uint32_t len) override;
            int Encode(char *data) override;
            void SetValue(const std::string &value);
            int EncodeSize() override;
            std::string Value() override;
        private:
            std::string value_{false};
        };

        class SsmsAmf0Object : public SsmsAmf0Type
        {
        public:
            int Parse(char *data, uint32_t len) override;
            int Encode(char *data) override;
            void SetValue(const std::string &key, SsmsAmf0TypePtr value);
            int EncodeSize() override;
            std::string Value() override;
            std::string GetProperty(const std::string &key) override;
        private:
            std::unordered_map<std::string, SsmsAmf0TypePtr> properties;
            std::string value_;
        };

        class SsmsAmf0NULL : public SsmsAmf0Type
        {
        public:
            int Encode(char *data) override;
            int EncodeSize() override;
        };

        class SsmsAmf0Undefined : public SsmsAmf0Type
        {
        public:
            int Encode(char *data) override;
            int EncodeSize() override;
        };

        class SsmsAmf0EcmaArray : public SsmsAmf0Type
        {
        public:
            int Parse(char *data, uint32_t len) override;
            int Encode(char *data) override;
            void SetValue(const std::string &key, SsmsAmf0TypePtr value);
            int EncodeSize() override;
            std::string Value() override;
            std::string GetProperty(const std::string &key) override;
        private:
            std::unordered_map<std::string, SsmsAmf0TypePtr> properties;
            std::string value_;
        };
    }
}