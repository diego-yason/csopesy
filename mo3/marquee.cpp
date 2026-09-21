#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include "input.cpp"

const int RESERVED_LINES = 13; // first command line is at 14
const int MAX_LINES = 24;
const int WIDTH = 80;
const int TEXT_WIDTH = 60;
const int MARQUEE_LINE = RESERVED_LINES - 4;

std::atomic<bool> marquee_active = false;
char marquee_text[TEXT_WIDTH + 1] = "Hello, World!";
std::mutex marquee_text_mutex;
std::atomic<int> marquee_speed = 50; // frames/second
std::atomic<int> marquee_pos = WIDTH - 1;
std::thread marquee_thread;

int cur_cmd_line = RESERVED_LINES;
int cur_cmd_col = 9; // column immediately after "Command> "
char *input = nullptr;
InputHandler *activeInputHandler = nullptr;

void new_command(const char cmd[]);

void print_prompt()
{
    std::cout.flush();

    if (activeInputHandler != nullptr)
    {
        activeInputHandler->preparePrompt();
    }

    if (activeInputHandler != nullptr)
    {
        activeInputHandler->writeCurrentLine("Command> ");
        cur_cmd_col = 9;
        return;
    }

    std::cout << "Command> ";
    cur_cmd_col = 9;
}

void onEnter()
{
    new_command(input);
}

int main()
{

    // clear console
    std::cout << "\033[H\033[2J" << std::flush;

    // open intro
    std::cout << "Welcome to CSOPESY MO3!" << std::endl;
    std::cout << "Group Developers:" << std::endl;
    std::cout << "Yason, Diego David (12308978)" << std::endl;
    std::cout << std::endl;
    std::cout << "Version Date: 2026-09-21" << std::endl;
    std::cout << std::endl;

    std::cout << std::endl
              << std::endl
              << std::endl;
    std::cout << marquee_text << std::endl;
    std::cout << std::endl
              << std::endl
              << std::endl;

    // InputHandler takes horizontal then vertical coordinates.
    InputHandler inputHandler(&cur_cmd_col, &cur_cmd_line, onEnter, MAX_LINES, RESERVED_LINES);
    activeInputHandler = &inputHandler;
    input = inputHandler.getInput();

    print_prompt();
    inputHandler.start();

    while (inputHandler.isRunning())
    {
        Sleep(1);
    }

    return 0;
}

void help()
{
    if (activeInputHandler != nullptr)
    {
        activeInputHandler->prepareLines(8);

        activeInputHandler->writeLine("Available commands:");
        activeInputHandler->writeLine("help - displays the commands and its descriptions");
        activeInputHandler->writeLine("start_marquee - starts the marquee \"animation\"");
        activeInputHandler->writeLine("stop_marquee - stops the marquee \"animation\"");
        activeInputHandler->writeLine("set_text - accepts a text input and displays it as a marquee");
        activeInputHandler->writeLine("set_speed - accepts a positive speed in Hz");
        activeInputHandler->writeLine("exit - exits the program");
        return;
    }

    std::cout << "Available commands:" << std::endl;
    std::cout << "help - displays the commands and its descriptions" << std::endl;
    std::cout << "start_marquee - starts the marquee \"animation\"" << std::endl;
    std::cout << "stop_marquee - stops the marquee \"animation\"" << std::endl;
    std::cout << "set_text - accepts a text input and displays it as a marquee" << std::endl;
    std::cout << "set_speed - accepts a positive speed in Hz" << std::endl;
    std::cout << "exit - exits the program" << std::endl;
}

