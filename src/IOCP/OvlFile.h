#pragma once
#include "IoPool.h"

namespace io
{
    typedef std::function<void(DWORD transferred)> CompleteHandler_t;

    class OvlFile
        : public io::IExecutor
    {
    public:
        OvlFile
            ( const io::IoPool& ioPool
            , const wchar_t* const fileName
            , const DWORD disposition
            , const DWORD access = GENERIC_READ | GENERIC_WRITE
            , const DWORD shareMode = FILE_SHARE_READ);
        ~OvlFile();

        void StartRead(uint64_t fileOffset, char* data, DWORD size, CompleteHandler_t handler);
        void StartWrite(uint64_t fileOffset, const char* data, DWORD size, CompleteHandler_t handler);

        uint64_t Size() const;

    private:
        struct FileIoOverlapped;
        std::unique_ptr<FileIoOverlapped> PrepareOverlapped(uint64_t fileOffset, CompleteHandler_t handler);

        // io::IExecutor
        void OnOperationCompleted(LPOVERLAPPED ovl, const DWORD transferred);
        void OnOperationCanceled(LPOVERLAPPED ovl);

    private:
        util::HandleGuard m_file;
    };

} // namespace io

