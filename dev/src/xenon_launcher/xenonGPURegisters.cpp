#include "build.h"
#include "xenonGPURegisters.h"

//-----------

CXenonGPUDirtyRegisterTracker::CXenonGPUDirtyRegisterTracker()
{
	ClearAll();
}

CXenonGPUDirtyRegisterTracker::~CXenonGPUDirtyRegisterTracker()
{
}

void CXenonGPUDirtyRegisterTracker::ClearAll()
{
	memset( m_mask, 0, sizeof(m_mask) );
}

void CXenonGPUDirtyRegisterTracker::SetAll()
{
	memset( m_mask, 0xFF, sizeof(m_mask) );
}

//-----------

CXenonGPURegisters::CXenonGPURegisters()
{
	memset( &m_values, 0, sizeof(m_values) );
}

const CXenonGPURegisters::Info& CXenonGPURegisters::GetInfo( const uint32 index )
{
	static Info RegInfo[ NUM_REGISTER_RAWS ];
	static bool RegInfoInitialized = false;

	static Info InvalidReg = { "INVALID", eType_Unknown };

	// initialize REG INFO table
	if ( !RegInfoInitialized )
	{
#define DECLARE_XENON_GPU_REGISTER_RAW(index, type, name) RegInfo[index].m_type = type; RegInfo[index].m_name = #name;
#include "xenonGPURegisterMap.h"
#undef DECLARE_XENON_GPU_REGISTER_RAW

		RegInfoInitialized = true;
	}

	/*FILE* crap = fopen( "H:\\crap.txt", "w" );
	for ( int i=0; i<ARRAYSIZE(RegInfo); ++i )
	{
		const auto& info = RegInfo[i];
		if ( info.m_name && *info.m_name )
		{
			fprintf( crap, "%hs = 0x%04X,\n", info.m_name, i );
		}
	}

	for ( int i=0; i<ARRAYSIZE(RegInfo); ++i )
	{
		const auto& info = RegInfo[i];
		if ( info.m_name && *info.m_name )
		{
			fprintf( crap, "RegInfo.Add( new GPURegisterInfo( %hs, GPURegisterValueType.%hs, 0x%04X ) );\n", info.m_name, (info.m_type == eType_Dword) ? "DataDWord" : "DataFloat", i );
		}
	}

	fclose(crap);*/

	// index out of bounds
	if ( index >= NUM_REGISTER_RAWS )
		return InvalidReg;

	// register invalid
	if ( RegInfo[ index ].m_type == eType_Unknown )
		return InvalidReg;

	// return register info
	return RegInfo[ index ];
}

//-----------
