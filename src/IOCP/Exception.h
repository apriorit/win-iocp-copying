#pragma once

namespace ex
{

    class WinException
        : public std::runtime_error
    {
    public:
        explicit WinException(const char* const what, const DWORD error = ::GetLastError())
            : runtime_error(std::string(what) + ", error = " + std::to_string(error))
        {
        }
    };

    template<class ParamType>
    inline void CheckZero(ParamType param, const char* const desc)
    {
        if (param)
            return;

        const DWORD lastError = ::GetLastError();
        throw WinException(desc, lastError);
    }

} // namespace ex