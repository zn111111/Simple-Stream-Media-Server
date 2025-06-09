#pragma once

#include <mutex>
#include "NonCopyable.h"

using namespace ssms::base;

namespace ssms
{
    namespace base
    {
        //单例模板, 通过once_flag确保初始化只会被调用一次, 确保线程安全
        template <typename T>
        class Singleton : public NonCopyable
        {
        public:
            Singleton() = delete;
            ~Singleton() = delete;
            static T *GetInstance();
        private:
            static void Init();

            static T *value_;
            static std::once_flag once_;
        };

        template <typename T>
        T *Singleton<T>::value_ = nullptr;

        template <typename T>
        std::once_flag Singleton<T>::once_;

        template <typename T>
        T *Singleton<T>::GetInstance()
        {
            std::call_once(once_, [] () {Init();});

            return value_;
        }

        template <typename T>
        void Singleton<T>::Init()
        {
            value_ = new T();
        }
    }
}