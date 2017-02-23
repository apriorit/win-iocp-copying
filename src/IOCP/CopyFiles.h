#pragma once

namespace io
{
    class IoPool;
}

namespace copy
{

    void CopyFiles(const io::IoPool& ioPool, const std::wstring& src, const std::wstring& dst);

} // namespace copy