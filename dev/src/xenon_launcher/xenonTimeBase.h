#pragma once

namespace xenon
{

	/// time base system
	class TimeBase
	{
	public:
		TimeBase(const launcher::Commandline& commandline);

		/// get frequency of the high-resolution timer 
		uint64_t GetHighResolutionFrequency();

		/// get current high-frequency timer value
		uint64_t GetHighResolutionTime();

		//---

		/// get system time frequency
		uint64_t GetSystemTimeFrequency();

		/// get current system time (10MHz)
		uint64_t GetSystemTime();

	private:
		uint64_t m_highResolutionTicksPerSecond;
		uint64_t m_highResolutionBase;

		uint64_t m_systemTimeBase;
		uint64_t m_systemTimeTicksPerSecond;
	};

} // xenon