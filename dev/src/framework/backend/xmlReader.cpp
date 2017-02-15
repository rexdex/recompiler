#include "build.h"
#include "xmlReader.h"

namespace xml
{

	Reader::Reader( const char* buffer, const bool makeCopy /*= false*/ )
	{
		// Setup buffer
		if ( makeCopy )
		{
			const auto size = strlen( buffer );
			char* localBuffer = new char [ size+1 ];
			memcpy( localBuffer, buffer, size + 1 );
			localBuffer[ size ] = 0;

			m_buffer = localBuffer;
			m_bufferOwned = true;
		}
		else
		{
			m_bufferOwned = false;
			m_buffer = buffer;
		}

		// Parse document
		bool valid = true;
		try
		{
			m_doc.parse<rapidxml::parse_full>( (char*)m_buffer );
		}
		catch ( rapidxml::parse_error& e )
		{
			char errorTxt[ 512 ];
			sprintf_s( errorTxt, 512, "XML parsing error '%s'", e.what() );
			m_error = errorTxt;
			valid = false;
		}

		// initialize document stack
		if ( valid )
		{
			m_stack.push_back( &m_doc );
		}
	}

	Reader::~Reader()
	{
		if ( m_bufferOwned )
		{
			delete [] m_buffer;
			m_buffer = nullptr;
		}
	}

	const char* Reader::GetName() const
	{
		if ( !m_stack.empty() )
		{
			const TNode* topNode = m_stack.back();
			topNode->name();
		}

		return "";
	}

	const char* Reader::GetValue() const
	{
		if ( !m_stack.empty() )
		{
			const TNode* topNode = m_stack.back();
			return topNode->value();
		}

		return "";
	}

	const uint32 Reader::GetInnerXML( std::string& outString ) const
	{
		if ( !m_stack.empty() )
		{
			const TNode* topNode = m_stack.back();

			const auto startLength = outString.length();
			rapidxml::print( std::back_inserter( outString ), *topNode, 0 );
			const auto numAdded = outString.length() - startLength;
			return (uint32)numAdded;
		}

		return 0;
	}

	const bool Reader::Open( const char* name )
	{
		// invalid document
		if ( m_stack.empty() )
		{
			m_error = "Trying to access empty XML document";
			return false;
		}

		// get current node
		const TNode* topNode = m_stack.back();
		const TNode* newNode = topNode->first_node( name );
		if ( nullptr != newNode )
		{
			m_error.clear();
			m_stack.push_back( newNode );
			return true;
		}

		// invalid section
		m_error = "XML node does not exist";
		return false;
	}

	const bool Reader::Iterate( const char* name )
	{
		// invalid document
		if ( m_stack.empty() )
		{
			m_error = "Trying to access empty XML document";
			return false;
		}

		// get current node
		const TNode* topNode = m_stack.back();
		if ( 0 == _stricmp( topNode->name(), name ) )
		{
			const TNode* newNode = topNode->next_sibling( name );
			if ( NULL != newNode )
			{
				m_error.clear();
				m_stack.back() = newNode;
				return true;
			}
			else
			{
				m_error.clear();
				m_stack.pop_back();
				return false;
			}
		}
		else
		{
			const TNode* newNode = topNode->first_node( name );
			if ( NULL != newNode )
			{
				m_error.clear();
				m_stack.push_back( newNode );
				return true;
			}
		}

		// invalid section
		m_error = "XML node does not exist";
		return false;
	}

	void Reader::Close()
	{
		if ( m_stack.size() > 1 )
		{
			m_error.clear();
			m_stack.pop_back();
		}
		else
		{
			m_error = "Trying to close top node";
		}
	}

	const uint32 Reader::GetNumAttributes() const
	{
		// load data
		if ( !m_stack.empty() )
		{
			uint32 count = 0;
			const TNode* topNode = m_stack.back();
			const TAttr* attr = topNode->first_attribute();
			while ( nullptr != attr )
			{
				count += 1;
				attr = attr->next_attribute();
			}

			return count;
		}

		return 0;
	}

	const std::string Reader::GetAttribute( const uint32 index, std::string* outName ) const
	{
		// load data
		if ( !m_stack.empty() )
		{
			uint32 count = 0;
			const TNode* topNode = m_stack.back();
			const TAttr* attr = topNode->first_attribute();
			while ( nullptr != attr )
			{
				if ( index == count )
				{
					if ( nullptr != outName )
					{
						*outName = attr->name();
					}

					return attr->value();
				}

				attr = attr->next_attribute();
				count += 1;
			}
		}

		return "";
	}

