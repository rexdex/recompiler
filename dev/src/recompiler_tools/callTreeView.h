#pragma once

#include "canvas.h"
#include "../recompiler_core/traceDataFile.h"
#include "../xenon_launcher/xenonUtils.h"

namespace tools
{

	// units to use
	enum class CallTreeViewUnits
	{
		Seq,
		Ticks,
	};

	// call tree navigation
	class ICallTreeViewNavigationHelper
	{
	public:
		virtual ~ICallTreeViewNavigationHelper() {};

		virtual bool NavigateToFrame(const TraceFrameID id) = 0;
	};

	// call tree, displays call stack of the whole trace
	class CallTreeView : public wxScrolledWindow
	{
		DECLARE_EVENT_TABLE();

	public:
		CallTreeView(wxWindow* parent, ICallTreeViewNavigationHelper* navigator);
		virtual ~CallTreeView();

		/// clear all data
		void Clear();

		/// bind data
		void ExtractTraceData(trace::DataFile& data);

		/// sync the cursor position
		void SetPosition(const TraceFrameID seq);

		/// ensure that given frame is visible
		void EnsureVisible(const TraceFrameID seq);

	private:
		static const uint32 LINE_HEIGHT = 20;

		uint64_t m_currentPosition;
		uint64_t m_ticksPerPixel;
		int64_t m_offset;

		ICallTreeViewNavigationHelper* m_navigator;

		trace::DataFile* m_data;

		void SetTicksPerPixel(uint64_t ticks, const uint64_t zoomCenterPos);

		//--

		struct DataBlock
		{
			trace::LocationInfo m_start;
			trace::LocationInfo m_end;
		};

		struct DataLine
		{
			std::vector<DataBlock> m_blocks;
		};

		struct DataGroup
		{
			trace::ContextType m_type;
			std::vector<DataLine*> m_lines;
			TraceFrameID m_lastId;

			inline DataGroup(const trace::ContextType type)
				: m_lastId(0)
				, m_type(type)
			{}

			inline ~DataGroup()
			{
				utils::ClearPtr(m_lines);
			}

			inline DataLine* GetLine(const uint32_t index)
			{
				if (m_lines.size() == index)
				{
					auto* newLine = new DataLine();
					m_lines.push_back(newLine);
					return newLine;
				}
				else
				{
					return m_lines[index];
				}
			}

			inline TraceFrameID GetLastSeq() const
			{
				TraceFrameID ret = 0;
				for (const auto* line : m_lines)
				{
					if (!line->m_blocks.empty())
					{
						const auto& lastBlock = line->m_blocks.back();
						ret = std::max(ret, lastBlock.m_end.m_seq);
					}
				}
				return ret;
			}
		};

		std::vector<DataGroup*> m_groups;

		//--

		struct RenderBlock
		{
			const DataBlock* m_firstBlock;
			uint32 m_numBlocks; // number of merged blocks
			uint64_t m_start; // pixels
			uint64_t m_end; // pixels
			wxString m_caption;
			const wxBrush* m_brush;
		};

		struct RenderLine
		{
			uint32_t m_yPosStart;
			uint32_t m_yPosEnd;
			std::vector<RenderBlock> m_blocks;
		};

		wxBrush m_selectedBrush;

		std::vector<wxBrush> m_renderBrushesThreads;
		std::vector<wxBrush> m_renderBrushesIRQ;
		std::vector<wxBrush> m_renderBrushesAPC;

		std::vector<RenderLine*> m_renderLines;
		wxBitmap m_drawBitmap;

		wxSize m_layoutSize;

		void LayoutAll();
		void LayoutGroups(const trace::ContextType type, uint32_t& yPos, uint32_t& xMax);
		void LayoutLine(const trace::ContextType type, const DataLine& line, const uint32_t lineIndex, RenderLine& outRenderLine, uint32_t& xMax);

		uint32_t MapCoordinate(const trace::LocationInfo& info);

		//--

		DataGroup* CreateDataGroup(const trace::LocationInfo firstEntry, const trace::ContextType type);

		void CreateCallStackBlocks(const trace::DataFile& data, DataGroup& group, const uint32_t lineIndex, const uint32_t callStackEntry);

		void ZoomOut(const uint64_t zoomCenterPos);
		void ZoomIn(const uint64_t zoomCenterPos);

		void OnPaint(wxPaintEvent& evt);
		void OnEraseBackground(wxEraseEvent& evt);
		void OnMouseClick(wxMouseEvent& evt);
		void OnMouseWheel(wxMouseEvent& evt);
	};

} // tools