#ifndef D3D12APP_H
#define D3D12APP_H

#include <string>
#include "stdfax.h"
#include "CommandFrame.h"
#include "Mesh.h"

class D3D12App
{
	static const int FRAME_BUFFER_COUNT { 3 };
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
	int frameIndex;
	int rtvDescriptorSize;
	int m_Width{ 600 };
	int m_Height{ 800 };
	HWND* m_pHandleToTheWindow{ nullptr };
	int m_CurrentFrameIndex;
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

	float RENDER_TARGET_CLEAR_COLOR[4]{ 0.0f, 0.2f, 0.4f, 1.0f };

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

		if(FAILED(hr))
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

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescriptor{};
		rootSignatureDescriptor.Init(0, nullptr, 
			0, nullptr, 
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		ID3DBlob* signature{ nullptr };
		hr = D3D12SerializeRootSignature(&rootSignatureDescriptor,
			D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr
		);

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
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};
		inputLayoutDescriptor.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		inputLayoutDescriptor.pInputElementDescs = inputLayout;

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

		m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		m_pCommandList->ClearRenderTargetView(rtvHandle, RENDER_TARGET_CLEAR_COLOR, 0, nullptr);
		m_pCommandList->RSSetViewports(1, &m_Viewport);
		m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);
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

	bool Init(HWND* pHandleToTheWindow, bool log=false)
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

		LoadMeshes();

		return true;
	}

	Mesh mesh{};

	// Input layout Signature + PSO + Shader = Rendering Process


	void LoadMeshes()
	{
		BeginFrame();

		std::vector<Vertex> data =
		{ 
			{ 0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
			{ 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
			{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f }
		};

		mesh.LoadMesh(*m_pDevice, *m_pCommandList, &data);
		
		EndFrame();
	}

	void Render()
	{
		BeginFrame();
		Setup();
		
		m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);
		m_pCommandList->SetPipelineState(m_pPipelineStateObject);

		m_pCommandList->IASetVertexBuffers(0, 1, &mesh.getVertexBufferView());
		m_pCommandList->DrawInstanced(3, 1, 0, 0);

		End();
		EndFrame();
	} 
}; 
#endif