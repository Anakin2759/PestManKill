#include <mimalloc-new-delete.h>
#include "view/MainWindow.h"
int main()
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    MainWindow app("PestManKill", 1200, 800);
    app.run();
    return 0;
}