#include "stdafx.h"
#include "GameFrameWork.h"


GameFrameWork::GameFrameWork()
{
	m_dxgiFactory = NULL;
	m_dxgiSwapChain = NULL;
	m_d3dDevice = NULL;

	m_d3dCommandAllocator = NULL;
	m_d3dCommandQueue = NULL;
	m_d3dCommandList = NULL;

	m_d3dPipelineState = NULL;

	for (int i = 0; i < m_SwapChainBuffers; i++) m_d3dRenderTargetBuffers[i] = NULL;
	m_d3dRtvDescriptorHeap = NULL;
	m_RtvDescriptorIncrementSize = 0;

	m_d3dDepthStencilBuffer = NULL;
	m_d3dDsvDescriptorHeap = NULL;
	m_DsvDescriptorIncrementSize = 0;

	m_SwapChainBufferIndex = 0;

	m_FenceEvent = NULL;
	m_d3dFence = NULL;
	m_FenceValue = 0;

	m_WndClientWidth = FRAME_BUFFER_WIDTH;
	m_WndClientWidth = FRAME_BUFFER_HEIGHT;

	_tcscpy_s(m_pszFrameRate, _T("Practice ("));
}

bool GameFrameWork::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
// 응용 프로그램이 실행되어 주 윈도우가 생성되면 호출되는 함수.
{
	m_hInstance = hInstance;
	m_hwnd = hMainWnd;

	// Direct3D 디바이스, 명령 큐와 명령 리스트, 스왑 체인 등을 생성하는 함수를 호출
	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	// 렌더링할 객체를 생성
	BuildObjects();

	return TRUE;
}

void GameFrameWork::OnDestroy()
{
	ReleaseObjects();

	::CloseHandle(m_FenceEvent);

#if defined(_DEBUG)
	if (m_d3dDebugController) m_d3dDebugController->Release();
#endif

	for (int i = 0; i < m_SwapChainBuffers; i++)
		if (m_d3dRenderTargetBuffers[i])
			m_d3dRenderTargetBuffers[i]->Release();
	if (m_d3dRtvDescriptorHeap) m_d3dRtvDescriptorHeap->Release();

	if (m_d3dDepthStencilBuffer) m_d3dDepthStencilBuffer->Release();
	if (m_d3dDsvDescriptorHeap) m_d3dDsvDescriptorHeap->Release();

	if (m_d3dCommandAllocator) m_d3dCommandAllocator->Release();
	if (m_d3dCommandQueue) m_d3dCommandQueue->Release();
	if (m_d3dCommandList) m_d3dCommandList->Release();

	if (m_d3dFence) m_d3dFence->Release();

	m_dxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_dxgiSwapChain) m_dxgiSwapChain->Release();
	if (m_d3dDevice) m_d3dDevice->Release();
	if (m_dxgiFactory) m_dxgiFactory->Release();
}

void GameFrameWork::OnResizeBackBuffers()
{
	WaitForGPUComplete();

	m_d3dCommandList->Reset(m_d3dCommandAllocator, NULL);
	
	for (int i = 0; i < m_SwapChainBuffers; i++)
	{
		if (m_d3dRenderTargetBuffers[i])
		{
			m_d3dRenderTargetBuffers[i]->Release();
		}
	}
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_dxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_dxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	m_SwapChainBufferIndex = 0;
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_dxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_dxgiSwapChain->ResizeBuffers(m_SwapChainBuffers, m_WndClientWidth, m_WndClientHeight,
		dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);
	m_SwapChainBufferIndex = 0;
#endif
	CreateRenderTargetView();
	CreateDepthStencilView();

	m_d3dCommandList->Close();

	ID3D12CommandList* d3dCommandLists[] = { m_d3dCommandList };
	m_d3dCommandQueue->ExecuteCommandLists(1, d3dCommandLists);

	WaitForGPUComplete();
}