	const std::string Reader::GetAttribute( const char* name, const std::string& defaultValue /*= std::string()*/ ) const
	{
		// load data
		if ( !m_stack.empty() )
		{
			const TNode* topNode = m_stack.back();
			const TAttr* attr = topNode->first_attribute( name );
			if ( nullptr != attr )
			{
				return std::string( attr->value() );
			}
		}

		// revert to default value
		return defaultValue;
	}

	const std::wstring Reader::GetAttributeW( const char* name, const std::wstring& defaultValue /*= std::string()*/ ) const
	{
		// load data
		if ( !m_stack.empty() )
		{
			const TNode* topNode = m_stack.back();
			const TAttr* attr = topNode->first_attribute( name );
			if ( nullptr != attr )
			{
				std::string ret(attr->value());
				return std::wstring(ret.begin(), ret.end());
			}
		}

		// revert to default value
		return defaultValue;
	}

	const char* Reader::GetAttributeRaw( const char* name, const char* defaultValue /*= ""*/ ) const
	{
		// load data
		if ( !m_stack.empty() )
		{
			const TNode* topNode = m_stack.back();
			const TAttr* attr = topNode->first_attribute( name );
			if ( nullptr != attr )
			{
				return attr->value();
			}
		}

		// revert to default value
		return defaultValue;
	}

	const int Reader::GetAttributeInt( const char* name, const int defaultValue /*= 0*/ ) const
	{
		// load data
		if ( !m_stack.empty() )
		{
			const TNode* topNode = m_stack.back();
			const TAttr* attr = topNode->first_attribute( name );
			if ( nullptr != attr )
			{
				return atoi( attr->value() );
			}
		}

		// revert to default value
		return defaultValue;
	}

	const uint32 Reader::GetAttributeUint( const char* name, const uint32 defaultValue /*= 0*/ ) const
	{
		// load data
		if ( !m_stack.empty() )
		{
			const TNode* topNode = m_stack.back();
			const TAttr* attr = topNode->first_attribute( name );
			if ( nullptr != attr )
			{
				return (uint32)atol( attr->value() );
			}
		}

		// revert to default value
		return defaultValue;
	}

	Reader* Reader::LoadXML( const std::string& filePath )
	{
		// try to open file
		FILE* f = nullptr;
		fopen_s( &f, filePath.c_str(), "rb" );
		if ( nullptr == f )
		{
			return nullptr;
		}

		// get file size
		fseek( f, 0, SEEK_END );
		size_t size = ftell( f );
		fseek( f, 0, SEEK_SET );
		if ( f == 0 )
		{
			// empty file
			fclose(f);
			return nullptr;
		}

		// allocate file buffer
		char* fileData = new char [ size + 1 ];
		if ( fileData == nullptr )
		{
			fclose(f);
			return nullptr;
		}

		// load file data
		if ( size != fread( fileData, 1, size, f ) )
		{
			fclose(f);
			delete [] fileData;
			return nullptr;
		}

		// close file (no longer needed)
		fclose(f);

		// zero terminate
		fileData[ size ] = 0;

		// create XML reader (may have parsing errors but they are to the callign site to handle)
		return new Reader( fileData, false );
	}

	Reader* Reader::LoadXML( const std::wstring& filePath )
	{
		// try to open file
		FILE* f = nullptr;
		_wfopen_s( &f, filePath.c_str(), L"rb" );
		if ( nullptr == f )
		{
			return nullptr;
		}

		// get file size
		fseek( f, 0, SEEK_END );
		size_t size = ftell( f );
		fseek( f, 0, SEEK_SET );
		if ( f == 0 )
		{
			// empty file
			fclose(f);
			return nullptr;
		}

		// allocate file buffer
		char* fileData = new char [ size + 1 ];
		if ( fileData == nullptr )
		{
			fclose(f);
			return nullptr;
		}

		// load file data
		if ( size != fread( fileData, 1, size, f ) )
		{
			fclose(f);
			delete [] fileData;
			return nullptr;
		}

		// close file (no longer needed)
		fclose(f);

		// zero terminate
		fileData[ size ] = 0;

		// create XML reader (may have parsing errors but they are to the callign site to handle)
		return new Reader( fileData, false );
	}

} // xml