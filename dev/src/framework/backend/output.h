#pragma once

// generic output interface user everywhere
class RECOMPILER_API ILogOutput
{
private:
	typedef std::vector< ILogOutput* >	TChildren;

	ILogOutput*		m_parent;
	TChildren					m_children;

	static const int kMaxBuffer = 4096;

public:
	ILogOutput( ILogOutput* parent = nullptr );
	virtual ~ILogOutput();

	// loging/errors
	void Log( _Printf_format_string_ const char* txt, ... );
	void Warn( _Printf_format_string_ const char* txt, ... );
	void Error( _Printf_format_string_ const char* txt, ... );

	// progress
	void SetTaskName( _Printf_format_string_  const char* txt, ... );
	void SetTaskProgress( int count, int max );
	void BeginTask( _Printf_format_string_ const char* txt, ... );
	void EndTask();

	// task cancellation
	bool IsTaskCanceled();

	// flush pending logs (from another threads)
	void Flush();

protected:
	// interface, return false to stop propagation
	virtual bool DoLog( const char* buffer ) = 0;
	virtual bool DoWarn( const char* buffer ) = 0;
	virtual bool DoError( const char* buffer ) = 0;
	virtual bool DoSetTaskName( const char* buffer ) = 0;
	virtual bool DoSetTaskProgress( int count, int max ) = 0;
	virtual bool DoBeginTask( const char* buffer ) = 0;
	virtual bool DoEndTask() = 0;
	virtual bool DoIsTaskCanceled() = 0;
	virtual bool DoFlush() = 0;

public:
	// NULL output (silent)
	static ILogOutput& DevNull();
};

// scoped task
class RECOMPILER_API CScopedTask
{
public:
	CScopedTask( ILogOutput& log, const char* taskName );
	~CScopedTask();

private:
	class ILogOutput* m_log;
};
