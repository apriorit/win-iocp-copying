#include "StdAfx.h"
#include "CopyFiles.h"
#include "IoPool.h"

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        if (argc < 3)
            throw std::runtime_error("Invalid argument");

        const wchar_t slash = L'\\';
        std::wstring src(argv[1]);
        if (src.back() != slash)
            src.push_back(slash);

        std::wstring dst(argv[2]);
        if (dst.back() != slash)
            dst.push_back(slash);

        io::IoPool ioPool(4);
        copy::CopyFiles(ioPool, src, dst);
    }
    catch (const std::exception& ex)
    {
        std::cout << "Error: " << ex.what() << std::endl;
    }

    std::cout << "\n\n";
    std::system("pause");
    return 0;
}