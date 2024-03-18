#ifndef TEXTURE_H
#define TEXTURE_H

#include "stdfax.h"

class Texture
{
private:
    ID3D12Resource* m_pTextureBuffer{ nullptr };
    ID3D12DescriptorHeap* m_pMainDescriptorHeap{ nullptr };
    ID3D12Resource* m_pTextureBufferUploadHeap{ nullptr };

    D3D12_RESOURCE_DESC m_TextureDescriptor;
    int m_ImageBytesPerRow{};
    BYTE* m_pImageData;
    int m_ImageSize{};

    bool LoadImageDataFromFIle(LPCWSTR fileName)
    {
        static IWICImagingFactory* pWicFactory;

        HRESULT hr{};

        IWICBitmapDecoder* pWicDecoder{ NULL };
        IWICBitmapFrameDecode* pWicFrame{ NULL };
        IWICFormatConverter* pWicConverter{ NULL };

        bool imageConverted{ false };

        if (pWicFactory == NULL)
        {
            CoInitialize(NULL);

            hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&pWicFactory)
            );

            if (FAILED(hr))
            {
                return 0;
            }
        }

        hr = pWicFactory->CreateDecoderFromFilename(
            fileName,
            NULL,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &pWicDecoder
        );

        if (FAILED(hr))
        {
            return 0;
        }

        // Getting image from decoder (decoding the frame).
        hr = pWicDecoder->GetFrame(0, &pWicFrame);
        if (FAILED(hr))
        {
            return 0;
        }

        // Getting wic pixel format of image.
        WICPixelFormatGUID pixelFormat{};
        hr = pWicFrame->GetPixelFormat(&pixelFormat);
        if (FAILED(hr))
        {
            return 0;
        }

        // Getting szie of image.
        UINT textureWidth{};
        UINT textureHeight{};
        hr = pWicFrame->GetSize(&textureWidth, &textureHeight);
        if (FAILED(hr))
        {
            return 0;
        }

        // Converting wic pixel format to dxgi pixel format.
        DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

        if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
        {
            WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

            if (convertToPixelFormat == GUID_WICPixelFormatDontCare)
            {
                return 0;
            }

            hr = pWicFactory->CreateFormatConverter(&pWicConverter);

            if (FAILED(hr))
            {
                return 0;
            }

            BOOL canConvert = FALSE;
            hr = pWicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);

            if (FAILED(hr) || !canConvert)
            {
                return 0;
            }

            hr = pWicConverter->Initialize(pWicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);

            imageConverted = true;

            dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);
        }

        int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat);
        m_ImageBytesPerRow = (textureWidth * bitsPerPixel) / 8;
        m_ImageSize = m_ImageBytesPerRow * textureHeight;

        m_pImageData = (BYTE*)malloc(m_ImageSize);

        m_TextureDescriptor = {};
        m_TextureDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        m_TextureDescriptor.Alignment = 0; // May be 0, 4 KB, 64 KB or 4 MB. 0 will let runtime decide between 64 KB and 4 MB (4 MB for multi-sampled textures).
        m_TextureDescriptor.Width = textureWidth;
        m_TextureDescriptor.Height = textureHeight;
        m_TextureDescriptor.DepthOrArraySize = 1;
        m_TextureDescriptor.MipLevels = 1;
        m_TextureDescriptor.Format = dxgiFormat;
        m_TextureDescriptor.SampleDesc.Count = 1;
        m_TextureDescriptor.SampleDesc.Quality = 0;
        m_TextureDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        m_TextureDescriptor.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (imageConverted)
        {
            hr = pWicConverter->CopyPixels(0, m_ImageBytesPerRow, m_ImageSize, m_pImageData);
            if (FAILED(hr))
            {
                return 0;
            }
        }
        else
        {
            hr = pWicFrame->CopyPixels(0, m_ImageBytesPerRow, m_ImageSize, m_pImageData);
            if (FAILED(hr))
            {
                return 0;
            }
        }

        return true;
    }
