#include "build.h"
#include "launcherOutputTTY.h"

namespace launcher
{
	OutputTTY::OutputTTY()
		: m_bVerboseMode(false)
	{
		if (!AttachConsole( GetCurrentProcessId() ))
			AllocConsole();

		m_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		m_hStdErr = GetStdHandle(STD_ERROR_HANDLE);
		m_bDebugerAttached = (TRUE == IsDebuggerPresent());

		InitializeCriticalSection(&m_outputLock);
	}

	OutputTTY::~OutputTTY()
	{
	}

	OutputTTY& OutputTTY::GetInstance()
	{
		static OutputTTY theInstance;
		return theInstance;
	}

	void OutputTTY::SetVerboseMode(const bool bVerboseMode)
	{
		m_bVerboseMode = bVerboseMode;
	}

	void OutputTTY::Spam(const char* txt, ...)
	{
		char buf[8192];
		va_list args;

		va_start(args, txt);
		vsprintf_s(buf, txt, args);
		va_end(args);

		//DoWrite(0, buf);
	}

	void OutputTTY::Log(const char* txt, ...)
	{
		char buf[8192];
		va_list args;

		va_start(args, txt);
		vsprintf_s(buf, txt, args);
		va_end(args);

		DoWrite(0, buf);
	}

	void OutputTTY::Warn(const char* txt, ...)
	{
		char buf[8192];
		va_list args;

		va_start(args, txt);
		vsprintf_s(buf, txt, args);
		va_end(args);

		char buf2[8192];
		strcpy_s(buf2, "Warning: ");
		strcat_s(buf2, buf);
		DoWrite(1, buf2);
	}

	void OutputTTY::Err(const char* txt, ...)
	{
		char buf[8192];
		va_list args;

		va_start(args, txt);
		vsprintf_s(buf, txt, args);
		va_end(args);

		char buf2[8192];
		strcpy_s(buf2, "Error: ");
		strcat_s(buf2, buf);
		DoWrite(2, buf2);
	}

	void OutputTTY::DoWrite(const int type, const char* buf)
	{
		// find the "channel" break
		const char* txt = buf;
		while (*txt != 0)
		{
			if (*txt == ':')
			{
				break;
			}

			txt += 1;
		}

		// log with or without the channel name
		if (*txt != 0)
		{
			do
			{
				*(char*)txt = 0; // assumes incoming buffer is writable
				txt++;
			} while (*txt != 0 && *txt <= ' ');

			DoWriteChannel(type, buf, txt);
		}
		else
		{
			DoWriteChannel(type, "Log", buf);
		}
	}

	void OutputTTY::DoWriteChannel(const int type, const char* channel, const char* buf)
	{
		// verbose filter
		if (!m_bVerboseMode)
		{
			// filter messages
			if (!type && (0 != strcmp(channel, "DebugOutput")))
				return;
		}

		// format local message
		char printBuffer[8192];
		if (m_bVerboseMode)
		{
			strcpy_s(printBuffer, channel);
			strcat_s(printBuffer, ": ");
			strcat_s(printBuffer, buf);
			strcat_s(printBuffer, "\n");
		}
		else
		{
			strcpy_s(printBuffer, buf);
			strcat_s(printBuffer, "\n");
		}

		// sync the printing
		EnterCriticalSection(&m_outputLock);

		// debug print
		if (m_bDebugerAttached)
		{
			OutputDebugStringA(printBuffer);
		}

		// colors
		if (type == 0)
			SetConsoleTextAttribute(m_hStdOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		else if (type == 1)
			SetConsoleTextAttribute(m_hStdOut, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
		else if (type == 2)
			SetConsoleTextAttribute(m_hStdOut, FOREGROUND_INTENSITY | FOREGROUND_RED);
		else
			SetConsoleTextAttribute(m_hStdOut, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // fatal

		// output print
		DWORD numPrinted = 0;
		WriteFile( (type==2) ? m_hStdErr : m_hStdOut, printBuffer, (DWORD)strlen(printBuffer), &numPrinted, NULL );
		FlushFileBuffers((type == 2) ? m_hStdErr : m_hStdOut);

		// restore color
		if  ( type != 0 )
			SetConsoleTextAttribute(m_hStdOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

		// unlock the printing
		LeaveCriticalSection(&m_outputLock);
	}

} // log