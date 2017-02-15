#include "build.h"
#include "memoryView.h"
#include "frame.h"

#include "project.h"
#include "projectMemoryView.h"
#include "projectMemoryContextMenu.h"

#include "../backend/image.h"
#include "../backend/decodingMemoryMap.h"
#include "../backend/decodingContext.h"
#include "../backend/decodingCommentMap.h"
#include "../backend/decodingNameMap.h"
#include "../backend/decodingAddressMap.h"
#include "../backend/decodingInstruction.h"
#include "../backend/decodingEnvironment.h"

CProjectMemoryView::CProjectMemoryView( class Project* project )
	: m_project( project )
	, m_image( project->GetEnv().GetImage() )
	, m_base( project->GetEnv().GetImage()->GetBaseAddress() )
	, m_decodingContext( project->GetEnv().GetDecodingContext() )
{
}

CProjectMemoryView::~CProjectMemoryView()
{
}

uint32 CProjectMemoryView::GetLength() const
{
	return m_image->GetMemorySize();
}

uint32 CProjectMemoryView::GetBaseAddress() const
{
	return m_image->GetBaseAddress();
}

uint32 CProjectMemoryView::GetRawBytes( const uint32 startOffset, const uint32 endOffset, const uint32 bufferSize, uint8* outBuffer ) const
{
	if ( startOffset >= m_image->GetMemorySize() )
	{
		return 0;
	}

	// clamp to memory size
	uint32 realEndOffset = endOffset;
	if ( realEndOffset > m_image->GetMemorySize() )
	{
		realEndOffset = m_image->GetMemorySize();
	}

	// clam tp buffer size
	if ( realEndOffset > (startOffset + bufferSize) )
	{
		realEndOffset = startOffset + bufferSize;
	}

	// copy memory
	memcpy( outBuffer, m_image->GetMemory() + startOffset, realEndOffset - startOffset );
	return realEndOffset - startOffset;
}

uint32 CProjectMemoryView::GetAddressInfo( const uint32 address, uint32& outNumLines, uint32& outNumBytes ) const
{
	const decoding::MemoryFlags flags = m_decodingContext->GetMemoryMap().GetMemoryInfo( m_base + address );

	// invalid content
	if ( !flags.IsValid() )
	{
		outNumBytes = 1;
		outNumLines = 1;
		return address;
	}

	// only work for the "first byte" stuff
	if ( !flags.IsFirstByte() )
	{
		outNumBytes = 1; // skip
		outNumLines = 0;
		return address;
	}

	// by default we always output 1 byte and one line
	outNumBytes = flags.GetSize();
	outNumLines = 1;

	// section stuff
	if ( flags.IsSectionStart() )
	{
		outNumLines += 7; // section header
	}

	// code
	if ( flags.IsExecutable() )
	{
		const decoding::InstructionFlags iflags = flags.GetInstructionFlags();

		// function start
		if ( iflags.IsFunctionStart() )
		{
			outNumLines += 3; // function header

			if ( flags.HasComment() ) outNumLines += 1;
		}
		else if ( iflags.IsBlockStart() )
		{
			outNumLines += 1; // block separator

			if ( flags.HasComment() ) outNumLines += 1;
		}

		// label
		if ( iflags.IsDynamicJumpTarget() || iflags.IsDynamicCallTarget() ||
			iflags.IsStaticCallTarget() || iflags.IsStaticJumpTarget() ||
			iflags.IsEntryPoint() )
		{
			outNumLines += 1; // label line
		}
	}
	else
	{
		if ( flags.HasComment() )
		{
			outNumLines += 1;
		}
	}

	return address;
}

uint32 CProjectMemoryView::GetAddressHitCount( const uint32 address ) const
{
	// extract hitcount from current trace data
	if (m_project && m_project->GetTrace())
		return m_project->GetHistogramValue(m_base + address);

	// no hit count without the trace data
	return 0;
}

bool CProjectMemoryView::GetAddressMarkers( const uint32 address, uint32& outMarkers, uint32& outLineOffset ) const
{
	const decoding::MemoryFlags flags = m_decodingContext->GetMemoryMap().GetMemoryInfo( m_base + address );

	// visited code marker
	if (flags.IsExecutable() && flags.WasVisited())
		outMarkers |= 1;

	// breakpoint marker
	if (flags.HasBreakpoint())
		outMarkers |= 2;

	// draw markers
	return true;
}

