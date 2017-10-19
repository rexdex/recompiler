#pragma once

namespace launcher
{
	/// Generic output interface (global)
	class LAUNCHER_API OutputTTY
	{
	public:
		OutputTTY();
		~OutputTTY();

		// global instance
		static OutputTTY& GetInstance();

		// toggle the verbose mode (displays non-exe messageS)
		void SetVerboseMode(const bool bVerboseMode);

		// logging function
		void Spam(const char* txt, ...);
		void Log(const char* txt, ...);
		void Warn(const char* txt, ...);
		void Err(const char* txt, ...);

	private:
		void DoWrite(const int type, const char* buf);
		void DoWriteChannel(const int type, const char* channel, const char* buf);

	private:
		HANDLE				m_hStdOut;
		HANDLE				m_hStdErr;
		bool				m_bDebugerAttached;
		CRITICAL_SECTION	m_outputLock;
		bool				m_bVerboseMode;
	};

} // launcher

#define GLog launcher::OutputTTY::GetInstance() 

#define DEBUG_CHECK(x) if (!(x)) { GLog.Err( "Debug check failed, %s(%d)", __FILE__, __LINE__ ); GLog.Err( #x ); DebugBreak(); }