void GameFrameWork::CreateSwapChain()
{
	// 스왑 체인할 화면 크기 설정
	RECT rcClient;
	::GetClientRect(m_hwnd, &rcClient);
	m_WndClientWidth = rcClient.right - rcClient.left;
	m_WndClientHeight = rcClient.bottom - rcClient.top;

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferDesc.Width = m_WndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_WndClientHeight;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.SampleDesc.Count = (m_MSAA4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_MSAA4xEnable) ? (m_MSAA4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
	dxgiSwapChainDesc.BufferCount = m_SwapChainBuffers;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스와핑 후 값 삭제
	dxgiSwapChainDesc.OutputWindow = m_hwnd;
	dxgiSwapChainDesc.Windowed = TRUE;

#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS;
	// 전체화면 모드에서 바탕화면의 해상도를 바꾸지 않고 후면버퍼의 크기를 바탕화면 크기로 변경
	dxgiSwapChainDesc.Flags = 0;
#else
	// 전체화면 모드에서 바탕화면의 해상도를 스왑체인(후면버퍼)의 크기에 맞게 변경
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif


	// 스왑체인 생성
	HRESULT hResult = m_dxgiFactory->CreateSwapChain(m_d3dCommandQueue,
		&dxgiSwapChainDesc, (IDXGISwapChain**)&m_dxgiSwapChain);

	// Alt + Enter키로 화면 크기 변환 비활성화
	m_dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);

	// 스왑체인의 현재 후면버퍼 인덱스를 저장
	m_SwapChainBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
}

void GameFrameWork::CreateDirect3DDevice()
{
#if defined(_DEBUG)
	D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&m_d3dDebugController);
	m_d3dDebugController->EnableDebugLayer();
#endif

	// DXGI 팩토리 생성
	::CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&m_dxgiFactory);

	IDXGIAdapter1* d3dAdapter = NULL;
	// 모든 하드웨어 어댑터에 대하여 특성 레벨 12.0을 지원하는 하드웨어 디바이스를 생성
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(i, &d3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		d3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(d3dAdapter, D3D_FEATURE_LEVEL_12_0, 
			_uuidof(ID3D12Device), (void**)&m_d3dDevice))) break;
	}

	// 특성 레벨 12.0을 지원하는 하드웨어 디바이스를 생성할 수 없으면 WARP 디바이스를 생성
	if (!d3dAdapter)
	{
		m_dxgiFactory->EnumWarpAdapter(_uuidof(IDXGIFactory4), (void**)&d3dAdapter);
		D3D12CreateDevice(d3dAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device),
			(void**)&m_d3dDevice);
	}

	// 디바이스가 지원하는 다중 샘플의 품질 수준을 확인
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMSAAQualityLevels;
	d3dMSAAQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMSAAQualityLevels.SampleCount = 4; // MSAA 4x 다중 샘플링
	d3dMSAAQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMSAAQualityLevels.NumQualityLevels = 0;
	m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&d3dMSAAQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_MSAA4xQualityLevels = d3dMSAAQualityLevels.NumQualityLevels;

	// 다중 샘플의 품질 수준이 1보다 크면 다중 샘플링을 활성
	m_MSAA4xEnable = (m_MSAA4xQualityLevels > 1) ? true : false;

	// 펜스를 생성하고 펜스 값을 0으로 설정
	m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence),
		(void**)&m_d3dFence);
	m_FenceValue = 0;

	// 펜스와 동기화를 위한 이벤트 객체를 생성(이벤트 객체의 초기값은 False)
	// 이벤트가 실행되면(Signal) 이벤트의 값이 자동적으로 False가 되도록 생성
	m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// 뷰포트를 주 윈도우의 클라이언트 영역 전체로 설정
	m_d3dViewport.TopLeftX = 0;
	m_d3dViewport.TopLeftY = 0;
	m_d3dViewport.Width = static_cast<float>(m_WndClientWidth);
	m_d3dViewport.Height = static_cast<float>(m_WndClientHeight);
	m_d3dViewport.MinDepth = 0.f;
	m_d3dViewport.MaxDepth = 1.f;

	// 씨저 사각형을 주 윈도우의 클라이언트 영역 전체로 설정
	m_d3dScissorRect = { 0, 0, m_WndClientWidth, m_WndClientHeight };

	if (d3dAdapter) d3dAdapter->Release();
}

