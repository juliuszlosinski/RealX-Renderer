#ifndef D3D12APPDEFAULT_H
#define D3D12APPDEFAULT_H

#include <string>
#include "stdfax.h"
#include "CommandFrame.h"
#include "Mesh.h"
#include "FPSCounter.h"
#include "PrimitiveType.h"

class D3D12AppDefault
{
	// Parameters:
	int													m_OutputWidth{ 1920 };
	int												    m_OutputHeight{ 1080 };
	int													m_CurrentFrameIndex{ 0 };
	int													m_RenderTargetViewDescriptorSize{};
	float												m_MovingObjectSpeed{ 0.008f };
	float												m_ModelBarrier{ 1.90f };
	float												m_AspectRatio{};
	static const int								    FRAME_COUNT{ 2 };
	float												RENDER_TARGET_CLEAR_COLOR[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
	DXGI_FORMAT											m_BackBufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
	HWND												m_HandleToTheWindow{ nullptr };
	D3D12_VIEWPORT										m_Viewport{};
	D3D12_RECT											m_ScissorRect{};
	FPSCounter											m_FPSCounter;
	LPCWSTR												m_PathToVertexShader{ L"VertexShader.hlsl" };
	LPCWSTR												m_PathToPixelShader{ L"PixelShader.hlsl" };
	PrimitiveType										m_PrimitiveType{ PrimitiveType::Triangle };

	// Adapters, graphics device and factory:
	Microsoft::WRL::ComPtr<IDXGISwapChain3>				m_SwapChain{ nullptr };
	Microsoft::WRL::ComPtr<IDXGIFactory4>				m_DXGIFactory{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Device>				m_Device{ nullptr };

	// Command lists, Queues and allocators:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_CommandQueue{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_CommandAllocators[FRAME_COUNT];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_CommandList{ nullptr };
	
	// Resources:
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_RenderTargets[FRAME_COUNT];
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_DepthStencilBuffer{ nullptr };

	// Descriptor heaps:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_DepthStencilViewDescriptorHeap{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_RenderTargetViewsDescriptorHeap{ nullptr };

	// Pipeline state object and its root signature:
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			m_PipelineStateObject{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12RootSignature>			m_RootSignature{ nullptr };

	// Synchronization/ fence system:
	Microsoft::WRL::ComPtr<ID3D12Fence>		m_Fence{ nullptr };
	UINT64									m_FenceValues[FRAME_COUNT];
	CommandFrame							m_Frames[FRAME_COUNT];
	HANDLE									m_FenceEvent{ nullptr };
	int										m_CurrentFenceValue{ 0 };

	// Shader and cpu implementation of constant buffer data:
	struct ConstantBufferPerObject { DirectX::XMFLOAT4X4 worldVIewProjectionMatrix{}; };
	int CONSTANT_BUFFER_PER_OBJECT_ALIGNED_SIZE{ (sizeof(ConstantBufferPerObject) + 255) & ~255 };
	ConstantBufferPerObject				   m_ConstantBufferPerObject{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ConstantBufferUploadHeaps[FRAME_COUNT];
	UINT8*								   m_pConstantBufferViewGPUAddress[FRAME_COUNT];

	// Model, View and projection:
	DirectX::XMFLOAT4X4 m_ProjectionMatrix{};
	DirectX::XMFLOAT4X4 m_ViewMatrix{};
	DirectX::XMFLOAT4X4 m_ModelMatrix{};
	DirectX::XMFLOAT4X4 m_ModelRotationMatrix{};
	DirectX::XMFLOAT4	m_CameraPosition{};
	DirectX::XMFLOAT4	m_CameraTarget{};
	DirectX::XMFLOAT4	m_CameraUp{};
	DirectX::XMFLOAT4	m_ModelPosition{};

	// Meshes:
	Mesh mesh{};

	// 1. Initializing Adapter/ graphics device.
	bool InitAdapter()
	{
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

		D3D12_COMMAND_QUEUE_DESC commandQueueDescriptor{};

		hr = m_Device->CreateCommandQueue(&commandQueueDescriptor, IID_PPV_ARGS(&m_CommandQueue));

		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}

	// 3. Initializing Swap Chain.
	bool InitSwapChain()
	{
		DXGI_MODE_DESC backBufferDescriptor{};
		backBufferDescriptor.Width = m_OutputWidth;
		backBufferDescriptor.Height = m_OutputHeight;
		backBufferDescriptor.Format = m_BackBufferFormat;

		DXGI_SAMPLE_DESC sampleDescriptor{};
		sampleDescriptor.Count = 1;

		DXGI_SWAP_CHAIN_DESC swapChainDescriptor{};
		swapChainDescriptor.BufferCount = FRAME_COUNT;
		swapChainDescriptor.BufferDesc = backBufferDescriptor;
		swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDescriptor.OutputWindow = m_HandleToTheWindow;
		swapChainDescriptor.SampleDesc = sampleDescriptor;
		swapChainDescriptor.Windowed = true;

		IDXGISwapChain* tmpSwapChain{};

		m_DXGIFactory->CreateSwapChain(
			m_CommandQueue.Get(),
			&swapChainDescriptor,
			&tmpSwapChain
		);

		m_SwapChain = static_cast<IDXGISwapChain3*>(tmpSwapChain);

		m_CurrentFrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

		return true;
	}

	// 4. Getting render targets views from swap chain.
	bool InitRenderTargetViewsFromSwapChain()
	{
		HRESULT hr{};

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDescriptor{};
		rtvHeapDescriptor.NumDescriptors = FRAME_COUNT;
		rtvHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		hr = m_Device->CreateDescriptorHeap(&rtvHeapDescriptor, IID_PPV_ARGS(&m_RenderTargetViewsDescriptorHeap));

		if (FAILED(hr))
		{
			return false;
		}

		m_RenderTargetViewDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RenderTargetViewsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (int i = 0; i < FRAME_COUNT; i++)
		{
			// Getting pointers to the resources.
			hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i]));

			if (FAILED(hr))
			{
				return false;
			}

			m_Device->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, rtvHandle);

			rtvHandle.Offset(1, m_RenderTargetViewDescriptorSize);
		}

		return true;
	}

