#include "build.h"
#include "memoryView.h"
#include "progressDialog.h"

namespace tools
{

	static const char HexChar[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	//-----------------------------------------------------------------------------

	MemoryLineEntry::MemoryLineEntry()
		: m_text(nullptr)
	{
	}

	MemoryLineEntry::~MemoryLineEntry()
	{
		Reset();
	}

	void MemoryLineEntry::Set(const char* textToCopy)
	{
		const uint32 len = strlen(textToCopy);
		m_text = new char[len + 1];
		memcpy(m_text, textToCopy, len);
		m_text[len] = 0;
	}

	void MemoryLineEntry::Reset()
	{
		if (nullptr != 0)
		{
			delete[] m_text;
			m_text = nullptr;
		}
	}

	//-----------------------------------------------------------------------------

	MemoryLineAllocator::MemoryLineAllocator()
		: m_freeList(nullptr)
		, m_freeBlock(nullptr)
	{
		AllocateNewBlock();
	}

	MemoryLineAllocator::~MemoryLineAllocator()
	{
		for (size_t i = 0; i<m_blocks.size(); ++i)
		{
			delete[](char*)(m_blocks[i]->m_base);
			delete m_blocks[i];
		}
	}

	void MemoryLineAllocator::AllocateNewBlock()
	{
		char* mem = new char[BLOCK_SIZE];
		const uint32 numLinesPerBlock = BLOCK_SIZE / sizeof(MemoryLineEntry);
		m_freeBlock = new Block(mem, numLinesPerBlock);
		m_blocks.push_back(m_freeBlock);
	}

	MemoryLineEntry* MemoryLineAllocator::AllocateLineEntry()
	{
		// use the free list if possible
		if (m_freeList != nullptr)
		{
			MemoryLineEntry* entry = m_freeList;
			m_freeList = (MemoryLineEntry*)m_freeList->m_text;
			return entry;
		}

		// allocate new block
		if (!m_freeBlock || m_freeBlock->m_count >= m_freeBlock->m_max)
		{
			AllocateNewBlock();
		}

		// try to get from current block
		MemoryLineEntry* entry = m_freeBlock->m_base + m_freeBlock->m_count;
		m_freeBlock->m_count += 1;
		return entry;
	}

	void MemoryLineAllocator::FreeLineEntry(MemoryLineEntry* lineEntry)
	{
		// free internals
		lineEntry->Reset();

		// add to free list
		lineEntry->m_text = (char*)m_freeList;
		m_freeList = lineEntry;
	}

	//-----------------------------------------------------------------------------

	MemoryLineCache::MemoryLineCache(MemoryLineAllocator* allocator)
		: m_lineAllocator(allocator)
	{
	}

	MemoryLineCache::~MemoryLineCache()
	{
	}

	void MemoryLineCache::Clear()
	{
		// cleanup the map
		for (TEntriesMap::const_iterator it = m_entries.begin();
			it != m_entries.end(); ++it)
		{
			it->second->Free(*m_lineAllocator);
		}

		m_entries.clear();
	}

	void MemoryLineCache::ClearRange(const uint32 firstAddress, const uint32 lastAddress)
	{
		for (TEntriesMap::iterator it = m_entries.begin();
			it != m_entries.end(); )
		{
			if (it->first >= firstAddress && it->first <= lastAddress)
			{
				it->second->Free(*m_lineAllocator);
				it = m_entries.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	const MemoryLineCache::Entry* MemoryLineCache::GetCachedLines(const uint32 offset, IMemoryDataView& dataView)
	{
		wxCriticalSectionLocker lock(m_entriesLock);

		// find a match for given offset
		const TEntriesMap::const_iterator it = m_entries.find(offset);
		if (it != m_entries.end())
		{
			return it->second;
		}

		// to many cached entries, cleanup the cache
		if (m_entries.size() > 1024)
		{
			Clear();
		}

		// reset printing
		m_newEntryHead = nullptr;
		m_newEntryTail = nullptr;

		// compile the lines
		dataView.GetAddressText(offset, *this);

		// lines added ?
		if (nullptr != m_newEntryHead)
		{
			// set the entry in the map
			m_entries[offset] = m_newEntryHead;
		}

		// return the cached entries
		return m_newEntryHead;
	}

	void MemoryLineCache::Print(const char* text, ...)
	{
		char buffer[4096];
		va_list args;

		// compile the text buffer
		va_start(args, text);
		vsprintf_s(buffer, sizeof(buffer), text, args);
		va_end(args);

		// create the line
		MemoryLineEntry* line = m_lineAllocator->AllocateLineEntry();
		if (nullptr != line)
		{
			// set the line text
			line->Set(buffer);

			// create entry
			new Entry(line, m_newEntryHead, m_newEntryTail);
		}
	}

	//-----------------------------------------------------------------------------

	BEGIN_EVENT_TABLE(MemoryView, wxWindow)
		EVT_LEFT_DOWN(MemoryView::OnMouseDown)
		EVT_LEFT_DCLICK(MemoryView::OnMouseDown)
		EVT_RIGHT_DOWN(MemoryView::OnMouseDown)
		EVT_LEFT_UP(MemoryView::OnMouseUp)
		EVT_RIGHT_UP(MemoryView::OnMouseUp)
		EVT_MOTION(MemoryView::OnMouseMove)
		EVT_MOUSEWHEEL(MemoryView::OnMouseWheel)
		EVT_MOUSE_CAPTURE_LOST(MemoryView::OnCaptureLost)
		EVT_PAINT(MemoryView::OnPaint)
		EVT_ERASE_BACKGROUND(MemoryView::OnErase)
		EVT_KEY_DOWN(MemoryView::OnKeyDown)
		EVT_CHAR_HOOK(MemoryView::OnCharHook)
		EVT_SCROLLWIN(MemoryView::OnScroll)
		EVT_SIZE(MemoryView::OnSize)
		END_EVENT_TABLE()

		MemoryView::DrawingStyle::DrawingStyle()
		: m_lineHeight(18)
		, m_lineSeparation(0)
		, m_addressOffset(20)
		, m_hitcountSize(40)
		, m_dataOffset(100)
		, m_textOffset(400)
		, m_tabSize(100)
		, m_markerWidth(8)
		, m_drawHitCount(false)
	{
		wxFontInfo fontInfo(8);
		fontInfo.Family(wxFONTFAMILY_SCRIPT);
		fontInfo.FaceName("Courier New");
		fontInfo.AntiAliased(true);
		m_font = wxFont(fontInfo);

		// default background color
		const wxColour defaultBackground(40, 40, 40);
		const wxColour defaultSelection(20, 20, 128);
		const wxColour defaultHighlight(20, 100, 128);


		// colors
		wxColour backColor[eDrawingStyle_MAX][eDrawingMode_MAX];
		wxColour textColor[eDrawingStyle_MAX][eDrawingMode_MAX];

		// set text colors
		textColor[eDrawingStyle_Normal][eDrawingMode_Normal] = wxColour(255, 255, 255);
		textColor[eDrawingStyle_Comment][eDrawingMode_Normal] = wxColour(128, 128, 128);
		textColor[eDrawingStyle_Error][eDrawingMode_Normal] = wxColour(255, 128, 128);
		textColor[eDrawingStyle_Label][eDrawingMode_Normal] = wxColour(128, 255, 255);
		textColor[eDrawingStyle_Address][eDrawingMode_Normal] = wxColour(200, 128, 200);
		textColor[eDrawingStyle_Validation][eDrawingMode_Normal] = wxColour(255, 128, 128);

		// selection drawing style - copy text params, override background param
		for (uint32 i = 0; i<eDrawingStyle_MAX; ++i)
		{
			backColor[i][eDrawingMode_Normal] = defaultBackground;
			backColor[i][eDrawingMode_Selected] = defaultSelection;
			textColor[i][eDrawingMode_Selected] = textColor[i][0];
			backColor[i][eDrawingMode_Highlighted] = defaultHighlight;
			textColor[i][eDrawingMode_Highlighted] = textColor[i][0];
		}

		// create pens and brushes
		for (uint32 j = 0; j<eDrawingMode_MAX; ++j)
		{
			for (uint32 i = 0; i<eDrawingStyle_MAX; ++i)
			{
				m_backBrush[i][j] = wxBrush(backColor[i][j], wxBRUSHSTYLE_SOLID);
				m_backPen[i][j] = wxPen(backColor[i][j], 1, wxPENSTYLE_SOLID);
				m_textPen[i][j] = wxPen(textColor[i][j], 1, wxPENSTYLE_SOLID);
			}
		}

		// marker colors
		wxColour markertColors[eDrawingMarker_MAX];
		markertColors[eDrawingMarker_Visited] = wxColour(128, 150, 128);
		markertColors[eDrawingMarker_Breakpoint] = wxColour(200, 128, 128);

		// create brushes/pens
		for (uint32 i = 0; i<eDrawingMarker_MAX; ++i)
		{
			m_markerBrush[i] = wxBrush(markertColors[i], wxBRUSHSTYLE_SOLID);
			m_markerPens[i] = wxPen(markertColors[i], 1, wxPENSTYLE_SOLID);
		}
	}

	MemoryView::MemoryView(wxWindow* parent)
		: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxWANTS_CHARS)
		, m_view(nullptr)
		, m_lineCache(&m_lineAllocator)
		, m_highlightedOffset(0)
		, m_startOffset(0)
		, m_endOffset(0)
		, m_selectionStart(0)
		, m_selectionEnd(0)
		, m_firstVisibleLine(0)
		, m_selectionDragMode(false)
	{
	}

	MemoryView::~MemoryView()
	{
		ClearContent();
	}

	void MemoryView::ClearContent()
	{
		// cleanup current lines (they are allocated from allocator so no delete)
		m_lineCache.Clear();
		m_lineMapping.Clear();

		// reset data
		m_view = nullptr;
		m_selectionCursorOffset = 0;
		m_firstVisibleLine = 0;
		m_selectionStart = 0;
		m_selectionEnd = 0;
		m_startOffset = 0;
		m_endOffset = 0;
	}

	void MemoryView::ClearCachedData()
	{
		m_lineCache.Clear();
	}

	uint32 MemoryView::GetNumLines() const
	{
		return m_lineMapping.GetNumberOfLines();
	}

	bool MemoryView::GetSelectionOffsets(uint32& startOffset, uint32& endOffset) const
	{
		if (m_view != nullptr)
		{
			// get selection lines
			uint32 selectionLineStart = m_selectionStart;
			uint32 selectionLineEnd = m_selectionEnd;
			if (selectionLineStart > selectionLineEnd)
			{
				std::swap(selectionLineStart, selectionLineEnd);
			}

			// get the memory offset for the start and end line
			uint32 startAddr = 0, startSize = 0;
			if (m_lineMapping.GetOffsetForLine(selectionLineStart, startAddr, startSize))
			{
				uint32 endAddr = 0, endSize = 0;
				if (m_lineMapping.GetOffsetForLine(selectionLineEnd, endAddr, endSize))
				{
					startOffset = startAddr;
					endOffset = endAddr + endSize;
					return true;
				}
			}
		}

		// not evaluated
		return false;
	}

	bool MemoryView::GetSelectionAddresses(uint64& startAddress, uint64& endAddress) const
	{
		if (m_view != nullptr)
		{
			uint32 localStart = 0, localEnd = 0;
			if (GetSelectionOffsets(localStart, localEnd))
			{
				startAddress = localStart + m_view->GetBaseAddress();
				endAddress = localEnd + m_view->GetBaseAddress();
				return true;
			}
		}

		// not evaluated
		return false;
	}

	uint64 MemoryView::GetCurrentRVA() const
	{
		if (m_view != nullptr)
		{
			uint64 startAddrses = 0, endAddress = 0;
			GetSelectionAddresses(startAddrses, endAddress);
			return startAddrses;
		}
		else
		{
			return 0;
		}
	}

	void MemoryView::SetDataView(IMemoryDataView* view)
	{
		// different ?
		if (view != m_view)
		{
			// switch
			ClearContent();
			m_view = view;

			// new view
			if (m_view != nullptr)
			{
				// range
				m_startOffset = 0;
				m_endOffset = m_view->GetLength();

				// build the inital mapping
				m_lineMapping.Initialize(*view);
			}
		}

		// Redraw
		UpdateScrollbar();
		Refresh();
	}

	bool MemoryView::SetHighlightedOffset(const uint32 offset)
	{
		m_highlightedOffset = offset;
		return true;
	}

	bool MemoryView::SetActiveOffset(const uint32 offset, bool createHistoryEntry /*= true*/)
	{
		bool status = false;

		if (m_view != nullptr)
		{
			// screen size (in lines)
			const int numLinesOnScreen = (GetClientSize().y - (m_style.m_lineHeight - 1)) / m_style.m_lineHeight;

			// get current line displacement for selection end vs first visible line
			int selectionDisplacement = m_selectionEnd - m_firstVisibleLine;
			if (selectionDisplacement < 0 || selectionDisplacement >= numLinesOnScreen || (m_firstVisibleLine == 0 && m_selectionEnd == 0))
			{
				selectionDisplacement = std::max(4, numLinesOnScreen / 2); // show in the middle of the screen
			}

			// find the matching line
			int lineIndex = 0;
			if (m_lineMapping.GetLineForOffset(offset, (uint32&)lineIndex))
			{
				// get number of lines at given offset
				uint32 baseOffset = 0, baseSize = 0, firstLineIndex = 0, numLines = 0;
				if (m_lineMapping.GetOffsetForLine(lineIndex, baseOffset, baseSize, &firstLineIndex, &numLines))
				{
					// always focus on the last line
					lineIndex = firstLineIndex + numLines - 1;

					// target line is within the current screen
					const int lineMargin = 3;
					if (lineIndex >= (m_firstVisibleLine + lineMargin) &&
						lineIndex <= (m_firstVisibleLine + numLinesOnScreen - lineMargin))
					{
						// just move to the target line
						m_selectionStart = lineIndex;
						m_selectionEnd = lineIndex;
					}
					else
					{
						// move the whole view, make sure that the relative placement of the target line stays the same within the view (to ease reading)
						SetFirstVisibleLine(lineIndex - selectionDisplacement);

						// reset selection to the target line
						m_selectionStart = lineIndex;
						m_selectionEnd = lineIndex;
					}

					// update the cursor offset
					m_selectionCursorOffset = offset;

					// generate history entry
					m_view->SelectionCursorMoved(this, offset, createHistoryEntry);

					// valid 
					status = true;
				}
			}
		}

		// redraw if valid
		if (status)
		{
			Refresh();
		}

		// return true if setting the value was successful
		return status;
	}

	void MemoryView::SetFirstVisibleLine(int line)
	{
		const int numLines = m_lineMapping.GetNumberOfLines();
		const int numLinesOnScreen = (GetClientSize().y - (m_style.m_lineHeight - 1)) / m_style.m_lineHeight;
		const int maxVisibleLine = wxMax(0, numLines - numLinesOnScreen);

		// clamp
		if (line <= 0)
		{
			line = 0;
		}
		else if (line >= maxVisibleLine)
		{
			line = wxMax(0, maxVisibleLine - 1);
		}

		// update visible address raneg
		m_firstVisibleLine = line;

		// update the scrollbar
		SetScrollPos(wxVERTICAL, m_firstVisibleLine, true);

		// redraw view
		Refresh();
	}

	void MemoryView::EnsureVisible(int line)
	{
		const int numLinesOnScreen = (GetClientSize().y - (m_style.m_lineHeight - 1)) / m_style.m_lineHeight;

		if (line < m_firstVisibleLine)
		{
			SetFirstVisibleLine(line);
		}
		else if (line >= m_firstVisibleLine + numLinesOnScreen - 1)
		{
			SetFirstVisibleLine(line - (numLinesOnScreen - 1));
		}
	}

	void MemoryView::SetSelectionCursor(int lineIndex, const bool selectionEnd)
	{
		const int numTotalLines = m_lineMapping.GetNumberOfLines();

		// set selection position
		int newSelectionEnd = lineIndex;
		if (newSelectionEnd <= 0)
		{
			newSelectionEnd = 0;
		}
		else if (newSelectionEnd >= numTotalLines)
		{
			newSelectionEnd = wxMax(0, numTotalLines - 1);
		}

		// drag the start selection if needed
		if (newSelectionEnd != m_selectionEnd)
		{
			if (!selectionEnd)
			{
				m_selectionStart = newSelectionEnd;
			}

			m_selectionEnd = newSelectionEnd;
			EnsureVisible(lineIndex);
			Refresh();

			// send to view
			if (!selectionEnd)
			{
				uint32 offset = 0, size = 0;
				if (m_lineMapping.GetOffsetForLine(m_selectionEnd, offset, size))
				{
					if (m_selectionCursorOffset != offset)
					{
						m_selectionCursorOffset = offset;
						m_view->SelectionCursorMoved(this, offset, false);
					}
				}
			}
		}
	}

	void MemoryView::MoveSelectionCursor(int numLines, const bool selectionEnd)
	{
		const int numTotalLines = m_lineMapping.GetNumberOfLines();

		// set selection position
		int newSelectionEnd = m_selectionEnd + numLines;
		if (newSelectionEnd <= 0)
		{
			newSelectionEnd = 0;
		}
		else if (newSelectionEnd >= numTotalLines)
		{
			newSelectionEnd = wxMax(0, numTotalLines - 1);
		}

		// drag the start selection if needed
		if (newSelectionEnd != m_selectionEnd)
		{
			if (!selectionEnd)
			{
				m_selectionStart = newSelectionEnd;
			}

			m_selectionEnd = newSelectionEnd;
			EnsureVisible(newSelectionEnd);
			Refresh();

			// send to view
			if (!selectionEnd)
			{
				uint32 offset = 0, size = 0;
				if (m_lineMapping.GetOffsetForLine(m_selectionEnd, offset, size))
				{
					if (m_selectionCursorOffset != offset)
					{
						m_selectionCursorOffset = offset;
						m_view->SelectionCursorMoved(this, offset, false);
					}
				}
			}
		}
	}

	void MemoryView::NavigateSelection()
	{
		if (nullptr != m_view)
		{
			uint32 selectionStart = 0, selectionEnd = 0;
			if (GetSelectionOffsets(selectionStart, selectionEnd))
			{
				const bool bShift = wxGetKeyState(WXK_SHIFT);
				m_view->Navigate(this, selectionStart, selectionEnd, bShift);
				Refresh();
			}
		}
	}

	void MemoryView::UpdateScrollbar()
	{
		// all lines in the view
		const int numLines = m_lineMapping.GetNumberOfLines();

		// how many line we can see in the view ?
		const int screenHeight = GetClientSize().y;
		const int numVisibleLines = (screenHeight - (m_style.m_lineHeight - 1)) / m_style.m_lineHeight;
		const int maxVisibleLine = wxMax(0, numLines);

		// update the scrollbar
		SetScrollbar(wxVERTICAL, m_firstVisibleLine, numVisibleLines, maxVisibleLine, true);
	}

	int MemoryView::GetLineAtPixel(const wxPoint& point) const
	{
		// size of the DC
		const wxSize size = GetClientSize();
		if (point.x >= 0 && point.y >= 0 && point.x < size.x && point.y < size.y)
		{
			uint32 line = m_firstVisibleLine;
			int lineY = 0;
			while (line < m_lineMapping.GetNumberOfLines() && lineY < size.y)
			{
				const int nextY = lineY + m_style.m_lineHeight;

				// that's the line
				if (point.y >= lineY && point.y < nextY)
				{
					return line;
				}

				// advance
				lineY += m_style.m_lineHeight;
				line += 1;
			}
		}

		// no line found
		return -1;
	}

	void MemoryView::SetHitcountDisplay(const bool enabled)
	{
		if (m_style.m_drawHitCount != enabled)
		{
			m_style.m_drawHitCount = enabled;
			Refresh();
		}
	}

	void MemoryView::RefreshDirtyMemoryRegion()
	{
		uint32 startOffset = 0, endOffset = 0;
		if (m_view != nullptr)
		{
			if (m_view->GetDirtyMemoryRegion(startOffset, endOffset))
			{
				RefreshMemoryRegion(startOffset, endOffset);
			}
		}
	}

	void MemoryView::RefreshMemoryRegion(const uint32 startOffset, const uint32 endOffset)
	{
		if (m_view != nullptr)
		{
			// update the internal mapping - this will pull new information from the actual memory view
			uint32 firstModifiedLine = 0;
			uint32 lastModifiedLine = 0;
			if (m_lineMapping.UpdateLines(startOffset, endOffset, *m_view, firstModifiedLine, lastModifiedLine))
			{
				// validate memory region
				m_view->ValidateDirtyMemoryRegion(startOffset, endOffset);

				// clear the line cache
				m_lineCache.ClearRange(startOffset, endOffset);

				// redraw
				UpdateScrollbar();
				Refresh();
			}
		}
	}

	void MemoryView::RefreshMemoryRegionRVA(const uint32 startRVA, const uint32 endRVA)
	{
	}

	void MemoryView::OnMouseDown(wxMouseEvent& event)
	{
		SetFocus();

		// get the clicked line
		if (event.LeftDown() || event.RightDown())
		{
			const bool selectionMode = wxGetKeyState(WXK_SHIFT);

			if (!m_selectionDragMode)
			{
				const int clickedLine = GetLineAtPixel(event.GetPosition());
				if (clickedLine != -1)
				{
					// right button click - if clicked inside the selection do nothing, if outside, update the selection
					if (event.RightDown())
					{
						if ((clickedLine >= m_selectionStart && clickedLine <= m_selectionEnd) ||
							(clickedLine >= m_selectionEnd && clickedLine <= m_selectionStart))
						{
							// valid to display context menu
							m_contextMenuMode = true;
						}
						else
						{
							// update selection
							SetSelectionCursor(clickedLine, false);
							m_contextMenuMode = true;
						}
					}
					else
					{
						// set selection
						SetSelectionCursor(clickedLine, selectionMode);
					}
				}
			}
		}

		// enter dragging mode
		if (event.LeftDown())
		{
			if (!m_selectionDragMode)
			{
				m_selectionDragMode = true;
				m_contextMenuMode = false;
				CaptureMouse();
			}
		}
	}

	void MemoryView::OnMouseUp(wxMouseEvent& event)
	{
		// release the selection drag
		if (event.LeftUp())
		{
			if (m_selectionDragMode)
			{
				m_selectionDragMode = false;
				m_contextMenuMode = false;
				ReleaseCapture();

			}
		}

		// right mouse button release
		if (event.RightUp())
		{
			if (m_contextMenuMode)
			{
				m_contextMenuMode = false;

				uint32 selectionStart = 0, selectionEnd = 0;
				if (GetSelectionOffsets(selectionStart, selectionEnd))
				{
					// show the context menu, if we return true the selection memory range is reevaluated
					if (m_view->ShowContextMenu(this, selectionStart, selectionEnd, wxGetMousePosition()))
					{
						Refresh();
					}
				}
			}
		}
	}

	void MemoryView::OnMouseMove(wxMouseEvent& event)
	{
		// movement
		if (m_selectionDragMode)
		{
			// get the line under cursor
			const int draggedLine = GetLineAtPixel(event.GetPosition());
			if (draggedLine != -1)
			{
				// extend selection
				SetSelectionCursor(draggedLine, true);
			}
		}
	}

	void MemoryView::OnMouseWheel(wxMouseEvent& event)
	{
		int numLines = 3;
		if (wxGetKeyState(WXK_SHIFT))
		{
			numLines = 10;
		}
		if (event.GetWheelRotation() < 0)
		{
			SetFirstVisibleLine(m_firstVisibleLine + numLines);
			Refresh();
		}
		else if (event.GetWheelRotation() > 0)
		{
			SetFirstVisibleLine(m_firstVisibleLine - numLines);
			Refresh();
		}
	}

	static inline void AppendDrawingBuffer(char*& stream, const char* max, const char ch)
	{
		if (stream < max)
			*stream++ = ch;
	}

	static inline bool IsLabelChar(const char ch)
	{
		if (ch == 'h') return true;
		if (ch == 'L') return true;
		if (ch >= '0' && ch <= '9') return true;
		if (ch >= 'A' && ch <= 'F') return true;
		return false;
	}

	void MemoryView::OnPaint(wxPaintEvent& event)
	{
		wxBufferedPaintDC dc(this);
		PrepareDC(dc);

		// size of the DC
		const wxSize size = GetClientSize();

		// nothing valid
		if (nullptr == m_view)
		{
			dc.SetBackground(m_style.m_backBrush[0][0]);
			dc.Clear();
			return;
		}

		// set font
		dc.SetFont(m_style.m_font);

		// all lines in the view
		const int numMappedLines = m_lineMapping.GetNumberOfLines();

		// start drawing
		int lineY = 0;
		int lineIndex = m_firstVisibleLine;
		uint32 lastMemoryOffset = 0;
		while (lineY <= size.y && lineIndex < numMappedLines)
		{
			// get the memory offset and size for given line
			uint32 memoryOffset = 0;
			uint32 memorySize = 0;
			uint32 firstLine = 0;
			uint32 numLines = 0;
			if (!m_lineMapping.GetOffsetForLine(lineIndex, memoryOffset, memorySize, &firstLine, &numLines))
			{
				// skip this line
				lineY += m_style.m_lineHeight;
				lineIndex += 1;
				continue;
			}

			// skip the line gaps
			if (lineIndex > m_firstVisibleLine && memoryOffset == lastMemoryOffset)
			{
				// skip this line
				lineY += m_style.m_lineHeight;
				lineIndex += 1;
				continue;
			}

			// remember last valid memory offset
			lastMemoryOffset = memoryOffset;

			// get the cached line printable entries
			const MemoryLineCache::Entry* curLine = m_lineCache.GetCachedLines(memoryOffset, *m_view);
			if (curLine == nullptr)
			{
				// skip this line
				lineY += m_style.m_lineHeight;
				lineIndex += 1;
				continue;
			}

			// get line flags
			uint32 lineMarkers = 0;
			uint32 lineMarkerOffset = numLines - 1;
			const bool drawMarkers = m_view->GetAddressMarkers(memoryOffset, lineMarkers, lineMarkerOffset);

			// skip the hidden initial lines
			int numLinesDrawn = 0;
			while ((int)firstLine < lineIndex && curLine != nullptr)
			{
				firstLine += 1;
				numLinesDrawn += 1;
				curLine = curLine->m_next;
			}

			// process the lines
			while (curLine != nullptr && numLinesDrawn <(int)numLines)
			{
				assert(curLine->m_line != nullptr);
				const char* lineText = curLine->m_line->GetText();

				// by default we draw the address and bytes of the code
				bool drawAddress = true;
				bool drawCode = true;
				bool drawHitCount = false;

				// override the intial style if the first char is special
				EDrawingStyle style = eDrawingStyle_Normal;
				if (lineText[0] == ';')
				{
					style = eDrawingStyle_Comment;
					drawAddress = false;
					drawCode = false;
				}
				else if (lineText[0] == ':')
				{
					style = eDrawingStyle_Label;
					drawCode = false;
				}
				else if (lineText[0] == '^')
				{
					style = eDrawingStyle_Error;
				}

				// push the initial style to the stack
				static const uint32 MAX_STYLES = 16;
				EDrawingStyle styleStack[MAX_STYLES];
				uint32 currentStyleIndex = 0;
				styleStack[0] = style;

				// text drawing position
				const int textY = lineY + 2;

				// is the line selected
				const bool isLineSelected = ((lineIndex >= m_selectionStart) && (lineIndex <= m_selectionEnd))
					|| ((lineIndex >= m_selectionEnd) && (lineIndex <= m_selectionStart)) ? 1 : 0;

				// is the line highlighted (only last line is highlighted) ?
				const bool isLastLine = (numLinesDrawn == numLines - 1);
				const bool isHighlighted = m_highlightedOffset && isLastLine && (memoryOffset == m_highlightedOffset);

				// get drawing mode
				EDrawingMode drawingMode = isLineSelected ? eDrawingMode_Selected :
					(isHighlighted ? eDrawingMode_Highlighted : eDrawingMode_Normal);

				// compute line rect, set font and draw line background
				const wxRect lineRect(0, lineY, size.x, m_style.m_lineHeight);
				dc.SetBrush(m_style.m_backBrush[0][drawingMode]);
				dc.SetPen(m_style.m_backPen[0][drawingMode]);
				dc.DrawRectangle(lineRect);

				// draw line markers
				if (drawMarkers && numLinesDrawn == lineMarkerOffset)
				{
					uint32 x = 0;
					for (uint32 i = 0; i<eDrawingMarker_MAX; ++i)
					{
						if (lineMarkers & (1 << i))
						{
							const wxRect markerRect(x, lineY, x + m_style.m_markerWidth, m_style.m_lineHeight);
							dc.SetBrush(m_style.m_markerBrush[i]);
							dc.SetPen(m_style.m_markerPens[i]);
							dc.DrawRectangle(markerRect);
							x += m_style.m_markerWidth;
						}
					}
				}

				// bind the default drawing style
				dc.SetTextForeground(m_style.m_textPen[style][drawingMode].GetColour());

				// if there is no actual memory draw the text a little bit earlier
				int textX = m_style.m_addressOffset;

				// print address text
				if (drawAddress)
				{
					uint32 printOffset = m_view->GetBaseAddress() + memoryOffset;

					char offsetText[10];
					const uint8 b0 = (printOffset >> 28) & 0xF;
					const uint8 b1 = (printOffset >> 24) & 0xF;

					offsetText[0] = b0 ? HexChar[(printOffset >> 28) & 0xF] : ' ';
					offsetText[1] = (b1 | b0) ? HexChar[(printOffset >> 24) & 0xF] : ' ';
					offsetText[2] = HexChar[(printOffset >> 20) & 0xF];
					offsetText[3] = HexChar[(printOffset >> 16) & 0xF];
					offsetText[4] = HexChar[(printOffset >> 12) & 0xF];
					offsetText[5] = HexChar[(printOffset >> 8) & 0xF];
					offsetText[6] = HexChar[(printOffset >> 4) & 0xF];
					offsetText[7] = HexChar[(printOffset >> 0) & 0xF];
					offsetText[8] = 'h';
					offsetText[9] = 0;

					dc.DrawText(offsetText, m_style.m_addressOffset, textY);
					textX += 100; // address size
				}

				// extract the actual memory bytes, at most 30 bytes
				if (drawCode)
				{
					uint8 memoryBytes[30];
					const uint32 numExtracted = m_view->GetRawBytes(memoryOffset, memoryOffset + memorySize, sizeof(memoryBytes), memoryBytes);
					if (numExtracted > 0)
					{
						assert(numExtracted <= 16);

						// convert the extarcted bytes to text
						char bytesText[16 * 2 + 5];
						for (uint32 i = 0; i<numExtracted && i < 16; ++i)
						{
							const uint8 data = memoryBytes[i];

							bytesText[2 * i + 0] = HexChar[data >> 4];
							bytesText[2 * i + 1] = HexChar[data & 0xF];
							bytesText[2 * i + 2] = 0;

							if (i > 16)
							{
								bytesText[2 * i + 2] = '.';
								bytesText[2 * i + 3] = '.';
								bytesText[2 * i + 4] = '.';
								bytesText[2 * i + 5] = 0;
								break;
							}
						}

						// print the memory bytes
						dc.DrawText(bytesText, textX, textY);
						textX += 8 * 12;
					}
				}

				// print address text
				// NOTE: if entry is multi-line the hit count is drawn at the last entry
				if (m_style.m_drawHitCount && (numLinesDrawn == (numLines - 1)))
				{
					char hitCountText[10];
					if (m_view->GetAddressHitCount(memoryOffset, hitCountText))
					{
						dc.DrawText(hitCountText, textX, textY);
					}
				}

				// address for the code
				if (drawAddress)
				{
					// general text
					textX = m_style.m_textOffset;

					// labels got indent
					if (style == eDrawingStyle_Label)
					{
						textX -= m_style.m_tabSize;
					}
				}

				// drawing buffer
				char drawingBuffer[512];
				char* drawingBufferEnd = &drawingBuffer[sizeof(drawingBuffer) - 1];

				// start parsing the text, change the drawing style as you go, apply tabs
				const char* cur = lineText;
				while (*cur != 0)
				{
					// reset drawing buffer content
					char* drawingBufferPos = &drawingBuffer[0];

					// parse till the next style is met
					bool popStyle = false;
					bool pushStyle = false;
					bool advanceChar = true;
					EDrawingStyle newStyle = style;
					while (*cur != 0)
					{
						if (*cur == ';' && newStyle == eDrawingStyle_Normal)
						{
							advanceChar = false;
							pushStyle = true;
							newStyle = eDrawingStyle_Comment;
							break;
						}
						/*					else if ( style == eDrawingStyle_Label && !IsLabelChar(*cur) )
						{
						advanceChar = false;
						popStyle = true;
						break;
						}*/
						/*else if ( *cur == '[' && style != eDrawingStyle_Comment && style != eDrawingStyle_Address )
						{
						advanceChar = false;
						AppendDrawingBuffer( drawingBufferPos, drawingBufferEnd, *cur ); // draw this char
						pushStyle = true;
						newStyle = eDrawingStyle_Address;
						break;
						}
						else if ( *cur == ']' && style == eDrawingStyle_Address )
						{
						AppendDrawingBuffer( drawingBufferPos, drawingBufferEnd, *cur ); // draw this char
						popStyle = true;
						break;
						}*/
						/*else if ( *cur == ':' && style != eDrawingStyle_Comment && style != eDrawingStyle_Label )
						{
						//AppendDrawingBuffer( drawingBufferPos, drawingBufferEnd, *cur ); // draw this char
						pushStyle = true;
						newStyle = eDrawingStyle_Label;
						break;
						}*/
						else if (*cur == '\t')
						{
							// no style change, just a tab 
							break;
						}
						else if (*cur == '<' && style != eDrawingStyle_Comment)
						{
							cur += 1; // skip controll char

									  // get new style
							if (*cur == '^')
							{
								pushStyle = true;
								newStyle = eDrawingStyle_Error;
								//cur += 1;
								break;
							}
							else if (*cur == ':')
							{
								pushStyle = true;
								newStyle = eDrawingStyle_Label;
								//cur += 1;
								break;
							}
							else if (*cur == '#')
							{
								pushStyle = true;
								newStyle = eDrawingStyle_Address;
								//cur += 1;
								break;
							}
							else if (*cur == '&')
							{
								pushStyle = true;
								newStyle = eDrawingStyle_Validation;
								//cur += 1;
								break;
							}
							else if (*cur == '$')
							{
								pushStyle = true;
								newStyle = eDrawingStyle_String;
								//cur += 1;
								break;
							}
							else if (*cur == '!')
							{
								pushStyle = true;
								newStyle = eDrawingStyle_Error;
								//cur += 1;
								break;
							}
						}
						else if (*cur == '>' && style != eDrawingStyle_Comment)
						{
							popStyle = true;
							break;
						}

						// normal char
						AppendDrawingBuffer(drawingBufferPos, drawingBufferEnd, *cur); // draw this char
						++cur;
					}

					// draw current part of text
					if (drawingBufferPos > drawingBuffer)
					{
						*drawingBufferPos = 0;

						const wxString drawText(drawingBuffer);
						const uint32 pixelLength = dc.GetTextExtent(drawText).x;
						dc.DrawText(drawText, textX, textY);
						textX += pixelLength;
					}

					// tab - move to next tab position
					if (*cur == '\t')
					{
						textX = ((textX / m_style.m_tabSize) + 1) * m_style.m_tabSize;
						cur += 1;
					}

					// restart
					if (!*cur) break;
					if (advanceChar) cur += 1;
					advanceChar = true;

					if (popStyle)
					{
						if (currentStyleIndex >= 1)
						{
							currentStyleIndex -= 1;
							style = styleStack[currentStyleIndex];
						}
						else
						{
							style = styleStack[0] = eDrawingStyle_Normal;
						}
					}
					else if (pushStyle && currentStyleIndex < MAX_STYLES)
					{
						styleStack[++currentStyleIndex] = newStyle;
						style = newStyle;
					}

					// set color for the new style
					dc.SetTextForeground(m_style.m_textPen[style][drawingMode].GetColour());
				}

				// advance the line
				lineY += m_style.m_lineHeight;
				lineIndex += 1;

				// next line in the entry
				curLine = curLine->m_next;
				numLinesDrawn += 1;
			}
		}

		// clear the rest
		if (lineY < size.y)
		{
			dc.SetBrush(m_style.m_backBrush[0][0]);
			dc.SetPen(m_style.m_backPen[0][0]);
			dc.DrawRectangle(0, lineY, size.x, size.y - lineY + 1);
		}
	}

	void MemoryView::OnErase(wxEraseEvent& event)
	{
		// done
	}

	void MemoryView::OnCharHook(wxKeyEvent& event)
	{
		if (event.GetKeyCode() == WXK_RETURN)
		{
			NavigateSelection();
		}
		else if (event.GetKeyCode() == WXK_BACK)
		{
			if (m_view != nullptr)
			{
				m_view->Navigate(this, NavigationType::Back);
			}
		}
		else
		{
			event.Skip();
		}
	}

	void MemoryView::OnKeyDown(wxKeyEvent& event)
	{
		const bool selectionMode = wxGetKeyState(WXK_SHIFT);
		const bool specialMode = wxGetKeyState(WXK_CONTROL);

		if (event.GetKeyCode() == WXK_UP)
		{
			MoveSelectionCursor(-1, selectionMode);
		}
		else if (event.GetKeyCode() == WXK_DOWN)
		{
			MoveSelectionCursor(1, selectionMode);
		}
		else if (event.GetKeyCode() == WXK_PAGEDOWN)
		{
			MoveSelectionCursor(20, selectionMode);
		}
		else if (event.GetKeyCode() == WXK_PAGEUP)
		{
			MoveSelectionCursor(-20, selectionMode);
		}
		else if (event.GetKeyCode() == WXK_HOME)
		{
			SetSelectionCursor(0, selectionMode);
		}
		else if (event.GetKeyCode() == WXK_END)
		{
			const int numLines = m_lineMapping.GetNumberOfLines();
			SetSelectionCursor(numLines - 1, selectionMode);
		}
		else if (event.GetKeyCode() == WXK_F11)
		{
			if (selectionMode)
			{
				if (specialMode)
					m_view->Navigate(this, NavigationType::GlobalStepBack);
				else
					m_view->Navigate(this, NavigationType::LocalStepBack);
			}
			else
			{
				if (specialMode)
					m_view->Navigate(this, NavigationType::GlobalStepIn);
				else
					m_view->Navigate(this, NavigationType::LocalStepIn);
			}
		}		
	}

	void MemoryView::OnScroll(wxScrollWinEvent& event)
	{
		SetFirstVisibleLine(event.GetPosition());
	}

	void MemoryView::OnSize(wxSizeEvent& event)
	{
		SetFirstVisibleLine(m_firstVisibleLine);
		event.Skip();
	}

	void MemoryView::OnCaptureLost(wxMouseCaptureLostEvent& event)
	{
		m_selectionDragMode = false;
		Refresh();
	}

	void MemoryView::OnSyncUpdate()
	{
		RefreshDirtyMemoryRegion();
	}

} // tools