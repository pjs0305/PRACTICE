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
// ���� ���α׷��� ����Ǿ� �� �����찡 �����Ǹ� ȣ��Ǵ� �Լ�.
{
	m_hInstance = hInstance;
	m_hwnd = hMainWnd;

	// Direct3D ����̽�, ��� ť�� ��� ����Ʈ, ���� ü�� ���� �����ϴ� �Լ��� ȣ��
	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	// �������� ��ü�� ����
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
	// ���� ü���� ȭ�� ũ�� ����
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
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ���� Ÿ������ ���
	dxgiSwapChainDesc.BufferCount = m_SwapChainBuffers;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ������ �� �� ����
	dxgiSwapChainDesc.OutputWindow = m_hwnd;
	dxgiSwapChainDesc.Windowed = TRUE;

#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS;
	// ��üȭ�� ��忡�� ����ȭ���� �ػ󵵸� �ٲ��� �ʰ� �ĸ������ ũ�⸦ ����ȭ�� ũ��� ����
	dxgiSwapChainDesc.Flags = 0;
#else
	// ��üȭ�� ��忡�� ����ȭ���� �ػ󵵸� ����ü��(�ĸ����)�� ũ�⿡ �°� ����
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif


	// ����ü�� ����
	HRESULT hResult = m_dxgiFactory->CreateSwapChain(m_d3dCommandQueue,
		&dxgiSwapChainDesc, (IDXGISwapChain**)&m_dxgiSwapChain);

	// Alt + EnterŰ�� ȭ�� ũ�� ��ȯ ��Ȱ��ȭ
	m_dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);

	// ����ü���� ���� �ĸ���� �ε����� ����
	m_SwapChainBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
}

void GameFrameWork::CreateDirect3DDevice()
{
#if defined(_DEBUG)
	D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&m_d3dDebugController);
	m_d3dDebugController->EnableDebugLayer();
#endif

	// DXGI ���丮 ����
	::CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&m_dxgiFactory);

	IDXGIAdapter1* d3dAdapter = NULL;
	// ��� �ϵ���� ����Ϳ� ���Ͽ� Ư�� ���� 12.0�� �����ϴ� �ϵ���� ����̽��� ����
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(i, &d3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		d3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(d3dAdapter, D3D_FEATURE_LEVEL_12_0, 
			_uuidof(ID3D12Device), (void**)&m_d3dDevice))) break;
	}

	// Ư�� ���� 12.0�� �����ϴ� �ϵ���� ����̽��� ������ �� ������ WARP ����̽��� ����
	if (!d3dAdapter)
	{
		m_dxgiFactory->EnumWarpAdapter(_uuidof(IDXGIFactory4), (void**)&d3dAdapter);
		D3D12CreateDevice(d3dAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device),
			(void**)&m_d3dDevice);
	}

	// ����̽��� �����ϴ� ���� ������ ǰ�� ������ Ȯ��
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMSAAQualityLevels;
	d3dMSAAQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMSAAQualityLevels.SampleCount = 4; // MSAA 4x ���� ���ø�
	d3dMSAAQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMSAAQualityLevels.NumQualityLevels = 0;
	m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&d3dMSAAQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_MSAA4xQualityLevels = d3dMSAAQualityLevels.NumQualityLevels;

	// ���� ������ ǰ�� ������ 1���� ũ�� ���� ���ø��� Ȱ��
	m_MSAA4xEnable = (m_MSAA4xQualityLevels > 1) ? true : false;

	// �潺�� �����ϰ� �潺 ���� 0���� ����
	m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence),
		(void**)&m_d3dFence);
	m_FenceValue = 0;

	// �潺�� ����ȭ�� ���� �̺�Ʈ ��ü�� ����(�̺�Ʈ ��ü�� �ʱⰪ�� False)
	// �̺�Ʈ�� ����Ǹ�(Signal) �̺�Ʈ�� ���� �ڵ������� False�� �ǵ��� ����
	m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// ����Ʈ�� �� �������� Ŭ���̾�Ʈ ���� ��ü�� ����
	m_d3dViewport.TopLeftX = 0;
	m_d3dViewport.TopLeftY = 0;
	m_d3dViewport.Width = static_cast<float>(m_WndClientWidth);
	m_d3dViewport.Height = static_cast<float>(m_WndClientHeight);
	m_d3dViewport.MinDepth = 0.f;
	m_d3dViewport.MaxDepth = 1.f;

	// ���� �簢���� �� �������� Ŭ���̾�Ʈ ���� ��ü�� ����
	m_d3dScissorRect = { 0, 0, m_WndClientWidth, m_WndClientHeight };

	if (d3dAdapter) d3dAdapter->Release();
}

void GameFrameWork::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// ���� ��� ť�� ����
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hResult = m_d3dDevice->CreateCommandQueue(&d3dCommandQueueDesc,
		__uuidof(ID3D12CommandQueue), (void**)&m_d3dCommandQueue);
	// ���� ��� �Ҵ��ڸ� ����
	hResult = m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		__uuidof(ID3D12CommandAllocator), (void**)&m_d3dCommandAllocator);
	// ���� ��� ����Ʈ�� ����
	hResult = m_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_d3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList),
		(void**)&m_d3dCommandList);
	
	// ��� ����Ʈ�� ���� ���·� �����ǹǷ� ���� ���·� �����.
	hResult = m_d3dCommandList->Close();
}

