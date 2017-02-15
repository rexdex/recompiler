#include "build.h"

ILogOutput::ILogOutput( ILogOutput* parent /*= nullptr*/ )
	: m_parent( parent ) 
{
}

ILogOutput::~ILogOutput()
{
	// unlink children
	for ( size_t i=0; i<m_children.size(); ++i )
	{
		m_children[i]->m_parent = nullptr;
	}

	// remove from parent
	if ( m_parent != nullptr )
	{
		TChildren::iterator it = std::find( m_parent->m_children.begin(), m_parent->m_children.end(), this );
		if ( it != m_parent->m_children.end() )
		{
			m_parent->m_children.erase( it );
		}

		m_parent = nullptr;
	}
}

void ILogOutput::Log( _Printf_format_string_ const char* txt, ... )
{
	char buffer[ kMaxBuffer ];

	va_list args;
	va_start( args, txt );
	vsprintf_s( buffer, kMaxBuffer, txt, args );
	va_end( args );

	ILogOutput* cur = this;
	while ( cur != nullptr && cur->DoLog( buffer ) )
	{
		cur = cur->m_parent;
	}
}

void ILogOutput::Warn(_Printf_format_string_ const char* txt, ... )
{
	char buffer[ kMaxBuffer ];

	va_list args;
	va_start( args, txt );
	vsprintf_s( buffer, kMaxBuffer, txt, args );
	va_end( args );

	ILogOutput* cur = this;
	while ( cur != nullptr && cur->DoWarn( buffer ) )
	{
		cur = cur->m_parent;
	}
}

void ILogOutput::Error(_Printf_format_string_ const char* txt, ... )
{
	char buffer[ kMaxBuffer ];

	va_list args;
	va_start( args, txt );
	vsprintf_s( buffer, kMaxBuffer, txt, args );
	va_end( args );

	ILogOutput* cur = this;
	while ( cur != nullptr && cur->DoError( buffer ) )
	{
		cur = cur->m_parent;
	}
}

void ILogOutput::SetTaskName( _Printf_format_string_ const char* txt, ... )
{
	char buffer[ kMaxBuffer ];

	va_list args;
	va_start( args, txt );
	vsprintf_s( buffer, kMaxBuffer, txt, args );
	va_end( args );

	ILogOutput* cur = this;
	while ( cur != nullptr && cur->DoSetTaskName( buffer ) )
	{
		cur = cur->m_parent;
	}
}

void ILogOutput::SetTaskProgress( int count, int max )
{	
	ILogOutput* cur = this;
	while ( cur != nullptr && cur->DoSetTaskProgress( count, max ) )
	{
		cur = cur->m_parent;
	}
}

void ILogOutput::BeginTask( _Printf_format_string_ const char* txt, ... )
{
	char buffer[ kMaxBuffer ];

	va_list args;
	va_start( args, txt );
	vsprintf_s( buffer, kMaxBuffer, txt, args );
	va_end( args );

	ILogOutput* cur = this;
	while ( cur != nullptr && cur->DoBeginTask( buffer ) )
	{
		cur = cur->m_parent;
	}
}

void ILogOutput::EndTask()
{
	ILogOutput* cur = this;
	while ( cur != nullptr && cur->DoEndTask() )
	{
		cur = cur->m_parent;
	}
}

void ILogOutput::Flush()
{
	ILogOutput* cur = this;
	while ( cur != nullptr && cur->DoFlush() )
	{
		cur = cur->m_parent;
	}
}

bool ILogOutput::IsTaskCanceled()
{
	ILogOutput* cur = this;
	while ( cur != nullptr )
	{
		if ( cur->DoIsTaskCanceled() )
			return true;

		cur = cur->m_parent;
	}
	return false;
}

ILogOutput& ILogOutput::DevNull()
{
	class NullImplementation : public ILogOutput
	{
		virtual bool DoLog( const char* buffer ) override { return true; }
		virtual bool DoWarn( const char* buffer ) override { return true; }
		virtual bool DoError( const char* buffer ) override { return true; }
		virtual bool DoSetTaskName( const char* buffer ) override { return true; }
		virtual bool DoSetTaskProgress( int count, int max ) override { return true; }
		virtual bool DoBeginTask( const char* buffer ) override { return true; }
		virtual bool DoIsTaskCanceled() override { return false; }
		virtual bool DoEndTask()  override { return true; }
		virtual bool DoFlush() override { return true; }
	};

	static NullImplementation theNullImplementation;
	return theNullImplementation;
}

CScopedTask::CScopedTask( ILogOutput& log, const char* taskName )
	: m_log(&log)
{
	m_log->BeginTask("%s", taskName);
}

CScopedTask::~CScopedTask()
{
	m_log->SetTaskProgress(100,100);
	m_log->EndTask();
}
