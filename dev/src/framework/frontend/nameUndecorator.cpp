#include "build.h"
#include "nameUndecorator.h"

CFunctionNameUndecorator::CFunctionNameUndecorator()
{
}

CFunctionNameUndecorator::~CFunctionNameUndecorator()
{
}

bool CFunctionNameUndecorator::IsValid() const
{
	return true;
}

bool CFunctionNameUndecorator::Undecorate( const std::string& name, std::string& outUndecoratedName )
{
	if ( name.c_str()[0] == '?' )
	{
		TNameMap::const_iterator it = m_names.find( name );
		if ( it != m_names.end() )
		{
			outUndecoratedName = it->second;
			return true;
		}

		char tempBuffer[ 4096 ];
		if ( 0 != UnDecorateSymbolName( name.c_str(), tempBuffer, _MAX_FNAME, 0 ) )
		{
			outUndecoratedName = tempBuffer;
			m_names[ name ] = outUndecoratedName;
			return true;
		}
	}

	return false;
}

CFunctionNameUndecorator& CFunctionNameUndecorator::GetInstance()
{
	static CFunctionNameUndecorator theInstance;
	return theInstance;
}