#pragma once

#include <string>
#include <utility>

// Base class for every UI panel in the application.
//
// Derive from Panel and override render() to draw the panel's contents.
// Call draw() to place the panel inside its own ImGui window, or call
// render() directly to embed the contents elsewhere.
class Panel
{
public:
    // `key` identifies the panel (used to find/toggle it) and is independent of
    // `title`, which is the name shown in the window's title bar.
    explicit Panel(std::string key, std::string title, bool open = true);
    virtual ~Panel() = default;

    // A Panel is a unique, stateful object; hold it via a pointer.
    Panel(const Panel &) = delete;
    Panel &operator=(const Panel &) = delete;

    // Draws the panel's contents. Called once per frame, between
    // ImGui::NewFrame() and ImGui::Render().
    virtual void render() = 0;

    // Renders the panel inside its own ImGui window, titled after the panel.
    // Virtual so a panel can customise how/where it is presented.
    virtual void draw();

    // Stable identity, e.g. the bottom bar button that opens this panel.
    const std::string &key() const { return m_key; }

    const std::string &title() const { return m_title; }
    void setTitle(std::string title) { m_title = std::move(title); }

    bool isOpen() const { return m_open; }
    void setOpen(bool open) { m_open = open; }

private:
    std::string m_key;
    std::string m_title;
    bool m_open;
};