void GameFrameWork::CreateRtvAndDsvDescriptorHeaps()
{
	// ���� Ÿ�� ������ ��(�������� ������ ����ü�� ������ ����)�� ����
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_SwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_d3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap), (void**)&m_d3dRtvDescriptorHeap);
	// ���� Ÿ�� ������ ���� ���� ũ�⸦ ����
	m_RtvDescriptorIncrementSize =
		m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// ����-���ٽ� ������ ��(�������� ������ 1)�� ����
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_d3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap), (void**)&m_d3dDsvDescriptorHeap);

	// ����-���ٽ� ������ ���� ���� ũ�⸦ ����
	m_DsvDescriptorIncrementSize =
		m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void GameFrameWork::CreateRenderTargetView()
{
	// ���� ü���� �� �ĸ� ���ۿ� ���� ���� Ÿ�� �並 ����
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

	// ����-���ٽ� ���۸� ����
	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.f;
	d3dClearValue.DepthStencil.Stencil = 0;
	m_d3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue,
		__uuidof(ID3D12Resource), (void**)&m_d3dDepthStencilBuffer);

	// ����-���ٽ� ���� �並 ����
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
		case VK_F9: // ��üȭ�� ����Ű
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
	case WM_SIZE: // �����찡 ������ �� Ȥ�� ������ ũ�Ⱑ ����� �� ȣ��.
	{
		m_WndClientWidth = LOWORD(lParam);
		m_WndClientHeight = HIWORD(lParam);

		// ������ ũ�Ⱑ ����Ǹ� �ĸ� ������ ũ�⸦ ����
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
	// CPU �潺�� �� ����
	m_FenceValue++;

	// GPU�� �潺�� ���� �����ϴ� ����� ��� ť�� �߰�
	const UINT64 Fence = m_FenceValue;
	HRESULT hResult = m_d3dCommandQueue->Signal(m_d3dFence, Fence);

	// �潺�� ���� ���� ������ ������ ������ �潺�� ���� ���� ������ ���� �� ������ ���
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

	// ��� �Ҵ��ڿ� ��� ����Ʈ�� ����
	HRESULT hResult = m_d3dCommandAllocator->Reset();
	hResult = m_d3dCommandList->Reset(m_d3dCommandAllocator, NULL);

	// ���� ���� Ÿ�ٿ� ���� ������Ʈ�� �����⸦ ��ٸ���.
	// ������Ʈ�� ������ ���� Ÿ�� ������ ���´� ������Ʈ ����(D3D12_RESOURCE_STATE_PRESENT)����
	// ���� Ÿ�� ����(D3D12_RESOURCE_STATE_RENDER_TARGET)�� �ٲ� ���̴�.
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_d3dRenderTargetBuffers[m_SwapChainBufferIndex];
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	// ����Ʈ�� ���� �簢���� ����
	m_d3dCommandList->RSSetViewports(1, &m_d3dViewport);
	m_d3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);

	// ������ ���� Ÿ�ٿ� �ش��ϴ� �������� CPU �ּ�(�ڵ�)�� ���
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle =
		m_d3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (m_SwapChainBufferIndex *
		m_RtvDescriptorIncrementSize);

	// ���� Ÿ��(��)�� ������ �������� �����.
	float ClearColor[4] = { 0.f, 0.125f, 0.3f, 1.f };
	m_d3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle,
		ClearColor, 0, NULL);

	// ����-���ٽ� �������� CPU �ּҸ� ���
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDesciptorHandle =
		m_d3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// ����-���ٽ�(��)�� ������ �������� �����.
	m_d3dCommandList->ClearDepthStencilView(d3dDsvCPUDesciptorHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, NULL);

	// ���� Ÿ�� ��(������)�� ����-���ٽ� ��(������)�� ���-���� �ܰ�(OM)�� ����
	m_d3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE,
		&d3dDsvCPUDesciptorHandle);

	// ������ �ڵ� �߰� ����
	
	//

	// ���� ���� Ÿ�ٿ� ���� �������� �����⸦ ��ٸ���.
	// GPU�� ���� Ÿ��(����)�� �� �̻� ������� ������
	// ���� Ÿ���� ���´� ������Ʈ ���·� �ٲ� ���̴�.
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	// ��� ����Ʈ�� ���� ���·� �����.
	hResult = m_d3dCommandList->Close();

	// ��� ����Ʈ�� ��� ť�� �߰��Ͽ� ����
	ID3D12CommandList* d3dCommandLists[] = { m_d3dCommandList };
	m_d3dCommandQueue->ExecuteCommandLists(1, d3dCommandLists);

	// GPU�� ��� ��� ����Ʈ�� ������ ������ ��ٸ���.
	WaitForGPUComplete();

	// ����ü���� ������Ʈ�Ѵ�.
	// ������Ʈ�� �ϸ� ���� ���� Ÿ���� ������ ������۷� �Ű����� ���� Ÿ�� �ε����� �ٲ��.
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	m_dxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);

	m_SwapChainBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

	// ������ ������ ����Ʈ�� ���ڿ��� �����ͼ� �� �������� Ÿ��Ʋ�� ���
	// m_pszBuffer ���ڿ��� "Practice ("���� �ʱ�ȭ�Ǿ����Ƿ� (m_pszFrameRate+12)��������
	// ������ ����Ʈ�� ���ڿ��� ����Ͽ� " FPS)" ���ڿ��� ��ģ��.
	m_GameTimer.GetFrameRate(m_pszFrameRate + 10, 37);
	::SetWindowText(m_hwnd, m_pszFrameRate);
}

GameFrameWork::~GameFrameWork()
{
}

