#pragma once

const ULONG MAX_SAMPLE_COUNT = 50; // n회의 프레임 처리시간을 누적하여 평균한다.

class GameTimer
{
public:
	GameTimer();
	virtual ~GameTimer();

	void Tick(float LockFPS = 0.0f); // 타이머의 시간을 갱신
	unsigned long GetFrameRate(LPTSTR lpszString = NULL, int Characters = 0); // 프레임 레이트를 반환
	float GetTimeElapsed(); // 프레임 평균 경과 시간을 반환

private:
	bool m_HardwareHasPerformanceCounter; // 컴퓨터가 Performance Counter를 가지고 있는가
	float m_TimeScale; // Scale Counter의 양
	float m_TimeElapsed; // 마지막 프레임 이후 지나간 시간
	__int64 m_CurrentTime; // 현재 시간
	__int64 m_LastTime; // 마지막 프레임 시간
	__int64 m_PerformanceFrequency; //컴퓨터의 Performance Frequency

	float m_FrameTime[MAX_SAMPLE_COUNT]; // 프레임 시간을 누적하는 배열
	ULONG m_SampleCount; // 누적된 프레임 횟수

	unsigned long m_CurrentFrameRate; // 현재 프레임 레이트
	unsigned long m_FramesPerSecond; // 초당 프레임 수
	float m_FPSTimeElapsed; // 프레임 레이트 계산 소요 시간
};

