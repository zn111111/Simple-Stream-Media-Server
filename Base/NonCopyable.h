#pragma once

namespace ssms
{
    namespace base
    {
        //不可拷贝的类需要继承该类
        class NonCopyable
        {
        protected:
            NonCopyable() = default;
            ~NonCopyable() = default;
            
            NonCopyable(const NonCopyable &) = delete;
            NonCopyable &operator=(const NonCopyable &) = delete;
        };
    }
}