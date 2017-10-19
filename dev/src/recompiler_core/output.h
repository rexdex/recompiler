#pragma once

// generic output interface user everywhere
class RECOMPILER_API ILogOutput
{
public:
	ILogOutput(ILogOutput* parent = nullptr);
	virtual ~ILogOutput();

	enum class LogLevel
	{
		Info,
		Warning,
		Error,
	};

	// loging/errors
	void Log(_Printf_format_string_ const char* txt, ...);
	void Warn(_Printf_format_string_ const char* txt, ...);
	void Error(_Printf_format_string_ const char* txt, ...);

	// progress
	void SetTaskName(_Printf_format_string_  const char* txt, ...);
	void SetTaskProgress(uint64_t count, uint64_t max);

	// task cancellation
	bool IsTaskCanceled();

	//--

	// NULL output (silent)
	static ILogOutput& DevNull();

protected:
	// interface, return false to stop propagation
	virtual void DoLog(const LogLevel level, const char* buffer) {};
	virtual void DoSetTaskName(const char* buffer) {};
	virtual void DoSetTaskProgress(uint64_t count, uint64_t max) {};
	virtual bool DoIsTaskCanceled() { return false; };

	//---

	typedef std::vector< ILogOutput* >	TChildren;

	ILogOutput* m_parent;
	TChildren m_children;

	void Attach(ILogOutput* parent);
	void Detach();

	static const int kMaxBuffer = 4096;
};
