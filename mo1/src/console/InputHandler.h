#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

// Owns the console keyboard input and the drawing cursor, so commands can print
// without fighting the prompt for space. Everything between `resetLine` and
// `maxLines` behaves like a scrolling terminal region.
class InputHandler
{
public:
    using EnterHandler = std::function<void()>;

    // `x` and `y` are the live cursor coordinates (column, line), both 0-based.
    explicit InputHandler(int *x, int *y, EnterHandler onEnter = {}, int maxLines = 0, int resetLine = 0);

    ~InputHandler();

    InputHandler(const InputHandler &) = delete;
    InputHandler &operator=(const InputHandler &) = delete;

    // Must be called before start().
    void setEnterHandler(EnterHandler onEnter);

    void start();
    void stop();

    void setPosition(int *x, int *y);
    bool isRunning() const;

    // The line typed so far, NUL terminated.
    char *getInput();

    // Reserves space for the prompt (or for command output) before drawing.
    void preparePrompt();
    void prepareLines(int lineCount);

    // Draws at the cursor line and moves to the next line.
    void writeLine(const char *text);

    // Draws at the cursor line without moving past it (used for the prompt).
    void writeCurrentLine(const char *text);

    // Redraws the prompt line as `prompt` followed by whatever has been typed
    // so far, so live output can be printed without losing partial input.
    void redrawPrompt(const char *prompt);

    // Erases the command area and moves the cursor back to its top.
    void clearScreen();

    // Lines between the reserved banner and the bottom of the command area.
    int availableLines() const;

    void writeLineAt(int line, const char *text);
    void writeAt(int line, int column, const char *text);
    void writeWrapped(int line, int firstColumn, const char *text, int width);

private:
    void poll();
    void writeSymbol(char symbol);
    void erasePreviousSymbol();
    void appendInput(char symbol);
    void clearInput();
    void moveToNextLine();
    void prepareNextPrompt(int lineCount);
    void clearLines(int firstLine, int lineCount);
    bool hasPosition() const;
    COORD currentPosition() const;

    static constexpr int INPUT_CAPACITY = 256;

    HANDLE console_ = INVALID_HANDLE_VALUE;
    int *x_ = nullptr;
    int *y_ = nullptr;
    EnterHandler onEnter_;
    int max_lines_ = 0;
    int reset_line_ = 0;
    char input_[INPUT_CAPACITY] = {'\0'};
    int input_col_ = 0;
    std::atomic<bool> running_{false};
    std::mutex console_mutex_;
    std::thread poller_;
};
