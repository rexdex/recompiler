#pragma once

//-----------------------------------------------------------------------------

#include "memoryViewMapping.h"
#include "eventListener.h"

//-----------------------------------------------------------------------------

namespace tools
{

	//-----------------------------------------------------------------------------

	/// Memory output printer (helper class)
	class IMemoryLinePrinter
	{
	public:
		virtual ~IMemoryLinePrinter() {};

		// Emit line information
		virtual void Print(const char* text, ...) = 0;
	};

	//-----------------------------------------------------------------------------

	/// Memory view interface
	class IMemoryDataView
	{
	public:
		virtual ~IMemoryDataView() {};

		//! Get length of the memory
		virtual uint32 GetLength() const = 0;

		//! Get base address, only used for display
		virtual uint64 GetBaseAddress() const = 0;

		//! Get memory bytes, returns the memory bytes actually extracted (may be zero)
		virtual uint32 GetRawBytes(const uint32 startOffset, const uint32 endOffset, const uint32 bufferSize, uint8* outBuffer) const = 0;

		//! For given address get the number of lines and bytes we should output, if the address is not aligned the returned values should be zero and the return value should contain the propper address
		virtual uint32 GetAddressInfo(const uint32 offset, uint32& outNumLines, uint32& outNumBytes) const = 0;

		//! Generate the text representation for, only valid addresses should be accepted
		virtual bool GetAddressText(const uint32 offset, IMemoryLinePrinter& printer) const = 0;

		//! Get address hit count (if any)
		virtual uint32 GetAddressHitCount(const uint32 offset) const { return 0; }

		//! Get any special markers to show on the line
		virtual bool GetAddressMarkers(const uint32 offset, uint32& outMarkers, uint32& outLineOffset) const { return false; }

		//! Show context memory for given line range
		virtual bool ShowContextMenu(class MemoryView* view, const uint32 startOffset, const uint32 endOffset, const wxPoint& point) { return false; }

		//! Activate selection (enter key)
		virtual bool Navigate(class MemoryView* view, const uint32 startOffset, const uint32 endOffset, const bool bShift) { return false; }

		//! Navigate back
		virtual bool NavigateBack(class MemoryView* view) { return false; }

		//! Notify about the cursor change
		virtual void SelectionCursorMoved(class MemoryView* view, const uint32 newOffset, const bool createHistoryEntry) {};

		//! Get the dirty region of the view
		virtual bool GetDirtyMemoryRegion(uint32& outStartOffset, uint32& outEndOffset) const { return false; }

		//! Validate dirty memory region
		virtual void ValidateDirtyMemoryRegion(const uint32 startOffset, const uint32 endOffset) { }

		//! Reset whatever cached data you have for given memory range
		virtual void ResetCachedData(const uint32 startOffset, const uint32 endOffset) { }
	};

	//-----------------------------------------------------------------------------

	/// Memory address info (for nice display)
	class MemoryLineEntry
	{
	public:
		inline const char* GetText() const { return m_text; }

		// Create/destroy the line
		void Set(const char* textToCopy);
		void Reset();

	private:
		MemoryLineEntry();
		~MemoryLineEntry();

		friend class MemoryLineAllocator;
		friend class CMemoryLinePrinter;

		// text buffer (managed locally)
		// text type is automatically determined via parsing
		//   anything after ';' is considered a comment unless it's in the string
		//   if the first character is ':' then it's a label
		//   anything inside the [] is an memory address, only the numerical value is considered
		char* m_text;
	};

	//-----------------------------------------------------------------------------

	/// Allocator for the memory line objects
	class MemoryLineAllocator
	{	
	public:
		MemoryLineAllocator();
		~MemoryLineAllocator();

		//! Allocate line entry
		MemoryLineEntry* AllocateLineEntry();

		//! Free line entry
		void FreeLineEntry(MemoryLineEntry* lineEntry);

	private:
		static const uint32 BLOCK_SIZE = 65536;

