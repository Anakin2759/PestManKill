#include "view/MainWindowECS.h"
#include "utils"
#define STB_IMAGE_IMPLEMENTATION
int main()
{
#if defined(_MSC_VER)
    // 设置控制台为 UTF-8 编码（Windows 特有）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setlocale(LC_ALL, "C");
    // 强制 C++ I/O 流语言环境为 "C" (英文)
    std::locale::global(std::locale("C"));
#endif

    app.run();
    return 0;
}