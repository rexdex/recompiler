#include "build.h"
#include "project.h"
#include "frame.h"

#include "../backend/decodingEnvironment.h"
#include "../backend/decodingContext.h"
#include "../backend/image.h"
#include "../backend/decodingMemoryMap.h"
#include "../backend/platformLibrary.h"
#include "../backend/platformDefinition.h"

//---------------------------------------------------------------------------

Project::Project()
	: m_env( nullptr )
	, m_traceData(nullptr)
	, m_histogramBase( 0 )
{
}

Project::~Project()
{
	delete m_traceData;
	m_traceData = nullptr;
	
	delete m_env;
	m_env = nullptr;
}

bool Project::IsModified() const
{
	return m_env->IsModified();
}

void Project::ClearCachedData()
{
	m_env->GetDecodingContext()->ClearCachedData();
}

bool Project::Save( ILogOutput& log )
{
	return m_env->Save( log, false );
}

bool Project::DecodeInstructionsFromMemory( ILogOutput& log, const uint32 startAddress, const uint32 endAddress )
{
	bool error = false;

	// process the range
	uint32 currentAddress = startAddress;
	while ( currentAddress < endAddress )
	{
		// decode instruction at given context
		uint32 decodedInstructionSize = m_env->GetDecodingContext()->ValidateInstruction( log, currentAddress, false );
		if ( !decodedInstructionSize )
		{
			error = true;
			break;
		}

		// advance
		currentAddress += decodedInstructionSize;
	}

	// return status
	return error;
}

bool Project::LoadTrace( ILogOutput& log, const std::wstring& traceFilePath /*= std::wstring()*/ )
{
	// load new trace data
	auto* trace = ProjectTraceData::LoadFromFile( log, this, traceFilePath );
	if (!trace)
		return false;

	// set new trace
	delete m_traceData;
	m_traceData = trace;
	return true;
}

bool Project::HasBreakpoint( const uint32 codeAddress ) const
{
	return m_env->GetDecodingContext()->GetMemoryMap().GetMemoryInfo( codeAddress ).HasBreakpoint();
}

bool Project::SetBreakpoint( const uint32 codeAddress, const bool flag )
{
	if (flag)
	{
		return m_env->GetDecodingContext()->GetMemoryMap().SetMemoryBlockType( *wxTheFrame->GetLogView(), codeAddress, (uint32)decoding::MemoryFlag::HasBreakpoint, 0 );
	}
	else
	{
		return m_env->GetDecodingContext()->GetMemoryMap().SetMemoryBlockType( *wxTheFrame->GetLogView(), codeAddress, 0, (uint32)decoding::MemoryFlag::HasBreakpoint );
	}	
}

bool Project::ClearAllBreakpoints()
{
	return false;
}

class ApplyHistogramValues : public trace::IBatchVisitor
{
private:
	uint32	m_base;
	uint32	m_last;
	uint32*	m_data;

public:
	ApplyHistogramValues(const uint32 base, const uint32 size, uint32* data)
		: m_data(data)
		, m_last(base+size)
		, m_base(base)
	{}

	virtual bool ProcessFrame(const trace::DataFrame& frame) override final
	{
		const uint32 addr = frame.GetAddress();
		if ( addr >= m_base && addr < m_last )
			m_data[addr-m_base] += 1;

		return true;
	}
};

void Project::BuildHistogram()
{
	// create buffer
	m_histogramBase = m_env->GetImage()->GetBaseAddress();
	m_histogram.resize( m_env->GetImage()->GetMemorySize() );

	// reset data
	memset( &m_histogram[0], 0, m_histogram.size()*sizeof(uint32) );

	// extract code data
	ApplyHistogramValues applyValues( m_histogramBase, m_histogram.size(), &m_histogram[0] );
	m_traceData->BatchVisitAll(applyValues);
}

const uint32 Project::GetHistogramValue(const uint32 memoryAddrses) const
{
	if ( memoryAddrses >= m_histogramBase )
	{
		const uint32 offset = memoryAddrses - m_histogramBase;
		if (offset < m_histogram.size())
			return m_histogram[offset];
	}

	return 0;
}

void Project::BuildMemoryMap()
{
	if ( m_traceData )
		m_traceData->BuildMemoryMap();
}

Project* Project::LoadImageFile( ILogOutput& log, const std::wstring& imagePath, const std::wstring& projectPath )
{
	// get the best platform for given image extension
	auto platforms = platform::Library::GetInstance().FindCompatiblePlatforms( imagePath );
	if ( platforms.empty() )
	{
		log.Error( "Project: No decoding platform found that can load image '%ls'", imagePath.c_str() );
		return nullptr;
	}

	// Use the first platform, TODO: selection
	const auto* platform = platforms[0];

	// importing image
	CScopedTask task( log, "Importing image" );

	// load the image
	const image::Binary* mainImage = platform->LoadImageFromFile( log, imagePath );
	if ( !mainImage )
	{
		log.Error( "Project: Failed to load image from '%ls', platform '%hs'", imagePath.c_str(), platform->GetName().c_str() );
		return nullptr;
	}

	// create decoding context
	decoding::Context* decodingContext = decoding::Context::Create( log, mainImage, platform ); 
	if ( !decodingContext )
	{
		log.Error( "Project: Failed to create decoding context for image '%ls', platform '%hs'", imagePath.c_str(), platform->GetName().c_str() );
		delete mainImage;
		return nullptr;
	}

	// create environment
	auto* env = decoding::Environment::Create( log, platform, mainImage, projectPath );
	if ( !env )
	{
		log.Error( "Project: Failed to create decoding environment for image '%ls', platform '%hs'", imagePath.c_str(), platform->GetName().c_str() );
		return nullptr;
	}

	// cache the image and decoding context
	auto* project = new Project();
	project->m_env = env;

	// initialize address history
	const uint32 entryAddress = mainImage->GetEntryAddress();
	project->m_addressHistory.Reset( entryAddress );

	// initialize configuration
	project->m_config.Reset( projectPath );

	// return created project
	return project;
}

Project* Project::LoadProject( ILogOutput& log, const std::wstring& projectPath )
{
	// create environment
	auto* env = decoding::Environment::Load( log, projectPath );
	if ( !env )
	{
		log.Error( "Project: Failed to load decoding environment from '%ls'", projectPath.c_str() );
		return nullptr;
	}

	// print image sections
	for (int i = 0; i < env->GetImage()->GetNumSections(); ++i)
	{
		auto* section = env->GetImage()->GetSection(i);
		char s[256];
		sprintf(s, "%hs 0x%08X-0x%08X %c%c%c\n", 
			section->GetName().c_str(), section->GetVirtualAddress(), section->GetVirtualAddress() + section->GetVirtualSize(),
			section->CanRead() ? 'r' : '_',
			section->CanWrite() ? 'w' : '_',
			section->CanExecute() ? 'x' : '_');
		OutputDebugStringA(s);
	}

	// cache the image and decoding context
	auto* project = new Project();
	project->m_env = env;

	// initialize address history
	const uint32 entryAddress = env->GetImage()->GetEntryAddress();
	project->m_addressHistory.Reset( entryAddress );

	// initialize configuration
	project->m_config.Reset( projectPath );

	// return created project
	return project;
}