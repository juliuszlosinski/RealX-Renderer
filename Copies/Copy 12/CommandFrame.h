#ifndef COMMAND_FRAME_H
#define COMMAND_FRAME_H

#include "stdfax.h"

class CommandFrame
{
	ID3D12CommandAllocator* m_pCommandAllocator{ nullptr };
	UINT64 m_FenceValue{};

public:
	void Wait(ID3D12Fence* pFence, HANDLE fenceEvent)
	{
		if (pFence->GetCompletedValue() < m_FenceValue)
		{
			pFence->SetEventOnCompletion(m_FenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		char msg[3000];
		sprintf_s(msg, "Frame[%d] finished waiting!\n", m_FenceValue);
		OutputDebugStringA(msg);
	}

	ID3D12CommandAllocator* getCommandAllocator()
	{
		return m_pCommandAllocator;
	}

	void setCommandAllocator(ID3D12CommandAllocator* pCommandAllocator)
	{
		m_pCommandAllocator = pCommandAllocator;
	}

	UINT64 getFenceValue()
	{
		return m_FenceValue;
	}

	void setFenceValue(UINT64 fenceValue)
	{
		m_FenceValue = fenceValue;
	}
};
#endif