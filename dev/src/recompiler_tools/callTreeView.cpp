#include "build.h"
#include "callTreeView.h"
#include "../recompiler_core/traceDataFile.h"

namespace tools
{
	//---

	BEGIN_EVENT_TABLE(CallTreeView, wxScrolledWindow)
		EVT_PAINT(CallTreeView::OnPaint)
		EVT_ERASE_BACKGROUND(CallTreeView::OnEraseBackground)
		EVT_LEFT_DOWN(CallTreeView::OnMouseClick)
		EVT_LEFT_UP(CallTreeView::OnMouseClick)
		EVT_MOTION(CallTreeView::OnMouseClick)
		EVT_MOUSEWHEEL(CallTreeView::OnMouseWheel)
	END_EVENT_TABLE()

	//---

	static const wxColor BlendColor(const wxColor& a, const wxColor& b, const float alpha)
	{
		const auto retR = wxColor::AlphaBlend(a.Red(), b.Red(), alpha);
		const auto retG = wxColor::AlphaBlend(a.Green(), b.Green(), alpha);
		const auto retB = wxColor::AlphaBlend(a.Blue(), b.Blue(), alpha);
		return wxColor(retR, retG, retB);
	}

	CallTreeView::CallTreeView(wxWindow* parent, INavigationHelper* navigator)
		: wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
		, m_data(nullptr)
		, m_ticksPerPixel(4)
		, m_navigator(navigator)
		, m_currentPosition(0)
		, m_offset(0)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetScrollPageSize(wxVERTICAL, LINE_HEIGHT);
		SetScrollPageSize(wxHORIZONTAL, m_ticksPerPixel);

		//const wxColor colorStartThread("#adadad");
		//const wxColor colorEndThread("#323232");
		const wxColor colorStartThread("#C880C8");
		const wxColor colorEndThread("#7F7398");

		const wxColor colorStartIRQ("#ffad2f");
		const wxColor colorEndIRQ("#cd3232");

		const wxColor colorStartAPC("#ff2fad");
		const wxColor colorEndAPC("#cd3232");

