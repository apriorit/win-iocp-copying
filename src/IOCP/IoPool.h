#pragma once
#include "Util.h"

namespace io
{
    struct IExecutor
    {
        virtual ~IExecutor() {}

        virtual void OnOperationCompleted(LPOVERLAPPED ovl, const DWORD transferred) = 0;
        virtual void OnOperationCanceled(LPOVERLAPPED ovl) = 0;
    };

    struct OVERLAPPEDPLUS
    {
        OVERLAPPED m_ovl;
        IExecutor* m_poolClient;

        explicit OVERLAPPEDPLUS(IExecutor* poolClient = nullptr)
            : m_poolClient(poolClient)
        {
            RtlZeroMemory(&m_ovl, sizeof(m_ovl));
        }
    };

    class IoPool
    {
        IoPool(const IoPool&);
        IoPool& operator=(const IoPool&);
    public:
        explicit IoPool(const DWORD threadCount);
        ~IoPool();

        void Associate(HANDLE file) const;
        void Destroy();

    private:
        util::HandleGuard m_iocp;
        std::vector<HANDLE> m_threads;
    };

} // namespace io