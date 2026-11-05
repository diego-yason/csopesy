#include "console/InputHandler.h"

#include <conio.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <utility>

InputHandler::InputHandler(int *x, int *y, EnterHandler onEnter, int maxLines, int resetLine)
    : console_(GetStdHandle(STD_OUTPUT_HANDLE)),
      x_(x),
      y_(y),
      onEnter_(std::move(onEnter)),
      max_lines_(maxLines),
      reset_line_(resetLine)
{
}

InputHandler::~InputHandler()
{
    stop();
}

void InputHandler::setEnterHandler(EnterHandler onEnter)
{
    onEnter_ = std::move(onEnter);
}

void InputHandler::start()
{
    if (running_.exchange(true))
    {
        return;
    }

    poller_ = std::thread(&InputHandler::poll, this);
}

void InputHandler::stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    if (poller_.joinable())
    {
        poller_.join();
    }
}

void InputHandler::setPosition(int *x, int *y)
{
    x_ = x;
    y_ = y;
}

bool InputHandler::isRunning() const
{
    return running_;
}

char *InputHandler::getInput()
{
    return input_;
}

void InputHandler::preparePrompt()
{
    prepareLines(1);
}

void InputHandler::prepareLines(int lineCount)
{
    std::lock_guard<std::mutex> lock(console_mutex_);
    prepareNextPrompt(lineCount);
}

void InputHandler::writeLine(const char *text)
{
    if (!hasPosition() || console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);
    prepareNextPrompt(1);

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(console_, &info))
    {
        return;
    }

    clearLines(*y_, 1);
    SetConsoleCursorPosition(console_, {0, static_cast<SHORT>(*y_)});

    const int textLength = std::min(static_cast<int>(std::strlen(text)), static_cast<int>(info.dwSize.X));
    DWORD written = 0;
    WriteConsoleA(console_, text, static_cast<DWORD>(textLength), &written, nullptr);

    *x_ = 0;
    ++*y_;
    SetConsoleCursorPosition(console_, currentPosition());
}

void InputHandler::writeCurrentLine(const char *text)
{
    if (!hasPosition() || console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);
    prepareNextPrompt(1);

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(console_, &info))
    {
        return;
    }

    clearLines(*y_, 1);
    SetConsoleCursorPosition(console_, {0, static_cast<SHORT>(*y_)});

    const int textLength = std::min(static_cast<int>(std::strlen(text)), static_cast<int>(info.dwSize.X));
    DWORD written = 0;
    WriteConsoleA(console_, text, static_cast<DWORD>(textLength), &written, nullptr);
    *x_ = textLength;
    SetConsoleCursorPosition(console_, currentPosition());
}

void InputHandler::redrawPrompt(const char *prompt)
{
    if (!hasPosition() || console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);
    prepareNextPrompt(1);

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(console_, &info))
    {
        return;
    }

    clearLines(*y_, 1);
    SetConsoleCursorPosition(console_, {0, static_cast<SHORT>(*y_)});

    // Prompt plus whatever the user has typed so far; the input buffer is read
    // under the console lock, so a keystroke cannot land half-way through.
    const std::string text = std::string(prompt) + input_;
    const int textLength = std::min(static_cast<int>(text.size()), static_cast<int>(info.dwSize.X));
    DWORD written = 0;
    WriteConsoleA(console_, text.c_str(), static_cast<DWORD>(textLength), &written, nullptr);

    *x_ = textLength;
    SetConsoleCursorPosition(console_, currentPosition());
}

void InputHandler::clearScreen()
{
    if (console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(console_, &info))
    {
        return;
    }

    const int firstLine = reset_line_ > 0 ? reset_line_ : 0;
    const int lastLine = max_lines_ > 0 ? std::min(max_lines_, static_cast<int>(info.dwSize.Y)) - 1
                                        : static_cast<int>(info.dwSize.Y) - 1;
    if (lastLine < firstLine)
    {
        return;
    }

    clearLines(firstLine, lastLine - firstLine + 1);

    if (hasPosition())
    {
        *x_ = 0;
        *y_ = firstLine;
        SetConsoleCursorPosition(console_, currentPosition());
    }
}

int InputHandler::availableLines() const
{
    if (max_lines_ <= 0)
    {
        return 0;
    }

    const int usable = max_lines_ - reset_line_;
    return usable > 0 ? usable : 0;
}

void InputHandler::writeLineAt(int line, const char *text)
{
    if (console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(console_, &info) || line < 0 || line >= info.dwSize.Y)
    {
        return;
    }

    clearLines(line, 1);
    SetConsoleCursorPosition(console_, {0, static_cast<SHORT>(line)});

    const int textLength = std::min(static_cast<int>(std::strlen(text)), static_cast<int>(info.dwSize.X));
    DWORD written = 0;
    WriteConsoleA(console_, text, static_cast<DWORD>(textLength), &written, nullptr);
    SetConsoleCursorPosition(console_, currentPosition());
}

void InputHandler::writeAt(int line, int column, const char *text)
{
    if (console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(console_, &info) || line < 0 || line >= info.dwSize.Y)
    {
        return;
    }

    clearLines(line, 1);
    const int textLength = static_cast<int>(std::strlen(text));
    const int firstCharacter = std::max(0, -column);
    const int outputColumn = std::max(0, column);
    const int visibleLength = std::min(textLength - firstCharacter, static_cast<int>(info.dwSize.X) - outputColumn);

    if (visibleLength > 0)
    {
        SetConsoleCursorPosition(console_, {static_cast<SHORT>(outputColumn), static_cast<SHORT>(line)});
        DWORD written = 0;
        WriteConsoleA(console_, text + firstCharacter, static_cast<DWORD>(visibleLength), &written, nullptr);
    }

    SetConsoleCursorPosition(console_, currentPosition());
}

