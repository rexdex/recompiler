#pragma once

/// helper class that allows to undecorate c++ function names
class CFunctionNameUndecorator
{
private:
	typedef DWORD (WINAPI *TUnDecorateSymbolName)( PCTSTR DecoratedName, PTSTR UnDecoratedName, DWORD UndecoratedLength, DWORD Flags );
	typedef DWORD (WINAPI *TUnDecorateSymbolName)( PCTSTR DecoratedName, PTSTR UnDecoratedName, DWORD UndecoratedLength, DWORD Flags );

	HMODULE						m_library;
	TUnDecorateSymbolName		m_func;

	typedef std::map< std::string, std::string > TNameMap;
	TNameMap					m_names;

public:
	CFunctionNameUndecorator();
	~CFunctionNameUndecorator();

	// is valid ? (can we undecorate function names)
	bool IsValid() const;

	// undecorate function name, returns false if failed
	bool Undecorate( const std::string& name, std::string& outUndecoratedName );

public:
	static CFunctionNameUndecorator& GetInstance();
};