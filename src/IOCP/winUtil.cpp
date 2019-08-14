#include "StdAfx.h"
#include "winUtil.h"
#include "..\IOCP\Exception.h"
#include <algorithm>

namespace utils
{
    CSearcher::CSearcher(ISearchExaminer * examiner, TSearchMode mode )
        : m_examiner(examiner),
          m_examiner2(0),
          m_examiner3(0),
          m_bSearchFiles ( (mode & SM_SEARCH_FILE) > 0 ),
          m_bSearchFolders ( (mode & SM_SEARCH_FOLDER) > 0 ),
          m_iMaximumLevels(0)
    {
    }

    CSearcher::CSearcher(ISearchExaminer2 * examiner, TSearchMode mode )
        : m_examiner(examiner),
          m_examiner2(examiner),
          m_examiner3(0),
          m_bSearchFiles ( (mode & SM_SEARCH_FILE) > 0 ),
          m_bSearchFolders ( (mode & SM_SEARCH_FOLDER) > 0 ),
          m_iMaximumLevels(0)
    {
    }

    CSearcher::CSearcher(ISearchExaminer3 * examiner, TSearchMode mode)
        : m_examiner(examiner),
          m_examiner2(examiner),
          m_examiner3(examiner),
          m_bSearchFiles ( (mode & SM_SEARCH_FILE) > 0 ),
          m_bSearchFolders ( (mode & SM_SEARCH_FOLDER) > 0 ),
          m_iMaximumLevels(0)
    {
    }
        
    std::wstring CSearcher::GetRootPath()  const
    { 
        if (m_states.empty())
            throw std::exception("Cannot get root path: empty states");

        return m_states.begin()->m_path;
    }

    void CSearcher::StartSearch(const std::wstring & wsMask, 
                                    const std::wstring & wstrPathName,
                                    int iMaximumLevels)
    {
        if (wstrPathName.empty())
            throw std::exception("Cannot start search: empty name");

        m_iMaximumLevels = iMaximumLevels;
        if (wstrPathName[wstrPathName.length()-1]!=L'\\' && wstrPathName[wstrPathName.length()-1]!=L'/')
                      m_states.push_back(State(wsMask, wstrPathName+L"\\"));
        else
                      m_states.push_back(State(wsMask, wstrPathName));
        FindFilesImpl();
    }

    bool CSearcher::RootWasFailed() const
    {
        if (m_states.size() != 1)
            return false;

        const State & state = m_states.back();
        return state.m_handle == INVALID_HANDLE_VALUE;
    }

    void CSearcher::FindFilesImpl()
    {
        CCleaner<CStateList> guard(&m_states, this);

        while(!m_states.empty())
        {
            State & state = m_states.back();
            if (!state.m_handle)
            {
                //state.m_handle = FindFirstFileW(state.m_query.c_str(), &state.m_findData);
                FindFirst(state);
                if (state.m_handle == INVALID_HANDLE_VALUE)
                {
                    m_examiner->OnCannotStart(state.m_path, GetLastError());
                    m_states.pop_back();
                    continue;
                }
            }
            else
            {
                if (!/*FindNextFileW(state.m_handle, &state.m_findData)*/FindNext(state))
                {
                    FindClose(state);
                    state.m_handle = 0;

                    if (m_examiner2)
                        m_examiner2->OnDoneDirectory( m_states.back().m_path );
                    
                    m_states.pop_back();
                    continue;
                }
            }

            if(state.m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!wcscmp(state.m_findData.cFileName, L".") || !wcscmp(state.m_findData.cFileName, L".."))
                    continue;

                if (state.m_findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                    continue;

                std::wstring path  = state.m_path + state.m_findData.cFileName + L"\\";

                bool bNeedToProcess = true;
			    if ( m_bSearchFolders )
                {
                    m_examiner->OnDirectoryFound( path, &state.m_findData );
                    if (m_examiner3)
                        bNeedToProcess = m_examiner3->CanProcessDirectory( path, &state.m_findData );
                }

                if (bNeedToProcess && ((!m_iMaximumLevels) || (int)m_states.size() < m_iMaximumLevels))
                    m_states.push_back(State(state.m_mask, path));
            } 
            else
            {
			    if ( m_bSearchFiles )
                    m_examiner->OnFileFound( state.m_path, &state.m_findData );
            }
           
        } // while
    }

    void EnsureDirectoriesChainForFile(const std::wstring& fileName)
    {
        size_t realSymbolsCount = 0;
        bool uncPath = false;
        int wordsCount = 0;

        if (fileName.size() < 2)
            return;

        uncPath = (fileName[0] == L'\\'  &&  fileName[1] == L'\\');

        std::wstring temp;
        bool wasSeparator = true;
        for (int i = 0; i < (int)fileName.size(); ++i)
        {
            if (fileName[i] == '\\')
            {
                if (realSymbolsCount > 1)
                {
                    if (!(uncPath && wordsCount < 2))
                    {
                        temp.assign(fileName.begin(), fileName.begin() + i);
                        if (!::CreateDirectoryW(temp.c_str(), 0))
                        {
                            DWORD err = ::GetLastError();
                            if (err != ERROR_ALREADY_EXISTS)
                                throw ex::WinException("[CannotCreateDirectory]:", err);
                        }
                    }
                }
                wasSeparator = true;
            }
            else
            {
                if (wasSeparator)
                    ++wordsCount;
                wasSeparator = false;
            }

            switch (fileName[i])
            {
            case L'~':
            case L'\\':
            case L'/':
            case L':':
                continue;
            }

            ++realSymbolsCount;
        }
    }
}