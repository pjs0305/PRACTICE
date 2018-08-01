#pragma once

const ULONG MAX_SAMPLE_COUNT = 50; // nȸ�� ������ ó���ð��� �����Ͽ� ����Ѵ�.

class GameTimer
{
public:
	GameTimer();
	virtual ~GameTimer();

	void Tick(float LockFPS = 0.0f); // Ÿ�̸��� �ð��� ����
	unsigned long GetFrameRate(LPTSTR lpszString = NULL, int Characters = 0); // ������ ����Ʈ�� ��ȯ
	float GetTimeElapsed(); // ������ ��� ��� �ð��� ��ȯ

private:
	bool m_HardwareHasPerformanceCounter; // ��ǻ�Ͱ� Performance Counter�� ������ �ִ°�
	float m_TimeScale; // Scale Counter�� ��
	float m_TimeElapsed; // ������ ������ ���� ������ �ð�
	__int64 m_CurrentTime; // ���� �ð�
	__int64 m_LastTime; // ������ ������ �ð�
	__int64 m_PerformanceFrequency; //��ǻ���� Performance Frequency

	float m_FrameTime[MAX_SAMPLE_COUNT]; // ������ �ð��� �����ϴ� �迭
	ULONG m_SampleCount; // ������ ������ Ƚ��

	unsigned long m_CurrentFrameRate; // ���� ������ ����Ʈ
	unsigned long m_FramesPerSecond; // �ʴ� ������ ��
	float m_FPSTimeElapsed; // ������ ����Ʈ ��� �ҿ� �ð�
};