	// 5. Initializing command allocators.
	bool InitCommandAllocators()
	{
		HRESULT hr{};

		for (int i = 0; i < FRAME_COUNT; i++)
		{
			hr = m_Device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&m_CommandAllocators[i])
			);

			if (FAILED(hr))
			{
				return false;
			}
		}

		return true;
	}

	// 6. Initializing Graphics command list.
	bool InitCommandList()
	{
		HRESULT hr{};

		hr = m_Device->CreateCommandList
		(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_CommandAllocators[0].Get(),
			NULL,
			IID_PPV_ARGS(&m_CommandList)
		);

		if (FAILED(hr))
		{
			return false;
		}

		m_CommandList->Close();

		return true;
	}

	// 7. Initializing synchronization system/ fence system.
	bool InitFenceSystem()
	{
		HRESULT hr{};

		hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));

		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		if (m_FenceEvent == nullptr)
		{
			return false;
		}

		for (int i = 0; i < FRAME_COUNT; i++)
		{
			m_Frames[i].setCommandAllocator(m_CommandAllocators[i].Get());
			m_Frames[i].setFenceValue(0);
		}

		m_CommandList->Reset(m_Frames[0].getCommandAllocator(), nullptr);
		m_CommandList->Close();

		return true;
	}

	// 8. Initializing Root signatue for the PSO.
	bool InitRootSignature()
	{
		HRESULT hr{};

		D3D12_ROOT_DESCRIPTOR rootCBVDescriptor{};
		rootCBVDescriptor.RegisterSpace = 0;
		rootCBVDescriptor.ShaderRegister = 0;

		D3D12_ROOT_PARAMETER rootParameters[1];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor = rootCBVDescriptor;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescriptor{};
		rootSignatureDescriptor.Init(_countof(rootParameters),
			rootParameters,
			0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		);


		ID3DBlob* signature{ nullptr };
		hr = D3D12SerializeRootSignature(&rootSignatureDescriptor, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
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

	// 9. Initializing Pipeline State Object (PSO).
	bool InitPipelineStateObject()
	{
		HRESULT hr{};

		ID3DBlob* vertexShader{ nullptr };
		ID3DBlob* errorBuffer{ nullptr };

		hr = D3DCompileFromFile(m_PathToVertexShader,
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
		hr = D3DCompileFromFile(m_PathToPixelShader,
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
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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
		psoDesc.pRootSignature = m_RootSignature.Get();
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

		hr = m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineStateObject));

		if (FAILED(hr))
		{
			return false;
		}

		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = m_OutputWidth;
		m_Viewport.Height = m_OutputHeight;
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;

		m_ScissorRect.left = 0;
		m_ScissorRect.top = 0;
		m_ScissorRect.right = m_OutputWidth;
		m_ScissorRect.bottom = m_OutputHeight;

		return true;
	}

	// 10. Initializing depth stencil resources and views.
	bool InitDepthStencil()
	{
		HRESULT hr{};

		D3D12_DESCRIPTOR_HEAP_DESC depthHeapDescriptor{};
		depthHeapDescriptor.NumDescriptors = 1;
		depthHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		depthHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		hr = m_Device->CreateDescriptorHeap(
			&depthHeapDescriptor,
			IID_PPV_ARGS(&m_DepthStencilViewDescriptorHeap)
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

		m_Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_D32_FLOAT,
				m_OutputWidth,
				m_OutputHeight,
				1, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
			),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&m_DepthStencilBuffer)
		);
		m_DepthStencilViewDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

		m_Device->CreateDepthStencilView(
			m_DepthStencilBuffer.Get(), &depthStencilViewDescriptor,
			m_DepthStencilViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
		);

		return true;
	}

	// 11. Initializing constant buffers.
	bool InitConstantBuffer()
	{
		HRESULT hr{};

		for (int i = 0; i < FRAME_COUNT; i++)
		{
			hr = m_Device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_ConstantBufferUploadHeaps[i])
			);

			ZeroMemory(&m_ConstantBufferPerObject, sizeof(m_ConstantBufferPerObject));

			CD3DX12_RANGE readRange(0, 0);

			hr = m_ConstantBufferUploadHeaps[i]->Map(
				0, &readRange, reinterpret_cast<void**>(&m_pConstantBufferViewGPUAddress[i])
			);
			memcpy(m_pConstantBufferViewGPUAddress[i],
				&m_ConstantBufferPerObject, sizeof(m_ConstantBufferPerObject)
			);
			memcpy(
				m_pConstantBufferViewGPUAddress[i] + CONSTANT_BUFFER_PER_OBJECT_ALIGNED_SIZE,
				&m_ConstantBufferPerObject, sizeof(m_ConstantBufferPerObject)
			);
		}

		return true;
	}

	// 12. Initializing model, view and projection matrices.
	bool InitModelViewProjectionMatrices()
	{
		// Building projection and view matrix.
		DirectX::XMMATRIX tmpMat = DirectX::XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0), (float)m_OutputWidth / (float)m_OutputHeight, 0.1f, 10.0f);
		DirectX::XMStoreFloat4x4(&m_ProjectionMatrix, tmpMat); // PROJECTION MATRIX

		// Setting starting camera state.
		m_CameraPosition = DirectX::XMFLOAT4(0.0f, 0.0f, -1.0f, 10.0f);
		m_CameraTarget = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		m_CameraUp = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

		// Building a view matrix.
		DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat4(&m_CameraPosition);
		DirectX::XMVECTOR cameraTarget = DirectX::XMLoadFloat4(&m_CameraTarget);
		DirectX::XMVECTOR cameraUp = DirectX::XMLoadFloat4(&m_CameraUp);
		tmpMat = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, cameraUp);
		DirectX::XMStoreFloat4x4(&m_ViewMatrix, tmpMat); // VIEW MATRIX

		// Setting starting cubes positions.
		m_ModelPosition = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR positionVector = DirectX::XMLoadFloat4(&m_ModelPosition);

		tmpMat = DirectX::XMMatrixTranslationFromVector(positionVector);
		DirectX::XMStoreFloat4x4(&m_ModelRotationMatrix, DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&m_ModelMatrix, tmpMat); // WORLD MATRIX

		return true;
	}

	// 13. Updating model, view and projection matrices.
	void UpdateModelViewProjectionMatrices()
	{
		// CUBE:
		m_ModelPosition.x += m_MovingObjectSpeed;

		if(m_ModelPosition.x > m_ModelBarrier)
		{
			m_ModelPosition.x = -m_ModelBarrier;
		}

		DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat4(&m_ModelPosition));
		DirectX::XMStoreFloat4x4(&m_ModelMatrix, translationMatrix);

		// CONCLUSION:
		DirectX::XMMATRIX viewMatrix = DirectX::XMLoadFloat4x4(&m_ViewMatrix);
		DirectX::XMMATRIX projectionMatrix = DirectX::XMLoadFloat4x4(&m_ProjectionMatrix);
		DirectX::XMMATRIX worldViewProjectionMatrix = DirectX::XMLoadFloat4x4(&m_ModelMatrix) * viewMatrix * projectionMatrix;
		DirectX::XMMATRIX transposedWorldViewProjectionMatrix = DirectX::XMMatrixTranspose(worldViewProjectionMatrix);
		DirectX::XMStoreFloat4x4(&m_ConstantBufferPerObject.worldVIewProjectionMatrix, transposedWorldViewProjectionMatrix);

		memcpy(m_pConstantBufferViewGPUAddress[m_CurrentFrameIndex], &m_ConstantBufferPerObject, sizeof(m_ConstantBufferPerObject));
	}

	// Starting frame.
	void BeginFrame()
	{
		CommandFrame& currentFrame = m_Frames[m_CurrentFrameIndex];
		currentFrame.Wait(m_Fence.Get(), m_FenceEvent);
		currentFrame.getCommandAllocator()->Reset();
		m_CommandList->Reset(currentFrame.getCommandAllocator(), nullptr);
	}

	// Ending frame.
	void EndFrame()
	{
		m_CommandList->Close();
		ID3D12CommandList* const commandLists[] = { m_CommandList.Get()};
		m_CommandQueue->ExecuteCommandLists(_countof(commandLists), &commandLists[0]);

		m_CurrentFenceValue++;
		CommandFrame& currentFrame = m_Frames[m_CurrentFrameIndex];
		currentFrame.setFenceValue(m_CurrentFenceValue);
		m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFenceValue);

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % FRAME_COUNT;
		m_SwapChain->Present(0, 0);
	}

	// Setting up commands.
	void Setup()
	{
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_RenderTargets[m_CurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		m_CommandList->ResourceBarrier(1, &transition);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RenderTargetViewsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentFrameIndex, m_RenderTargetViewDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_DepthStencilViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		m_CommandList->OMSetRenderTargets(1, &rtvHandle, 1, &dsvHandle);
		m_CommandList->ClearRenderTargetView(rtvHandle, RENDER_TARGET_CLEAR_COLOR, 0, nullptr);
		m_CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		m_CommandList->RSSetViewports(1, &m_Viewport);
		m_CommandList->RSSetScissorRects(1, &m_ScissorRect);
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// Ending command.
	void End()
	{
		CD3DX12_RESOURCE_BARRIER transition02 = CD3DX12_RESOURCE_BARRIER::Transition(
			m_RenderTargets[m_CurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
		);
		m_CommandList->ResourceBarrier(1, &transition02);
	}

	// Loading meshes.
	void LoadMeshes()
	{
		BeginFrame();

		std::vector<Vertex> data{};
		std::vector<DWORD> indices{};

		switch (m_PrimitiveType)
		{
		case PrimitiveType::Triangle:
			data.push_back({ {0.0f, 0.25f * m_AspectRatio, 0.95f}, {1.0f, 0.0f, 0.0f, 1.0f} });
			data.push_back({ {0.25f, -0.25f * m_AspectRatio, 0.95f}, {0.0f, 1.0f, 0.0f, 1.0f} });
			data.push_back({ {-0.25f, -0.25f * m_AspectRatio, 0.95f}, {0.0f, 0.0f, 1.0f, 1.0f} });
			indices.push_back(0);
			indices.push_back(1);
			indices.push_back(2);
			break;
		case PrimitiveType::Rectangle:
			data.push_back({ {-0.25f, -0.25f * m_AspectRatio, 0.95f}, {1.0f, 0.0f, 0.0f, 1.0f} });  // Left down
			data.push_back({ {-0.25f, 0.25f * m_AspectRatio, 0.95f}, {0.0f, 0.0f, 1.0f, 1.0f } });  // Left up
			data.push_back({ {0.25f, 0.25f * m_AspectRatio, 0.95f}, {0.0f, 1.0f, 0.0f, 1.0f} });    // Right up
			data.push_back({ {0.25f, -0.25f * m_AspectRatio, 0.95f}, {0.25f, 0.0f, 0.25f, 1.0f} }); // Right down
			indices.push_back(0);
			indices.push_back(1);
			indices.push_back(2);
			indices.push_back(2);
			indices.push_back(3);
			indices.push_back(0);
			break;
		case PrimitiveType::Square:
			data.push_back({ {-0.25f * m_AspectRatio, -0.25f * m_AspectRatio, 0.95f}, {1.0f, 0.0f, 0.0f, 1.0f} });  // Left down
			data.push_back({ {-0.25f * m_AspectRatio, 0.25f * m_AspectRatio, 0.95f}, {0.0f, 0.0f, 1.0f, 1.0f } });  // Left up
			data.push_back({ {0.25f * m_AspectRatio, 0.25f * m_AspectRatio, 0.95f}, {0.0f, 1.0f, 0.0f, 1.0f} });    // Right up
			data.push_back({ {0.25f * m_AspectRatio, -0.25f * m_AspectRatio, 0.95f}, {0.25f, 0.0f, 0.25f, 1.0f} }); // Right down
			indices.push_back(0);
			indices.push_back(1);
			indices.push_back(2);
			indices.push_back(2);
			indices.push_back(3);
			indices.push_back(0);
			break;
		}

		mesh.LoadMesh(*m_Device.Get(), *m_CommandList.Get(), &data, &indices);

		EndFrame();
	}

public:
	// Custom constructor.
	D3D12AppDefault(int width, int height, PrimitiveType primitiveType)
	{
		m_OutputWidth = width;
		m_OutputHeight = height;
		m_AspectRatio = static_cast<float>(m_OutputWidth) / static_cast<float>(m_OutputHeight);
		m_PrimitiveType = primitiveType;

		switch (m_PrimitiveType)
		{
		case PrimitiveType::Triangle:
			m_FPSCounter = FPSCounter("D3D12_App_Default-Triangle-fps_counter.csv");
			break;
		case PrimitiveType::Rectangle:
			m_FPSCounter = FPSCounter("D3D12_App_Default-Rectangle-fps_counter.csv");
			break;
		case PrimitiveType::Square:
			m_FPSCounter = FPSCounter("D3D12_App_Default-Square-fps_counter.csv");
			break;
		}
	}

	// Default constructor.
	D3D12AppDefault()
	{
		m_AspectRatio = static_cast<float>(m_OutputWidth) / static_cast<float>(m_OutputHeight);

		switch (m_PrimitiveType)
		{
		case PrimitiveType::Triangle:
			m_FPSCounter = FPSCounter("D3D12_App_Default-Triangle-fps_counter.csv");
			break;
		case PrimitiveType::Rectangle:
			m_FPSCounter = FPSCounter("D3D12_App_Default-Rectangle-fps_counter.csv");
			break;
		case PrimitiveType::Square:
			m_FPSCounter = FPSCounter("D3D12_App_Default-Square-fps_counter.csv");
			break;
		}
	}

	// Initializing application.
	bool Init(HWND pHandleToTheWindow)
	{
		m_HandleToTheWindow = pHandleToTheWindow;
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
		if (InitRenderTargetViewsFromSwapChain())
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
		if (InitPipelineStateObject())
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
		// 12
		if (InitConstantBuffer())
		{
			OutputDebugStringA("DEBUG::LOG::11. Initialization of Constant buffer resources done\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::11. Initialization of Constant buffer creation failed X!\n");
			status = false;
		}
		// 11
		if (InitModelViewProjectionMatrices())
		{
			OutputDebugStringA("DEBUG::LOG::11. World View Projections matrices created!\n");
		}
		else
		{
			OutputDebugStringA("DEBUG::LOG::11. World View Projection matrices creation failed X!\n");
			status = false;
		}

		LoadMeshes();

		return status;
	}

	// Updating command lists.
	void Update()
	{
		UpdateModelViewProjectionMatrices();
	}

	// Rendering primitives with default D3D12 technology.
	void Render()
	{
		m_FPSCounter.GetFrameDelta();

		BeginFrame();
		Setup();

		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
		m_CommandList->SetGraphicsRootConstantBufferView(0, m_ConstantBufferUploadHeaps[m_CurrentFrameIndex]->GetGPUVirtualAddress());
		m_CommandList->SetPipelineState(m_PipelineStateObject.Get());

		mesh.Render(*m_CommandList.Get());

		End();
		EndFrame();

		m_FPSCounter.PrintFPS();
	}

	// Stop working.
	void Exit()
	{

	}
};
#endif