void GameFrameWork::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// 직접 명령 큐를 생성
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hResult = m_d3dDevice->CreateCommandQueue(&d3dCommandQueueDesc,
		__uuidof(ID3D12CommandQueue), (void**)&m_d3dCommandQueue);
	// 직접 명령 할당자를 생성
	hResult = m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		__uuidof(ID3D12CommandAllocator), (void**)&m_d3dCommandAllocator);
	// 직접 명령 리스트를 생성
	hResult = m_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_d3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList),
		(void**)&m_d3dCommandList);
	
	// 명령 리스트는 열린 상태로 생성되므로 닫힌 상태로 만든다.
	hResult = m_d3dCommandList->Close();
}

void GameFrameWork::CreateRtvAndDsvDescriptorHeaps()
{
	// 렌더 타겟 서술자 힙(서술자의 갯수는 스왑체인 버퍼의 갯수)을 생성
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_SwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_d3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap), (void**)&m_d3dRtvDescriptorHeap);
	// 렌더 타겟 서술자 힙의 원소 크기를 저장
	m_RtvDescriptorIncrementSize =
		m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 깊이-스텐실 서술자 힙(서술자의 갯수는 1)을 생성
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_d3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap), (void**)&m_d3dDsvDescriptorHeap);

	// 깊이-스텐실 서술자 힙의 원소 크기를 저장
	m_DsvDescriptorIncrementSize =
		m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void GameFrameWork::CreateRenderTargetView()
{
	// 스왑 체인의 각 후면 버퍼에 대한 렌더 타겟 뷰를 생성
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle =
		m_d3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_SwapChainBuffers; i++)
	{
		HRESULT hResult = m_dxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource),
			(void**)&m_d3dRenderTargetBuffers[i]);
		m_d3dDevice->CreateRenderTargetView(m_d3dRenderTargetBuffers[i], NULL,
			d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += m_RtvDescriptorIncrementSize;
	}

}

void GameFrameWork::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_WndClientWidth;
	d3dResourceDesc.Height = m_WndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_MSAA4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_MSAA4xEnable) ? (m_MSAA4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	// 깊이-스텐실 버퍼를 생성
	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.f;
	d3dClearValue.DepthStencil.Stencil = 0;
	m_d3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue,
		__uuidof(ID3D12Resource), (void**)&m_d3dDepthStencilBuffer);

	// 깊이-스텐실 버퍼 뷰를 생성
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle =
		m_d3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_d3dDevice->CreateDepthStencilView(m_d3dDepthStencilBuffer, NULL,
		d3dDsvCPUDescriptorHandle);
}

void GameFrameWork::BuildObjects()
{
}

void GameFrameWork::ReleaseObjects()
{
}

void GameFrameWork::OnProcessingMouseMessage(HWND hWnd, UINT MessageID,
	WPARAM wParam, LPARAM lParam)
{
	switch (MessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

void GameFrameWork::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM
	wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;
		case VK_RETURN:
			break;
		case VK_F9: // 전체화면 동작키
		{
			BOOL FullScreenState = FALSE;
			m_dxgiSwapChain->GetFullscreenState(&FullScreenState, NULL);
			m_dxgiSwapChain->SetFullscreenState(!FullScreenState, NULL);
			DXGI_MODE_DESC dxgiTargetParameters;
			dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			dxgiTargetParameters.Width = m_WndClientWidth;
			dxgiTargetParameters.Height = m_WndClientHeight;
			dxgiTargetParameters.RefreshRate.Numerator = 60;
			dxgiTargetParameters.RefreshRate.Denominator = 1;
			dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			m_dxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

			OnResizeBackBuffers();
			break;
		}
		default:
			break;
		}
		break;
	default:
		break;
	}
}

LRESULT CALLBACK GameFrameWork::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID,
	WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_ACTIVATE:
	{
		//if (LOWORD(wParam) == WA_INACTIVE)
		//	m_GameTimer.Stop();
		//else
		//	m_GameTimer.Start();
		break;
	}
	case WM_SIZE: // 윈도우가 생성될 때 혹은 윈도우 크기가 변경될 때 호출.
	{
		m_WndClientWidth = LOWORD(lParam);
		m_WndClientHeight = HIWORD(lParam);

		// 윈도우 크기가 변경되면 후면 버퍼의 크기를 변경
		OnResizeBackBuffers();
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}
	return(0);
}

void GameFrameWork::ProcessInput()
{
}

void GameFrameWork::AnimateObjects()
{
}

