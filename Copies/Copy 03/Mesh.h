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

	void Render(ID3D12GraphicsCommandList& pCommandList)
	{
		pCommandList.IASetVertexBuffers(0, 1, &vertexBufferView);
		pCommandList.DrawInstanced(3, 1, 0, 0);
	}

	D3D12_VERTEX_BUFFER_VIEW LoadMesh(
		ID3D12Device& pDevice,
		ID3D12GraphicsCommandList& pCommandList,
		std::vector<Vertex>* vertices)
	{
		int numberOfElements = vertices->size();
		Vertex* data = new Vertex[numberOfElements];

		std::copy(vertices->begin(), vertices->end(), data);

		D3D12_SUBRESOURCE_DATA vertexData{};
		int verticesBufferSize{};

		verticesBufferSize = sizeof(Vertex) * numberOfElements;

		pDevice.CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(verticesBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer)
		);

		vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

		pDevice.CreateCommittedResource(
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

		UpdateSubresources(&pCommandList, vertexBuffer, vertexBufferUploadHeap, 0, 0, 1, &vertexData);

		pCommandList.ResourceBarrier(
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

