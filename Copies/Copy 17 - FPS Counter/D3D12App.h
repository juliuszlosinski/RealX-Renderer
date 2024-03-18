#ifndef D3D12APP_H
#define D3D12APP_H

#include <string>
#include "stdfax.h"
#include "CommandFrame.h"
#include "Mesh.h"
#include "Texture.h"
#include "Camera.h"
#include "FPSCounter.h"

class D3D12App
{
	static const int FRAME_BUFFER_COUNT{ 3 };
	DXGI_FORMAT m_pBackBufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
	ID3D12Device* m_pDevice{ nullptr };
	IDXGISwapChain3* m_pSwapChain{ nullptr };
	ID3D12CommandQueue* m_pCommandQueue{ nullptr };
	ID3D12Resource* m_pRenderTargets[FRAME_BUFFER_COUNT];
	ID3D12CommandAllocator* m_pCommandAllocators[FRAME_BUFFER_COUNT];
	ID3D12GraphicsCommandList* m_pCommandList{ nullptr };
	ID3D12Fence* m_pFences[FRAME_BUFFER_COUNT];
	IDXGIFactory4* m_pDXGIFactory{ nullptr };
	ID3D12DescriptorHeap* m_pRTVDescriptorHeap{ nullptr };
	int frameIndex{};
	int rtvDescriptorSize{};
	int m_Width{ 600 };
	int m_Height{ 800 };
	HWND* m_pHandleToTheWindow{ nullptr };
	int m_CurrentFrameIndex{ 0 };
	int m_RTVDescriptorSize{};

	UINT64 m_FenceValues[FRAME_BUFFER_COUNT];

	CommandFrame m_Frames[FRAME_BUFFER_COUNT];
	ID3D12Fence* m_pFence{ nullptr };
	HANDLE m_FenceEvent{ nullptr };
	int m_CurrentFenceValue{ 0 };

	ID3D12PipelineState* m_pPipelineStateObject{ nullptr };
	ID3D12RootSignature* m_pRootSignature{ nullptr };

	D3D12_VIEWPORT m_Viewport{};
	D3D12_RECT m_ScissorRect{};

	ID3D12Resource* m_pDepthStencilBuffer{ nullptr };
	ID3D12DescriptorHeap* m_pDepthStencilViewDescriptorHeap{ nullptr };

	float RENDER_TARGET_CLEAR_COLOR[4]{ 0.0f, 0.2f, 0.4f, 1.0f };

	// ***********************************************************

