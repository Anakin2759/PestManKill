#include <mimalloc-new-delete.h>
#include "view/Application.h"

int main()
{
    ui::Application app(
        "PestManKill Client", ui::Application::DEFAULT_WINDOW_WIDTH, ui::Application::DEFAULT_WINDOW_HEIGHT);

    // 渲染界面
    app.run();

    return 0;
}