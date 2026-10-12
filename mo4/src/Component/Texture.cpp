#include "Texture.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <GLFW/glfw3.h> // Brings in <GL/gl.h>

#include <vector>

#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F // OpenGL 1.2; not in the Windows <GL/gl.h>
#endif

namespace
{
    std::wstring utf8ToWide(const std::string &text)
    {
        if (text.empty())
            return {};

        const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        std::wstring wide(static_cast<size_t>(length), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), length);
        wide.resize(static_cast<size_t>(length) - 1); // Drop the trailing null
        return wide;
    }
}

Texture::~Texture()
{
    destroy();
}

void Texture::destroy()
{
    if (m_id != 0)
    {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
    m_width = 0;
    m_height = 0;
}

bool Texture::loadFromFile(const std::string &path, float scale)
{
    destroy();

    const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool com_owned = SUCCEEDED(com_result);

    std::vector<unsigned char> pixels;
    UINT width = 0;
    UINT height = 0;

    {
        ComPtr<IWICImagingFactory> factory;
        ComPtr<IWICBitmapDecoder> decoder;
        ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICFormatConverter> converter;
        ComPtr<IWICBitmapScaler> scaler;
        ComPtr<IWICBitmapSource> source;

        const std::wstring wide_path = utf8ToWide(path);

        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&factory));
        if (SUCCEEDED(hr))
            hr = factory->CreateDecoderFromFilename(wide_path.c_str(), nullptr, GENERIC_READ,
                                                    WICDecodeMetadataCacheOnLoad, &decoder);
        if (SUCCEEDED(hr))
            hr = decoder->GetFrame(0, &frame);
        if (SUCCEEDED(hr))
            hr = factory->CreateFormatConverter(&converter);
        if (SUCCEEDED(hr))
            hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                                       WICBitmapDitherTypeNone, nullptr, 0.0,
                                       WICBitmapPaletteTypeCustom);
        if (SUCCEEDED(hr))
            hr = converter.As(&source);

        // Optionally downscale, so the texture looks soft when magnified.
        if (SUCCEEDED(hr) && scale < 1.0f)
        {
            hr = source->GetSize(&width, &height);
            if (SUCCEEDED(hr))
                hr = factory->CreateBitmapScaler(&scaler);
            if (SUCCEEDED(hr))
            {
                const UINT scaled_width = width * scale > 1.0f ? static_cast<UINT>(width * scale) : 1;
                const UINT scaled_height = height * scale > 1.0f ? static_cast<UINT>(height * scale) : 1;
                hr = scaler->Initialize(source.Get(), scaled_width, scaled_height,
                                        WICBitmapInterpolationModeFant);
            }
            if (SUCCEEDED(hr))
                hr = scaler.As(&source);
        }

        if (SUCCEEDED(hr))
            hr = source->GetSize(&width, &height);
        if (SUCCEEDED(hr) && width > 0 && height > 0)
        {
            const UINT stride = width * 4;
            pixels.resize(static_cast<size_t>(stride) * height);
            hr = source->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.size()), pixels.data());
        }

        if (FAILED(hr) || pixels.empty())
        {
            if (com_owned)
                CoUninitialize();
            return false;
        }
    }

    if (com_owned)
        CoUninitialize();

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_id = texture;
    m_width = static_cast<int>(width);
    m_height = static_cast<int>(height);
    return true;
}
