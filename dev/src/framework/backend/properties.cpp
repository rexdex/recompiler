#include "build.h"
#include "internalFile.h"
#include "properties.h"

//---------------------------------------------------------------------------

CProperty::CProperty()
	: m_name( "none" )
	, m_type( eType_None )
	, m_valuePtr( NULL )
{
}

CProperty::CProperty( const char* name, const EType type, const void* srcDataToCopyFrom )
	: m_name( name )
	, m_type( eType_None )
	, m_valuePtr( NULL )
{
	Set( type, srcDataToCopyFrom );
}

CProperty::CProperty( const CProperty& other )
	: m_name( other.m_name )
	, m_type( other.m_type )
	, m_valuePtr( NULL )
{
	if ( other.m_type != eType_None )
	{
		Set( other.m_type, other.m_valuePtr );
	}
}

CProperty::CProperty( const char* name, const uint32 addressValue )
	: m_name( name )
	, m_type( eType_None )
	, m_valuePtr( NULL )
{
	Set( eType_Address, &addressValue );
}

CProperty::CProperty( const char* name, const int numberValue )
	: m_name( name )
	, m_type( eType_None )
	, m_valuePtr( NULL )
{
	Set( eType_Number, &numberValue );
}

CProperty::CProperty( const char* name, const bool booleanValue )
	: m_name( name )
	, m_type( eType_Boolean )
	, m_valuePtr( NULL )
{
	Set( eType_Number, &booleanValue );
}

CProperty::CProperty( const char* name, const char* stringValue )
	: m_name( name )
	, m_type( eType_String )
	, m_valuePtr( NULL )
{
	std::string str(stringValue);
	Set( eType_String, &str );
}

CProperty::~CProperty()
{
	Clear();
}

CProperty& CProperty::operator=( const CProperty& other )
{
	if ( this != &other )
	{
		Set( other.m_type, other.m_valuePtr );
	}

	return *this;
}


bool CProperty::GetValueString( char* outBuffer, const uint32 outBufferSize ) const
{
	if ( m_type == eType_String )
	{
		const std::string& val = *(const std::string*) m_valuePtr;
		if ( outBufferSize < val.length() + 1 ) 
		{
			return false;
		}

		strcpy_s( outBuffer, outBufferSize, val.c_str() );
		return true;
	}
	else if ( m_type == eType_Address )
	{
		if ( outBufferSize < 11 )
		{
			return false;
		}

		const uint32& val = *(const uint32*) m_valuePtr;
		sprintf_s( outBuffer, outBufferSize, "0x%08X", val );
		return true;
	}
	else if ( m_type == eType_Number )
	{
		if ( outBufferSize < 16 )
		{
			return false;
		}

		const int& val = *(const int*) m_valuePtr;
		sprintf_s( outBuffer, outBufferSize, "%d", val );
		return true;
	}
	else if ( m_type == eType_Boolean )
	{
		if ( outBufferSize < 6 )
		{
			return false;
		}

		const bool& val = *(const bool*) m_valuePtr;
		strcpy_s( outBuffer, outBufferSize, val ? "true" : "false" );
		return true;
	}

	return false;
}

void CProperty::Save( class IBinaryFileWriter& writer ) const
{
	CBinaryFileChunkScope chunk( writer, eFileChunk_Property );

	// name & type
	writer << m_name;
	writer << (uint8)m_type;

	// value
	if ( m_type == eType_String )
	{
		const std::string& val = *(const std::string*) m_valuePtr;
		writer << val;
	}
	else if ( m_type == eType_Address )
	{
		const uint32& val = *(const uint32*) m_valuePtr;
		writer << val;
	}
	else if ( m_type == eType_Number )
	{
		const int& val = *(const int*) m_valuePtr;
		writer << val;
	}
	else if ( m_type == eType_Boolean )
	{
		const bool& val = *(const bool*) m_valuePtr;
		writer << val;
	}
	else if ( m_type == eType_DateTime )
	{
		const uint64& val = *(const uint64*) m_valuePtr;
		writer << val;
	}
}

void CProperty::Load( class IBinaryFileReader& reader )
{
	CBinaryFileChunkScope chunk( reader , eFileChunk_Property );
	if ( chunk )
	{
		// name & type
		reader >> m_name;

		// type
		uint8 typeSaved = 0;
		reader >> typeSaved;

		// value
		if ( typeSaved == eType_String )
		{
			m_type = eType_String;
			reader >> *(std::string*) m_valuePtr;
		}
		else if ( typeSaved == eType_Address )
		{
			m_type = eType_Address;
			reader >> *(uint32*) m_valuePtr;
		}
		else if ( typeSaved == eType_Number )
		{
			m_type = eType_Number;
			reader >> *(int*) m_valuePtr;
		}
		else if ( typeSaved == eType_Boolean )
		{
			m_type = eType_Boolean;
			reader >> *(bool*) m_valuePtr;
		}
		else if ( typeSaved == eType_DateTime )
		{
			m_type = eType_DateTime;
			reader >> *(uint64*) m_valuePtr;
		}
		else
		{
			m_type = eType_None;
		}
	}
}

void CProperty::Clear()
{
	if ( NULL != m_valuePtr )
	{
		if ( m_type == eType_String )
		{
			std::string* ptr = (std::string*) m_valuePtr;
			delete ptr;
		}
		else
		{
			free( m_valuePtr );
		}

		m_valuePtr = NULL;
	}

	// reset type
	m_type = eType_None;
}

void CProperty::Set( const EType type, const void* srcDataToCopyFrom )
{
	Clear();

	// set new type
	if ( type == eType_String )
	{
		std::string* val = new std::string( *(const std::string*) srcDataToCopyFrom );
		m_valuePtr = val;
		m_type = eType_String;
	}
	else if ( type == eType_Address )
	{
		m_valuePtr = new uint32;
		*(uint32*)m_valuePtr = *(const uint32*) srcDataToCopyFrom;
		m_type = eType_Address;
	}
	else if ( type == eType_Number )
	{
		m_valuePtr = new int;
		*(int*)m_valuePtr = *(const int*) srcDataToCopyFrom;
		m_type = eType_Number;
	}
	else if ( type == eType_Boolean )
	{
		m_valuePtr = new bool;
		*(bool*)m_valuePtr = *(const bool*) srcDataToCopyFrom;
		m_type = eType_Boolean;
	}
	else if ( type == eType_DateTime )
	{
		m_valuePtr = new uint64;
		*(uint64*)m_valuePtr = *(const uint64*) srcDataToCopyFrom;
		m_type = eType_DateTime;
	}
}

//---------------------------------------------------------------------------
