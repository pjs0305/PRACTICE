#pragma once
#include"GameTimer.h"

class GameFrameWork
{
private:
	HINSTANCE m_hInstance;
	HWND m_hwnd;

	int m_WndClientWidth;
	int m_WndClientHeight;

	// DXGI ���丮 �������̽��� ���� ������
	IDXGIFactory4* m_dxgiFactory; 


	// MSAA ���� ���ø��� Ȱ��ȭ�ϰ� ���� ���ø� ������ ����
	bool m_MSAA4xEnable = false;
	UINT m_MSAA4xQualityLevels = 0;



	// ���� ü�� �������̽��� ���� ������. �ַ� ���÷��̸� �����ϱ� ���� ���
	IDXGISwapChain3* m_dxgiSwapChain;

	// ���� ü���� �ĸ� ������ ����
	static const UINT m_SwapChainBuffers = 2;

	// ���� ���� ü���� �ĸ� ���� �ε���
	UINT m_SwapChainBufferIndex;



	// Direct3D ����̽� �������̽��� ���� ������. �ַ� ���ҽ��� �����ϱ� ���� ���
	ID3D12Device* m_d3dDevice;

	// ���� Ÿ�� ����, ������ �� �������̽� ������, ���� Ÿ�� ������ ������ ũ��
	ID3D12Resource* m_d3dRenderTargetBuffers[m_SwapChainBuffers];
	ID3D12DescriptorHeap* m_d3dRtvDescriptorHeap;
	UINT m_RtvDescriptorIncrementSize;

	// ����-���ٽ� ����, ������ �� �������̽� ������, ����-���ٽ� ������ ������ ũ��
	ID3D12Resource* m_d3dDepthStencilBuffer;
	ID3D12DescriptorHeap* m_d3dDsvDescriptorHeap;
	UINT m_DsvDescriptorIncrementSize;

	// ��� ť, ��� �Ҵ���, ��� ����Ʈ �������̽� ������
	ID3D12CommandQueue* m_d3dCommandQueue;
	ID3D12CommandAllocator* m_d3dCommandAllocator;
	ID3D12GraphicsCommandList* m_d3dCommandList;

	// �׷��Ƚ� ���������� ���� ��ü�� ���� �������̽� ������
	ID3D12PipelineState* m_d3dPipelineState;

	// �潺 �������̽� ������, �潺��, �̺�Ʈ �ڵ�
	ID3D12Fence* m_d3dFence;
	UINT64 m_FenceValue;
	HANDLE m_FenceEvent;

	// ����Ʈ�� ���� ����
#if defined(_DEBUG)
	ID3D12Debug* m_d3dDebugController;
#endif

	D3D12_VIEWPORT m_d3dViewport;
	D3D12_RECT m_d3dScissorRect;

public:
	GameFrameWork();
	~GameFrameWork();

	// �����ӿ�ũ�� �ʱ�ȭ�ϴ� �Լ�( �� ������ ������ ȣ�� )
	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	// ���� ü��, ����̽�, ������ ��, ��� ť/�Ҵ���/����Ʈ�� �����ϴ� �Լ�
	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateRtvAndDsvDescriptorHeaps();
	void CreateCommandQueueAndList();
	void CreateRenderTargetView();
	void CreateDepthStencilView();
	
	// �������� �޽��� ���� ��ü�� �����ϰ� �Ҹ��ϴ� �Լ�
	void BuildObjects();
	void ReleaseObjects();

	// �����ӿ�ũ�� �ٽ�( ����� �Է�, �ִϸ��̼�, ������ )�� �����ϴ� �Լ�
	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	// CPU�� GPU�� ����ȭ�ϴ� �Լ�
	void WaitForGPUComplete();

	// �������� �޽���( Ű����, ���콺 �Է� )�� ó���ϴ� �Լ�
	void OnProcessingMouseMessage(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam);

private:
	// ���� �����ӿ�ũ���� ����� Ÿ�̸�
	GameTimer m_GameTimer;

	// ������ ����Ʈ�� �� �������� Ÿ��Ʋ�� ����ϱ� ���� ���ڿ�
	_TCHAR m_pszFrameRate[50];

public:
	void OnResizeBackBuffers();
};