		struct Block
		{
			MemoryLineEntry*	m_base;
			MemoryLineEntry*	m_end;
			uint32				m_count;
			uint32				m_max;

			Block(void* base, uint32 count)
				: m_base((MemoryLineEntry*)base)
				, m_end(m_base + count)
				, m_max(count)
				, m_count(0)
			{}
		};

		MemoryLineEntry*		m_freeList;
		Block*					m_freeBlock;

		std::vector< Block* >	m_blocks;

		//! Allocate new block of lines
		void AllocateNewBlock();
	};

	//-----------------------------------------------------------------------------

	/// Cache for drawable lines for given memory offset
	class MemoryLineCache : private IMemoryLinePrinter
	{
	public:
		// cache entry
		struct Entry
		{
			MemoryLineEntry*	m_line;
			Entry*				m_next;		// if we have more lines

			inline Entry(MemoryLineEntry* line, Entry*& listHead, Entry*& listTail)
				: m_line(line)
				, m_next(nullptr)
			{
				if (!listHead)
				{
					listHead = listTail = this;
				}
				else
				{
					listTail->m_next = this;
					listTail = this;
				}
			}

			inline void Free(class MemoryLineAllocator& lineAlloc)
			{
				if (nullptr != m_line)
				{
					lineAlloc.FreeLineEntry(m_line);
					m_line = nullptr;
				}

				if (nullptr != m_next)
				{
					m_next->Free(lineAlloc);
					m_next = nullptr;
				}

				delete this;
			}
		};


	public:
		MemoryLineCache(class MemoryLineAllocator* allocator);
		~MemoryLineCache();

		// clear everything
		void Clear();

		// clear cached region
		void ClearRange(const uint32 firstAddress, const uint32 lastAddress);

		// get cached entries for given offset, if not found they will be generated
		const Entry* GetCachedLines(const uint32 offset, IMemoryDataView& dataView);

	private:
		// line allocator 
		class MemoryLineAllocator* m_lineAllocator;

		// exact mapping for offset->entry
		typedef std::map< uint32, Entry* > TEntriesMap;
		TEntriesMap m_entries;

		// access lock
		wxCriticalSection m_entriesLock;

		// line printing
		Entry* m_newEntryHead;
		Entry* m_newEntryTail;

		// IMemoryLinePrinter interface
		virtual void Print(const char* text, ...);
	};

	//-----------------------------------------------------------------------------

	/// Text-like window capable of displaying specialized memory content, the data is supplied via the IMemoryDataView interface
	class MemoryView : public wxWindow, public IEventListener
	{
		DECLARE_EVENT_TABLE();

		friend class CMemoryViewContext;

	protected:
		//! The view on the memory
		IMemoryDataView*			m_view;

		//! Range of memory represented
		int							m_startOffset;
		int							m_endOffset;

		//! First visible line in the current block and first visible memory offect (current)
		int							m_firstVisibleLine;

		//! Range of selected offsets (in lines)
		int							m_selectionStart;
		int							m_selectionEnd;

		//! Line allocator
		MemoryLineAllocator		m_lineAllocator;

		//! Memory <-> Line mapping
		LineMemoryMapping			m_lineMapping;

		//! Cache for lines
		MemoryLineCache			m_lineCache;

		//! Last selection cursor offset
		uint32						m_selectionCursorOffset;

		//! Highlighted address (not selection)
		uint32						m_highlightedOffset;

		//! Mouse modes
		bool						m_selectionDragMode;
		bool						m_contextMenuMode;

		//! Drawing styles
		enum EDrawingStyle
		{
			eDrawingStyle_Normal,
			eDrawingStyle_Comment,
			eDrawingStyle_Error,
			eDrawingStyle_Label,
			eDrawingStyle_Address,
			eDrawingStyle_Validation,
			eDrawingStyle_String,

			eDrawingStyle_MAX,
		};

		//! Drawing modes
		enum EDrawingMode
		{
			eDrawingMode_Normal,
			eDrawingMode_Highlighted,
			eDrawingMode_Selected,