void GameFrameWork::WaitForGPUComplete()
{
	// CPU 펜스의 값 증가
	m_FenceValue++;

	// GPU가 펜스의 값을 설정하는 명령을 명령 큐에 추가
	const UINT64 Fence = m_FenceValue;
	HRESULT hResult = m_d3dCommandQueue->Signal(m_d3dFence, Fence);

	// 펜스의 현재 값이 설정한 값보다 작으면 펜스의 현재 값이 설정한 값이 될 때까지 대기
	if (m_d3dFence->GetCompletedValue() < Fence)
	{
		hResult = m_d3dFence->SetEventOnCompletion(Fence, m_FenceEvent);
		::WaitForSingleObject(m_FenceEvent, INFINITE);
	}
}

void GameFrameWork::FrameAdvance()
{
	m_GameTimer.Tick(0.f);

	ProcessInput();

	AnimateObjects();

	// 명령 할당자와 명령 리스트를 리셋
	HRESULT hResult = m_d3dCommandAllocator->Reset();
	hResult = m_d3dCommandList->Reset(m_d3dCommandAllocator, NULL);

	// 현재 렌더 타겟에 대한 프리젠트가 끝나기를 기다린다.
	// 프리젠트가 끝나면 렌더 타겟 버퍼의 상태는 프리젠트 상태(D3D12_RESOURCE_STATE_PRESENT)에서
	// 렌더 타겟 상태(D3D12_RESOURCE_STATE_RENDER_TARGET)로 바뀔 것이다.
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_d3dRenderTargetBuffers[m_SwapChainBufferIndex];
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	// 뷰포트와 씨저 사각형을 설정
	m_d3dCommandList->RSSetViewports(1, &m_d3dViewport);
	m_d3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);

	// 현재의 렌더 타겟에 해당하는 서술자의 CPU 주소(핸들)를 계산
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle =
		m_d3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (m_SwapChainBufferIndex *
		m_RtvDescriptorIncrementSize);

	// 렌더 타겟(뷰)을 설정한 색상으로 지운다.
	float ClearColor[4] = { 0.f, 0.125f, 0.3f, 1.f };
	m_d3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle,
		ClearColor, 0, NULL);

	// 깊이-스텐실 서술자의 CPU 주소를 계산
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDesciptorHandle =
		m_d3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 깊이-스텐실(뷰)을 설정한 색상으로 지운다.
	m_d3dCommandList->ClearDepthStencilView(d3dDsvCPUDesciptorHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, NULL);

	// 렌더 타겟 뷰(서술자)와 깊이-스텐실 뷰(서술자)를 출력-병합 단계(OM)에 연결
	m_d3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE,
		&d3dDsvCPUDesciptorHandle);

	// 렌더링 코드 추가 영역
	
	//

	// 현재 렌더 타겟에 대한 렌더링이 끝나기를 기다린다.
	// GPU가 렌더 타겟(버퍼)을 더 이상 사용하지 않으면
	// 렌더 타겟의 상태는 프리젠트 상태로 바뀔 것이다.
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	// 명령 리스트를 닫힌 상태로 만든다.
	hResult = m_d3dCommandList->Close();

	// 명령 리스트를 명령 큐에 추가하여 실행
	ID3D12CommandList* d3dCommandLists[] = { m_d3dCommandList };
	m_d3dCommandQueue->ExecuteCommandLists(1, d3dCommandLists);

	// GPU가 모든 명령 리스트를 실행할 때까지 기다린다.
	WaitForGPUComplete();

	// 스왑체인을 프리젠트한다.
	// 프리젠트를 하면 현재 렌더 타겟의 내용이 전면버퍼로 옮겨지고 렌더 타겟 인덱스가 바뀐다.
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	m_dxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);

	m_SwapChainBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

	// 현재의 프레임 레이트를 문자열로 가져와서 주 윈도우의 타이틀로 출력
	// m_pszBuffer 문자열이 "Practice ("으로 초기화되었으므로 (m_pszFrameRate+12)에서부터
	// 프레임 레이트를 문자열로 출력하여 " FPS)" 문자열과 합친다.
	m_GameTimer.GetFrameRate(m_pszFrameRate + 10, 37);
	::SetWindowText(m_hwnd, m_pszFrameRate);
}

GameFrameWork::~GameFrameWork()
{
}

