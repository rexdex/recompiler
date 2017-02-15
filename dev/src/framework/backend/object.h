#pragma once

//---------------------------------------------------------------------------

class CProperty;
class IBinaryFileWriter;
class IBinaryFileReader;

//---------------------------------------------------------------------------

/// Savable object (with optional properties)
class RECOMPILER_API CObject
{
private:
	typedef std::vector< CProperty* >		TProperties;
	TProperties					m_properties;

public:
	inline const uint32 GetNumProperties() const { return (uint32)m_properties.size(); }
	inline CProperty* GetProperty( const uint32 index ) const { return m_properties[index]; }

public:
	CObject();
	virtual ~CObject();

	// save/load the object
	virtual bool Save( class IBinaryFileWriter& writer ) const;
	virtual bool Load( class IBinaryFileReader& reader );
};

//---------------------------------------------------------------------------