void InputHandler::writeWrapped(int line, int firstColumn, const char *text, int width)
{
    if (console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(console_, &info) || line < 0 || line >= info.dwSize.Y || info.dwSize.X == 0)
    {
        return;
    }

    clearLines(line, 1);
    const int drawWidth = std::min(width, static_cast<int>(info.dwSize.X));
    if (drawWidth <= 0)
    {
        return;
    }

    const int textLength = static_cast<int>(std::strlen(text));
    for (int index = 0; index < textLength; ++index)
    {
        int column = (firstColumn + index) % drawWidth;
        if (column < 0)
        {
            column += drawWidth;
        }

        SetConsoleCursorPosition(console_, {static_cast<SHORT>(column), static_cast<SHORT>(line)});
        DWORD written = 0;
        WriteConsoleA(console_, text + index, 1, &written, nullptr);
    }

    SetConsoleCursorPosition(console_, currentPosition());
}

void InputHandler::poll()
{
    while (running_)
    {
        if (!_kbhit())
        {
            Sleep(1);
            continue;
        }

        const int key = _getch();

        // Function and arrow keys produce a second code.
        if (key == 0 || key == 224)
        {
            _getch();
            continue;
        }

        if (key == '\b')
        {
            erasePreviousSymbol();
            continue;
        }

        if (key == '\r')
        {
            moveToNextLine();
            prepareLines(1);

            if (onEnter_)
            {
                onEnter_();
            }

            clearInput();

            continue;
        }

        if (key >= 32 && key <= 126)
        {
            writeSymbol(static_cast<char>(key));
        }
    }
}

void InputHandler::writeSymbol(char symbol)
{
    if (!hasPosition() || console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);
    SetConsoleCursorPosition(console_, currentPosition());
    DWORD written = 0;
    WriteConsoleA(console_, &symbol, 1, &written, nullptr);

    appendInput(symbol);
    ++*x_;
}

void InputHandler::erasePreviousSymbol()
{
    if (!hasPosition() || input_col_ == 0 || *x_ == 0 || console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);
    --*x_;
    SetConsoleCursorPosition(console_, currentPosition());

    const char blank = ' ';
    DWORD written = 0;
    WriteConsoleA(console_, &blank, 1, &written, nullptr);
    SetConsoleCursorPosition(console_, currentPosition());
    input_[--input_col_] = '\0';
}

void InputHandler::appendInput(char symbol)
{
    if (input_col_ < INPUT_CAPACITY - 1)
    {
        input_[input_col_++] = symbol;
        input_[input_col_] = '\0';
    }
}

void InputHandler::clearInput()
{
    input_col_ = 0;
    input_[0] = '\0';
}

void InputHandler::moveToNextLine()
{
    if (!hasPosition())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(console_mutex_);
    *x_ = 0;
    ++*y_;
    SetConsoleCursorPosition(console_, currentPosition());
}

void InputHandler::prepareNextPrompt(int lineCount)
{
    if (!hasPosition() || lineCount <= 0 || max_lines_ <= 0 || *y_ + lineCount <= max_lines_)
    {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (console_ == INVALID_HANDLE_VALUE || console_ == nullptr || !GetConsoleScreenBufferInfo(console_, &info))
    {
        return;
    }

    const int lastLine = std::min(max_lines_ - 1, static_cast<int>(info.dwSize.Y) - 1);
    const int commandHeight = lastLine - reset_line_ + 1;
    if (commandHeight <= 0)
    {
        return;
    }

    const int lastRequiredLine = *y_ + lineCount - 1;
    const int linesToScroll = std::min(lastRequiredLine - lastLine, commandHeight);
    if (linesToScroll >= commandHeight)
    {
        clearLines(reset_line_, commandHeight);
        *y_ = reset_line_;
    }
    else
    {
        SMALL_RECT source{
            0,
            static_cast<SHORT>(reset_line_ + linesToScroll),
            static_cast<SHORT>(info.dwSize.X - 1),
            static_cast<SHORT>(lastLine)};
        CHAR_INFO fill{' ', info.wAttributes};
        ScrollConsoleScreenBufferA(console_, &source, nullptr, {0, static_cast<SHORT>(reset_line_)}, &fill);
        *y_ -= linesToScroll;
    }

    SetConsoleCursorPosition(console_, currentPosition());
}

void InputHandler::clearLines(int firstLine, int lineCount)
{
    if (console_ == INVALID_HANDLE_VALUE || console_ == nullptr)
    {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(console_, &info))
    {
        return;
    }

    const int availableLines = info.dwSize.Y - firstLine;
    const int linesToClear = std::min(lineCount, availableLines);
    if (linesToClear <= 0)
    {
        return;
    }

    const COORD start{0, static_cast<SHORT>(firstLine)};
    const DWORD cellsToClear = static_cast<DWORD>(info.dwSize.X * linesToClear);
    DWORD cleared = 0;
    FillConsoleOutputCharacterA(console_, ' ', cellsToClear, start, &cleared);
    FillConsoleOutputAttribute(console_, info.wAttributes, cellsToClear, start, &cleared);
}

bool InputHandler::hasPosition() const
{
    return x_ != nullptr && y_ != nullptr;
}

COORD InputHandler::currentPosition() const
{
    return {static_cast<SHORT>(*x_), static_cast<SHORT>(*y_)};
}
