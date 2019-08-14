#ifndef WIN_UTIL_H
#define WIN_UTIL_H

#include <string>
#include <windows.h>
#include <list>

namespace utils
{

    struct State
    {
        WIN32_FIND_DATAW m_findData;
        HANDLE m_handle;
        std::wstring m_path, m_mask, m_query;
        void* m_context;
    public:
        State(const std::wstring & mask = L"", const std::wstring & path = L"")
            : m_path( path ),
            m_mask( mask ),
            m_handle(0),
            m_context(0)
        {
            m_query = path + mask;
        }
    };

struct ISearchExaminer
{
    virtual ~ISearchExaminer(){}
    virtual void OnCannotStart(const std::wstring & sPath, DWORD dwError)=0;
    virtual void OnFileFound(const std::wstring & sPath, WIN32_FIND_DATAW * pFindData)=0;
    virtual void OnDirectoryFound(const std::wstring & sPath, WIN32_FIND_DATAW * pFindData)=0;
};

struct ISearchExaminer2:public ISearchExaminer
{
    virtual void OnDoneDirectory(const std::wstring & sPath)=0;
};

struct ISearchExaminer3:public ISearchExaminer2
{
    virtual bool CanProcessDirectory(const std::wstring & sPath, WIN32_FIND_DATAW * pFindData)=0;
};
// Cautions:
//      Mask affects on directories too!
//      To avoid this behaviour use "*.*" mask and GetFileExtension function

typedef enum { SM_SEARCH_FILE = 0x1, SM_SEARCH_FOLDER = 0x2, SM_SEARCH_ALL = 0xF } TSearchMode;

class CSearcher
{
    template<class ContainerType>
    class CCleaner
    {
        ContainerType * m_pContainer;
        CSearcher * m_pSearcher;

        CCleaner(const CCleaner&);
        CCleaner& operator = (const CCleaner&);
    public:
        CCleaner(ContainerType * pContainer, CSearcher * pSearcher)
            : m_pContainer(pContainer), m_pSearcher(pSearcher)
        {
        }
        ~CCleaner()
        {
            for(ContainerType::iterator it = m_pContainer->begin(); it != m_pContainer->end(); ++it)
            {
                m_pSearcher->FindClose(*it);
            }
        }
    };

        typedef std::list< State > CStateList;
        CStateList m_states;
        ISearchExaminer * m_examiner;
        ISearchExaminer2 * m_examiner2;
        ISearchExaminer3 * m_examiner3;
        
        int m_iMaximumLevels;
        void FindFilesImpl();

        bool m_bSearchFiles;
        bool m_bSearchFolders;
protected:
        virtual void FindFirst(utils::State& state)
        {
            state.m_handle = ::FindFirstFileW(state.m_query.c_str(), &state.m_findData);
        }

        virtual BOOL FindNext(utils::State& state)
        {
            return ::FindNextFileW(state.m_handle, &state.m_findData);
        }

        virtual BOOL FindClose(utils::State& state)
        {
            return ::FindClose(state.m_handle);
        }
public:
        CSearcher(ISearchExaminer * examiner, TSearchMode mode = SM_SEARCH_ALL );
        CSearcher(ISearchExaminer2 * examiner, TSearchMode mode = SM_SEARCH_ALL );
        CSearcher(ISearchExaminer3 * examiner, TSearchMode mode = SM_SEARCH_ALL );

        // if iMaximumLevels=0 - without limitation
        void StartSearch(const std::wstring & wsMask, 
                         const std::wstring & wstrPathName,
                         int iMaximumLevels);

        std::wstring GetRootPath()  const; 
        bool RootWasFailed() const;

};

void EnsureDirectoriesChainForFile(const std::wstring& fileName);

}

#endif