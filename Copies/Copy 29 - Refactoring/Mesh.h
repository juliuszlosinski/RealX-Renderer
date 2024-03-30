#ifndef MESH_H
#define MESH

#include "stdfax.h"
#include "Vertex.h"

class Mesh
{
	ID3D12Resource* m_pVertexBuffer{ nullptr };
	ID3D12Resource* m_pVertexBufferUploadHeap{ nullptr };

	ID3D12Resource* m_pIndexBuffer{ nullptr };
	ID3D12Resource* m_pIndexBufferUploadHeap{ nullptr };

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView{};

	D3D12_SUBRESOURCE_DATA m_VerticesData{};
	int m_VerticesBufferSize{};
	int m_NumberOfVertices{};

	D3D12_SUBRESOURCE_DATA m_IndicesData{};
	int m_IndicesBufferSize{};
	int m_NumberOfIndices{};

public:
	Mesh()
	{
		// Nothing to do.
	}

	void Render(ID3D12GraphicsCommandList& pCommandList)
	{
		pCommandList.IASetVertexBuffers(0, 1, &m_VertexBufferView);
		pCommandList.IASetIndexBuffer(&m_IndexBufferView);
		pCommandList.DrawIndexedInstanced(m_NumberOfIndices, 1, 0, 0, 0);
	}

	void LoadMesh(
		ID3D12Device& pDevice,
		ID3D12GraphicsCommandList& pCommandList,
		std::vector<Vertex>* vertices,
		std::vector<DWORD>* indices)
	{

		// ************** VERTEX BUFFER *************

		m_NumberOfVertices = vertices->size();
		Vertex* data = new Vertex[m_NumberOfVertices];
		std::copy(vertices->begin(), vertices->end(), data);

		m_VerticesBufferSize = sizeof(Vertex) * m_NumberOfVertices;

		pDevice.CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_VerticesBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_pVertexBuffer)
		);

		m_pVertexBuffer->SetName(L"Vertex Buffer Resource Heap");

		pDevice.CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_VerticesBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pVertexBufferUploadHeap)
		);
		m_pVertexBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

		m_VerticesData.pData = reinterpret_cast<BYTE*>(data);
		m_VerticesData.RowPitch = m_VerticesBufferSize;
		m_VerticesData.SlicePitch = m_VerticesBufferSize;

		UpdateSubresources(&pCommandList, m_pVertexBuffer, m_pVertexBufferUploadHeap, 0, 0, 1, &m_VerticesData);

		pCommandList.ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				m_pVertexBuffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			)
		);

		m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
		m_VertexBufferView.StrideInBytes = sizeof(Vertex);
		m_VertexBufferView.SizeInBytes = m_VerticesBufferSize;

		// ********** INDEX BUFFER ***********

		m_NumberOfIndices = indices->size();
		DWORD* c_Indices = new DWORD[m_NumberOfIndices];
		std::copy(indices->begin(), indices->end(), c_Indices);

		m_IndicesBufferSize = sizeof(DWORD) * m_NumberOfIndices;

		pDevice.CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_IndicesBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_pIndexBuffer)
		);

		m_pIndexBuffer->SetName(L"Index Buffer Default Heap");

		pDevice.CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_IndicesBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pIndexBufferUploadHeap)
		);
		m_pIndexBufferUploadHeap->SetName(L"Index Buffer Upload Heap");

		m_IndicesData.pData = reinterpret_cast<BYTE*>(c_Indices);
		m_IndicesData.RowPitch = m_IndicesBufferSize;
		m_IndicesData.SlicePitch = m_IndicesBufferSize;

		UpdateSubresources(&pCommandList, m_pIndexBuffer, m_pIndexBufferUploadHeap, 0, 0, 1, &m_IndicesData);

		m_IndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
		m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_IndexBufferView.SizeInBytes = m_IndicesBufferSize;
	}
};
#endif