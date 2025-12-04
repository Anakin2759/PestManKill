# TextEdit 使用示例

## 简介

`TextEdit` 是一个基于 ImGui 的多行文本编辑控件，支持文本输入、编辑和显示。

## 特性

- ✅ 多行文本输入
- ✅ 只读/可编辑模式切换
- ✅ 占位符文本显示
- ✅ 文本变更回调
- ✅ 最大长度限制
- ✅ 自动换行控制
- ✅ 文本追加和清空

## 基本用法

### 1. 创建 TextEdit 控件

```cpp
#include "src/client/ui/TextEdit.h"

// 创建一个文本编辑控件
auto textEdit = std::make_shared<ui::TextEdit>();

// 设置初始文本
textEdit->setText("Hello, World!");

// 设置固定尺寸
textEdit->setFixedSize(400.0f, 300.0f);
```

### 2. 设置占位符

```cpp
auto textEdit = std::make_shared<ui::TextEdit>();
textEdit->setPlaceholder("请输入聊天消息...");
```

### 3. 只读模式

```cpp
auto textEdit = std::make_shared<ui::TextEdit>();
textEdit->setText("这是只读文本");
textEdit->setReadOnly(true);
```

### 4. 文本变更回调

```cpp
auto textEdit = std::make_shared<ui::TextEdit>();
textEdit->setOnTextChanged([](const std::string& newText) {
    utils::LOG_INFO("文本已更改: {}", newText);
});
```

### 5. 聊天窗口示例

```cpp
// 创建聊天消息显示区（只读）
auto chatDisplay = std::make_shared<ui::TextEdit>();
chatDisplay->setReadOnly(true);
chatDisplay->setFixedSize(400.0f, 400.0f);
chatDisplay->setText("欢迎来到游戏聊天室！\n");

// 创建输入框（可编辑）
auto chatInput = std::make_shared<ui::TextEdit>();
chatInput->setPlaceholder("输入消息...");
chatInput->setFixedSize(400.0f, 80.0f);
chatInput->setOnTextChanged([chatDisplay](const std::string& text) {
    // 当用户输入回车时，将消息添加到显示区
    if (text.find('\n') != std::string::npos) {
        std::string message = text;
        message.pop_back(); // 移除换行符
        chatDisplay->appendText("你: " + message + "\n");
        chatInput->clear();
    }
});

// 添加到布局
layout->addWidget(chatDisplay);
layout->addWidget(chatInput);
```

### 6. 日志查看器示例

```cpp
auto logViewer = std::make_shared<ui::TextEdit>();
logViewer->setReadOnly(true);
logViewer->setFixedSize(600.0f, 400.0f);
logViewer->setWordWrap(false); // 禁用自动换行

// 添加日志
logViewer->appendText("[INFO] 服务器启动成功\n");
logViewer->appendText("[WARN] 连接超时，正在重试...\n");
logViewer->appendText("[ERROR] 无法连接到数据库\n");
```

### 7. 设置最大长度

```cpp
auto textEdit = std::make_shared<ui::TextEdit>();
textEdit->setMaxLength(1000); // 限制最多 1000 个字符
```

### 8. 文本操作

```cpp
auto textEdit = std::make_shared<ui::TextEdit>();

// 设置文本
textEdit->setText("初始文本");

// 追加文本
textEdit->appendText("\n新的一行");

// 获取当前文本
std::string currentText = textEdit->getText();

// 清空文本
textEdit->clear();
```

## 完整示例：聊天客户端

```cpp
#include "src/client/ui/TextEdit.h"
#include "src/client/ui/Button.h"
#include "src/client/ui/Layout.h"

class ChatWindow {
public:
    ChatWindow() {
        // 创建垂直布局
        auto layout = std::make_shared<ui::VBoxLayout>();
        
        // 聊天记录显示区
        m_chatDisplay = std::make_shared<ui::TextEdit>();
        m_chatDisplay->setReadOnly(true);
        m_chatDisplay->setFixedSize(500.0f, 400.0f);
        layout->addWidget(m_chatDisplay, 1);
        
        // 输入区布局
        auto inputLayout = std::make_shared<ui::HBoxLayout>();
        
        // 消息输入框
        m_chatInput = std::make_shared<ui::TextEdit>();
        m_chatInput->setPlaceholder("输入消息并按回车发送...");
        m_chatInput->setFixedSize(400.0f, 60.0f);
        m_chatInput->setOnTextChanged([this](const std::string& text) {
            onTextChanged(text);
        });
        inputLayout->addWidget(m_chatInput, 1);
        
        // 发送按钮
        auto sendButton = std::make_shared<ui::Button>("发送", [this]() {
            sendMessage();
        });
        inputLayout->addWidget(sendButton);
        
        layout->addWidget(inputLayout);
    }
    
    void onTextChanged(const std::string& text) {
        // 检测回车键
        if (text.find('\n') != std::string::npos) {
            sendMessage();
        }
    }
    
    void sendMessage() {
        std::string message = m_chatInput->getText();
        if (message.empty()) return;
        
        // 移除尾部换行符
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }
        
        if (!message.empty()) {
            // 添加到显示区
            m_chatDisplay->appendText("你: " + message + "\n");
            
            // 发送到服务器
            // gameClient->sendChatMessage(message);
            
            // 清空输入框
            m_chatInput->clear();
        }
    }
    
    void addMessage(const std::string& sender, const std::string& message) {
        m_chatDisplay->appendText(sender + ": " + message + "\n");
    }

private:
    std::shared_ptr<ui::TextEdit> m_chatDisplay;
    std::shared_ptr<ui::TextEdit> m_chatInput;
};
```

## API 参考

### 文本操作

- `void setText(const std::string& text)` - 设置文本内容
- `std::string getText() const` - 获取当前文本
- `void appendText(const std::string& text)` - 追加文本
- `void clear()` - 清空文本

### 属性设置

- `void setReadOnly(bool readOnly)` - 设置只读模式
- `bool isReadOnly() const` - 获取是否只读
- `void setPlaceholder(const std::string& placeholder)` - 设置占位符
- `void setMaxLength(size_t maxLength)` - 设置最大长度
- `void setWordWrap(bool wordWrap)` - 设置自动换行
- `void setOnTextChanged(std::function<void(const std::string&)>)` - 设置文本变更回调

### 继承自 Widget

- `void setFixedSize(float width, float height)` - 设置固定尺寸
- `void setMinimumSize(float width, float height)` - 设置最小尺寸
- `void setVisible(bool visible)` - 设置可见性
- `void setAlpha(float alpha)` - 设置透明度
- `void setBackgroundColor(const ImVec4& color)` - 设置背景颜色

## 注意事项

1. **缓冲区大小**: 默认最大长度为 4096 字符，可通过 `setMaxLength()` 调整
2. **回车处理**: 在多行文本中，回车会插入换行符。如需在回车时执行操作，需在回调中检测 `\n`
3. **性能**: 对于大量文本，考虑使用分页或虚拟滚动来优化性能
4. **占位符**: 占位符仅在文本为空且控件未获得焦点时显示

## 常见问题

**Q: 如何禁用 Tab 键输入？**
A: 目前 TextEdit 默认允许 Tab 输入。如需禁用，需要修改 `ImGuiInputTextFlags`。

**Q: 如何滚动到底部？**
A: ImGui 的 `InputTextMultiline` 不直接支持此功能。可以考虑在追加文本后设置滚动位置。

**Q: 如何实现语法高亮？**
A: 需要使用 `ImDrawList` 手动绘制带颜色的文本，这超出了基本 TextEdit 的范围。
