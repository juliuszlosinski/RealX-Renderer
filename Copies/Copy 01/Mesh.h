#ifndef MESH_H
#define MESH

#include "stdfax.h"
#include "Vertex.h"

class Mesh
{
	ID3D12Resource* vertexBuffer{ nullptr };
	ID3D12Resource* vertexBufferUploadHeap{ nullptr };
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

public:
	D3D12_VERTEX_BUFFER_VIEW getVertexBufferView()
	{
		return vertexBufferView;
	}

	D3D12_VERTEX_BUFFER_VIEW LoadMesh(
		ID3D12Device& m_pDevice,
		ID3D12GraphicsCommandList& m_pCommandList,
		std::vector<Vertex>* vertices)
	{
		int numberOfElements = vertices->size();
		Vertex* data = new Vertex[numberOfElements];

		std::copy(vertices->begin(), vertices->end(), data);

		D3D12_SUBRESOURCE_DATA vertexData{};
		int verticesBufferSize{};

		verticesBufferSize = sizeof(Vertex) * numberOfElements;

		m_pDevice.CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(verticesBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer)
		);

		vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

		m_pDevice.CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(verticesBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBufferUploadHeap)
		);
		vertexBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

		vertexData.pData = reinterpret_cast<BYTE*>(data);
		vertexData.RowPitch = verticesBufferSize;
		vertexData.SlicePitch = verticesBufferSize;

		UpdateSubresources(&m_pCommandList, vertexBuffer, vertexBufferUploadHeap, 0, 0, 1, &vertexData);

		m_pCommandList.ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				vertexBuffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			)
		);

		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(Vertex);
		vertexBufferView.SizeInBytes = verticesBufferSize;

		return vertexBufferView;
	}
};

#endif

