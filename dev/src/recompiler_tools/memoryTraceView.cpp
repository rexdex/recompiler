#include "build.h"
#include "memoryTraceView.h"
#include "../recompiler_core/traceDataFile.h"

namespace tools
{

	BEGIN_EVENT_TABLE(MemoryTraceView, wxScrolledWindow)
		EVT_PAINT(MemoryTraceView::OnPaint)
		EVT_ERASE_BACKGROUND(MemoryTraceView::OnEraseBackground)
		EVT_LEFT_DOWN(MemoryTraceView::OnMouseClick)
		EVT_LEFT_UP(MemoryTraceView::OnMouseClick)
		EVT_MOTION(MemoryTraceView::OnMouseClick)
	END_EVENT_TABLE();

	MemoryTraceView::MemoryTraceView(wxWindow* parent, const trace::DataFile* traceData)
		: wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxVSCROLL | wxHSCROLL)
		, m_traceData(traceData)
		, m_windowBaseAddress(0x00000000)
		, m_displayRowSize(64)
		, m_displayMode(MemoryDisplayMode::Hex8)
		, m_displayOrder(MemoryEndianess::BigEndian) // hack
		, m_seq(INVALID_TRACE_FRAME_ID)
		, m_selectionStart(nullptr)
		, m_selectionEnd(nullptr)
		, m_selectionDrag(false)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);

		// set mono spaced font for the window
		wxFontInfo fontInfo(8);
		fontInfo.Family(wxFONTFAMILY_SCRIPT);
		fontInfo.FaceName("Courier New");
		fontInfo.AntiAliased(true);
		m_font = wxFont(fontInfo);

		// measure fort size
		{
			wxClientDC dc(this);
			dc.SetFont(m_font);
			m_drawCharSize = dc.GetTextExtent("0");
			m_lineHeight = m_drawCharSize.y + 4; // 2pt margins
		}

		// compute scrolling ranges
		SetScrollRate(1, m_lineHeight);
		UpdateScrollRange();
	}

	void MemoryTraceView::ClearSelection()
	{
		InvalidSelection();
		Refresh();
	}

	const bool MemoryTraceView::GetSelection(uint64& outStartAddress, uint64& outEndAddress) const
	{
		if (m_selectionStart && m_selectionEnd)
		{
			if (m_selectionStart->m_baseAddress <= m_selectionEnd->m_baseAddress)
			{
				outStartAddress = m_selectionStart->m_baseAddress;
				outEndAddress = m_selectionEnd->m_baseAddress + m_selectionEnd->m_size;
				return true;
			}
			else
			{
				outStartAddress = m_selectionEnd->m_baseAddress;
				outEndAddress = m_selectionStart->m_baseAddress + m_selectionStart->m_size;
				return true;
			}
		}

		return false;
	}

	//---

	const uint32_t MemoryTraceView::GetMemoryGroupSize(const MemoryDisplayMode displayMode)
	{
		switch (displayMode)
		{
			case MemoryDisplayMode::Int8:
			case MemoryDisplayMode::Hex8:
			case MemoryDisplayMode::Uint8:
			case MemoryDisplayMode::AnsiChars:
				return 1;

			case MemoryDisplayMode::Hex16:
			case MemoryDisplayMode::Uint16:
			case MemoryDisplayMode::Int16:
			case MemoryDisplayMode::UniChars:
				return 2;

			case MemoryDisplayMode::Hex32:
			case MemoryDisplayMode::Uint32:
			case MemoryDisplayMode::Int32:
			case MemoryDisplayMode::Float:
				return 4;

			case MemoryDisplayMode::Hex64:
			case MemoryDisplayMode::Uint64:
			case MemoryDisplayMode::Int64:
			case MemoryDisplayMode::Double:
				return 8;
		}

		return 1;
	}

	const uint32_t MemoryTraceView::GetCharGroupSize(const MemoryDisplayMode displayMode)
	{
		switch (displayMode)
		{
			case MemoryDisplayMode::Int8:
				return 4; // sign + up to 255

			case MemoryDisplayMode::Hex8:
				return 2; // FF

			case MemoryDisplayMode::Hex16:
				return 4; // FFFF

			case MemoryDisplayMode::Hex32:
				return 8; // FFFFFFFF

			case MemoryDisplayMode::Hex64:
				return 16; // FFFFFFFFFFFFFFFF

			case MemoryDisplayMode::Uint8:
				return 3; // up to 255

			case MemoryDisplayMode::AnsiChars:
				return 1; // single char

			case MemoryDisplayMode::Uint16:
				return 5; // up to 65535

			case MemoryDisplayMode::Int16:
				return 6; // 5 chars + sign

			case MemoryDisplayMode::UniChars:
				return 1; // single char

			case MemoryDisplayMode::Uint32:
				return 10; 

			case MemoryDisplayMode::Int32:
				return 11; // 10 + sign

			case MemoryDisplayMode::Float:
				return 15; // limited by us

			case MemoryDisplayMode::Double:
				return 20; // limited by us

			case MemoryDisplayMode::Uint64:
				return 19;

			case MemoryDisplayMode::Int64:
				return 20;
		}

		return 1;
	}

	const wxString MemoryTraceView::GetMemoryDisplayText(const void* data, const MemoryDisplayMode displayMode, const MemoryEndianess endianess)
	{
		if (endianess == MemoryEndianess::BigEndian)
		{			
			uint8_t tempData[16];

			const auto size = GetMemoryGroupSize(displayMode);
			for (uint32_t i = 0; i < size; ++i)
				tempData[i] = ((const uint8_t*)data)[size - 1 - i];

			return GetMemoryDisplayText(tempData, displayMode);
		}
		else
		{
			return GetMemoryDisplayText(data, displayMode);
		}
	}

	const wxString MemoryTraceView::GetMemoryDisplayText(const void* data, const MemoryDisplayMode displayMode)
	{
		switch (displayMode)
		{
			case MemoryDisplayMode::Hex8:
			{
				return wxString::Format("%02X", *(const uint8*)data);
			}

			case MemoryDisplayMode::Hex16:
			{
				return wxString::Format("%04X", *(const uint16*)data);
			}

			case MemoryDisplayMode::Hex32:
			{
				return wxString::Format("%08X", *(const uint32*)data);
			}

			case MemoryDisplayMode::Hex64:
			{
				return wxString::Format("%016llX", *(const uint64*)data);
			}

			case MemoryDisplayMode::Uint8:
			{
				return wxString::Format("%u", *(const uint8*)data);
			}

			case MemoryDisplayMode::Uint16:
			{
				return wxString::Format("%u", *(const uint16*)data);
			}

			case MemoryDisplayMode::Uint32:
			{
				return wxString::Format("%u", *(const uint32*)data);
			}

			case MemoryDisplayMode::Uint64:
			{
				return wxString::Format("%llu", *(const uint64*)data);
			}

			case MemoryDisplayMode::Int8:
			{
				return wxString::Format("%d", *(const int8*)data);
			}

			case MemoryDisplayMode::Int16:
			{
				return wxString::Format("%d", *(const int16*)data);
			}

			case MemoryDisplayMode::Int32:
			{
				return wxString::Format("%d", *(const int32*)data);
			}

			case MemoryDisplayMode::Int64:
			{
				return wxString::Format("%lld", *(const int64*)data);
			}

			case MemoryDisplayMode::Float:
			{
				return wxString::Format("%f", *(const float*)data);
			}

			case MemoryDisplayMode::Double:
			{
				return wxString::Format("%f", *(const double*)data);
			}

			case MemoryDisplayMode::AnsiChars:
			{
				const auto ch = *(const char*)data;
				const char str[2] = { ch < 32 ? ' ' : ch, 0 };
				return str;
			}

			case MemoryDisplayMode::UniChars:
			{
				const auto ch = *(const wchar_t*)data;
				const wchar_t str[2] = { ch < 32 ? (wchar_t)' ' : ch, 0 };
				return str;
			}
		}

		return wxString("-");
	}

	//---

	const uint32_t MemoryTraceView::CalcNumberOfCharsPerLine(const MemoryDisplayMode displayMode, const uint32_t rowSize)
	{
		uint32_t numChars = 0;

		const auto numGroups = std::max<uint32>(1, rowSize / GetMemoryGroupSize(displayMode));
		numChars += GetCharGroupSize(displayMode) * numGroups;
		numChars += (numGroups - 1) * 1; // separators

		numChars += 10; // address
		numChars += 2; // address separator

		return numChars;
	}

	void MemoryTraceView::UpdateScrollRange()
	{
		// calculate the size of the display area in lines and chars
		const auto numLines = std::max<uint64_t>(1, WINDOW_SIZE / m_displayRowSize);
		const auto numChars = CalcNumberOfCharsPerLine(m_displayMode, m_displayRowSize);

		// compute scroll area size in pixels
		const auto scrollSizeX = numChars * m_drawCharSize.x;
		const auto scrollSizeY = numLines;

		// update crap
		SetVirtualSize(scrollSizeX, scrollSizeY * m_lineHeight);
	}

	void MemoryTraceView::SetTraceFrame(const TraceFrameID seq)
	{
		if (m_seq != seq)
		{
			m_seq = seq;

			bool somethingChanged = false;

			somethingChanged |= ResetRenderLinesChangeState();

			for (auto& it : m_renderLines)
				somethingChanged |= RewindRenderLine(*it.second);

			if (somethingChanged)
				Refresh();
		}
	}

	void MemoryTraceView::SetDisplayMode(const MemoryDisplayMode mode, const uint32_t rowSize, const MemoryEndianess endianess)
	{
		if (m_displayRowSize != rowSize || m_displayMode != mode || m_displayOrder != endianess)
		{
			auto oldRowSize = m_displayRowSize;
			m_displayRowSize = std::max<uint32>(8, rowSize);

			m_displayMode = mode;
			m_displayOrder = endianess;

			UpdateScrollRange();

			if (oldRowSize != m_displayRowSize)
			{
				const auto topAddress = m_windowBaseAddress + GetViewStart().y * oldRowSize;
				SetAddress(topAddress);
			}
			else
			{
				InvalidSelection();
				InvalidateRenderLineCache();
				Refresh();
			}
		}
	}

	const bool MemoryTraceView::RewindRenderLine(RenderLine& line)
	{
		bool somethingChanged = false;

		// sync memory locations
		for (auto& cell : line.m_cells)
			somethingChanged |= cell.Rewind(m_seq);

		// update text
		if (somethingChanged)
			UpdateRenderLineCaptions(line);

		return somethingChanged;
	}

	void MemoryTraceView::UpdateRenderLineCaptions(RenderLine& line)
	{
		uint8_t dataBuffer[128];

		// collect memory values
		for (uint32_t i = 0; i < m_displayRowSize; ++i)
			dataBuffer[i] = line.m_cells[i].GetValue();

		// update the captions
		uint32_t offset = 0;
		const auto size = GetMemoryGroupSize(m_displayMode);
		for (auto& field : line.m_fields)
		{
			uint64_t seqHash = 0;
			for (uint32_t i = 0; i < size; ++i)
				seqHash = seqHash * 371 + line.m_cells[offset + i].GetFrameID();

			auto newText = GetMemoryDisplayText(dataBuffer + offset, m_displayMode, m_displayOrder);
			if (newText != field.m_text)
			{
				field.m_changed = field.m_text.empty() ? 0 : 2; // full change ?
				field.m_text = std::move(newText);
				field.m_seqHash = seqHash;
			}
			else
			{
				field.m_changed = (seqHash == field.m_seqHash) ? 0 : 1; // touch
				field.m_seqHash = seqHash;
			}
			offset += size;
		}
	}

	void MemoryTraceView::InvalidateRenderLineCache()
	{
		for (auto& it : m_renderLines)
			delete it.second;
		m_renderLines.clear();
	}

	const bool MemoryTraceView::ResetRenderLinesChangeState()
	{
		bool somethingChanged = false;

		for (auto& it : m_renderLines)
		{
			for (auto& field : it.second->m_fields)
			{
				if (field.m_changed)
				{
					somethingChanged = true;
					field.m_changed = 0;
				}
			}
		}

		return somethingChanged;
	}

	MemoryTraceView::RenderLine* MemoryTraceView::CompileRenderLine(const uint64_t baseAddress)
	{
		auto* line = new MemoryTraceView::RenderLine();
		line->m_baseAddress = baseAddress;

		// capture memory cells
		line->m_cells.resize(m_displayRowSize);
		for (uint32_t i = 0; i < m_displayRowSize; ++i)
		{
			line->m_cells[i] = m_traceData->GetMemoryCell(baseAddress + i);
			line->m_cells[i].Rewind(m_seq);
		}

		// get draw size for the whole line
		const auto numChars = CalcNumberOfCharsPerLine(m_displayMode, m_displayRowSize);

		// setup rendering layout
		const auto renderLineIndex = (baseAddress - m_windowBaseAddress) / m_displayRowSize;
		line->m_drawRect.x = 0;
		line->m_drawRect.y = renderLineIndex * m_lineHeight;
		line->m_drawRect.width = m_drawCharSize.x * numChars;
		line->m_drawRect.height = m_lineHeight;

		// create the rendering elements (fields)
		const auto fieldSize = GetMemoryGroupSize(m_displayMode);
		const auto numFields = m_displayRowSize / fieldSize;
		line->m_fields.resize(numFields);
		auto fieldDataAddres = baseAddress;
		for (auto& field : line->m_fields)
		{
			field.m_baseAddress = fieldDataAddres;
			field.m_changed = 0;
			field.m_size = fieldSize;
			fieldDataAddres += fieldSize;
		}

		// layout the fields
		auto charsPerField = GetCharGroupSize(m_displayMode);
		auto charIndex = 10 + 2; // address + separator
		for (auto& field : line->m_fields)
		{
			field.m_drawRect.x = charIndex * m_drawCharSize.x - (m_drawCharSize.x/2);
			field.m_drawRect.y = line->m_drawRect.y;
			field.m_drawRect.width = charsPerField * m_drawCharSize.x + m_drawCharSize.x;
			field.m_drawRect.height = line->m_drawRect.height;
			charIndex += charsPerField + 1;
		}

		// update the captions of the created fields
		UpdateRenderLineCaptions(*line);

		// return created line
		return line;
	}

	MemoryTraceView::RenderLine* MemoryTraceView::FetchRenderLine(const uint64_t baseAddress)
	{
		// lookup in cache
		auto it = m_renderLines.find(baseAddress);
		if (it != m_renderLines.end())
			return (*it).second;

		// create new line
		auto* line = CompileRenderLine(baseAddress);
		m_renderLines[baseAddress] = line;
		return line;
	}

	void MemoryTraceView::InvalidSelection()
	{
		m_selectionStart = nullptr;
		m_selectionEnd = nullptr;
	}

	const MemoryTraceView::RenderField* MemoryTraceView::GetFieldAt(const wxPoint& point) const
	{
		for (const auto& line : m_renderLines)
		{
			const auto lineTop = line.second->m_drawRect.y;
			const auto lineBottom = lineTop + line.second->m_drawRect.height;
			if (point.y >= lineTop && point.y < lineBottom)
			{
				for (const auto& field : line.second->m_fields)
				{
					const auto fieldLeft = field.m_drawRect.x;
					const auto fieldRight = fieldLeft + field.m_drawRect.width;
					if (point.x >= fieldLeft && point.x <= fieldRight)
					{
						return &field;
					}
				}
			}
		}

		return nullptr;
	}

	void MemoryTraceView::SetAddress(const uint64_t address)
	{
		// clear current rendering cache
		InvalidateRenderLineCache();

		// reset selection
		InvalidSelection();

		// move window
		const auto windowSize = ((WINDOW_SIZE / 2) / m_displayRowSize) * m_displayRowSize;
		const auto newWindowBase = std::max<uint64_t>(address, windowSize) - windowSize;
		m_windowBaseAddress = newWindowBase;

		// set scroll to given line
		const auto lineIndex = (address - m_windowBaseAddress) / m_displayRowSize;
		const auto curScroll = GetViewStart();
		Scroll(curScroll.x, lineIndex);
		Refresh();
	}

	void MemoryTraceView::RenderRenderLine(wxDC& dc, const RenderLine& line) const
	{
		// draw line address
		char address[20];
		sprintf_s(address, "%10llX: ", line.m_baseAddress);
		dc.SetTextForeground(wxColor(255, 255, 255));
		dc.DrawText(address, wxPoint(line.m_drawRect.x, line.m_drawRect.y + 2));

		// selection brush
		wxBrush selctionBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));

		// get selection render range
		uint64_t startSelection = 0;
		uint64_t endSelection = 0;
		GetSelection(startSelection, endSelection);

		// draw line fields
		for (const auto& field : line.m_fields)
		{
			// selection
			if (field.m_baseAddress >= startSelection && field.m_baseAddress + field.m_size <= endSelection)
			{
				dc.SetBrush(selctionBrush);
				dc.DrawRectangle(field.m_drawRect);
			}

			// color
			if (field.m_changed == 2)
				dc.SetTextForeground(wxColor(255, 32, 32));
			else if (field.m_changed == 1)
				dc.SetTextForeground(wxColor(255, 180, 32));
			else
				dc.SetTextForeground(wxColor(255, 255, 255));

			dc.SetClippingRegion(field.m_drawRect);
			dc.DrawText(field.m_text, wxPoint(field.m_drawRect.x + m_drawCharSize.x/2, field.m_drawRect.y + 2));
			dc.DestroyClippingRegion();
		}
	}

	void MemoryTraceView::OnPaint(wxPaintEvent& evt)
	{
		// setup drawing
		wxBufferedPaintDC dc(this);
		DoPrepareDC(dc);

		// get the draw range
		const auto size = GetClientSize();

		// colors
		const auto backColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
		const auto textColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
		const auto shadowColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
		const auto selectColor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

		// draw background
		dc.SetBackground(wxBrush(wxColor(40,40,40)));
		dc.Clear();

		// draw lines
		const auto firstLine = std::max<int32>(0, GetViewStart().y);
		const auto numLines = (size.y + (m_lineHeight - 1)) / m_lineHeight;
		for (uint32_t lineIndex = 0; lineIndex < numLines; ++lineIndex)
		{
			const auto baseAddress = m_windowBaseAddress + (firstLine + lineIndex) * m_displayRowSize;
			const auto* renderLine = FetchRenderLine(baseAddress);
			if (renderLine)
				RenderRenderLine(dc, *renderLine);
		}
	}

	void MemoryTraceView::OnEraseBackground(wxEraseEvent& evt)
	{
	}

	void MemoryTraceView::OnMouseClick(wxMouseEvent& evt)
	{
		if (evt.LeftDown())
		{
			auto* field = GetFieldAt(CalcUnscrolledPosition(evt.GetPosition()));
			if (field != nullptr)
			{
				m_selectionStart = field;
				m_selectionEnd = field;
				m_selectionDrag = true;
				Refresh();
				CaptureMouse();
			}
			else
			{
				InvalidSelection();
				Refresh();
			}
		}
		else if (evt.LeftUp())
		{
			if (m_selectionDrag)
			{
				m_selectionDrag = false;
				ReleaseMouse();
			}
		}
		else if (evt.Dragging() && m_selectionDrag)
		{
			auto* field = GetFieldAt(CalcUnscrolledPosition(evt.GetPosition()));
			if (field != nullptr)
			{
				if (m_selectionEnd != field)
				{
					m_selectionEnd = field;
					Refresh();
				}
			}
		}		
	}

} // tools