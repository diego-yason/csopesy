#pragma once

#include "Component/Button.h"
#include "Panel.h"

#include <string>
#include <vector>

// The bottom bar: a full-width strip pinned to the bottom that is always
// visible. It is a Panel, but it has no title bar and cannot be closed.
class BottomPanel : public Panel
{
public:
    BottomPanel();

    // Draws the bar in a fixed position along the bottom. The open/closed
    // state is ignored: the bar is always shown.
    void draw() override;

    // GL texture drawn behind the contents (a downscaled copy of the
    // background image, which reads as a blur when magnified).
    void setBackgroundTexture(unsigned int gl_id) { m_background_id = gl_id; }

    // Adds an entry to the button list. Buttons share a single row.
    void addButton(std::string label, Button::Action onClick);

    void render() override;

private:
    unsigned int m_background_id = 0;

    // Contents: laid out left-to-right in one row.
    std::vector<Button> m_buttons;
};
