#ifndef D3D12APPXESS_H
#define D3D12APPXESS_H

#include "stdfax.h"
#include "FPSCounter.h"
#include "xess_d3d12.h"
#include "Vertex.h"

class D3D12AppXeSS
{
    float GetCorput(std::uint32_t index, std::uint32_t base)
    {
        float result = 0;
        float bk = 1.f;

        while (index > 0)
        {
            bk /= (float)base;
            result += (float)(index % base) * bk;
            index /= base;
        }

        return result;
    }
    std::vector<std::pair<float, float>> GenerateHalton(std::uint32_t base1, std::uint32_t base2, std::uint32_t start_index, std::uint32_t count, float offset1 = -0.5f, float offset2 = -0.5f)
    {
        std::vector<std::pair<float, float>> result;
        result.reserve(count);

        for (std::uint32_t a = start_index; a < count + start_index; ++a)
        {
            result.emplace_back(GetCorput(a, base1) + offset1, GetCorput(a, base2) + offset2);
        }
        return result;
    }

    const static UINT DESCRIPTORS_PER_FRAME{ 5 };
    const static UINT FRAME_COUNT{ 2 };
    const static UINT APP_DESCRIPTOR_COUNT{ 64 };
    const static UINT RT_COUNT{ 3 };

    enum DescriptorHeapIndices
    {
        DHI_Scene = 0,
        DHI_Color = 1,
        DHI_Velocity = 2,
        DHI_Depth = 3,
        DHI_XeSSOutputUAV = 4,
        DHI_XeSSOutputSRV = 5
    };

    enum RTIndices
    {
        RT_Present = 0,
        RT_Color = 1,
        RT_Velocity = 2,
        RT_Depth = 3,
    };

    struct SceneConstantBuffer
    {
        DirectX::XMFLOAT4 offset{};     //  4 * 4 [B] =  16 [B]
        DirectX::XMFLOAT4 velocity{};   //  4 * 4 [B] =  16 [B]
        DirectX::XMFLOAT4 resolution{}; //  4 * 4 [B] =  16 [B]
        float padding[52];              // 52 * 4 [B] = 208 [B]
    };                                  // Total  = 3 * 16 [B] + 208 [B] = 48 [B] + 208 [B] = 256 [B]

    UINT                                    m_ViewPortWidth{};
    UINT                                    m_ViewPortHeight{};
    float                                   m_AspectRatio{};
    CD3DX12_VIEWPORT                        m_ViewPort{};
    CD3DX12_RECT                            m_ScissorRect{};
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain{};
    Microsoft::WRL::ComPtr<ID3D12Device>    m_Device{};
    Microsoft::WRL::ComPtr<IDXGIFactory4>   m_DXGIFactory{};
    DXGI_FORMAT                             m_DSVFormat{};
    DXGI_FORMAT                             m_DSVTypedFormat{};
    FPSCounter                              m_FPSCounter{};

