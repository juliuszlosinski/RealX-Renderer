#ifndef CAMERA_H
#define CAMERA_H

#include "stdfax.h"

class Camera
{
private:
	struct ConstantBufferPerObject {
		DirectX::XMFLOAT4X4 worldVIewProjectionMatrix{};
	};

	int CONSTANT_BUFFER_PER_OBJECT_ALIGNED_SIZE = (sizeof(ConstantBufferPerObject) + 255) & ~255;

	ConstantBufferPerObject m_ConstantBufferPerObject{};

	ID3D12Resource* m_pConstantBufferUploadHeaps[3];
	UINT8* m_pConstantBufferViewGPUAddress[3];

	DirectX::XMFLOAT4X4 m_CameraProjectionMatrix{};
	DirectX::XMFLOAT4X4 m_CameraViewMatrix{};

	DirectX::XMFLOAT4 m_CameraPosition{};
	DirectX::XMFLOAT4 m_CameraTarget{};
	DirectX::XMFLOAT4 m_CameraUp{};

	DirectX::XMFLOAT4X4 m_ModelWorldMatrix{};
	DirectX::XMFLOAT4X4 m_ModelRotationMatrix{};
	DirectX::XMFLOAT4 m_ModelPosition{};

	int m_Width{};
	int m_Height{};
	int m_FrameBufferCount{};
	int* m_pCurrentFrameIndex;

	DirectX::XMFLOAT3 m_ModelRotation{ 0.0f, 0.0f, 0.0f };

	bool InitConstantBufferResources(ID3D12Device& pDevice)
	{
		HRESULT hr{};

		int* data;
		data = new int[10];

		for (int i = 0; i < m_FrameBufferCount; i++)
		{
			hr = pDevice.CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_pConstantBufferUploadHeaps[i])
			);

			ZeroMemory(&m_ConstantBufferPerObject, sizeof(m_ConstantBufferPerObject));

			CD3DX12_RANGE readRange(0, 0);

			hr = m_pConstantBufferUploadHeaps[i]->Map(
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
public:

	DirectX::XMFLOAT4 getPosition()
	{
		return m_CameraPosition;
	}

	void setPosition(float x, float y, float z)
	{
		m_CameraPosition.x = x;
		m_CameraPosition.y = y;
		m_CameraPosition.z = z;
	}

	DirectX::XMFLOAT3 getModelPosition()
	{
		return DirectX::XMFLOAT3(m_ModelPosition.x, m_ModelPosition.y, m_ModelPosition.z);
	}

	void setModelPosition(float x, float y, float z)
	{
		m_ModelPosition.x = x;
		m_ModelPosition.y = y;
		m_ModelPosition.z = z;
	}

	DirectX::XMFLOAT3 getModelRotation()
	{
		return m_ModelRotation;
	}

	void setModelRotation(float x, float y, float z)
	{
		m_ModelRotation.x = x;
		m_ModelRotation.y = y;
		m_ModelRotation.z = z;
	}

	bool Init(ID3D12Device& pDevice, int width, int height, int frameBufferCount)
	{
		m_Width = width;
		m_Height = height;
		m_FrameBufferCount = frameBufferCount;
		
		// Building projection and view matrix.
		DirectX::XMMATRIX tmpMat = DirectX::XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0), (float)width / (float)height, 0.1f, 1000.0f);
		DirectX::XMStoreFloat4x4(&m_CameraProjectionMatrix, tmpMat); // PROJECTION MATRIX

		// Setting starting camera state.
		m_CameraPosition = DirectX::XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
		m_CameraTarget = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		m_CameraUp = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

		// Building a view matrix.
		DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat4(&m_CameraPosition);
		DirectX::XMVECTOR cameraTarget = DirectX::XMLoadFloat4(&m_CameraTarget);
		DirectX::XMVECTOR cameraUp = DirectX::XMLoadFloat4(&m_CameraUp);
		tmpMat = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, cameraUp);
		DirectX::XMStoreFloat4x4(&m_CameraViewMatrix, tmpMat); // VIEW MATRIX

		// Setting starting cubes positions.
		m_ModelPosition = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR positionVector = DirectX::XMLoadFloat4(&m_ModelPosition);

		tmpMat = DirectX::XMMatrixTranslationFromVector(positionVector);
		DirectX::XMStoreFloat4x4(&m_ModelRotationMatrix, DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&m_ModelWorldMatrix, tmpMat); // WORLD MATRIX

		return InitConstantBufferResources(pDevice);
	}

	void Use(ID3D12GraphicsCommandList& pCommandList, int currentFrameIndex)
	{
		pCommandList.SetGraphicsRootConstantBufferView(0, m_pConstantBufferUploadHeaps[currentFrameIndex]->GetGPUVirtualAddress());
	}

	void Update(int currentFrameIndex)
	{
		// Building a view matrix.
		DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat4(&m_CameraPosition);
		DirectX::XMVECTOR cameraTarget = DirectX::XMLoadFloat4(&m_CameraTarget);
		DirectX::XMVECTOR cameraUp = DirectX::XMLoadFloat4(&m_CameraUp);
		DirectX::XMMATRIX tmpMat = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, cameraUp);
		DirectX::XMStoreFloat4x4(&m_CameraViewMatrix, tmpMat); // VIEW MATRIX

		DirectX::XMMATRIX rotationXMatrix = DirectX::XMMatrixRotationX(m_ModelRotation.x);
		DirectX::XMMATRIX rotationYMatrix = DirectX::XMMatrixRotationY(m_ModelRotation.y);
		DirectX::XMMATRIX rotationZMatrix = DirectX::XMMatrixRotationZ(m_ModelRotation.z);

		DirectX::XMMATRIX rotationMatrix = rotationXMatrix * rotationYMatrix * rotationZMatrix;
		DirectX::XMStoreFloat4x4(&m_ModelRotationMatrix, rotationMatrix);

		DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat4(&m_ModelPosition));
		DirectX::XMMATRIX worldMatrix = rotationMatrix * translationMatrix;

		DirectX::XMStoreFloat4x4(&m_ModelWorldMatrix, worldMatrix);

		// CONCLUSION:

		DirectX::XMMATRIX viewMatrix = DirectX::XMLoadFloat4x4(&m_CameraViewMatrix);
		DirectX::XMMATRIX projectionMatrix = DirectX::XMLoadFloat4x4(&m_CameraProjectionMatrix);
		DirectX::XMMATRIX worldViewProjectionMatrix = DirectX::XMLoadFloat4x4(&m_ModelWorldMatrix) * viewMatrix * projectionMatrix;
		DirectX::XMMATRIX transposedWorldViewProjectionMatrix = DirectX::XMMatrixTranspose(worldViewProjectionMatrix);
		DirectX::XMStoreFloat4x4(&m_ConstantBufferPerObject.worldVIewProjectionMatrix, transposedWorldViewProjectionMatrix);

		memcpy(m_pConstantBufferViewGPUAddress[currentFrameIndex], &m_ConstantBufferPerObject, sizeof(m_ConstantBufferPerObject));
	}
};
#endif