			eDrawingMode_MAX,
		};

		//! Line markers
		enum EDrawingMarkers
		{
			eDrawingMarker_Visited,
			eDrawingMarker_Breakpoint,

			eDrawingMarker_MAX,
		};

		//! Drawing style
		struct DrawingStyle
		{
			wxFont		m_font;
			wxBrush		m_backBrush[eDrawingStyle_MAX][eDrawingMode_MAX];
			wxPen		m_backPen[eDrawingStyle_MAX][eDrawingMode_MAX];
			wxPen		m_textPen[eDrawingStyle_MAX][eDrawingMode_MAX];

			wxBrush		m_markerBrush[eDrawingMarker_MAX];
			wxPen		m_markerPens[eDrawingMarker_MAX];

			uint32		m_lineHeight;
			uint32		m_lineSeparation;
			uint32		m_addressOffset;
			uint32		m_hitcountSize;
			uint32		m_dataOffset;
			uint32		m_textOffset;

			uint32		m_markerWidth;
			uint32		m_tabSize;

			bool		m_drawHitCount;

			DrawingStyle();
		};

		//! Drawing style data
		DrawingStyle		m_style;

	public:
		MemoryView(wxWindow* parent);
		virtual ~MemoryView();

		//! Clear content (delete all lines)
		void ClearContent();

		//! Clear cached data (resolved lines)
		void ClearCachedData();

		//! Set the data view, will build the initial layout
		void SetDataView(IMemoryDataView* view);

		//! Set highlighted offset
		bool SetHighlightedOffset(const uint32 offset);

		//! Ensure the given offset is visible, will update the selection cursor as well
		bool SetActiveOffset(const uint32 offset, bool createHistoryEntry = false);

		//! Enable/disable drawing of the hit coutn
		void SetHitcountDisplay(const bool enabled);

		//! Refresh the diry part of the memory region (if any)
		void RefreshDirtyMemoryRegion();

		//! Refresh memory region
		void RefreshMemoryRegion(const uint32 startOffset, const uint32 endOffset);

		//! Refresh memory region using RVA addressing
		void RefreshMemoryRegionRVA(const uint32 startRVA, const uint32 endRVA);

		//! Get memory offsets for current selection (may be the same)
		bool GetSelectionOffsets(uint32& startOffset, uint32& endOffset) const;

		//! Get RVA offsets for current selection (may be the same)
		bool GetSelectionAddresses(uint64& startAddress, uint64& endAddress) const;

		//! Get the RVA for cursor position (selection end)
		uint64 GetCurrentRVA() const;

	private:
		//! Update scrollbar limits
		void UpdateScrollbar();

		//! Set new first visible address
		void SetFirstVisibleLine(int line);

		//! Ensure the memory location is visible
		void EnsureVisible(int address);

		//! Move current selection
		void MoveSelectionCursor(int numLines, const bool selectionEnd);

		//! Select the line
		void SetSelectionCursor(int lineIndex, const bool selectionEnd);

		//! Get total number of lines in the view
		uint32 GetNumLines() const;

		//! find line at given pixel coordinates
		int GetLineAtPixel(const wxPoint& point) const;

		//! Navigate to selection (enter key pressed)
		void NavigateSelection();

	private:
		void OnMouseDown(wxMouseEvent& event);
		void OnMouseUp(wxMouseEvent& event);
		void OnMouseMove(wxMouseEvent& event);
		void OnMouseWheel(wxMouseEvent& event);
		void OnPaint(wxPaintEvent& event);
		void OnErase(wxEraseEvent& event);
		void OnKeyDown(wxKeyEvent& event);
		void OnCharHook(wxKeyEvent& event);
		void OnScroll(wxScrollWinEvent& event);
		void OnSize(wxSizeEvent& event);
		void OnCaptureLost(wxMouseCaptureLostEvent& event);

		void OnContextMenu(wxCommandEvent& context);

		void OnMarkCode(wxCommandEvent& event);

	private:
		virtual void OnSyncUpdate();
	};

} // tools