	bool InitAdapter()
	{
		HRESULT hr{};

		hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_pDXGIFactory));
		if (FAILED(hr))
		{
			return false;
		}

		IDXGIAdapter1* adapter{ nullptr };
		int adapterIndex{ 0 };
		bool adapterFound{ false };

		while (m_pDXGIFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
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
			IID_PPV_ARGS(&m_pDevice)
		);

		if (!SUCCEEDED(hr))
		{
			return false;
		}

		return true;
	}

	bool InitCommandQueue()
	{
		HRESULT hr{};

		D3D12_COMMAND_QUEUE_DESC commandQueueDescriptor{};

		hr = m_pDevice->CreateCommandQueue(&commandQueueDescriptor, IID_PPV_ARGS(&m_pCommandQueue));

		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}

	bool InitSwapChain()
	{
		DXGI_MODE_DESC backBufferDescriptor{};
		backBufferDescriptor.Width = m_Width;
		backBufferDescriptor.Height = m_Height;
		backBufferDescriptor.Format = m_pBackBufferFormat;

		DXGI_SAMPLE_DESC sampleDescriptor{};
		sampleDescriptor.Count = 1;

		DXGI_SWAP_CHAIN_DESC swapChainDescriptor{};
		swapChainDescriptor.BufferCount = FRAME_BUFFER_COUNT;
		swapChainDescriptor.BufferDesc = backBufferDescriptor;
		swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDescriptor.OutputWindow = *m_pHandleToTheWindow;
		swapChainDescriptor.SampleDesc = sampleDescriptor;
		swapChainDescriptor.Windowed = true;

		IDXGISwapChain* tmpSwapChain{};

		m_pDXGIFactory->CreateSwapChain(
			m_pCommandQueue,
			&swapChainDescriptor,
			&tmpSwapChain
		);

		m_pSwapChain = static_cast<IDXGISwapChain3*>(tmpSwapChain);

		m_CurrentFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		return true;
	}

	bool InitRTVsFromSwapChain()
	{
		HRESULT hr{};

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDescriptor{};
		rtvHeapDescriptor.NumDescriptors = FRAME_BUFFER_COUNT;
		rtvHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		hr = m_pDevice->CreateDescriptorHeap(&rtvHeapDescriptor, IID_PPV_ARGS(&m_pRTVDescriptorHeap));

		if (FAILED(hr))
		{
			return false;
		}

		m_RTVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			// Getting pointers to the resources.
			hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTargets[i]));

			if (FAILED(hr))
			{
				return false;
			}

			m_pDevice->CreateRenderTargetView(m_pRenderTargets[i], nullptr, rtvHandle);

			rtvHandle.Offset(1, m_RTVDescriptorSize);
		}

		return true;
	}

	bool InitCommandAllocators()
	{
		HRESULT hr{};

		for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			hr = m_pDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&m_pCommandAllocators[i])
			);

			if (FAILED(hr))
			{
				return false;
			}
		}

		return true;
	}

	bool InitCommandList()
	{
		HRESULT hr{};

		hr = m_pDevice->CreateCommandList
		(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_pCommandAllocators[0],
			NULL,
			IID_PPV_ARGS(&m_pCommandList)
		);

		if (FAILED(hr))
		{
			return false;
		}

		m_pCommandList->Close();

		return true;
	}

	bool InitFenceSystem()
	{
		HRESULT hr{};

		hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));

		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		if (m_FenceEvent == nullptr)
		{
			return false;
		}

		for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			m_Frames[i].setCommandAllocator(m_pCommandAllocators[i]);
			m_Frames[i].setFenceValue(0);
		}

		m_pCommandList->Reset(m_Frames[0].getCommandAllocator(), nullptr);
		m_pCommandList->Close();

		return true;
	}

	bool InitRootSignature()
	{
		HRESULT hr{};

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

		D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1];
		descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorTableRanges[0].NumDescriptors = 1;
		descriptorTableRanges[0].BaseShaderRegister = 0;
		descriptorTableRanges[0].RegisterSpace = 0;
		descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable{};
		descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
		descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

		D3D12_ROOT_DESCRIPTOR rootCBVDescriptor{};
		rootCBVDescriptor.RegisterSpace = 0;
		rootCBVDescriptor.ShaderRegister = 0;

		D3D12_ROOT_PARAMETER rootParameters[2];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor = rootCBVDescriptor;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].DescriptorTable = descriptorTable;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescriptor{};
		rootSignatureDescriptor.Init(_countof(rootParameters),
			rootParameters,
			1,
			&sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		);


		ID3DBlob* signature{ nullptr };
		ID3DBlob* errorBuff{ nullptr };
		hr = D3D12SerializeRootSignature(&rootSignatureDescriptor, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff);
		if (FAILED(hr))
		{
			OutputDebugStringA((char*)errorBuff->GetBufferPointer());
			return false;
		}

		hr = m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}

	bool InitPipelineStateObject(LPCWSTR pathToVertexShader, LPCWSTR pathToPixelShader)
	{
		HRESULT hr{};

		ID3DBlob* vertexShader{ nullptr };
		ID3DBlob* errorBuffer{ nullptr };

		hr = D3DCompileFromFile(pathToVertexShader,
			nullptr,
			nullptr,
			"main",
			"vs_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
			0,
			&vertexShader,
			&errorBuffer
		);
		if (FAILED(hr))
		{
			OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
			return false;
		}

		D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
		vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
		vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

		ID3DBlob* pixelShader;
		hr = D3DCompileFromFile(pathToPixelShader,
			nullptr,
			nullptr,
			"main",
			"ps_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
			0,
			&pixelShader,
			&errorBuffer);

		if (FAILED(hr))
		{
			OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
			return false;
		}

		D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
		pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
		pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

		D3D12_INPUT_LAYOUT_DESC inputLayoutDescriptor = {};

		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};
		inputLayoutDescriptor.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		inputLayoutDescriptor.pInputElementDescs = inputLayout;

		/*
		D3D12_DEPTH_STENCIL_DESC depthStencilDescriptor{};
		depthStencilDescriptor.DepthEnable = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDescriptor.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilDescriptor.StencilEnable = FALSE;
		depthStencilDescriptor.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		depthStencilDescriptor.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOperation = {
			D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
		};
		depthStencilDescriptor.FrontFace = defaultStencilOperation;
		depthStencilDescriptor.BackFace = defaultStencilOperation;
		*/

		DXGI_SAMPLE_DESC sampleDescriptor{};
		sampleDescriptor.Count = 1;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = inputLayoutDescriptor;
		psoDesc.pRootSignature = m_pRootSignature;
		psoDesc.VS = vertexShaderBytecode;
		psoDesc.PS = pixelShaderBytecode;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc = sampleDescriptor;
		psoDesc.SampleMask = 0xffffffff;
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.NumRenderTargets = 1;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineStateObject));

		if (FAILED(hr))
		{
			return false;
		}

		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = m_Width;
		m_Viewport.Height = m_Height;
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;

		m_ScissorRect.left = 0;
		m_ScissorRect.top = 0;
		m_ScissorRect.right = m_Width;
		m_ScissorRect.bottom = m_Height;

		return true;
	}

	bool InitDepthStencil()
	{
		HRESULT hr{};

		D3D12_DESCRIPTOR_HEAP_DESC depthHeapDescriptor{};
		depthHeapDescriptor.NumDescriptors = 1;
		depthHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		depthHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		hr = m_pDevice->CreateDescriptorHeap(
			&depthHeapDescriptor,
			IID_PPV_ARGS(&m_pDepthStencilViewDescriptorHeap)
		);

		if (FAILED(hr))
		{
			return false;
		}

		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDescriptor{};
		depthStencilViewDescriptor.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilViewDescriptor.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDescriptor.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue{};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0.0f;

		m_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_D32_FLOAT,
				m_Width,
				m_Height,
				1, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
			),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&m_pDepthStencilBuffer)
		);
		m_pDepthStencilViewDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

		m_pDevice->CreateDepthStencilView(
			m_pDepthStencilBuffer, &depthStencilViewDescriptor,
			m_pDepthStencilViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
		);

		return true;
	}

	void BeginFrame()
	{
		CommandFrame& currentFrame = m_Frames[m_CurrentFrameIndex];
		currentFrame.Wait(m_pFence, m_FenceEvent);
		currentFrame.getCommandAllocator()->Reset();
		m_pCommandList->Reset(currentFrame.getCommandAllocator(), nullptr);
	}

	void EndFrame()
	{
		m_pCommandList->Close();
		ID3D12CommandList* const commandLists[] = { m_pCommandList };
		m_pCommandQueue->ExecuteCommandLists(_countof(commandLists), &commandLists[0]);

		m_CurrentFenceValue++;
		CommandFrame& currentFrame = m_Frames[m_CurrentFrameIndex];
		currentFrame.setFenceValue(m_CurrentFenceValue);
		m_pCommandQueue->Signal(m_pFence, m_CurrentFenceValue);

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % FRAME_BUFFER_COUNT;
		m_pSwapChain->Present(0, 0);
	}

	void Setup()
	{
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_pRenderTargets[m_CurrentFrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		m_pCommandList->ResourceBarrier(1, &transition);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentFrameIndex, m_RTVDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDepthStencilViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		m_pCommandList->OMSetRenderTargets(1, &rtvHandle, 1, &dsvHandle);
		m_pCommandList->ClearRenderTargetView(rtvHandle, RENDER_TARGET_CLEAR_COLOR, 0, nullptr);
		m_pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		m_pCommandList->RSSetViewports(1, &m_Viewport);
		m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);
		m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void End()
	{
		CD3DX12_RESOURCE_BARRIER transition02 = CD3DX12_RESOURCE_BARRIER::Transition(
			m_pRenderTargets[m_CurrentFrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
		);
		m_pCommandList->ResourceBarrier(1, &transition02);
	}

public:

	D3D12App(int width, int height)
	{
		m_Width = width;
		m_Height = height;
	}

	// Input layout Signature + Shader = Pipeline State Object (PSO) / Rendering process.
	/*
		Final/ expected input:
			class Vertex
			{
				DirectX::XMFLOAT3 position{};
				DirectX::XMFLOAT2 textureCoordinates{};
			}
		// Correct input layout signature + Shader.
	*/

	Camera camera{};
	Mesh mesh{};
	FPSCounter counter{};
	Texture texture{};

	bool Init(HWND* pHandleToTheWindow, bool log = false)
	{
		m_pHandleToTheWindow = pHandleToTheWindow;
		bool status{ true };
		// 1.
		if (InitAdapter())
		{
			OutputDebugStringA("DEBUG::LOG::1. Device created!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::1. Device creation failed X!\n");
			status = false;
		}
		// 2.
		if (InitCommandQueue())
		{
			OutputDebugStringA("DEBUG::LOG::2. CommandQueue created!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::2. CommandQueue creation failed X!\n");
			status = false;
		}
		// 3.
		if (InitSwapChain())
		{
			OutputDebugStringA("DEBUG::LOG::3. SwapChain created!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::3. SwapChain creation failed X!\n");
			status = false;
		}
		// 4.
		if (InitRTVsFromSwapChain())
		{
			OutputDebugStringA("DEBUG::LOG::4. Saving pointers to the resources done!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::4. Saving pointers to the resources failed X!\n");
			status = false;
		}
		// 5.
		if (InitCommandAllocators())
		{
			OutputDebugStringA("DEBUG::LOG::5. Command allocators created!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::5. Command allocators creation failed X!\n");
			status = false;
		}
		// 6.
		if (InitCommandList())
		{
			OutputDebugStringA("DEBUG::LOG::6. Command list created!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::6. Command list creation failed X!\n");
			status = false;
		}
		// 7.
		if (InitFenceSystem())
		{
			OutputDebugStringA("DEBUG::LOG::7. Fence system created!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::7. Fence system creation failed X!\n");
			status = false;
		}
		// 8.
		if (InitRootSignature())
		{
			OutputDebugStringA("DEBUG::LOG::8. Root signature created!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::8. Root signature creation failed X!\n");
			status = false;
		}
		// 9.
		if (InitPipelineStateObject(L"VertexShader.hlsl", L"PixelShader.hlsl"))
		{
			OutputDebugStringA("DEBUG::LOG::9. PSO created!!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::9. PSO creation failed X!\n");
			status = false;
		}
		// 10
		if (InitDepthStencil())
		{
			OutputDebugStringA("DEBUG::LOG::10. Depth Stencil View created!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::10. Depth Stencil View creation failed X!\n");
			status = false;
		}

		camera.Init(*m_pDevice, m_Width, m_Height, FRAME_BUFFER_COUNT);

		LoadMeshes();
		LoadTextures();

		return status;
	}

	void LoadTextures()
	{
		BeginFrame();

		texture.Load(*m_pDevice, *m_pCommandList, L"image.jpg");

		EndFrame();
	}

	void LoadMeshes()
	{
		BeginFrame();

		std::vector<Vertex> data = {
			// front face
			{ -0.5f,  0.5f, -0.5f, 0.0f, 0.0f },
			{  0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
			{ -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
			{  0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

			// right side face
			{  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
			{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
			{  0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
			{  0.5f,  0.5f, -0.5f, 0.0f, 0.0f },

			// left side face
			{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
			{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
			{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
			{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

			// back face
			{  0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
			{ -0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
			{  0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
			{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f },

			// top face
			{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },
			{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
			{  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
			{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },

			// bottom face
			{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
			{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
			{  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
			{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f },
		};

		std::vector<DWORD> indices = {
			// ffront face
			0, 1, 2, // first triangle
			0, 3, 1, // second triangle

			// left face
			4, 5, 6, // first triangle
			4, 7, 5, // second triangle

			// right face
			8, 9, 10, // first triangle
			8, 11, 9, // second triangle

			// back face
			12, 13, 14, // first triangle
			12, 15, 13, // second triangle

			// top face
			16, 17, 18, // first triangle
			16, 19, 17, // second triangle

			// bottom face
			20, 21, 22, // first triangle
			20, 23, 21, // second triangle
		};

		mesh.LoadMesh(*m_pDevice, *m_pCommandList, &data, &indices);

		EndFrame();
	}

	void Render()
	{
		counter.GetFrameDelta();
		BeginFrame();
		Setup();

		/**************************** TODO *****************************/

		DirectX::XMFLOAT3 velocity{ 0.001f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 position = camera.getModelPosition();
		DirectX::XMFLOAT3 rotation = camera.getModelRotation();
		camera.setModelPosition(position.x + velocity.x, position.y + velocity.y, position.z + velocity.z);
		camera.setModelRotation(rotation.x + 0.01f, rotation.y + 0.01f, rotation.z + 0.01f);
		camera.Update(m_CurrentFrameIndex);

		m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);
		m_pCommandList->SetPipelineState(m_pPipelineStateObject);

		camera.Use(*m_pCommandList, m_CurrentFrameIndex); // b0
		texture.Use(*m_pCommandList);					  // t0
		mesh.Render(*m_pCommandList);

		/**************************************************************/

		End();
		EndFrame();
		counter.PrintFPS();
	}
};
#endif
/*

--------------VS--------------------

struct VS_INPUT
{
	float4 pos : POSITION;
	float2 texCoord: TEXCOORD;
};

struct VS_OUTPUT
{
	float4 pos: SV_POSITION;
	float2 texCoord: TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 wvpMat;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.pos = mul(input.pos, wvpMat);
	output.texCoord = input.texCoord;
	return output;
}

----------------PS-------------------

Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

struct VS_OUTPUT
{
	float4 pos: SV_POSITION;
	float2 texCoord: TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	// return interpolated color
	return t1.Sample(s1, input.texCoord);
}

*/