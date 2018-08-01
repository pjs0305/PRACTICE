#include "stdafx.h"
#include "GameTimer.h"

GameTimer::GameTimer()
{
	if (::QueryPerformanceFrequency((LARGE_INTEGER*)&m_PerformanceFrequency))
	{
		m_HardwareHasPerformanceCounter = TRUE;
		::QueryPerformanceCounter((LARGE_INTEGER*)&m_LastTime);
		m_TimeScale = 1.0f / m_PerformanceFrequency;
	}
	else
	{
		m_HardwareHasPerformanceCounter = FALSE;
		m_LastTime = ::timeGetTime();
		m_TimeScale = 0.001f;
	}
	
	m_SampleCount = 0;
	m_CurrentFrameRate = 0;
	m_FramesPerSecond = 0;
	m_FPSTimeElapsed = 0.f;
}

void GameTimer::Tick(float LockFPS)
{
	if (m_HardwareHasPerformanceCounter)
	{
		::QueryPerformanceCounter((LARGE_INTEGER*)&m_CurrentTime);
	}
	else
	{
		m_CurrentTime = ::timeGetTime();
	}

	// ���������� �� �Լ��� ȣ���� ���� ����� �ð��� ���
	float TimeElapsed = (m_CurrentTime - m_LastTime) * m_TimeScale;

	if (LockFPS > 0.f)
	{
		// �� �Լ��� LockFPS�� 0���� ũ�� �� �ð���ŭ ȣ���� �Լ��� ��ٸ��� �Ѵ�.
		while (TimeElapsed < (1.f / LockFPS))
		{
			if (m_HardwareHasPerformanceCounter)
			{
				::QueryPerformanceCounter((LARGE_INTEGER*)&m_CurrentTime);
			}
			else
			{
				m_CurrentTime = ::timeGetTime();
			}

			// �ٽ� �� �Լ��� ȣ���� ���� ����� �ð��� ���
			float TimeElapsed = (m_CurrentTime - m_LastTime) * m_TimeScale;
		}
	}

	// ���� �ð��� m_LastTime�� ����
	m_LastTime = m_CurrentTime;

	// ������ ������ ó�� �ð��� ���� ������ ó�� �ð��� ���̰� 1�ʺ��� ������
	// ���� ������ ó�� �ð��� m_FrameTime[0]�� �����Ѵ�.
	if (fabsf(TimeElapsed - m_TimeElapsed) < 1.f)
	{
		::memmove(&m_FrameTime[1], m_FrameTime, (MAX_SAMPLE_COUNT - 1) * sizeof(float));
		m_FrameTime[0] = TimeElapsed;
		if (m_SampleCount < MAX_SAMPLE_COUNT) m_SampleCount++;
	}

	// �ʴ� ������ ���� 1 ������Ű�� ���� ������ ó�� �ð��� �����Ͽ� ����
	m_FramesPerSecond++;
	m_FPSTimeElapsed += TimeElapsed;
	if (m_FPSTimeElapsed > 1.f)
	{
		m_CurrentFrameRate = m_FramesPerSecond;
		m_FramesPerSecond = 0;
		m_FPSTimeElapsed = 0.0f;
	}

	// ������ ������ ó�� �ð��� ����� ���Ͽ� ������ ó�� �ð��� ���Ѵ�.
	m_TimeElapsed = 0.f;
	for (ULONG i = 0; i < m_SampleCount; i++)
		m_TimeElapsed += m_FrameTime[i];
	if (m_SampleCount > 0) m_TimeElapsed /= m_SampleCount;
}

unsigned long GameTimer::GetFrameRate(LPTSTR lpszString, int Characters)
{
	// ���� ������ ����Ʈ�� ���ڿ��� ��ȯ�Ͽ� lpszString ���ۿ� ���� " FPS"�� ����
	if (lpszString)
	{
		_itow_s(m_CurrentFrameRate, lpszString, Characters, 10);
		wcscat_s(lpszString, Characters, _T(" FPS)"));
	}

	return m_CurrentFrameRate;
}

float GameTimer::GetTimeElapsed()
{
	return m_TimeElapsed;
}

GameTimer::~GameTimer()
{
}
