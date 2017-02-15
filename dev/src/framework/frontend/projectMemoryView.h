#pragma once

//-----------------------------------------------------------------------------

/// Memory view for displaying the project data
class CProjectMemoryView : public IMemoryDataView
{
public:
	CProjectMemoryView( class Project* project );
	virtual ~CProjectMemoryView();

private:
	//! IMemoryDataView interface
	virtual uint32 GetLength() const;
	virtual uint32 GetBaseAddress() const;
	virtual uint32 GetRawBytes( const uint32 startOffset, const uint32 endOffset, const uint32 bufferSize, uint8* outBuffer ) const;
	virtual uint32 GetAddressInfo( const uint32 address, uint32& outNumLines, uint32& outNumBytes ) const;
	virtual bool GetAddressText( const uint32 address, IMemoryLinePrinter& printer ) const;
	virtual bool GetAddressMarkers( const uint32 address, uint32& outMarkers, uint32& outLineOffset ) const;
	virtual uint32 GetAddressHitCount( const uint32 address ) const;
	virtual bool ShowContextMenu( class CMemoryView* view, const uint32 startOffset, const uint32 endOffset, const wxPoint& point );
	virtual bool Navigate( class CMemoryView* view, const uint32 startOffset, const uint32 endOffset, const bool bShift );
	virtual bool NavigateBack( class CMemoryView* view );
	virtual void SelectionCursorMoved( class CMemoryView* view, const uint32 newOffset, const bool createHistoryEntry );
	virtual bool GetDirtyMemoryRegion( uint32& outStartOffset, uint32& outEndOffset ) const;
	virtual void ValidateDirtyMemoryRegion( const uint32 startOffset, const uint32 endOffset );

private:
	// project
	class Project*			m_project;

	// base address
	const uint32			m_base;

	// image
	const image::Binary*	m_image;

	// parent decoding context
	decoding::Context*		m_decodingContext;
};

//-----------------------------------------------------------------------------
