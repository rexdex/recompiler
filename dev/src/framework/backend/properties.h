#pragma once

//---------------------------------------------------------------------------

/// Generic property
class RECOMPILER_API CProperty
{
public:
	// property type
	enum EType
	{
		eType_None,
		eType_String,
		eType_Address, // uint32/uint64
		eType_Number,  // integer number
		eType_Boolean,
		eType_DateTime,  // string
	};

private:
	// property name
	std::string		m_name;

	// property type
	EType			m_type;

	// property value
	void*			m_valuePtr;

public:
	// get property name
	inline const char* GetName() const { return m_name.c_str(); }

	// get property type
	inline const EType GetType() const { return m_type; }

	// get raw property value (not safe)
	inline const void* GetRawValue() const { return m_valuePtr; }

public:
	CProperty();
	CProperty( const char* name, const EType type, const void* srcDataToCopyFrom );
	CProperty( const CProperty& other );
	explicit CProperty( const char* name, const uint32 addressValue );
	explicit CProperty( const char* name, const int numberValue );
	explicit CProperty( const char* name, const bool booleanValue );
	explicit CProperty( const char* name, const char* stringValue );
	~CProperty();

	// get value as string
	bool GetValueString( char* outBuffer, const uint32 outBufferSize ) const;

	// save/load property to binary file
	void Save( class IBinaryFileWriter& writer ) const;
	void Load( class IBinaryFileReader& reader );

	// copy
	CProperty& operator=( const CProperty& other );

private:
	void Clear();
	void Set( const EType type, const void* srcDataToCopyFrom );
};

