#include "build.h"
#include "xenonTimeBase.h"

namespace xenon
{

	TimeBase::TimeBase(const launcher::Commandline& commandline)
		: m_systemTimeBase(0)
		, m_systemTimeTicksPerSecond(10000000)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&m_highResolutionBase);
		QueryPerformanceFrequency((LARGE_INTEGER*)&m_highResolutionTicksPerSecond);
	}

	uint64_t TimeBase::GetHighResolutionFrequency()
	{
		return m_highResolutionTicksPerSecond;
	}

	uint64_t TimeBase::GetHighResolutionTime()
	{
		uint64_t ret = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&ret);
		return ret - m_highResolutionBase;
	}

	uint64_t TimeBase::GetSystemTime()
	{
		const auto highRes = GetHighResolutionTime();
		return m_systemTimeBase + (highRes * m_systemTimeTicksPerSecond) / m_highResolutionTicksPerSecond;
	}

	uint64_t TimeBase::GetSystemTimeFrequency()
	{
		return m_systemTimeTicksPerSecond;
	}

} // xenon