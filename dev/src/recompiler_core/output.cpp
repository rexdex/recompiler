#include "build.h"
#include <algorithm>

ILogOutput::ILogOutput(ILogOutput* parent /*= nullptr*/)
	: m_parent(nullptr)
{
	Attach(parent);
}

ILogOutput::~ILogOutput()
{
	Detach();
}

void ILogOutput::Attach(ILogOutput* parent)
{
	if (m_parent != parent)
	{
		Detach();

		m_parent = parent;

		if (m_parent)
			m_parent->m_children.push_back(this);
	}
}

void ILogOutput::Detach()
{
	if (m_parent != nullptr)
	{
		std::remove(m_parent->m_children.begin(), m_parent->m_children.end(), this);
		m_parent = nullptr;
	}
}

void ILogOutput::Log(_Printf_format_string_ const char* txt, ...)
{
	char buffer[kMaxBuffer];

	va_list args;
	va_start(args, txt);
	vsprintf_s(buffer, kMaxBuffer, txt, args);
	va_end(args);

	ILogOutput* cur = this;
	while (cur != nullptr)
	{
		cur->DoLog(LogLevel::Info, buffer);
		cur = cur->m_parent;
	}

}

void ILogOutput::Warn(_Printf_format_string_ const char* txt, ...)
{
	char buffer[kMaxBuffer];

	va_list args;
	va_start(args, txt);
	vsprintf_s(buffer, kMaxBuffer, txt, args);
	va_end(args);

	ILogOutput* cur = this;
	while (cur != nullptr)
	{
		cur->DoLog(LogLevel::Warning, buffer);
		cur = cur->m_parent;
	}
}

void ILogOutput::Error(_Printf_format_string_ const char* txt, ...)
{
	char buffer[kMaxBuffer];

	va_list args;
	va_start(args, txt);
	vsprintf_s(buffer, kMaxBuffer, txt, args);
	va_end(args);

	ILogOutput* cur = this;
	while (cur != nullptr)
	{
		cur->DoLog(LogLevel::Error, buffer);
		cur = cur->m_parent;
	}

}

void ILogOutput::SetTaskName(_Printf_format_string_ const char* txt, ...)
{
	char buffer[kMaxBuffer];

	va_list args;
	va_start(args, txt);
	vsprintf_s(buffer, kMaxBuffer, txt, args);
	va_end(args);

	ILogOutput* cur = this;
	while (cur != nullptr)
	{
		cur->DoSetTaskName(buffer);
		cur = cur->m_parent;
	}
}

void ILogOutput::SetTaskProgress(uint64_t count, uint64_t max)
{
	ILogOutput* cur = this;
	while (cur != nullptr)
	{
		cur->DoSetTaskProgress(count, max);
		cur = cur->m_parent;
	}
}

bool ILogOutput::IsTaskCanceled()
{
	ILogOutput* cur = this;
	while (cur != nullptr)
	{
		if (cur->DoIsTaskCanceled())
			return true;

		cur = cur->m_parent;
	}
	return false;
}

ILogOutput& ILogOutput::DevNull()
{
	class NullImplementation : public ILogOutput
	{
		virtual void DoLog(const LogLevel, const char*) override {}
		virtual void DoSetTaskName(const char*) override {}
		virtual void DoSetTaskProgress(uint64_t, uint64_t) override {}
		virtual bool DoIsTaskCanceled() override { return false; }
	};

	static NullImplementation theNullImplementation;
	return theNullImplementation;
}
