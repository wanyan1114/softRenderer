#include "resource/loaders/ImageTextureLoader.h"

#include "render/Color.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace sr::resource {
namespace {

using Microsoft::WRL::ComPtr;

class ScopedComInitialization {
public:
    ScopedComInitialization()
    {
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        m_ShouldUninitialize = hr == S_OK || hr == S_FALSE;
        m_Ok = m_ShouldUninitialize || hr == RPC_E_CHANGED_MODE;
    }

    ~ScopedComInitialization()
    {
        if (m_ShouldUninitialize) {
            CoUninitialize();
        }
    }

    bool Ok() const { return m_Ok; }

private:
    bool m_ShouldUninitialize = false;
    bool m_Ok = false;
};

std::string NarrowPath(const std::filesystem::path& path)
{
    return path.string();
}

std::string LowercaseExtension(const std::filesystem::path& path)
{
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return extension;
}

} // namespace

bool IsSupportedBaseColorTexturePath(const std::filesystem::path& path)
{
    const std::string extension = LowercaseExtension(path);
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg";
}

ImageTextureLoadResult LoadImageTexture2D(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        return { {}, "Texture file does not exist: " + NarrowPath(path) };
    }

    if (!IsSupportedBaseColorTexturePath(path)) {
        return { {}, "Unsupported texture file extension for: " + NarrowPath(path) };
    }

    ScopedComInitialization comInitialization;
    if (!comInitialization.Ok()) {
        return { {}, "Failed to initialize COM while loading texture: " + NarrowPath(path) };
    }

    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return { {}, "Failed to create WIC imaging factory for: " + NarrowPath(path) };
    }

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromFilename(
        path.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &decoder);
    if (FAILED(hr)) {
        return { {}, "Failed to decode texture file: " + NarrowPath(path) };
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        return { {}, "Failed to read first image frame from: " + NarrowPath(path) };
    }

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr)) {
        return { {}, "Failed to create WIC format converter for: " + NarrowPath(path) };
    }

    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        return { {}, "Failed to convert texture pixels to RGBA for: " + NarrowPath(path) };
    }

    UINT width = 0;
    UINT height = 0;
    hr = converter->GetSize(&width, &height);
    if (FAILED(hr) || width == 0 || height == 0) {
        return { {}, "Texture has invalid size: " + NarrowPath(path) };
    }

    const UINT stride = width * 4;
    std::vector<std::uint8_t> rgbaPixels(static_cast<std::size_t>(stride) * static_cast<std::size_t>(height));
    hr = converter->CopyPixels(
        nullptr,
        stride,
        static_cast<UINT>(rgbaPixels.size()),
        rgbaPixels.data());
    if (FAILED(hr)) {
        return { {}, "Failed to copy decoded texture pixels for: " + NarrowPath(path) };
    }

    std::vector<render::Color> pixels;
    pixels.reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            const std::size_t offset = static_cast<std::size_t>(y) * static_cast<std::size_t>(stride)
                + static_cast<std::size_t>(x) * 4U;
            pixels.emplace_back(
                rgbaPixels[offset + 0],
                rgbaPixels[offset + 1],
                rgbaPixels[offset + 2],
                rgbaPixels[offset + 3]);
        }
    }

    return {
        render::Texture2D(static_cast<int>(width), static_cast<int>(height), std::move(pixels)),
        {}
    };
}

} // namespace sr::resource