void set_text(const std::string &text)
{
    if (text.size() > TEXT_WIDTH)
    {
        if (activeInputHandler != nullptr)
        {
            activeInputHandler->writeLine("Text exceeds the maximum length.");
        }

        return;
    }

    std::lock_guard<std::mutex> lock(marquee_text_mutex);
    std::strncpy(marquee_text, text.c_str(), TEXT_WIDTH);
    marquee_text[TEXT_WIDTH] = '\0';

    if (activeInputHandler != nullptr)
    {
        int textPosition = marquee_pos.load() + 1;
        if (textPosition >= WIDTH)
        {
            textPosition = 0;
        }

        marquee_pos.store(textPosition);
        activeInputHandler->writeWrapped(MARQUEE_LINE, textPosition, marquee_text, WIDTH);
    }
    if (activeInputHandler != nullptr)
    {
        activeInputHandler->writeLine("Marquee text updated.");
    }
}

void set_speed(const std::string &text)
{
    char *end = nullptr;
    const long parsedSpeed = std::strtol(text.c_str(), &end, 10);
    while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)))
    {
        end++;
    }

    if (text.empty() || end == text.c_str() || *end != '\0' || parsedSpeed <= 0 || parsedSpeed > (std::numeric_limits<int>::max)())
    {
        if (activeInputHandler != nullptr)
        {
            activeInputHandler->writeLine("Invalid marquee speed.");
        }

        return;
    }

    marquee_speed.store(static_cast<int>(parsedSpeed));
    if (activeInputHandler != nullptr)
    {
        activeInputHandler->writeLine("Marquee speed updated.");
    }
}

void marquee()
{
    if (marquee_active.exchange(true))
    {
        return;
    }

    if (marquee_thread.joinable())
    {
        marquee_thread.join();
    }

    marquee_thread = std::thread([]()
                                 {
        while (marquee_active)
        {
            std::string text;
            {
                std::lock_guard<std::mutex> lock(marquee_text_mutex);
                text = marquee_text;
            }

            if (activeInputHandler != nullptr)
            {
                activeInputHandler->writeWrapped(MARQUEE_LINE, marquee_pos.load(), text.c_str(), WIDTH);
            }

            --marquee_pos;
            if (marquee_pos < 0)
            {
                marquee_pos = WIDTH - 1;
            }

            const int frameDelay = max(1, 1000 / max(1, marquee_speed.load()));
            Sleep(frameDelay);
        } });
}

void stop_marquee()
{
    marquee_active.store(false);

    if (marquee_thread.joinable())
    {
        marquee_thread.join();
    }
}

void new_command(const char cmd[])
{
    const std::string command(cmd);
    const std::size_t commandStart = command.find_first_not_of(" \t\r\n");
    if (commandStart == std::string::npos)
    {
        print_prompt();
        return;
    }

    const std::size_t commandEnd = command.find_first_of(" \t\r\n", commandStart);
    const std::string commandName = command.substr(commandStart, commandEnd - commandStart);
    std::string arguments;
    if (commandEnd != std::string::npos)
    {
        arguments = command.substr(commandEnd + 1);
        const std::size_t argumentStart = arguments.find_first_not_of(" \t\r\n");
        if (argumentStart == std::string::npos)
        {
            arguments.clear();
        }
        else
        {
            arguments.erase(0, argumentStart);
        }
    }

    if (commandName == "help")
    {
        help();
    }

    else if (commandName == "set_text")
    {
        set_text(arguments);
    }

    else if (commandName == "set_speed")
    {
        set_speed(arguments);
    }

    else if (commandName == "start_marquee")
    {
        marquee();
        if (activeInputHandler != nullptr)
        {
            activeInputHandler->writeLine("Marquee started.");
        }
    }

    else if (commandName == "stop_marquee")
    {
        stop_marquee();
        if (activeInputHandler != nullptr)
        {
            activeInputHandler->writeLine("Marquee stopped.");
        }
    }

    else if (commandName == "exit")
    {
        if (activeInputHandler != nullptr)
        {
            activeInputHandler->writeLine("Thank you! Goodbye!");
        }

        exit(0);
    }
    else
    {
        if (activeInputHandler != nullptr)
        {
            activeInputHandler->writeLine("Unknown command.");
        }
        else
        {
            std::cout << "Unknown command." << std::endl;
            cur_cmd_line++;
        }
    }

    print_prompt();
}

void process_line(char line[])
{
}