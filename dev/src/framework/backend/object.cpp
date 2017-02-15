#include "build.h"
#include "internalFile.h"
#include "internalUtils.h"
#include "properties.h"
#include "object.h"

CObject::CObject()
{
}

CObject::~CObject()
{
	DeleteVector( m_properties );
}

bool CObject::Save( class IBinaryFileWriter& writer ) const
{
	CBinaryFileChunkScope chunk( writer, eFileChunk_PropertyList );
	if ( !chunk ) return false;

	writer << m_properties;
	return true;
}

bool CObject::Load( class IBinaryFileReader& reader )
{
	CBinaryFileChunkScope chunk( reader, eFileChunk_PropertyList );
	if ( !chunk ) return false;

	reader >> m_properties;
	return true;
}