		// create rendering brushes
		const auto numSteps = 64;
		for (uint32 i = 0; i < numSteps; ++i)
		{
			const auto frac = rand() / (float)RAND_MAX;

			{
				const auto color = BlendColor(colorStartThread, colorEndThread, frac);
				m_renderBrushesThreads.push_back(wxBrush(color, wxBRUSHSTYLE_SOLID));
			}

			{
				const auto color = BlendColor(colorStartAPC, colorEndAPC, frac);
				m_renderBrushesAPC.push_back(wxBrush(color, wxBRUSHSTYLE_SOLID));
			}

			{
				const auto color = BlendColor(colorStartIRQ, colorEndIRQ, frac);
				m_renderBrushesIRQ.push_back(wxBrush(color, wxBRUSHSTYLE_SOLID));
			}
		}
	}

	CallTreeView::~CallTreeView()
	{
		Clear();
	}

	void CallTreeView::Clear()
	{
		utils::ClearPtr(m_groups);
	}

	void CallTreeView::EnsureVisible(const TraceFrameID seq)
	{
		const auto scroll = GetViewStart();
		const auto size = GetClientSize();

		auto newScroll = scroll;

		// horizontal alignment
		const auto pos = (int)(seq / m_ticksPerPixel) - (int)scroll.x;
		if (pos < 50)
		{
			const auto delta = 50 - pos;
			newScroll.x = std::max(0, scroll.x - delta);
			Refresh();
		}
		else if (pos > (size.x-50))
		{
			const auto delta = pos - (size.x - 50);
			newScroll.x = scroll.x + delta;
		}

		if (newScroll != scroll)
		{
			Scroll(newScroll.x, newScroll.y);
		}
	}

	void CallTreeView::SetPosition(const TraceFrameID seq)
	{
		m_currentPosition = seq;
		EnsureVisible(seq);
		Refresh();
	}

	void CallTreeView::ExtractTraceData(trace::DataFile& data)
	{
		// clear current data
		Clear();

		// process context data
		uint32_t numContexts = data.GetContextList().size();
		for (uint32_t i=0; i<numContexts; ++i)
		{ 
			// get context data
			// get group that matches the context type
			const auto& context = data.GetContextList()[i];
			if (context.m_rootCallFrame != 0)
			{
				auto* group = CreateDataGroup(context.m_first, context.m_type);

				// create the callstack elements
				CreateCallStackBlocks(data, *group, 0, context.m_rootCallFrame);

				// update group end pos
				group->m_lastId = group->GetLastSeq();
			}
		}

		// layout the data
		LayoutAll();
	}

	void CallTreeView::CreateCallStackBlocks(const trace::DataFile& data, DataGroup& group, const uint32_t lineIndex, const uint32_t callStackEntry)
	{
		// get/create line
		auto* line = group.GetLine(lineIndex);

		// get the trace entry
		const auto& callEntry = data.GetCallFrames()[callStackEntry];

		// create the block
		DataBlock block;
		block.m_start = callEntry.m_enterLocation;
		block.m_end = callEntry.m_leaveLocation;
		line->m_blocks.push_back(block);

		// recurse to children
		auto childIndex = callEntry.m_firstChildFrame;
		while (childIndex != 0)
		{
			// extract block
			CreateCallStackBlocks(data, group, lineIndex + 1, childIndex);

			// get next child
			const auto& childCallEntry = data.GetCallFrames()[childIndex];
			childIndex = childCallEntry.m_nextChildFrame;
		}
	}

	CallTreeView::DataGroup* CallTreeView::CreateDataGroup(const trace::LocationInfo firstEntry, const trace::ContextType type)
	{
		// look for first matching group that has ended BEFORE the stuff we want to add
		for (auto* group : m_groups)
		{
			if (group->m_type == type && group->m_lastId < firstEntry.m_seq)
			{ 
				return group;
			}
		}

		// create new group
		auto* group = new CallTreeView::DataGroup(type);
		m_groups.push_back(group);
		return group;
	}

	void CallTreeView::SetTicksPerPixel(uint64_t ticks, const uint64_t zoomCenterPos)
	{
		if (m_ticksPerPixel != ticks)
		{
			const auto scrollX = GetViewStart().x;

			const auto extraPixels = (zoomCenterPos / m_ticksPerPixel) - scrollX;
			m_ticksPerPixel = ticks;
			const auto newScrollX = (zoomCenterPos / m_ticksPerPixel) - extraPixels;

			Freeze();
			LayoutAll();
			Scroll(newScrollX, GetViewStart().y);
			Thaw();
			Refresh();
		}
	}

	uint32_t CallTreeView::MapCoordinate(const trace::LocationInfo& info)
	{
		return info.m_seq / m_ticksPerPixel;
	}

	void CallTreeView::LayoutLine(const trace::ContextType type, const DataLine& line, const uint32_t lineIndex, RenderLine& outRenderLine, uint32_t& xMax)
	{
		const auto MinRequiredSeparation = 2;

		// start gluing stuff
		const auto numBlocks = line.m_blocks.size();
		uint32_t curBlock = 0;
		uint32_t maxRenderX = 0;
		while (curBlock < numBlocks)
		{
			uint32 numMergedBlocks = 1;

			// start glued block
			RenderBlock renderBlock;
			renderBlock.m_firstBlock = &line.m_blocks[curBlock];
			renderBlock.m_start = MapCoordinate(renderBlock.m_firstBlock->m_start);
			renderBlock.m_end = MapCoordinate(renderBlock.m_firstBlock->m_end);
			renderBlock.m_logicStart = renderBlock.m_firstBlock->m_start.m_seq;
			renderBlock.m_logicEnd = renderBlock.m_firstBlock->m_end.m_seq;
			curBlock += 1;

			// set brush
			if (type == trace::ContextType::Thread)
				renderBlock.m_brush = &m_renderBrushesThreads[lineIndex];
			else if (type == trace::ContextType::IRQ)
				renderBlock.m_brush = &m_renderBrushesIRQ[lineIndex];
			else
				renderBlock.m_brush = &m_renderBrushesAPC[lineIndex];

			// glue more blocks
			if (m_ticksPerPixel > 1)
			{
				while (curBlock < numBlocks)
				{
					const auto& nextBlock = line.m_blocks[curBlock];
					const auto nextBlockStart = MapCoordinate(nextBlock.m_start);
					if (nextBlockStart < renderBlock.m_end + MinRequiredSeparation)
					{
						renderBlock.m_logicEnd = nextBlock.m_end.m_seq;
						renderBlock.m_end = MapCoordinate(nextBlock.m_end);
						curBlock += 1;
						numMergedBlocks += 1;
					}
					else
					{
						break;
					}
				}
			}

			// ensure block size
			renderBlock.m_end = std::max(renderBlock.m_end, renderBlock.m_start + MinRequiredSeparation);
			renderBlock.m_numBlocks = numMergedBlocks;

			// set caption
			const auto renderWidth = renderBlock.m_end - renderBlock.m_start;
			if (renderWidth > 40)
			{
				renderBlock.m_caption = wxString::Format("%08llXh", renderBlock.m_firstBlock->m_start.m_ip);
				if (numMergedBlocks > 1)
					renderBlock.m_caption += wxString::Format(" (%u blocks)", numMergedBlocks);
			}

			// add to render list
			outRenderLine.m_blocks.push_back(renderBlock);
			maxRenderX = renderBlock.m_end;
		}

		// update max position
		xMax = std::max(xMax, maxRenderX);
	}

	void CallTreeView::LayoutGroups(const trace::ContextType type, uint32_t& yPos, uint32_t& xMax, const char* name)
	{
		for (const auto* group : m_groups)
		{
			// skip groups that don't match
			if (group->m_type != type)
				continue;

			// space for the group title
			yPos += LINE_HEIGHT;

			// pack lines
			auto lineIndex = 0;
			for (const auto* line : group->m_lines)
			{
				auto* renderLine = new RenderLine();
				renderLine->m_yPosStart = yPos;
				renderLine->m_yPosEnd = yPos + LINE_HEIGHT;
				yPos += LINE_HEIGHT + 2;
				m_renderLines.push_back(renderLine);

				if (lineIndex == 0)
					renderLine->m_text = name;

				LayoutLine(type, *line, lineIndex, *renderLine, xMax);
				lineIndex += 1;
			}
		}
	}

	void CallTreeView::LayoutAll()
	{
		// clear rendering data
		utils::ClearPtr(m_renderLines);

		// than threads, than IRQ groups, than APC
		uint32_t y = 0;
		uint32_t x = 0;

		LayoutGroups(trace::ContextType::Thread, y, x, "Threads");
		LayoutGroups(trace::ContextType::IRQ, y, x, "IRQ");
		LayoutGroups(trace::ContextType::APC, y, x, "APC");

		// update render size
		m_layoutSize = wxSize(x, y);

		// update scrollbar
		SetScrollRate(1, 1);
		SetVirtualSize(m_layoutSize.x, m_layoutSize.y);
	}

	void CallTreeView::OnPaint(wxPaintEvent& evt)
	{
		// setup drawing
		wxBufferedPaintDC dc(this);
		DoPrepareDC(dc);

		// get the draw range
		const auto size = GetClientSize();
		const auto start = GetViewStart();
		const auto end = start + size;

		// colors
		const auto backColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
		const auto textColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
		const auto shadowColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
		const auto selectColor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

		// draw background
		dc.SetBackground(wxBrush(backColor));
		dc.Clear();

		// pen
		wxPen solidPen(wxColor(0, 0, 0), 1);
		wxPen writePen(wxColor(255, 255, 255), 1);
		wxPen shadowPen(shadowColor, 1);
		wxBrush selectionBrush(selectColor);

		// draw lines
		for (const auto& line : m_renderLines)
		{
			const auto height = line->m_yPosEnd - line->m_yPosStart;

			// draw line caption
			if (!line->m_text.empty())
			{
				dc.SetPen(writePen);
				dc.DrawText(line->m_text, wxPoint(start.x + 10, line->m_yPosStart - LINE_HEIGHT));

				dc.SetTextForeground(textColor);
				dc.DrawLine(start.x, line->m_yPosStart - 4, end.x, line->m_yPosStart - 4);
			}

			// skip if not visible
			if ((int)line->m_yPosEnd <= start.y)
				continue;

			// end when becomes invisible
			if ((int)line->m_yPosStart > end.y + LINE_HEIGHT)
				break;

			// render line blocks
			for (const auto& block : line->m_blocks)
			{
				// skip if not visible
				if ((int)block.m_end <= start.x)
					continue;

				// end when becomes invisible
				if ((int)block.m_start > end.x)
					break;

				// draw block
				{
					const auto width = block.m_end - block.m_start;
					const wxRect rect(block.m_start, line->m_yPosStart, width, height);

					if (m_currentPosition >= block.m_logicStart && m_currentPosition <= block.m_logicEnd)
						dc.SetBrush(selectionBrush);
					else
						dc.SetBrush(*block.m_brush);

					dc.SetPen(shadowPen);
					dc.DrawRectangle(rect);

					// draw text
					if (!block.m_caption.empty())
					{
						dc.SetClippingRegion(rect);
						dc.SetTextForeground(textColor);
						dc.DrawText(block.m_caption, wxPoint(block.m_start + 5, line->m_yPosStart + 2));
						dc.DestroyClippingRegion();
					}
				}
			}
		}

		// draw the current frame
		{
			auto pos = m_currentPosition / m_ticksPerPixel;
			dc.SetPen(wxPen(wxColor(0, 0, 0), 3));
			dc.DrawLine(pos, start.y - 100, pos, end.y + 100);
		}
	}

	void CallTreeView::OnEraseBackground(wxEraseEvent& evt)
	{
	}

	void CallTreeView::OnMouseClick(wxMouseEvent& evt)
	{
		if (evt.LeftIsDown())
		{
			const auto scrollX = GetViewStart().x;
			const uint64 pos = (scrollX + evt.GetPosition().x) * m_ticksPerPixel;
			m_navigator->NavigateToFrame(pos);
		}
	}

	void CallTreeView::ZoomOut(const uint64_t zoomCenterPos)
	{
		const auto zoomLimit = 1 << 16;
		if (m_ticksPerPixel < zoomLimit)
			SetTicksPerPixel(m_ticksPerPixel * 2, zoomCenterPos);
	}

	void CallTreeView::ZoomIn(const uint64_t zoomCenterPos)
	{
		if (m_ticksPerPixel > 1)
			SetTicksPerPixel(m_ticksPerPixel / 2, zoomCenterPos);
	}

	void CallTreeView::OnMouseWheel(wxMouseEvent& evt)
	{
		const auto scrollX = GetViewStart().x;
		const uint64 pos = (scrollX + evt.GetPosition().x) * m_ticksPerPixel;

		if (evt.GetWheelRotation() < 0)
			ZoomOut(pos);
		else
			ZoomIn(pos);
	}


}  // tools