#include "stdafx.h"
#include "IoPool.h"
#include "Exception.h"

enum CompletionKeys
{
    ThreadExitKey = 1
};

namespace
{

    unsigned __stdcall WorkThreadFunction(void* param)
    {
        try
        {
            const HANDLE iocp = param;

            for (;;)
            {
                // get completion

                LPOVERLAPPED ovl = nullptr;
                DWORD transferred = 0;
                ULONG_PTR completionKey = 0;
                const BOOL result = ::GetQueuedCompletionStatus(iocp, &transferred, &completionKey, &ovl, INFINITE);

                if (!ovl && !result)
                {
                    continue;
                }

                io::OVERLAPPEDPLUS* ovlPlus = CONTAINING_RECORD(ovl, io::OVERLAPPEDPLUS, m_ovl);
                
                if (!result) // Operation canceled
                {
                    if (!ovlPlus->m_poolClient)
                        continue;

                    ovlPlus->m_poolClient->OnOperationCanceled(ovl);
                    continue;
                }

                if (completionKey == ThreadExitKey)
                    return 0;

                // Operation completed
                if (ovlPlus->m_poolClient)
                {
                    ovlPlus->m_poolClient->OnOperationCompleted(ovl, transferred);
                }

            } // loop
        }
        catch (const std::exception&)
        {
        }

        return 0;
    }

} // namespace

namespace io
{

    IoPool::IoPool(const DWORD concurrency)
        : m_iocp(nullptr)
    {
        // Create IOCP
        HANDLE iocp = ::CreateIoCompletionPort
            ( INVALID_HANDLE_VALUE // don't associate files now
            , nullptr // create new IOCP
            , 0 // completion key is ignored
            , concurrency);

        ex::CheckZero(iocp, "IoPool. Create I/O completion port");
        m_iocp.reset(iocp);

        // Create threads
        for (DWORD i = 0; i < concurrency * 2; ++i)
        {
            const uintptr_t thread = ::_beginthreadex(0, 0,
                &WorkThreadFunction, m_iocp.get(), 0, nullptr);
            if (!thread)
                throw ex::WinException("IoPool. Create thread", errno);

            m_threads.push_back(reinterpret_cast<HANDLE>(thread));
        }
    }

    IoPool::~IoPool()
    {
        Destroy();
    }

    void IoPool::Associate(HANDLE file) const
    {
        HANDLE existingPort = ::CreateIoCompletionPort
            ( file
            , m_iocp.get()
            , 0 // don't use files' completion keys
            , 0); // concurrency is ignored with existing ports

        ex::CheckZero(existingPort, "IoPool. Associate handle with IOCP");
    }

    void IoPool::Destroy()
    {
        // send exit packets to the worker threads
        OVERLAPPEDPLUS destroyOvl;
        for (const HANDLE& thread : m_threads)
            ::PostQueuedCompletionStatus(m_iocp.get(), 0, ThreadExitKey, &destroyOvl.m_ovl);

        // wait threads exit and close handles
        for (HANDLE& thread : m_threads)
        {
            ::WaitForSingleObject(thread, INFINITE);
            ::CloseHandle(thread);
            thread = nullptr;
        }

        m_threads.clear();
    }

} // namespace io