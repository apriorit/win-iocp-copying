#include "stdafx.h"
#include "CopyFiles.h"
#include "Exception.h"
#include "Util.h"
#include "OvlFile.h"
#include "winUtil.h"

namespace copy
{
    enum consts
    {
        ConcurrentFiles = 1000, // Copy 1000 files simultaneously
        BufferSize = 0x4000 // Use 16K read/write buffers
    };

    class CopyFile
        : public std::enable_shared_from_this<CopyFile>
    {
    public:
        typedef std::function<void()> CompleteHandler_t;

        CopyFile
            ( const io::IoPool& ioPool
            , const std::wstring& srcPath
            , const std::wstring& dstPath
            , CompleteHandler_t onComplete
            )
            : m_srcFile(ioPool, srcPath.c_str(), OPEN_EXISTING, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE)
            , m_srcSize(m_srcFile.Size())
            , m_dstFile(ioPool, dstPath.c_str(), CREATE_ALWAYS)
            , m_onComplete(onComplete)
            , m_copied(0)
            , m_buf(BufferSize)
        {
        }
        void Start()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            StartRead();
        }
    private:
        void StartRead()
        {
            io::CompleteHandler_t completeHandler = std::bind
                ( std::mem_fn(&CopyFile::OnReadComplete)
                , shared_from_this()
                , std::placeholders::_1);

            m_srcFile.StartRead(m_copied, &m_buf[0],
                static_cast<DWORD>(m_buf.size()), completeHandler);
        }
        void OnReadComplete(DWORD transferred)
        {
            try
            {
                if (!transferred)
                {
                    m_onComplete();
                    return;
                }

                std::lock_guard<std::mutex> lock(m_mutex);

                io::CompleteHandler_t completeHandler = std::bind
                    ( std::mem_fn(&CopyFile::OnWriteComplete)
                    , shared_from_this()
                    , std::placeholders::_1);

                m_dstFile.StartWrite(m_copied, &m_buf[0], transferred, completeHandler);
            }
            catch (const std::exception&)
            {                
            }
        }
        void OnWriteComplete(DWORD transferred)
        {
            try
            {
                if (!transferred)
                {
                    m_onComplete();
                    return;
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                m_copied += transferred;

                if (m_copied == m_srcSize)
                {
                    m_onComplete();
                    return;
                }

                StartRead();
            }
            catch (const std::exception&)
            {
            }
        }
    private:
        io::OvlFile m_srcFile;
        const uint64_t m_srcSize;
        io::OvlFile m_dstFile;
        std::vector<char> m_buf;
        CompleteHandler_t m_onComplete;
        mutable std::mutex m_mutex;
        uint64_t m_copied;
    };

    class FileCopier
        : public utils::ISearchExaminer3
    {
        std::vector<std::wstring> m_files;
        const std::wstring& m_dstPath;
        const std::wstring& m_srcPath;
        uint64_t m_totalFilesProcessed;
        const io::IoPool& m_ioPool;
    public:
        FileCopier(const io::IoPool& ioPool, const std::wstring& srcPath, const std::wstring& dstPath)
            : m_ioPool(ioPool)
            , m_srcPath(srcPath)
            , m_dstPath(dstPath)
            , m_totalFilesProcessed(0)
        {
        }
        uint64_t TotalProcessed() const
        {
            return m_totalFilesProcessed;
        }
    private:
        void ProcessFiles()
        {
            const std::size_t fileCount = m_files.size();

            HANDLE completeEvent = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
            ex::CheckZero(completeEvent, "ProcessFiles. Create event");

            util::HandleGuard eventGuard(completeEvent);

            std::atomic_int64_t filesProcessed = 0;
            CopyFile::CompleteHandler_t onFileComplete = [&filesProcessed, completeEvent, fileCount]()
            {
                if (++filesProcessed == fileCount)
                    ::SetEvent(completeEvent);
            };

            for (const std::wstring& srcFile : m_files)
            {
                try
                {
                    std::wstring dstFile = srcFile;
                    dstFile.replace(0, m_srcPath.size(), m_dstPath);

                    utils::EnsureDirectoriesChainForFile(dstFile);

                    std::shared_ptr<CopyFile> copyFile = std::make_shared<CopyFile>
                            ( m_ioPool
                            , srcFile
                            , dstFile
                            , onFileComplete);

                    copyFile->Start();
                }
                catch (const std::exception& ex)
                {
                    std::cerr << ex.what() << "\n";
                }
            }

            ::WaitForSingleObject(completeEvent, INFINITE);
            m_totalFilesProcessed += filesProcessed;
        }
        // cmn::ISearchExaminer3
        void OnCannotStart(const std::wstring&, DWORD dwError)
        {
            std::cerr << "Can't start search, error = " << dwError << "\n";
        }
        void OnFileFound(const std::wstring& path, WIN32_FIND_DATAW* findData)
        {
            if (path.empty())
                return;

            std::wstring filePath = path;
            if (filePath.back() != L'\\'
                && filePath.back() != L'/')
            {
                filePath.push_back(L'\\');
            }

            filePath.append(findData->cFileName);

            m_files.push_back(std::move(filePath));
            if (m_files.size() != ConcurrentFiles)
                return;

            ProcessFiles();
            m_files.clear();
        }
        void OnDirectoryFound(const std::wstring&, WIN32_FIND_DATAW*)
        {
        }
        void OnDoneDirectory(const std::wstring& path)
        {
            if (path.empty() || path != m_srcPath)
                return;

            ProcessFiles();
            m_files.clear();
        }
        bool CanProcessDirectory(const std::wstring&, WIN32_FIND_DATAW*)
        {
            return true;
        }
    };

    void CopyFiles
        ( const io::IoPool& ioPool
        , const std::wstring& src
        , const std::wstring& dst
        )
    {
        FileCopier copier(ioPool, src, dst);
        utils::CSearcher searcher(&copier);
        searcher.StartSearch(L"*", src, 0);
        
        std::cout << copier.TotalProcessed() << " files processed\n";
    }

} // namespace copy