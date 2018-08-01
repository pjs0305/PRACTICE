#pragma once
#include"GameTimer.h"

class GameFrameWork
{
private:
	HINSTANCE m_hInstance;
	HWND m_hwnd;

	int m_WndClientWidth;
	int m_WndClientHeight;

	// DXGI 팩토리 인터페이스에 대한 포인터
	IDXGIFactory4* m_dxgiFactory; 


	// MSAA 다중 샘플링을 활성화하고 다중 샘플링 레벨을 설정
	bool m_MSAA4xEnable = false;
	UINT m_MSAA4xQualityLevels = 0;



	// 스왑 체인 인터페이스에 대한 포인터. 주로 디스플레이를 제어하기 위해 사용
	IDXGISwapChain3* m_dxgiSwapChain;

	// 스왑 체인의 후면 버퍼의 개수
	static const UINT m_SwapChainBuffers = 2;

	// 현재 스왑 체인의 후면 버퍼 인덱스
	UINT m_SwapChainBufferIndex;



	// Direct3D 디바이스 인터페이스에 대한 포인터. 주로 리소스를 생성하기 위해 사용
	ID3D12Device* m_d3dDevice;

	// 렌더 타겟 버퍼, 서술자 힙 인터페이스 포인터, 렌더 타겟 서술자 원소의 크기
	ID3D12Resource* m_d3dRenderTargetBuffers[m_SwapChainBuffers];
	ID3D12DescriptorHeap* m_d3dRtvDescriptorHeap;
	UINT m_RtvDescriptorIncrementSize;

	// 깊이-스텐실 버퍼, 서술자 힙 인터페이스 포인터, 깊이-스텐실 서술자 원소의 크기
	ID3D12Resource* m_d3dDepthStencilBuffer;
	ID3D12DescriptorHeap* m_d3dDsvDescriptorHeap;
	UINT m_DsvDescriptorIncrementSize;

	// 명령 큐, 명령 할당자, 명령 리스트 인터페이스 포인터
	ID3D12CommandQueue* m_d3dCommandQueue;
	ID3D12CommandAllocator* m_d3dCommandAllocator;
	ID3D12GraphicsCommandList* m_d3dCommandList;

	// 그래픽스 파이프라인 상태 객체에 대한 인터페이스 포인터
	ID3D12PipelineState* m_d3dPipelineState;

	// 펜스 인터페이스 포인터, 펜스값, 이벤트 핸들
	ID3D12Fence* m_d3dFence;
	UINT64 m_FenceValue;
	HANDLE m_FenceEvent;

	// 뷰포트와 시저 영역
#if defined(_DEBUG)
	ID3D12Debug* m_d3dDebugController;
#endif

	D3D12_VIEWPORT m_d3dViewport;
	D3D12_RECT m_d3dScissorRect;

public:
	GameFrameWork();
	~GameFrameWork();

	// 프레임워크를 초기화하는 함수( 주 윈도우 생성시 호출 )
	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	// 스왑 체인, 디바이스, 서술자 힙, 명령 큐/할당자/리스트를 생성하는 함수
	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateRtvAndDsvDescriptorHeaps();
	void CreateCommandQueueAndList();
	void CreateRenderTargetView();
	void CreateDepthStencilView();
	
	// 렌더링할 메쉬와 게임 객체를 생성하고 소멸하는 함수
	void BuildObjects();
	void ReleaseObjects();

	// 프레임워크의 핵심( 사용자 입력, 애니메이션, 렌더링 )을 구성하는 함수
	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	// CPU와 GPU를 동기화하는 함수
	void WaitForGPUComplete();

	// 윈도우의 메시지( 키보드, 마우스 입력 )를 처리하는 함수
	void OnProcessingMouseMessage(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam);

private:
	// 게임 프레임워크에서 사용할 타이머
	GameTimer m_GameTimer;

	// 프레임 레이트를 주 윈도우의 타이틀에 출력하기 위한 문자열
	_TCHAR m_pszFrameRate[50];

public:
	void OnResizeBackBuffers();
};

