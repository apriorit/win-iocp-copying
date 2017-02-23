#pragma once

namespace util
{

    class HandleGuard
    {
        HANDLE m_handle;
        HandleGuard(const HandleGuard&);
        HandleGuard& operator=(const HandleGuard&);
    public:
        explicit HandleGuard(HANDLE handle = nullptr)
            : m_handle(handle)
        {
        }
        ~HandleGuard()
        {
            reset();
        }
        HANDLE get() const
        {
            return m_handle;
        }
        HANDLE release()
        {
            HANDLE tmp = m_handle;
            m_handle = nullptr;
            return tmp;
        }
        void reset(HANDLE handle = nullptr)
        {
            if (m_handle)
                ::CloseHandle(m_handle);
            m_handle = handle;
        }
    };

} // namespace util