bool CProjectMemoryView::GetAddressText( const uint32 address, IMemoryLinePrinter& printer ) const
{
	const decoding::MemoryFlags flags = m_decodingContext->GetMemoryMap().GetMemoryInfo( m_base + address );
	
	// empty section data
	if ( !flags.IsValid() )
	{
		printer.Print( "^ Invalid memory" );
		return true;
	}

	// section start, always output big header
	if ( flags.IsSectionStart() )
	{
		// find the section by current address
		const image::Section* section = nullptr;
		const uint32 numSections = m_image->GetNumSections();
		for ( uint32 i=0; i<numSections; ++i )
		{
			const image::Section* test = m_image->GetSection(i);
			if ( address >= test->GetVirtualAddress() && address < (test->GetVirtualAddress() + test->GetVirtualSize()) )
			{
				section = test;
				break;
			}
		}

		printer.Print( ";------------------------------------------------" );
		printer.Print( "; Memory dump of section \"%s\"", section->GetName() );
		printer.Print( ";  - Virtual Address: 0x%08X", section->GetVirtualAddress() );
		printer.Print( ";  - Virtual Size: 0x%08X", section->GetVirtualSize() );
		printer.Print( ";  - Physical Address: 0x%08X", section->GetPhysicalOffset() );
		printer.Print( ";  - Physical Size: 0x%08X", section->GetPhysicalSize() );
		printer.Print( ";------------------------------------------------" );
	}

	// extra comment
	char extraBuf[4096];
	extraBuf[0] = 0;

	// code stuff
	if ( flags.IsExecutable() )
	{
		const decoding::InstructionFlags iflags = flags.GetInstructionFlags();

		// extra comment ?
		const char* comment = "";
		if ( flags.HasComment() )
		{
			comment = m_decodingContext->GetCommentMap().GetComment( m_base + address );
		}

		// function start
		if ( iflags.IsFunctionStart() )
		{
			printer.Print( ";------------------------------------------------" );

			// function type
			const char* functionType = "Function";
			if ( iflags.IsImportFunction() )
			{
				functionType = "Import function";
			}

			// get name
			const char* functionName = m_decodingContext->GetNameMap().GetName( m_base + address );
			if ( functionName[0] )
			{
				printer.Print( "; %s %s", functionType, functionName );
			}
			else
			{
				printer.Print( "; %s at %Xh", functionType, address  );
			}

			// show decoded function information: arguments, return value, stack size
			printer.Print( "; No header/stack information avaiable" );

			// extra command is printed directly
			if ( flags.HasComment() )
			{
				printer.Print( "; %s", comment );
			}
		}
		else if ( iflags.IsBlockStart() )
		{
			printer.Print( ";------------------------------------------------" ); // block separator

			// extra command is printed directly
			if ( flags.HasComment() )
			{
				printer.Print( "; %s", comment );
			}
		}

		// label
		if ( iflags.IsDynamicJumpTarget() || iflags.IsDynamicCallTarget() ||
			iflags.IsStaticCallTarget() || iflags.IsStaticJumpTarget() ||
			iflags.IsEntryPoint() )
		{
			// get the label name
			// TODO: implement the know functions names ?
			char labelName[ 64 ];
			if ( iflags.IsEntryPoint() )
			{
				strcpy_s( labelName, sizeof(labelName), "EntryPoint" );
			}
			else
			{
				sprintf_s( labelName, sizeof(labelName), "L%06X", address );
			}

			// label flags
			std::string labelFlags;
			if ( iflags.IsDynamicCallTarget() ) labelFlags += "DCALL ";
			if ( iflags.IsDynamicJumpTarget() ) labelFlags += "DJUMP ";
			if ( iflags.IsStaticCallTarget() ) labelFlags += "CALL ";
			if ( iflags.IsReferencedInData() ) labelFlags += "PTR ";

			// label flags
			if ( labelFlags.empty() )
			{
				printer.Print( ":%s", labelName );
			}
			else
			{
				printer.Print( ":%s\t; %s", labelName, labelFlags.c_str() );
			}
		}

		// try to get the (cached) decoded instruction for dispaly
		bool hadValidCode = false;
		if ( flags.GetInstructionFlags().IsThunk() )
		{
			const uint32 data = *(uint32*)(m_image->GetMemory() + address);
			printer.Print( "dd %08Xh ; THUNK - filled at runtime", data );
			hadValidCode = true;
		}
		else if ( flags.GetInstructionFlags().IsValid() )
		{
			decoding::Instruction decoded;
			const uint32 codeAddress = m_base + address;
			m_decodingContext->DecodeInstruction( ILogOutput::DevNull(), codeAddress, decoded );
			if ( decoded.IsValid() )
			{
				char buffer[ 4096 ];
				char* bufferWritter = buffer;
				decoded.GenerateText( codeAddress, bufferWritter, buffer + sizeof(buffer) );

				const bool hasBranchTarget = flags.GetInstructionFlags().IsBranchTarget();
				if ( hasBranchTarget )
				{
					const uint32 branchAddr = m_decodingContext->GetAddressMap().GetReferencedAddress( m_base + address );
					if ( branchAddr != 0 )
					{
						const char* branchName = m_decodingContext->GetNameMap().GetName( branchAddr );
						if ( branchName[0] )
						{
							sprintf_s( extraBuf, "; TARGET=%s %s", branchName, comment );
						}
						else
						{
							sprintf_s( extraBuf, "; TARGET=%06Xh %s", branchAddr, comment );
						}
					}
				}

				// return flags
				if (flags.GetInstructionFlags().IsRet())
				{
					if (!extraBuf[0]) strcpy_s(extraBuf, "; ");
					strcat_s(extraBuf, "RETURN ");
				}

				// memory mapped
				if (flags.GetInstructionFlags().IsMappedMemory())
				{
					if (!extraBuf[0]) strcpy_s(extraBuf, "; ");
					strcat_s(extraBuf, "MMAP ");
				}

				// generic comment
				if (comment && comment[0])
				{
					if (!extraBuf[0]) strcpy_s(extraBuf, "; ");
					strcat_s(extraBuf, comment);
				}

				// instruction comment
				char tempComment[4096];
				char* tempCommentWriter = tempComment;
				if (decoded.GenerateComment(codeAddress, tempCommentWriter, tempComment + sizeof(tempComment)))
				{
					if (!extraBuf[0]) strcpy_s(extraBuf, "; ");
					strcat_s(extraBuf, tempComment);
				}					

				// final assembple
				printer.Print( "%s %s", buffer, extraBuf );
				hadValidCode = true;
			}
		}

		// code was not valid - output tempshit
		if ( !hadValidCode )
		{			
			// invalid :-(
			if ( flags.GetSize() == 1 )
			{
				const uint32 data = *(uint8*)(m_image->GetMemory() + address);
				printer.Print( "db %02Xh ; Unknown instruction", data );
			}
			else if ( flags.GetSize() == 2 )
			{
				const uint32 data = *(uint16*)(m_image->GetMemory() + address);
				printer.Print( "dw %04Xh ; Unknown instruction", data );
			}
			else if ( flags.GetSize() == 4 )
			{
				const uint32 data = *(uint32*)(m_image->GetMemory() + address);
				printer.Print( "dd %08Xh ; Unknown instruction", data );
			}
			else
			{
				printer.Print( " ; Unknown instruction" );
			}
		}

		return true;
	}

	// data comment is in new line
	if ( flags.HasComment() )
	{
		const char* comment = m_decodingContext->GetCommentMap().GetComment( m_base + address );
		printer.Print( "; %s", comment );
	}

	// swap bytes
	const bool swapEndianess = true;

	// target code location
	if ( flags.GetDataFlags().IsCodePtr() )
	{
		if ( flags.GetSize() == 4 )
		{
			const uint32 codeAddress = _byteswap_ulong( *(uint32*)(m_image->GetMemory() + address ) );
			const char* name = m_decodingContext->GetNameMap().GetName( codeAddress );
			if (name && *name)
			{
				sprintf_s( extraBuf, "; CODE PTR = %06Xh (%s)", codeAddress, name );
			}
			else
			{
				sprintf_s( extraBuf, "; CODE PTR = %06Xh", codeAddress );
			}
		}
		else
		{
			sprintf_s( extraBuf, "; CODE PTR " );
		}
	}

	// target data location
	if ( flags.GetDataFlags().IsDataPtr() )
	{
		if ( flags.GetSize() == 4 )
		{
			const uint32 dataAddres = _byteswap_ulong( *(uint32*)(m_image->GetMemory() + address ) );
			const char* name = m_decodingContext->GetNameMap().GetName( dataAddres );
			if (name && *name)
			{
				sprintf_s( extraBuf, "; DATA PTR = %06Xh (%s)", dataAddres, name );
			}
			else
			{
				sprintf_s( extraBuf, "; DATA PTR = %06Xh", dataAddres );
			}
		}
		else
		{
			sprintf_s( extraBuf, "; DATA PTR " );
		}
	}

	// floating point value
	else  if ( flags.GetDataFlags().IsFloat() )
	{
		const uint32 data = *(uint32*)(m_image->GetMemory() + address);
		const uint32 rdata = swapEndianess ? _byteswap_ulong(data) : data;
		sprintf_s( extraBuf, "; float = %f", *(float*)&rdata );
	}

	// references
	if ( flags.GetDataFlags().IsCodeRef() )
	{
		if (!extraBuf[0]) strcpy_s( extraBuf, "; ");
		strcat_s( extraBuf, "CODEREF " );
	}
	if ( flags.GetDataFlags().IsDataRef() )
	{
		if (!extraBuf[0]) strcpy_s( extraBuf, "; ");
		strcat_s( extraBuf, "DATAREF " );
	}

	// sort by size
	if ( flags.GetSize() == 1 )
	{
		const uint32 data = *(uint8*)(m_image->GetMemory() + address);
		printer.Print( "db %02Xh %s", data, extraBuf );
	}
	else if ( flags.GetSize() == 2 )
	{
		const uint16 data = *(uint16*)(m_image->GetMemory() + address);
		const uint16 rdata = swapEndianess ? _byteswap_ushort(data) : data;
		printer.Print( "dw %04Xh %s", rdata, extraBuf );
	}
	else if ( flags.GetSize() == 4 )
	{
		const uint32 data = *(uint32*)(m_image->GetMemory() + address);
		const uint32 rdata = swapEndianess ? _byteswap_ulong(data) : data;
		printer.Print( "dd %08Xh %s", rdata, extraBuf );
	}
	else if ( flags.GetSize() == 8 )
	{
		const uint64 data = *(const uint64*)(m_image->GetMemory() + address);
		const uint64 rdata = swapEndianess ? _byteswap_uint64(data) : data;
		printer.Print( "dd %0168llXh %s", rdata, extraBuf );
	}
	else if ( flags.GetSize() == 16 )
	{
		const uint32* data = (uint32*)(m_image->GetMemory() + address);
		printer.Print( "dd %08Xh %08Xh %08Xh %08Xh %s", data[0], data[1], data[2], data[3], extraBuf );
	}
	else
	{
		printer.Print( "; unknown data format ");
	}
	
	// done
	return true;
}

