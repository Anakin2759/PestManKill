#include "view/MainWindowECS.h"
#include "utils"
#define STB_IMAGE_IMPLEMENTATION
int main()
{
#if defined(_MSC_VER)
    // 设置控制台为 UTF-8 编码（Windows 特有）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    MainWindowECS app("PestManKill", 1200, 800);
    app.run();
    return 0;
}