public:
	
    void Use(ID3D12GraphicsCommandList& pGraphicsCommandList)
    {
        ID3D12DescriptorHeap* descriptorHeaps[] = { m_pMainDescriptorHeap };
        pGraphicsCommandList.SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        pGraphicsCommandList.SetGraphicsRootDescriptorTable(1, m_pMainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    }
   
    bool Load(ID3D12Device& pDevice, ID3D12GraphicsCommandList& pGraphicsCommandList, LPCWSTR pathToImage)
    {
        LoadImageDataFromFIle(pathToImage);

        HRESULT hr{};

        hr = pDevice.CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &m_TextureDescriptor,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_pTextureBuffer)
        );

        if (FAILED(hr))
        {
            return false;
        }

        m_pTextureBuffer->SetName(L"Texture Buffer Resource Heap");

        UINT64 textureUploadBufferSize{};

        pDevice.GetCopyableFootprints(&m_TextureDescriptor, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
    
        hr = pDevice.CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_pTextureBufferUploadHeap)
        );

        if (FAILED(hr))
        {
            return false;
        }

        m_pTextureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

        D3D12_SUBRESOURCE_DATA textureData{};
        textureData.pData = &m_pImageData[0];
        textureData.RowPitch = m_ImageBytesPerRow;
        textureData.SlicePitch = m_ImageBytesPerRow * m_TextureDescriptor.Height;

        UpdateSubresources(&pGraphicsCommandList, m_pTextureBuffer, m_pTextureBufferUploadHeap, 0, 0, 1, &textureData);

        pGraphicsCommandList.ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTextureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hr = pDevice.CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pMainDescriptorHeap));

        if (FAILED(hr))
        {
            return false;
        }
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
        srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDescriptor.Format = m_TextureDescriptor.Format;
        srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDescriptor.Texture2D.MipLevels = 1;
        pDevice.CreateShaderResourceView(m_pTextureBuffer, &srvDescriptor, m_pMainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        return true;
    }

    static DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
    {
        if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat)         return DXGI_FORMAT_R32G32B32A32_FLOAT;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf)      return DXGI_FORMAT_R16G16B16A16_FLOAT;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA)          return DXGI_FORMAT_R16G16B16A16_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA)          return DXGI_FORMAT_R8G8B8A8_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA)          return DXGI_FORMAT_B8G8R8A8_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR)           return DXGI_FORMAT_B8G8R8X8_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102)   return DXGI_FORMAT_R10G10B10A2_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551)      return DXGI_FORMAT_B5G5R5A1_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565)        return DXGI_FORMAT_B5G6R5_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat)     return DXGI_FORMAT_R32_FLOAT;
        else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf)      return DXGI_FORMAT_R16_FLOAT;
        else if (wicFormatGUID == GUID_WICPixelFormat16bppGray)          return DXGI_FORMAT_R16_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat8bppGray)           return DXGI_FORMAT_R8_UNORM;
        else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha)          return DXGI_FORMAT_A8_UNORM;
        else                                                             return DXGI_FORMAT_UNKNOWN;
    }

    static WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID)
    {
        if (wicFormatGUID == GUID_WICPixelFormatBlackWhite)                 return GUID_WICPixelFormat8bppGray;
        else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed)           return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed)           return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed)           return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed)           return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat2bppGray)              return GUID_WICPixelFormat8bppGray;
        else if (wicFormatGUID == GUID_WICPixelFormat4bppGray)              return GUID_WICPixelFormat8bppGray;
        else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint)   return GUID_WICPixelFormat16bppGrayHalf;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint)   return GUID_WICPixelFormat32bppGrayFloat;
        else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555)           return GUID_WICPixelFormat16bppBGRA5551;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010)        return GUID_WICPixelFormat32bppRGBA1010102;
        else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR)              return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB)              return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA)            return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA)            return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB)              return GUID_WICPixelFormat64bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR)              return GUID_WICPixelFormat64bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA)             return GUID_WICPixelFormat64bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA)            return GUID_WICPixelFormat64bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA)            return GUID_WICPixelFormat64bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint)    return GUID_WICPixelFormat64bppRGBAHalf;
        else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint)    return GUID_WICPixelFormat64bppRGBAHalf;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint)   return GUID_WICPixelFormat64bppRGBAHalf;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint)   return GUID_WICPixelFormat64bppRGBAHalf;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint)    return GUID_WICPixelFormat64bppRGBAHalf;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf)          return GUID_WICPixelFormat64bppRGBAHalf;
        else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf)          return GUID_WICPixelFormat64bppRGBAHalf;
        else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat)      return GUID_WICPixelFormat128bppRGBAFloat;
        else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat)        return GUID_WICPixelFormat128bppRGBAFloat;
        else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint)  return GUID_WICPixelFormat128bppRGBAFloat;
        else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint)   return GUID_WICPixelFormat128bppRGBAFloat;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE)             return GUID_WICPixelFormat128bppRGBAFloat;
        else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK)             return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK)             return GUID_WICPixelFormat64bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha)        return GUID_WICPixelFormat64bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha)        return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
        else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB)              return GUID_WICPixelFormat32bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB)              return GUID_WICPixelFormat64bppRGBA;
        else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf)        return GUID_WICPixelFormat64bppRGBAHalf;
#endif
        else return GUID_WICPixelFormatDontCare;
    }

    static int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
    {
        if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)              return 128;
        else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT)         return 64;
        else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM)         return 64;
        else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM)             return 32;
        else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM)             return 32;
        else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM)             return 32;
        else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;
        else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM)          return 32;
        else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM)             return 16;
        else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM)               return 16;
        else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT)                  return 32;
        else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT)                  return 16;
        else if (dxgiFormat == DXGI_FORMAT_R16_UNORM)                  return 16;
        else if (dxgiFormat == DXGI_FORMAT_R8_UNORM)                   return 8;
        else if (dxgiFormat == DXGI_FORMAT_A8_UNORM)                   return 8;
    }
};
#endif