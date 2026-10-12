#pragma once

#include <string>

// A 2D texture loaded from an image file and uploaded to OpenGL.
//
// Requires a current OpenGL context while loading and while being destroyed,
// so own it for no longer than the GLFW window it draws into.
class Texture
{
public:
    Texture() = default;
    ~Texture();

    // A texture owns a GL object, so it is not copyable.
    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    // Loads an image file (PNG, BMP, JPG, ...) via Windows Imaging Component.
    // `scale` > 0 downscales on load; a small value (e.g. 0.08) yields a tiny
    // texture that looks blurred when magnified. Returns false on failure.
    bool loadFromFile(const std::string &path, float scale = 1.0f);

    // OpenGL texture name (0 when not loaded). Cast to ImTextureID for ImGui.
    unsigned int id() const { return m_id; }
    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    void destroy();

    unsigned int m_id = 0;
    int m_width = 0;
    int m_height = 0;
};
