#pragma once
#include "memoryView.h"

namespace tools
{
	//-----------------------------------------------------------------------------

	class ProjectImageTab;

	//-----------------------------------------------------------------------------

	/// Memory view for displaying the project data
	class ImageMemoryView : public IMemoryDataView
	{
	public:
		ImageMemoryView(const std::shared_ptr<ProjectImage>& projectImage, ProjectImageTab* imageTab);
		virtual ~ImageMemoryView();

	protected:
		//! IMemoryDataView interface
		virtual uint32 GetLength() const;
		virtual uint64 GetBaseAddress() const;
		virtual uint32 GetRawBytes(const uint32 startOffset, const uint32 endOffset, const uint32 bufferSize, uint8* outBuffer) const;
		virtual uint32 GetAddressInfo(const uint32 offset, uint32& outNumLines, uint32& outNumBytes) const;
		virtual bool GetAddressText(const uint32 offset, IMemoryLinePrinter& printer) const;
		virtual bool GetAddressMarkers(const uint32 offset, uint32& outMarkers, uint32& outLineOffset) const;
		virtual uint32 GetAddressHitCount(const uint32 offset) const;
		virtual bool ShowContextMenu(class MemoryView* view, const uint32 startOffset, const uint32 endOffset, const wxPoint& point);
		virtual bool Navigate(class MemoryView* view, const uint32 startOffset, const uint32 endOffset, const bool bShift);
		virtual bool NavigateBack(class MemoryView* view);
		virtual void SelectionCursorMoved(class MemoryView* view, const uint32 newOffset, const bool createHistoryEntry);
		virtual bool GetDirtyMemoryRegion(uint32& outStartOffset, uint32& outEndOffset) const;
		virtual void ValidateDirtyMemoryRegion(const uint32 startOffset, const uint32 endOffset);

	private:
		// project
		const std::shared_ptr<ProjectImage> m_projectImage;

		// decoded image
		const image::Binary* m_imageData; // ref

		// parent decoding context
		decoding::Context* m_decodingContext; // ref

		// owner
		ProjectImageTab* m_imageTab;

		// base address
		uint64 m_base;
		uint32 m_size;
	};

	//-----------------------------------------------------------------------------

} // tools