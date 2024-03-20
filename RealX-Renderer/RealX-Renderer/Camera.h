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

	DirectX::XMFLOAT4X4 m_ProjectionMatrix{};
	DirectX::XMFLOAT4X4 m_ViewMatrix{};

	DirectX::XMFLOAT4 m_CameraPosition{};
	DirectX::XMFLOAT4 m_CameraTarget{};
	DirectX::XMFLOAT4 m_CameraUp{};

	DirectX::XMFLOAT4X4 m_ModelMatrix{};
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

	DirectX::XMFLOAT3 getCameraPosition()
	{
		return DirectX::XMFLOAT3(m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z);
	}

	void setModelPosition(float x, float y, float z)
	{
		m_CameraPosition.x = x;
		m_CameraPosition.y = y;
		m_CameraPosition.z = z;
	}

	DirectX::XMFLOAT3 getModelPosition()
	{
		return DirectX::XMFLOAT3(m_ModelPosition.x, m_ModelPosition.y, m_ModelPosition.z);
	}
	
	void setModelRotation(float x, float y, float z)
	{
		m_ModelRotation.x = x;
		m_ModelRotation.y = y;
		m_ModelRotation.z = z;
	}

	DirectX::XMFLOAT3 getModelRotation()
	{
		return m_ModelRotation;
	}

	void setCameraPosition(float x, float y, float z)
	{
		m_CameraPosition.x = x;
		m_CameraPosition.y = y;
		m_CameraPosition.z = z;
	}

	bool Init(ID3D12Device& pDevice, int width, int height, int frameBufferCount)
	{
		m_Width = width;
		m_Height = height;
		m_FrameBufferCount = frameBufferCount;
		
		// Building projection and view matrix.
		DirectX::XMMATRIX tmpMat = DirectX::XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0), (float)width / (float)height, 0.1f, 1000.0f);
		DirectX::XMStoreFloat4x4(&m_ProjectionMatrix, tmpMat); // PROJECTION MATRIX

		// Setting starting camera state.
		m_CameraPosition = DirectX::XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
		m_CameraTarget = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		m_CameraUp = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
		m_WorldUp = DirectX::XMLoadFloat4(&m_CameraUp);

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

		return InitConstantBufferResources(pDevice);
	}

	DirectX::XMVECTOR m_CameraDirectionFront{};
	DirectX::XMVECTOR m_CameraDirectionRight{};
	DirectX::XMVECTOR m_CameraDirectionUp{};
	DirectX::XMVECTOR m_WorldUp{};

	float m_YawAngle{1.0f};
	float m_PitchAngle{1.0f};

	float getYawAngle()
	{
		return m_YawAngle;
	}

	float getPitchAngle()
	{
		return m_PitchAngle;
	}

	void setAngles(float yaw, float pitch)
	{
		setYawAngle(yaw);
		setPitchAngle(pitch);
	}

	void setYawAngle(float angle)
	{
		m_YawAngle = angle;
		char msg[300];
		sprintf_s(msg, "Yaw angle: %f \n", m_YawAngle);
		OutputDebugStringA(msg);
	}

	void setPitchAngle(float angle)
	{
		m_PitchAngle = angle;
		char msg[300];
		sprintf_s(msg, "Pitch angle: %f \n", m_PitchAngle);
		OutputDebugStringA(msg);
	}

	void Use(ID3D12GraphicsCommandList& pCommandList, int currentFrameIndex)
	{
		pCommandList.SetGraphicsRootConstantBufferView(0, m_pConstantBufferUploadHeaps[currentFrameIndex]->GetGPUVirtualAddress());
	}

	void UpdateCameraVectors()
	{
		// Updating camera directions/ vectors
		DirectX::XMFLOAT3 front{};
		front.x = cos(m_YawAngle) * cos(m_PitchAngle);
		front.y = sin(m_PitchAngle);
		front.z = sin(m_YawAngle)* cos(m_PitchAngle);
		m_CameraDirectionFront = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&front));
		m_CameraDirectionRight = DirectX::XMVector3Cross(m_CameraDirectionFront, m_WorldUp);
		m_CameraDirectionUp = DirectX::XMVector3Cross(m_CameraDirectionRight, m_CameraDirectionFront);
	}

	void UpdateViewMatrix()
	{
		// Updating camera vectorz.
		UpdateCameraVectors();

		// Building a view matrix.
		DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat4(&m_CameraPosition);
		DirectX::XMVECTOR cameraTarget = DirectX::XMVectorAdd(cameraPosition, m_CameraDirectionFront);
		DirectX::XMVECTOR cameraUp = m_CameraDirectionUp;
		DirectX::XMMATRIX tmpMat = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, cameraUp);
		DirectX::XMStoreFloat4x4(&m_ViewMatrix, tmpMat); // VIEW MATRIX
	}

	void UpdateModelMatrix()
	{
		// ROTATION:
		DirectX::XMMATRIX rotationXMatrix = DirectX::XMMatrixRotationX(m_ModelRotation.x);
		DirectX::XMMATRIX rotationYMatrix = DirectX::XMMatrixRotationY(m_ModelRotation.y);
		DirectX::XMMATRIX rotationZMatrix = DirectX::XMMatrixRotationZ(m_ModelRotation.z);
		DirectX::XMMATRIX rotationMatrix = rotationXMatrix * rotationYMatrix * rotationZMatrix;
		DirectX::XMStoreFloat4x4(&m_ModelRotationMatrix, rotationMatrix);

		// TRANSLATION:
		DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat4(&m_ModelPosition));
		
		// MERGING:
		DirectX::XMMATRIX worldMatrix = rotationMatrix * translationMatrix;
		DirectX::XMStoreFloat4x4(&m_ModelMatrix, worldMatrix);
	}

	void Update(int currentFrameIndex)
	{
		// 1. Updating Model matrix.
		UpdateModelMatrix();

		// 2. Updating View matrix.
		UpdateViewMatrix();

		// 3. Updating Projection matrix.
		// Not neccesary, constant matrix.

		// CONCLUSION:
		DirectX::XMMATRIX viewMatrix = DirectX::XMLoadFloat4x4(&m_ViewMatrix);
		DirectX::XMMATRIX projectionMatrix = DirectX::XMLoadFloat4x4(&m_ProjectionMatrix);
		DirectX::XMMATRIX worldViewProjectionMatrix = DirectX::XMLoadFloat4x4(&m_ModelMatrix) * viewMatrix * projectionMatrix;
		DirectX::XMMATRIX transposedWorldViewProjectionMatrix = DirectX::XMMatrixTranspose(worldViewProjectionMatrix);
		DirectX::XMStoreFloat4x4(&m_ConstantBufferPerObject.worldVIewProjectionMatrix, transposedWorldViewProjectionMatrix);
		memcpy(m_pConstantBufferViewGPUAddress[currentFrameIndex], &m_ConstantBufferPerObject, sizeof(m_ConstantBufferPerObject));
	}
};
#endif