#pragma once

#include <functional>
#include <string>
#include <utility>

// A clickable UI button. The button itself knows nothing about what it
// triggers: it just runs the Action passed to its constructor when clicked,
// which is how buttons pop up panels (or do anything else).
class Button
{
public:
    using Action = std::function<void()>;

    Button(std::string label, Action onClick);

    // Draws the button. Returns true if it was clicked this frame.
    // Pass width/height to size it (e.g. stretch to the parent's width);
    // 0.0f on an axis means "automatic".
    bool draw(float width = 0.0f, float height = 0.0f);

    const std::string &label() const { return m_label; }
    void setLabel(std::string label) { m_label = std::move(label); }

private:
    std::string m_label;
    Action m_onClick;
};