    // RESOURCES:
    Microsoft::WRL::ComPtr<ID3D12Resource>  m_RenderTargets[FRAME_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Resource>  m_DepthTargets[FRAME_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Resource>  m_RenderTargetsVelocity[FRAME_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Resource>  m_PresentRenderTargets[FRAME_COUNT];

    // DESCRIPTOR HEAPS:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>   m_RenderTargetsViewsDescriptorHeap{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>   m_DepthStencilViewsDescriptorHeap{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>   m_AppDescriptorHeap{};

    // COMMAND LISTS / QUEUES
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_CommandAllocator{};
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_GraphicsCommandList{};
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>        m_CommandQueue{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       m_RootSignature{};

    // PIPELINE STATES:
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_PipelineStateColorPass{};
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_PipelineStateVelocityPass{};

    // APP RESOURCES:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer{};
    D3D12_VERTEX_BUFFER_VIEW               m_VertexBufferView{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ConstantBuffer{};
    SceneConstantBuffer                    m_ConstantBufferData{};
    UINT8* m_pCbvDataBegin;


    // DATA:
    std::uint32_t m_OutputIndex{ DHI_XeSSOutputSRV };
    UINT m_RTVDescriptorSize{};
    UINT m_DSVDescriptorSize{};
    UINT m_UAVDescriptorSize{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignatureFSQ{};
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineStateFSQPass{};
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_SamplerFSQ{};
    const float m_ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // SYNCHRONIZATION OBJECTS:
    UINT                                m_FrameIndex{};
    HANDLE                              m_FenceEvent{};
    Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence{};
    UINT64                              m_FenceValue{};
    bool                                m_Pause{ false };

    // JITTER:
    std::vector<std::pair<float, float>>  m_HaltonPointSet{};
    std::size_t                           m_HaltonIndex{};
    float jitter[2];

    // XeSS:
    xess_context_handle_t                  m_XessContext{ nullptr };
    xess_2d_t                              m_DesiredOutputResolution{};
    xess_2d_t                              m_InputRenderResolution{};
    Microsoft::WRL::ComPtr<ID3D12Heap>     m_TexturesHeap{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_XessOutput[FRAME_COUNT];
    std::chrono::time_point<std::chrono::high_resolution_clock> m_LastTime{};
    const xess_quality_settings_t m_Quality{ XESS_QUALITY_SETTING_PERFORMANCE };

    HWND m_Hwnd{};

public:
    // Custom constructor.
    D3D12AppXeSS(UINT width, UINT height)
    {
        m_ViewPortWidth = width;
        m_ViewPortHeight = height;
        m_ViewPort = CD3DX12_VIEWPORT(0.0f, 0.0f, m_ViewPortWidth, m_ViewPortHeight);
        m_ScissorRect = CD3DX12_RECT(0, 0, m_ViewPortWidth, m_ViewPortHeight);
        m_RTVDescriptorSize = 0;
        m_FrameIndex = 0;
        m_DesiredOutputResolution = { m_ViewPortWidth, m_ViewPortHeight };
        m_HaltonPointSet = GenerateHalton(2, 3, 1, 32);
        m_AspectRatio = static_cast<float>(m_ViewPortWidth) / static_cast<float>(m_ViewPortHeight);
    }

    // Default constructor.
    D3D12AppXeSS()
    {
        m_ViewPortWidth = 1920;
        m_ViewPortHeight = 1080;
        m_ViewPort = CD3DX12_VIEWPORT(0.0f, 0.0f, m_ViewPortWidth, m_ViewPortHeight);
        m_ScissorRect = CD3DX12_RECT(0, 0, m_ViewPortWidth, m_ViewPortHeight);
        m_RTVDescriptorSize = 0;
        m_FrameIndex = 0;
        m_DesiredOutputResolution = { m_ViewPortWidth, m_ViewPortHeight };
        m_HaltonPointSet = GenerateHalton(2, 3, 1, 32);
        m_AspectRatio = static_cast<float>(m_ViewPortWidth) / static_cast<float>(m_ViewPortHeight);
    }

private:
    // 1. Initializing Adapter.
    bool InitAdapter()
    {
        /*
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
        */

        HRESULT hr{};

        hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_DXGIFactory));
        if (FAILED(hr))
        {
            return false;
        }

        IDXGIAdapter1* adapter{ nullptr };
        int adapterIndex{ 0 };
        bool adapterFound{ false };

        while (m_DXGIFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapterIndex++;
                continue;
            }

            hr = D3D12CreateDevice(
                adapter, D3D_FEATURE_LEVEL_11_0,
                __uuidof(ID3D12Device), nullptr
            );

            if (SUCCEEDED(hr))
            {
                adapterFound = true;
                break;
            }
        }

        if (!adapterFound)
        {
            return false;
        }

        hr = D3D12CreateDevice(
            adapter, D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_Device)
        );

        if (!SUCCEEDED(hr))
        {
            return false;
        }

        return true;
    }

    // 2. Initializing Command Queue.
    bool InitCommandQueue()
    {
        HRESULT hr{};

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        hr = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue));

        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    // 3. Initializing Swap chain.
    bool InitSwapChain()
    {
        HRESULT hr{};

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = FRAME_COUNT;
        swapChainDesc.Width = m_ViewPortWidth;
        swapChainDesc.Height = m_ViewPortHeight;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain{};
        hr = m_DXGIFactory->CreateSwapChainForHwnd(
            m_CommandQueue.Get(),
            m_Hwnd, &swapChainDesc, nullptr, nullptr, &swapChain
        );
        if (FAILED(hr))
        {
            return false;
        }

        hr = m_DXGIFactory->MakeWindowAssociation(m_Hwnd, DXGI_MWA_NO_ALT_ENTER);
        if (FAILED(hr))
        {
            return false;
        }

        hr = swapChain.As(&m_SwapChain);
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    // 4. Initialzing App descriptor heap.
    bool InitAppDescriptorHeap()
    {
        HRESULT hr{};

        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescriptor{
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            APP_DESCRIPTOR_COUNT,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            0
        };

        hr = m_Device->CreateDescriptorHeap(&descriptorHeapDescriptor, IID_PPV_ARGS(&m_AppDescriptorHeap));
        if (FAILED(hr))
        {
            return false;
        }

        m_AppDescriptorHeap->SetName(L"App descriptor heap");

        return true;
    }

    // 5. Initializing XeSS components.
    bool InitXeSS()
    {
        HRESULT hr{};

        auto status = xessD3D12CreateContext(m_Device.Get(), &m_XessContext);
        if (status != XESS_RESULT_SUCCESS)
        {
            OutputDebugString(L"Unable to create XeSS context");
            return false;
        }

        if (XESS_RESULT_WARNING_OLD_DRIVER == xessIsOptimalDriver(m_XessContext))
        {
            OutputDebugString(L"Invalid version of driver");
            return false;
        }

        xess_properties_t properties{};
        status = xessGetProperties(m_XessContext, &m_DesiredOutputResolution, &properties);
        if (status != XESS_RESULT_SUCCESS)
        {
            OutputDebugString(L"Unable to get XeSS props");
            return false;
        }

        xess_version_t xefxVersion{};
        status = xessGetIntelXeFXVersion(m_XessContext, &xefxVersion);
        if (status != XESS_RESULT_SUCCESS)
        {
            OutputDebugString(L"Unable to get XeFX version");
            return false;
        }

        D3D12_HEAP_DESC texturesHeapDescriptor
        {
            properties.tempTextureHeapSize,
            {
                D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                D3D12_MEMORY_POOL_UNKNOWN, 0, 0
            },
            0, D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES
        };

        hr = m_Device->CreateHeap(&texturesHeapDescriptor, IID_PPV_ARGS(&m_TexturesHeap));
        if (FAILED(hr))
        {
            return false;
        }

        m_TexturesHeap->SetName(L"XeSS Textures Heap");

        m_UAVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        xess_d3d12_init_params_t params = {
            m_DesiredOutputResolution,
            m_Quality,
            #if defined(USE_LOWRES_MV)
            XESS_INIT_FLAG_NONE,
            #else
            XESS_INIT_FLAG_HIGH_RES_MV,
            #endif
            0,
            0,
            nullptr,
            0,
            m_TexturesHeap.Get(),
            0,
            NULL
        };

        status = xessD3D12Init(m_XessContext, &params);
        if (status != XESS_RESULT_SUCCESS)
        {
            OutputDebugString(L"Unable to get XeSS props");
            return false;
        }

        status = xessGetInputResolution(m_XessContext, &m_DesiredOutputResolution, m_Quality, &m_InputRenderResolution);
        if (status != XESS_RESULT_SUCCESS)
        {
            OutputDebugString(L"Unable to get XeSS props");
            return false;
        }

        return true;
    }

    // 6. Initializing Descriptor heaps (Render Targets Views(RTV) and Depth Stencil Views(DSV)).
    bool InitDesciptorHeaps()
    {
        HRESULT hr{};

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDescriptor{};
        rtvHeapDescriptor.NumDescriptors = FRAME_COUNT * RT_COUNT;
        rtvHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        hr = m_Device->CreateDescriptorHeap(&rtvHeapDescriptor, IID_PPV_ARGS(&m_RenderTargetsViewsDescriptorHeap));
        if (FAILED(hr))
        {
            return false;
        }

        m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDescriptor{};
        dsvHeapDescriptor.NumDescriptors = FRAME_COUNT;
        dsvHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        hr = m_Device->CreateDescriptorHeap(&dsvHeapDescriptor, IID_PPV_ARGS(&m_DepthStencilViewsDescriptorHeap));
        if (FAILED(hr))
        {
            return false;
        }

        m_DSVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        return true;
    }

    // 7. Initializing resources and views.
    bool InitResourcesAndViews()
    {
        HRESULT hr{};

        m_DSVFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
        m_DSVTypedFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

        // Creating frame resources.
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RenderTargetsViewsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_DepthStencilViewsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // Creating render targets and XeSS output for each frame.
        for (UINT i = 0; i < FRAME_COUNT; i++)
        {
            D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            D3D12_RESOURCE_DESC textureDescriptor =
                CD3DX12_RESOURCE_DESC::Tex2D(format, m_InputRenderResolution.x, m_InputRenderResolution.y);
            textureDescriptor.MipLevels = 1;

            // Getting swap chain
            m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_PresentRenderTargets[i]));

            D3D12_CLEAR_VALUE clearValue;
            clearValue.Color[0] = m_ClearColor[0];
            clearValue.Color[1] = m_ClearColor[1];
            clearValue.Color[2] = m_ClearColor[2];
            clearValue.Color[3] = m_ClearColor[3];
            clearValue.Format = format;

            // Render Target (Input resolution)
            textureDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                &textureDescriptor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &clearValue,
                IID_PPV_ARGS(&m_RenderTargets[i]));
            if (FAILED(hr))
            {
                return false;
            }

            // Depth Stencil (Input resolution)
            textureDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            textureDescriptor.Format = m_DSVFormat;
            hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                &textureDescriptor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr,
                IID_PPV_ARGS(&m_DepthTargets[i]));
            if (FAILED(hr))
            {
                return false;
            }

            // XeSS output (Unordered Access) (Desired output resolution)
            textureDescriptor = CD3DX12_RESOURCE_DESC::Tex2D(
                format, m_DesiredOutputResolution.x, m_DesiredOutputResolution.y
            );
            textureDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                &textureDescriptor, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                IID_PPV_ARGS(&m_XessOutput[i]));
            if (FAILED(hr))
            {
                return false;
            }

