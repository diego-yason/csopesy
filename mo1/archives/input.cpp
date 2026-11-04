#include <conio.h>
#include <windows.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

class InputHandler
{
public:
    using EnterHandler = std::function<void()>;

    InputHandler(int *x, int *y, EnterHandler onEnter = {}, int maxLines = 0, int resetLine = 0)
        : x_(x), y_(y), onEnter_(std::move(onEnter)), max_lines_(maxLines), reset_line_(resetLine), running_(false)
    {
        console_ = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    ~InputHandler()
    {
        stop();
    }

    InputHandler(const InputHandler &) = delete;
    InputHandler &operator=(const InputHandler &) = delete;

    void start()
    {
        if (running_.exchange(true))
        {
            return;
        }

        poller_ = std::thread(&InputHandler::poll, this);
    }

    void stop()
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

    void setPosition(int *x, int *y)
    {
        x_ = x;
        y_ = y;
    }

    bool isRunning() const
    {
        return running_;
    }

    char *getInput()
    {
        return input_;
    }

    void preparePrompt()
    {
        prepareLines(1);
    }

    void prepareLines(int lineCount)
    {
        std::lock_guard<std::mutex> lock(console_mutex_);
        prepareNextPrompt(lineCount);
    }

    void writeLine(const char *text)
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

        const int textLength = min(static_cast<int>(std::strlen(text)), static_cast<int>(info.dwSize.X));
        DWORD written = 0;
        WriteConsoleA(console_, text, static_cast<DWORD>(textLength), &written, nullptr);

        *x_ = 0;
        ++*y_;
        SetConsoleCursorPosition(console_, currentPosition());
    }

    void writeCurrentLine(const char *text)
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

        const int textLength = min(static_cast<int>(std::strlen(text)), static_cast<int>(info.dwSize.X));
        DWORD written = 0;
        WriteConsoleA(console_, text, static_cast<DWORD>(textLength), &written, nullptr);
        *x_ = textLength;
        SetConsoleCursorPosition(console_, currentPosition());
    }

    void writeLineAt(int line, const char *text)
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

        const int textLength = min(static_cast<int>(std::strlen(text)), static_cast<int>(info.dwSize.X));
        DWORD written = 0;
        WriteConsoleA(console_, text, static_cast<DWORD>(textLength), &written, nullptr);
        SetConsoleCursorPosition(console_, currentPosition());
    }

    void writeAt(int line, int column, const char *text)
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
        const int firstCharacter = max(0, -column);
        const int outputColumn = max(0, column);
        const int visibleLength = min(textLength - firstCharacter, static_cast<int>(info.dwSize.X) - outputColumn);

        if (visibleLength > 0)
        {
            SetConsoleCursorPosition(console_, {static_cast<SHORT>(outputColumn), static_cast<SHORT>(line)});
            DWORD written = 0;
            WriteConsoleA(console_, text + firstCharacter, static_cast<DWORD>(visibleLength), &written, nullptr);
        }

        SetConsoleCursorPosition(console_, currentPosition());
    }

    void writeWrapped(int line, int firstColumn, const char *text, int width)
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
        const int drawWidth = min(width, static_cast<int>(info.dwSize.X));
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

private:
    void poll()
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

    void writeSymbol(char symbol)
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

    void erasePreviousSymbol()
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

    void appendInput(char symbol)
    {
        if (input_col_ < INPUT_CAPACITY - 1)
        {
            input_[input_col_++] = symbol;
            input_[input_col_] = '\0';
        }
    }

    void clearInput()
    {
        input_col_ = 0;
        input_[0] = '\0';
    }

    void moveToNextLine()
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

    void prepareNextPrompt(int lineCount)
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

        const int lastLine = min(max_lines_ - 1, static_cast<int>(info.dwSize.Y) - 1);
        const int commandHeight = lastLine - reset_line_ + 1;
        if (commandHeight <= 0)
        {
            return;
        }

        const int lastRequiredLine = *y_ + lineCount - 1;
        const int linesToScroll = min(lastRequiredLine - lastLine, commandHeight);
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

    void clearLines(int firstLine, int lineCount)
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
        const int linesToClear = min(lineCount, availableLines);
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

    bool hasPosition() const
    {
        return x_ != nullptr && y_ != nullptr;
    }

    COORD currentPosition() const
    {
        return {static_cast<SHORT>(*x_), static_cast<SHORT>(*y_)};
    }

    HANDLE console_ = INVALID_HANDLE_VALUE;
    int *x_ = nullptr;
    int *y_ = nullptr;
    EnterHandler onEnter_;
    int max_lines_ = 0;
    int reset_line_ = 0;
    static constexpr int INPUT_CAPACITY = 256;
    char input_[INPUT_CAPACITY] = {'\0'};
    int input_col_ = 0;
    std::atomic<bool> running_;
    std::mutex console_mutex_;
    std::thread poller_;
};