bool CProjectMemoryView::ShowContextMenu( class CMemoryView* view, const uint32 startOffset, const uint32 endOffset, const wxPoint& point )
{
	// no range
	if ( startOffset == endOffset )
	{
		return false;
	}

	// get the source section of the image
	const image::Section* section = m_image->FindSectionForOffset( startOffset );
	if ( section == nullptr || section != m_image->FindSectionForOffset( endOffset-1 ) )
	{
		return false;
	}

	// calculate start and ending address
	const uint32 startAddress = m_image->GetBaseAddress() + startOffset;
	const uint32 endAddress = m_image->GetBaseAddress() + endOffset;

	// show the context menu
	CProjectMemoryViewContextMenu menu( m_project, section, startAddress, endAddress );
	menu.Show( view, point );

	// done
	return true;
}

bool CProjectMemoryView::Navigate( class CMemoryView* view, const uint32 startOffset, const uint32 endOffset, const bool bShift )
{
	// no range
	if ( startOffset == endOffset )
	{
		return false;
	}

	// get the source section of the image
	const image::Section* section = m_image->FindSectionForOffset( startOffset );
	if ( section == nullptr || section != m_image->FindSectionForOffset( endOffset-1 ) )
	{
		return false;
	}

	// calculate start and ending address
	const uint32 startAddress = m_image->GetBaseAddress() + startOffset;
	const uint32 endAddress = m_image->GetBaseAddress() + endOffset;

	// navigate to branch address
	if (bShift && wxTheFrame->NavigateUrl(wxString::Format("referencers:%08Xh", startAddress)))
		return true;

	return wxTheFrame->NavigateUrl(wxString::Format("reference:%08Xh", startAddress));
}

bool CProjectMemoryView::NavigateBack( class CMemoryView* view )
{
	return wxTheFrame->NavigateUrl("back");
}

void CProjectMemoryView::SelectionCursorMoved( class CMemoryView* view, const uint32 newOffset, const bool createHistoryEntry )
{
	const uint32 rva = m_image->GetBaseAddress() + newOffset;
	if ( m_image->IsValidAddress( rva ) )
	{
		m_project->GetAddressHistory().UpdateAddress( rva, createHistoryEntry );
	}
}

bool CProjectMemoryView::GetDirtyMemoryRegion( uint32& outStartOffset, uint32& outEndOffset ) const
{
	// get the current dirty range
	uint32 startRVA=0, endRVA = 0;
	if ( m_decodingContext->GetMemoryMap().GetDirtyRange( startRVA, endRVA ) )
	{
		outStartOffset = startRVA - m_base;
		outEndOffset = endRVA - m_base;
		return true;
	}

	// no dirty range
	return false;
}

void CProjectMemoryView::ValidateDirtyMemoryRegion( const uint32 startOffset, const uint32 endOffset )
{
	m_decodingContext->GetMemoryMap().Validate();
}