            // Velocity (Render Target)
            format = DXGI_FORMAT_R16G16_FLOAT;
            textureDescriptor = CD3DX12_RESOURCE_DESC::Tex2D(
                format, m_DesiredOutputResolution.x, m_DesiredOutputResolution.y
            );
            textureDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            clearValue.Format = format;
            hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                &textureDescriptor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &clearValue,
                IID_PPV_ARGS(&m_RenderTargetsVelocity[i]));
            if (FAILED(hr))
            {
                return false;
            }

            // Creating Render Target VIEWS
            // a) RT_Present
            m_Device->CreateRenderTargetView(
                m_PresentRenderTargets[i].Get(), nullptr, rtvHandle
            );
            rtvHandle.Offset(1, m_RTVDescriptorSize);

            // b) RT_Color
            m_Device->CreateRenderTargetView(
                m_RenderTargets[i].Get(), nullptr, rtvHandle
            );
            rtvHandle.Offset(1, m_RTVDescriptorSize);

            // c) RT_Velocity
            m_Device->CreateRenderTargetView(
                m_RenderTargetsVelocity[i].Get(), nullptr, rtvHandle
            );
            rtvHandle.Offset(1, m_RTVDescriptorSize);

            // d) RT_Depth
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescriptor = D3D12_DEPTH_STENCIL_VIEW_DESC{};
            dsvDescriptor.Format = m_DSVTypedFormat;
            dsvDescriptor.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDescriptor.Flags = D3D12_DSV_FLAG_NONE;
            dsvDescriptor.Texture2D = D3D12_TEX2D_DSV{};
            dsvDescriptor.Texture2D.MipSlice = 0;
            m_Device->CreateDepthStencilView(
                m_DepthTargets[i].Get(), &dsvDescriptor, dsvHandle
            );
            dsvHandle.Offset(1, m_DSVDescriptorSize);

            // Setting up names:
            m_PresentRenderTargets[i]->SetName((std::wstring(L"Present") + std::to_wstring(i)).c_str());;
            m_RenderTargets[i]->SetName((std::wstring(L"Color") + std::to_wstring(i)).c_str());
            m_DepthTargets[i]->SetName((std::wstring(L"Depth") + std::to_wstring(i)).c_str());
            m_XessOutput[i]->SetName((std::wstring(L"XeSS-Output") + std::to_wstring(i)).c_str());
            m_RenderTargetsVelocity[i]->SetName((std::wstring(L"Velocity") + std::to_wstring(i)).c_str());
        }

        return true;
    }

    // 8. Initializing command allocator.
    bool InitCommandAllocator()
    {
        HRESULT hr{};

        hr = m_Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)
        );

        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    // 9. Initializing Root signature.
    bool InitRootSignature()
    {
        HRESULT hr{};

        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
        if (FAILED(hr))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];

        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescriptor{};
        rootSignatureDescriptor.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        Microsoft::WRL::ComPtr<ID3DBlob> signature{};
        Microsoft::WRL::ComPtr<ID3DBlob> error{};

        hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescriptor, featureData.HighestVersion, &signature, &error);
        if (FAILED(hr))
        {
            return false;
        }

        hr = m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    // 10. Initializing Pipeline state object (PSO).
    bool InitPipelineStateObjects()
    {
        HRESULT hr{};

        // Color PSO:
        Microsoft::WRL::ComPtr<ID3DBlob> colorVertexShader{};
        Microsoft::WRL::ComPtr<ID3DBlob> colorPixelShader{};

        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        hr = D3DCompileFromFile(L"shaders.hlsl", nullptr,
            nullptr, "VSMainColor", "vs_5_0", compileFlags, 0, &colorVertexShader, nullptr);
        if (FAILED(hr))
        {
            return false;
        }

        hr = D3DCompileFromFile(L"shaders.hlsl", nullptr,
            nullptr, "PSMainColor", "ps_5_0", compileFlags, 0, &colorPixelShader, nullptr);
        if (FAILED(hr))
        {
            return false;
        }

        D3D12_INPUT_ELEMENT_DESC inputElementDescriptor[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescriptor{};
        psoDescriptor.InputLayout = { inputElementDescriptor, _countof(inputElementDescriptor) };
        psoDescriptor.pRootSignature = m_RootSignature.Get();
        psoDescriptor.VS = CD3DX12_SHADER_BYTECODE(colorVertexShader.Get());
        psoDescriptor.PS = CD3DX12_SHADER_BYTECODE(colorPixelShader.Get());
        psoDescriptor.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDescriptor.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDescriptor.DepthStencilState.DepthEnable = TRUE;
        psoDescriptor.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDescriptor.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        psoDescriptor.DepthStencilState.StencilEnable = TRUE;
        psoDescriptor.DepthStencilState.FrontFace = psoDescriptor.DepthStencilState.BackFace =
            D3D12_DEPTH_STENCILOP_DESC{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
                D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
        psoDescriptor.DepthStencilState.StencilReadMask = (UINT8)0xFF;
        psoDescriptor.DepthStencilState.StencilWriteMask = (UINT8)0xFF;
        psoDescriptor.SampleMask = UINT_MAX;
        psoDescriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDescriptor.NumRenderTargets = 1;
        psoDescriptor.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        psoDescriptor.DSVFormat = m_DSVTypedFormat;
        psoDescriptor.SampleDesc.Count = 1;

        hr = m_Device->CreateGraphicsPipelineState(
            &psoDescriptor, IID_PPV_ARGS(&m_PipelineStateColorPass)
        );

        if (FAILED(hr))
        {
            return false;
        }

        // Velocity PSO:
        Microsoft::WRL::ComPtr<ID3DBlob> velocityVertexShader{};
        Microsoft::WRL::ComPtr<ID3DBlob> velocityPixelShader{};

        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        hr = D3DCompileFromFile(L"shaders.hlsl", nullptr,
            nullptr, "VSMainVelocity", "vs_5_0", compileFlags, 0, &velocityVertexShader, nullptr);
        if (FAILED(hr))
        {
            return false;
        }

        hr = D3DCompileFromFile(L"shaders.hlsl", nullptr,
            nullptr, "PSMainVelocity", "ps_5_0", compileFlags, 0, &velocityPixelShader, nullptr);
        if (FAILED(hr))
        {
            return false;
        }

        D3D12_INPUT_ELEMENT_DESC velocityInputElementDescriptor[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC velocityPsoDescriptor{};
        velocityPsoDescriptor.InputLayout = { velocityInputElementDescriptor, _countof(velocityInputElementDescriptor) };
        velocityPsoDescriptor.pRootSignature = m_RootSignature.Get();
        velocityPsoDescriptor.VS = CD3DX12_SHADER_BYTECODE(velocityVertexShader.Get());
        velocityPsoDescriptor.PS = CD3DX12_SHADER_BYTECODE(velocityPixelShader.Get());
        velocityPsoDescriptor.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        velocityPsoDescriptor.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        velocityPsoDescriptor.DepthStencilState.DepthEnable = FALSE;
        velocityPsoDescriptor.DepthStencilState.StencilEnable = FALSE;
        velocityPsoDescriptor.SampleMask = UINT_MAX;
        velocityPsoDescriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        velocityPsoDescriptor.NumRenderTargets = 1;
        velocityPsoDescriptor.RTVFormats[0] = DXGI_FORMAT_R16G16_FLOAT;
        velocityPsoDescriptor.SampleDesc.Count = 1;

        hr = m_Device->CreateGraphicsPipelineState(
            &velocityPsoDescriptor, IID_PPV_ARGS(&m_PipelineStateVelocityPass)
        );

        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    // 11. Initializing command list.
    bool InitCommandList()
    {
        HRESULT hr{};

        hr = m_Device->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_CommandAllocator.Get(), m_PipelineStateColorPass.Get(),
            IID_PPV_ARGS(&m_GraphicsCommandList)
        );
        if (FAILED(hr))
        {
            return false;
        }

        m_GraphicsCommandList->Close();

        return true;
    }

    // 12. Init Vertex Buffer.
    bool InitVertexBuffer()
    {
        HRESULT hr{};

        Vertex triangleVertices[] =
        {
           { {0.0f, 0.25f * m_AspectRatio, 0.01f}, {1.0f, 0.0f, 0.0f, 1.0f}   },
           { {0.25f, -0.25f * m_AspectRatio, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}  },
           { {-0.25f, -0.25f * m_AspectRatio, 0.2f}, {0.0f, 0.0f, 1.0f, 1.0f} }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

        hr = m_Device->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescriptor,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexBuffer)
        );
        if (FAILED(hr))
        {
            return false;
        }

        UINT8* pVertexDataBegin{};
        CD3DX12_RANGE readRange(0, 0);
        hr = m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        if (FAILED(hr))
        {
            return false;
        }
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_VertexBuffer->Unmap(0, nullptr);

        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.StrideInBytes = sizeof(Vertex);
        m_VertexBufferView.SizeInBytes = vertexBufferSize;

        return true;
    }

    // 13. Init Constant Buffer.
    bool InitConstantBuffer()
    {
        HRESULT hr{};

        const UINT constantBufferSize = sizeof(SceneConstantBuffer);
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

        hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
            &resourceDescriptor, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ConstantBuffer));
        if (FAILED(hr))
        {
            return false;
        }

        D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDescriptor{};
        constantBufferViewDescriptor.BufferLocation = m_ConstantBuffer->GetGPUVirtualAddress();
        constantBufferViewDescriptor.SizeInBytes = constantBufferSize;

        m_Device->CreateConstantBufferView(&constantBufferViewDescriptor, m_AppDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        CD3DX12_RANGE readRange(0, 0);
        hr = m_ConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin));
        if (FAILED(hr))
        {
            return false;
        }

        memcpy(m_pCbvDataBegin, &m_ConstantBufferData, sizeof(m_ConstantBufferData));

        return true;
    }

    // 14. Initializing Synchronization System.
    bool InitSynchronizationSystem()
    {
        HRESULT hr{};

        hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
        if (FAILED(hr))
        {
            return false;
        }

        m_FenceValue = 1;

        m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        if (m_FenceEvent == nullptr)
        {
            return false;
        }

        WaitForPreviousFrame();

        return true;
    }

    // 15. Filling App descriptor heap.
    bool FillAppDescriptorHeap()
    {
        auto addTexture = [&](std::uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format, bool isUav = false)
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle(m_AppDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (INT)index, m_UAVDescriptorSize);
            if (isUav)
            {
                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescriptor{};
                uavDescriptor.Format = format;
                uavDescriptor.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                uavDescriptor.Texture2D.MipSlice = 0;
                uavDescriptor.Texture2D.PlaneSlice = 0;
                m_Device->CreateUnorderedAccessView(resource, nullptr, &uavDescriptor, cpuDescriptorHandle);
            }
            else
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
                srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDescriptor.Format = format;
                srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDescriptor.Texture2D.MipLevels = 1;
                srvDescriptor.Texture2D.MostDetailedMip = 0;
                srvDescriptor.Texture2D.PlaneSlice = 0;
                m_Device->CreateShaderResourceView(resource, &srvDescriptor, cpuDescriptorHandle);
            }
        };

        for (UINT i = 0; i < FRAME_COUNT; ++i)
        {
            addTexture(DESCRIPTORS_PER_FRAME * i + DHI_Color, m_RenderTargets[i].Get(),
                DXGI_FORMAT_R16G16B16A16_FLOAT);
            addTexture(DESCRIPTORS_PER_FRAME * i + DHI_Velocity, m_RenderTargetsVelocity[i].Get(),
                DXGI_FORMAT_R16G16_FLOAT);
            addTexture(DESCRIPTORS_PER_FRAME * i + DHI_XeSSOutputUAV, m_XessOutput[i].Get(),
                DXGI_FORMAT_R16G16B16A16_FLOAT, true);
            addTexture(DESCRIPTORS_PER_FRAME * i + DHI_XeSSOutputSRV, m_XessOutput[i].Get(),
                DXGI_FORMAT_R16G16B16A16_FLOAT);
        }

        return true;
    }

    // 16. Initializing FSQ Root Signature.
    bool InitFSQRootSignature()
    {
        HRESULT hr{};

        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};

        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        hr = m_Device->CheckFeatureSupport(
            D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)
        );
        if (FAILED(hr))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE
        );

        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescriptor{};
        rootSignatureDescriptor.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Microsoft::WRL::ComPtr<ID3DBlob> signature{};
        Microsoft::WRL::ComPtr<ID3DBlob> error{};

        hr = D3DX12SerializeVersionedRootSignature(
            &rootSignatureDescriptor, featureData.HighestVersion, &signature, &error
        );
        if (FAILED(hr))
        {
            return false;
        }

        hr = m_Device->CreateRootSignature(0, signature->GetBufferPointer(),
            signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignatureFSQ));
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    // 17. Initializing FSQ Pipeline State Object.
    bool InitFSQPipelineStateObject()
    {
        HRESULT hr{};

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShader{};
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShader{};

        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        hr = D3DCompileFromFile(L"shaders.hlsl", nullptr,
            nullptr, "VSMainFSQ", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
        if (FAILED(hr))
        {
            return false;
        }

        hr = D3DCompileFromFile(L"shaders.hlsl", nullptr,
            nullptr, "PSMainFSQ", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);
        if (FAILED(hr))
        {
            return false;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescriptor{};
        psoDescriptor.InputLayout = {};
        psoDescriptor.pRootSignature = m_RootSignatureFSQ.Get();
        psoDescriptor.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDescriptor.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDescriptor.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDescriptor.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDescriptor.DepthStencilState.DepthEnable = FALSE;
        psoDescriptor.DepthStencilState.StencilEnable = FALSE;
        psoDescriptor.SampleMask = UINT_MAX;
        psoDescriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDescriptor.NumRenderTargets = 1;
        psoDescriptor.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDescriptor.SampleDesc.Count = 1;

        hr = m_Device->CreateGraphicsPipelineState(
            &psoDescriptor, IID_PPV_ARGS(&m_PipelineStateFSQPass)
        );
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    // Initializing DirectX 12 and XeSS.
    bool InitD3D12()
    {
        if (!InitAdapter())
        {
            OutputDebugString(L"DEBUG::LOG::1. Failed initialization of adapter!\n");
            return false;
        }
        if (!InitCommandQueue())
        {
            OutputDebugString(L"DEBUG::LOG::2. Failed initialization of command queue!\n");
            return false;
        }
        if (!InitSwapChain())
        {
            OutputDebugString(L"DEBUG::LOG::3. Failed initialization of swap chain!\n");
            return false;
        }
        if (!InitAppDescriptorHeap())
        {
            OutputDebugString(L"DEBUG::LOG::4. Failed initialization of app descriptor heap!\n");
            return false;
        }
        if (!InitXeSS())
        {
            OutputDebugString(L"DEBUG::LOG:5. Failed initialization of XeSS technology!\n");
            return false;
        }
        if (!InitDesciptorHeaps())
        {
            OutputDebugString(L"DEBUG::LOG::6. Failed initialization of Descriptor Heaps!\n");
            return false;
        }
        if (!InitResourcesAndViews())
        {
            OutputDebugString(L"DEBUG::LOG::7. Failed initialization of Resources and views!\n");
            return false;
        }
        if (!InitCommandAllocator())
        {
            OutputDebugString(L"DEBUG::LOG::8. Failed initialization of command allocator!\n");
            return false;
        }
        if (!InitRootSignature())
        {
            OutputDebugString(L"DEBUG::LOG::9. Failed initialization of Root Signature!\n");
            return false;
        }
        if (!InitPipelineStateObjects())
        {
            OutputDebugString(L"DEBUG::LOG::10. Failed initialization of Pipeline State Objects!\n");
            return false;
        }
        if (!InitCommandList())
        {
            OutputDebugString(L"DEBUG::LOG::11. Failed initialization of Command List!\n");
            return false;
        }
        if (!InitVertexBuffer())
        {
            OutputDebugString(L"DEBUG::LOG::12. Failed initialization of Vertex Buffer!\n");
            return false;
        }
        if (!InitConstantBuffer())
        {
            OutputDebugString(L"DEBUG::LOG::13. Failed initialization of constant buffer!\n");
            return false;
        }
        if (!InitSynchronizationSystem())
        {
            OutputDebugString(L"DEBUG::LOG::14. Failed initialition of synchronization system!\n");
            return false;
        }
        if (!FillAppDescriptorHeap())
        {
            OutputDebugString(L"DEBUG::LOG::15. Failed filling app desciptor heap!\n");
            return false;
        }
        if (!InitFSQRootSignature())
        {
            OutputDebugString(L"DEBUG::LOG::16. Failed initialization of FSQ Root signature!\n");
            return false;
        }
        if (!InitFSQPipelineStateObject())
        {
            OutputDebugString(L"DEBUG::LOG::17. Failed initialization of FSQ Pipeline state object!\n");
            return false;
        }

        return true;
    }

public:
    // Initializing application.
    bool Init(HWND handleToTheWindow)
    {
        m_Hwnd = handleToTheWindow;
        return InitD3D12();
    }

    // Updating command lits.
    void Update()
    {
        if (m_LastTime.time_since_epoch().count() == 0)
        {
            m_LastTime = std::chrono::high_resolution_clock::now();
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedSeconds = currentTime - m_LastTime;
        m_LastTime = currentTime;

        const double speed = 1. / 2;
        float transaltionSpeed = (float)(speed * elapsedSeconds.count());
        const float offsetBounds = 1.25f;

        m_ConstantBufferData.offset.x += transaltionSpeed;
        if (m_ConstantBufferData.offset.x > offsetBounds)
        {
            m_ConstantBufferData.offset.x = -offsetBounds;
        }

        auto haltonValue = m_HaltonPointSet[m_HaltonIndex];
        m_HaltonIndex = (m_HaltonIndex + 1) % m_HaltonPointSet.size();

        jitter[0] = haltonValue.first;
        jitter[1] = haltonValue.second;

        m_ConstantBufferData.offset.z = jitter[0];
        m_ConstantBufferData.offset.w = jitter[1];
        m_ConstantBufferData.resolution.x = (float)m_InputRenderResolution.x;
        m_ConstantBufferData.resolution.y = (float)m_InputRenderResolution.y;
        m_ConstantBufferData.velocity.x = -transaltionSpeed * ((float)m_DesiredOutputResolution.x / 2.f);

        memcpy(m_pCbvDataBegin, &m_ConstantBufferData, sizeof(m_ConstantBufferData));
    }

    // Rendering primitives with XeSS technology.
    void Render()
    {
        // 0. Getting time from FPS counter.
        m_FPSCounter.GetFrameDelta();

        // 1. Filling command lists.
        FillCommandList();

        // 2. Executinig command lists.
        ID3D12CommandList* ppCommandLists[] = { m_GraphicsCommandList.Get() };
        m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        HRESULT hr = m_SwapChain->Present(1, 0);
        if (FAILED(hr))
        {
            return;
        }

        // 3. Waiting for previous frame.
        WaitForPreviousFrame();
    
        // 4. Printing FPS.
        m_FPSCounter.PrintFPS();
    }

    // Stop working.
    void Exit()
    {
        m_FPSCounter.Exit();
        OutputDebugString(L"Exiting D3D12App!\n");
    }

private:
    // Filling command lists.
    void FillCommandList()
    {
        HRESULT hr{};

        hr = m_CommandAllocator->Reset();
        if (FAILED(hr))
        {
            return;
        }

        hr = m_GraphicsCommandList->Reset(m_CommandAllocator.Get(), m_PipelineStateColorPass.Get());
        if (FAILED(hr))
        {
            return;
        }

        // 1. Running color pass. -----------------------------------------------------]

        // a) Setting up necessary state.
        m_GraphicsCommandList->SetGraphicsRootSignature(m_RootSignature.Get());

        ID3D12DescriptorHeap* ppHeaps[] = { m_AppDescriptorHeap.Get() };
        m_GraphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        CD3DX12_VIEWPORT renderResolutionViewPort(
            0.f, 0.f, (float)m_InputRenderResolution.x, (float)m_InputRenderResolution.y
        );
        CD3DX12_RECT renderResolutionScissors(
            0, 0, (LONG)m_InputRenderResolution.x, (LONG)m_InputRenderResolution.y
        );
        m_GraphicsCommandList->SetGraphicsRootDescriptorTable(0, m_AppDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        m_GraphicsCommandList->RSSetViewports(1, &renderResolutionViewPort);
        m_GraphicsCommandList->RSSetScissorRects(1, &renderResolutionScissors);

        // b) Transition Color buffer to render target.
        std::vector<CD3DX12_RESOURCE_BARRIER> transitions =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
            CD3DX12_RESOURCE_BARRIER::Transition(m_DepthTargets[m_FrameIndex].Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE)
        };
        m_GraphicsCommandList->ResourceBarrier((UINT)transitions.size(), transitions.data());

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(
                m_RenderTargetsViewsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                (INT)(m_FrameIndex * RT_COUNT + RT_Color), m_RTVDescriptorSize
            );       // np.: 0 * 3 + 1 (Nice!)
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(
                m_DepthStencilViewsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                (INT)(m_FrameIndex), m_DSVDescriptorSize
            );
        m_GraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        // c) Recording commands.
        m_GraphicsCommandList->ClearRenderTargetView(rtvHandle, m_ClearColor, 0, nullptr);
        m_GraphicsCommandList->ClearDepthStencilView(
            dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr
        );
        m_GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_GraphicsCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        m_GraphicsCommandList->DrawInstanced(3, 1, 0, 0);

        // 2. Running velocity pass --------------------------------------------------]
        m_GraphicsCommandList->SetPipelineState(m_PipelineStateVelocityPass.Get());
        m_GraphicsCommandList->RSSetViewports(1, &m_ViewPort);
        m_GraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);

        // a) Indicating that the back buffer will be used as a render target.
        CD3DX12_RESOURCE_BARRIER transition =
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_RenderTargetsVelocity[m_FrameIndex].Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET
            );
        m_GraphicsCommandList->ResourceBarrier(1, &transition);

        rtvHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RenderTargetsViewsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                (INT)(m_FrameIndex * RT_COUNT + RT_Velocity), m_RTVDescriptorSize
            );
        m_GraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // b) Recording commands.
        m_GraphicsCommandList->ClearRenderTargetView(rtvHandle, m_ClearColor, 0, nullptr);
        m_GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_GraphicsCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        m_GraphicsCommandList->DrawInstanced(3, 1, 0, 0);

        // 3. Running XeSS pass -----------------------------------------------------]

        // a) Transition render targets D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE for XeSS
        std::vector<CD3DX12_RESOURCE_BARRIER> transitions_ = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargetsVelocity[m_FrameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_DepthTargets[m_FrameIndex].Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        m_GraphicsCommandList->ResourceBarrier((UINT)transitions_.size(), transitions_.data());

        // b) Setting up
        xess_d3d12_execute_params_t executionParams{};
        executionParams.inputWidth = m_InputRenderResolution.x;
        executionParams.inputHeight = m_InputRenderResolution.y;
        executionParams.jitterOffsetX = jitter[0];
        executionParams.jitterOffsetY = -jitter[1];
        executionParams.exposureScale = 1.0f;
        executionParams.pColorTexture = m_RenderTargets[m_FrameIndex].Get();
        executionParams.pVelocityTexture = m_RenderTargetsVelocity[m_FrameIndex].Get();
        executionParams.pOutputTexture = m_XessOutput[m_FrameIndex].Get();
#if defined(USE_LOWRES_MV)
        executionParams.pDepthTexture = m_depthTargets[m_frameIndex].Get();
#else
        executionParams.pDepthTexture = nullptr;
#endif
        executionParams.pExposureScaleTexture = 0;

        // c) Executing
        auto status = xessD3D12Execute(m_XessContext, m_GraphicsCommandList.Get(), &executionParams);
        if (status != XESS_RESULT_SUCCESS)
        {
            OutputDebugString(L"Unable to run XeSS technology");
        }

        // 4. Rendering XeSS output using full screen quad ---------------------]

        ID3D12DescriptorHeap* xessPpHeaps[] = { m_AppDescriptorHeap.Get() };
        m_GraphicsCommandList->SetDescriptorHeaps(_countof(xessPpHeaps), xessPpHeaps);

        // a) Transition XeSS output is in UAV so we need to transition it to SRV.
        transition = CD3DX12_RESOURCE_BARRIER::Transition(
            m_XessOutput[m_FrameIndex].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        );
        m_GraphicsCommandList->ResourceBarrier(1, &transition);
        m_GraphicsCommandList->SetPipelineState(m_PipelineStateFSQPass.Get());
        m_GraphicsCommandList->SetGraphicsRootSignature(m_RootSignatureFSQ.Get());

        // b) Use selected output (DHI_XeSSoutputSRV by default)
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle(
            m_AppDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
            (INT)((DESCRIPTORS_PER_FRAME * m_FrameIndex) + m_OutputIndex), m_UAVDescriptorSize
        );
        m_GraphicsCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorHandle);
        m_GraphicsCommandList->RSSetViewports(1, &m_ViewPort);
        m_GraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);

        // c) Indicate that the back buffer will be used as render target.
        transition =
            CD3DX12_RESOURCE_BARRIER::Transition(m_PresentRenderTargets[m_FrameIndex].Get(),
                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_GraphicsCommandList->ResourceBarrier(1, &transition);

        rtvHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RenderTargetsViewsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                (INT)(m_FrameIndex * RT_COUNT), m_RTVDescriptorSize);
        m_GraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // d) Recording commands
        m_GraphicsCommandList->ClearRenderTargetView(rtvHandle, m_ClearColor, 0, nullptr);
        m_GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_GraphicsCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        m_GraphicsCommandList->DrawInstanced(3, 1, 0, 0);

        // e) Transition Render Target/ RT to present
        transition =
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_PresentRenderTargets[m_FrameIndex].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT
            );
        m_GraphicsCommandList->ResourceBarrier(1, &transition);

        // f) Transition XeSS output to UAV for future use
        transition =
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_XessOutput[m_FrameIndex].Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS
            );
        m_GraphicsCommandList->ResourceBarrier(1, &transition);

        hr = m_GraphicsCommandList->Close();
        if (FAILED(hr))
        {
            OutputDebugString(L"Failed recording commands");
        }
    }

    // Waiting for previous frame to finished.
    void WaitForPreviousFrame()
    {
        HRESULT hr{};

        const UINT64 fence = m_FenceValue;

        hr = m_CommandQueue->Signal(m_Fence.Get(), fence);
        if (FAILED(hr))
        {
            return;
        }

        m_FenceValue++;

        if (m_Fence->GetCompletedValue() < fence)
        {
            hr = m_Fence->SetEventOnCompletion(fence, m_FenceEvent);
            if (FAILED(hr))
            {
                return;
            }

            WaitForSingleObject(m_FenceEvent, INFINITE);
        }

        m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

        char msg[3000];
        sprintf_s(msg, "Frame [%d] finished waiting!\n", m_FenceValue);
        OutputDebugStringA(msg);
    }
};
#endif