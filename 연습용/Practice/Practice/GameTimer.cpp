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

	// 마지막으로 이 함수를 호출한 이후 경과한 시간을 계산
	float TimeElapsed = (m_CurrentTime - m_LastTime) * m_TimeScale;

	if (LockFPS > 0.f)
	{
		// 이 함수의 LockFPS가 0보다 크면 이 시간만큼 호출한 함수를 기다리게 한다.
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

			// 다시 이 함수를 호출한 이후 경과한 시간을 계산
			float TimeElapsed = (m_CurrentTime - m_LastTime) * m_TimeScale;
		}
	}

	// 현재 시간을 m_LastTime에 저장
	m_LastTime = m_CurrentTime;

	// 마지막 프레임 처리 시간과 현재 프레임 처리 시간의 차이가 1초보다 작으면
	// 현재 프레임 처리 시간을 m_FrameTime[0]에 저장한다.
	if (fabsf(TimeElapsed - m_TimeElapsed) < 1.f)
	{
		::memmove(&m_FrameTime[1], m_FrameTime, (MAX_SAMPLE_COUNT - 1) * sizeof(float));
		m_FrameTime[0] = TimeElapsed;
		if (m_SampleCount < MAX_SAMPLE_COUNT) m_SampleCount++;
	}

	// 초당 프레임 수를 1 증가시키고 현재 프레임 처리 시간을 누적하여 저장
	m_FramesPerSecond++;
	m_FPSTimeElapsed += TimeElapsed;
	if (m_FPSTimeElapsed > 1.f)
	{
		m_CurrentFrameRate = m_FramesPerSecond;
		m_FramesPerSecond = 0;
		m_FPSTimeElapsed = 0.0f;
	}

	// 누적된 프레임 처리 시간의 평균을 구하여 프레임 처리 시간을 구한다.
	m_TimeElapsed = 0.f;
	for (ULONG i = 0; i < m_SampleCount; i++)
		m_TimeElapsed += m_FrameTime[i];
	if (m_SampleCount > 0) m_TimeElapsed /= m_SampleCount;
}

unsigned long GameTimer::GetFrameRate(LPTSTR lpszString, int Characters)
{
	// 현재 프레임 레이트를 문자열로 변환하여 lpszString 버퍼에 쓰고 " FPS"와 결합
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
