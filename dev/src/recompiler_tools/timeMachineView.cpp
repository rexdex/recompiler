#include "build.h"
#include "timeMachineView.h"

namespace tools
{

	//---------------------------------------------------------------------------

	static wxPoint RectCenter(const wxRect& rect)
	{
		return wxPoint(rect.GetLeft() + rect.GetWidth() / 2, rect.GetTop() + rect.GetHeight() / 2);
	}

	//---------------------------------------------------------------------------

	TimeMachineView::SlotInfo::SlotInfo(timemachine::Trace* trace, LayoutInfo* node, const timemachine::Entry::AbstractDependency* dependency)
		: m_name(dependency->GetCaption())
		, m_node(node)
		, m_offset(0, 0)
		, m_abstractDep(dependency)
		, m_abstractSource(nullptr)
	{
		m_value = dependency->GetValue(trace);
	}

	TimeMachineView::SlotInfo::SlotInfo(timemachine::Trace* trace, LayoutInfo* node, const timemachine::Entry::AbstractSource* source)
		: m_name(source->GetCaption())
		, m_node(node)
		, m_offset(0, 0)
		, m_abstractDep(nullptr)
		, m_abstractSource(source)
	{
		m_value = source->GetValue(trace);
	}

	TimeMachineView::SlotInfo::~SlotInfo()
	{
	}

	//---------------------------------------------------------------------------

	TimeMachineView::LayoutInfo::LayoutInfo(timemachine::Trace* trace, const timemachine::Entry* entry)
		: m_position(0, 0)
		, m_size(100, 30)
		, m_entry(entry)
		, m_orderingIndex(entry->GetTraceIndex())
		, m_renderingLevel(0)
		, m_basePositionY(0)
	{
		// determine display
		m_caption = entry->GetDisplay();

		// create input slots
		const uint32 numInputs = entry->GetNumDependencies();
		for (uint32 i = 0; i < numInputs; ++i)
		{
			const auto* dep = entry->GetDependency(i);
			m_inputs.push_back(new SlotInfo(trace, this, dep));
		}

		// create output slots
		const uint32 numOutputs = entry->GetNumSources();
		for (uint32 i = 0; i < numOutputs; ++i)
		{
			const auto* res = entry->GetSource(i);
			m_outputs.push_back(new SlotInfo(trace, this, res));
		}
	}

	TimeMachineView::LayoutInfo::~LayoutInfo()
	{
		// disconnect all sockets
		for (auto it : m_inputs)
			delete it;
		for (auto it : m_outputs)
			delete it;
	}

	//---------------------------------------------------------------------------

	TimeMachineView::TimeMachineView(wxWindow* parent, class timemachine::Trace* trace)
		: canvas::CanvasPanel(parent, wxCLIP_CHILDREN | wxCLIP_SIBLINGS | wxNO_BORDER)
		, m_requestedLayoutChange(0)
		, m_trace(trace)
		, m_mouseMode(0)
		, m_highlightSlot(nullptr)
		, m_highlightNode(nullptr)
	{
		// create the root node layout
		if (GetLayoutNode(m_trace->GetRoot()))
		{
			ShowRootNode();
		}
	}

	TimeMachineView::~TimeMachineView()
	{
		// delete visual nodes
		ClearNodes();

		// trace data is owned by the view, delete it
		if (m_trace)
		{
			delete m_trace;
			m_trace = nullptr;
		}
	}

	const int32 TimeMachineView::GetRootTraceIndex() const
	{
		return m_trace->GetRoot()->GetTraceIndex();
	}

	void TimeMachineView::ShowRootNode()
	{
		m_requestedLayoutChange = 1;
	}

	void TimeMachineView::ShowAllNodes()
	{
		m_requestedLayoutChange = 2;
	}

	void TimeMachineView::DoShowRootNode(const int w, const int h)
	{
		// no nodes
		if (m_nodes.empty())
			return;

		// make sure block layout is up to date
		PrepareLayout();

		// find center of the root node
		auto rootNode = GetLayoutNode(m_trace->GetRoot());
		const wxPoint rootCenter = rootNode->m_position + (rootNode->m_size / 2);

		// offset to center node, reset zoom
		SetOffset((w / 2) - rootCenter.x, (h / 2) - rootCenter.y);
		SetScale(1.0f);

		// redraw
		CanvasPanel::Repaint();
	}

	void TimeMachineView::DoShowAllNodes(const int w, const int h)
	{
		// no nodes
		if (m_nodes.empty())
			return;

		// make sure block layout is up to date
		PrepareLayout();

		// compute 
	}

	void TimeMachineView::ClearNodes()
	{
		// reset trace
		m_trace = nullptr;

		// delete all display nodes
		for (auto it : m_nodes)
			delete it.second;
	}

	void TimeMachineView::DeselectAll()
	{
		m_selection.clear();
		Refresh();
	}

	void TimeMachineView::Select(const timemachine::Entry* entry, const bool isSelected)
	{
		auto it = std::find(m_selection.begin(), m_selection.end(), entry);
		if (!isSelected && (it != m_selection.end()))
		{
			m_selection.erase(it);
			Refresh();
		}
		else if (isSelected && it == m_selection.end())
		{
			m_selection.push_back(entry);
			Refresh();
		}
	}

	const bool TimeMachineView::IsSelected(const timemachine::Entry* entry) const
	{
		return std::find(m_selection.begin(), m_selection.end(), entry) != m_selection.end();
	}

	const timemachine::Entry* TimeMachineView::GetEntryAtPos(const wxPoint mousePos)
	{
		PrepareLayout();

		for (auto it : m_nodes)
		{
			const auto rect = it.second->GetRect();
			if (rect.Contains(mousePos))
				return it.first;
		}

		return nullptr;
	}

	TimeMachineView::SlotInfo* TimeMachineView::GetUnconnectedSlotAtPos(const wxPoint mousePos)
	{
		PrepareLayout();

		for (auto it : m_nodes)
		{
			const auto* node = it.second;

			// check inputs
			for (auto jt : node->m_inputs)
			{
				if (!jt->HasConnections())
				{
					const auto rect = jt->GetUnconnectedBox();
					if (rect.Contains(mousePos))
						return jt;
				}
			}

			// check outputs
			for (auto jt : node->m_outputs)
			{
				if (!jt->HasConnections())
				{
					const auto rect = jt->GetUnconnectedBox();
					if (rect.Contains(mousePos))
						return jt;
				}
			}
		}

		// nothing found
		return nullptr;
	}

	void TimeMachineView::ExploreSlot(SlotInfo* slot)
	{
		// resolve the dependencies
		if (slot && slot->m_abstractDep)
		{
			// resolve the sources
			std::vector< const timemachine::Entry::AbstractSource* > resolvedSources;
			slot->m_abstractDep->Resolve(wxTheApp->GetLogWindow(), m_trace, resolvedSources);

			// create the layouts and connect the blocks
			std::vector< SlotInfo* > addedSlots;
			for (auto it : resolvedSources)
			{
				// get matching slot
				auto otherSlot = GetLayoutSlot(it);
				if (otherSlot != nullptr)
				{
					// collect the added nodes
					addedSlots.push_back(otherSlot);

					// connect
					if (slot->ConnectOneSide(otherSlot))
					{
						otherSlot->ConnectOneSide(slot);
					}
				}
			}

			// resolve layout
			PrepareLayout();

			// compute placement of the new nodes
			if (addedSlots.size() == 1)
			{
				auto* otherSlot = addedSlots[0];

				// align the input and output socket
				const int inputX = slot->m_offset.x + slot->m_node->m_position.x;
				const int outputX = otherSlot->m_offset.x + otherSlot->m_node->m_position.x;

				// how much should we move the output block ?
				const int deltaX = outputX - inputX;
				otherSlot->m_node->m_position.x -= deltaX;
			}
		}
	}

	TimeMachineView::LayoutInfo* TimeMachineView::GetLayoutNode(const timemachine::Entry* entry)
	{
		// invalid entry
		if (!entry)
			return nullptr;

		// find existing
		auto it = m_nodes.find(entry);
		if (it != m_nodes.end())
			return it->second;

		// create new entry
		LayoutInfo* info = new LayoutInfo(m_trace, entry);
		m_nodes[entry] = info;

		// add to list of dirty nodes
		m_dirtyNodes.push_back(info);

		// return created layout info
		return info;
	}

	TimeMachineView::SlotInfo* TimeMachineView::GetLayoutSlot(const timemachine::Entry::AbstractSource* source)
	{
		// invalid source
		if (!source)
			return nullptr;

		// get the layout for the parent node
		auto entryLayout = GetLayoutNode(source->GetEntry());
		if (!entryLayout)
			return nullptr;

		// find matching stuff
		for (auto it : entryLayout->m_inputs)
		{
			if (it->m_abstractSource == source)
				return it;
		}

		// find matching stuff
		for (auto it : entryLayout->m_outputs)
		{
			if (it->m_abstractSource == source)
				return it;
		}

		// not found
		return nullptr;
	}

	TimeMachineView::SlotInfo* TimeMachineView::GetLayoutSlot(const timemachine::Entry::AbstractDependency* dep)
	{
		// invalid source
		if (!dep)
			return nullptr;

		// get the layout for the parent node
		auto entryLayout = GetLayoutNode(dep->GetEntry());
		if (!entryLayout)
			return nullptr;

		// find matching stuff
		for (auto it : entryLayout->m_inputs)
		{
			if (it->m_abstractDep == dep)
				return it;
		}

		// find matching stuff
		for (auto it : entryLayout->m_outputs)
		{
			if (it->m_abstractDep == dep)
				return it;
		}

		// not found
		return nullptr;
	}

	void TimeMachineView::PrepareNodePlacement()
	{
		// reset depth of all nodes
		for (auto it : m_nodes)
		{
			auto node = it.second;
			node->m_renderingLevel = INT_MIN;
		}

		// layout params
		const int marginX = 40;
		const int marginY = 80;

		// root node is at depth 0
		auto rootNode = GetLayoutNode(m_trace->GetRoot());

		// get all nodes
		std::vector< LayoutInfo* > allNodes;
		for (const auto it : m_nodes)
		{
			allNodes.push_back(it.second);
		}

		// sort nodes by the ordering value
		std::sort(allNodes.begin(), allNodes.end(), LayoutInfo::SortByOrderingInfo);

		// setup levels
		int rootIndex = 0;
		for (int i = 0; i < (int)allNodes.size(); ++i)
		{
			if (allNodes[i] == rootNode)
			{
				rootIndex = i;
				break;
			}
		}

		// enumerate nodes
		for (int i = 0; i < (int)allNodes.size(); ++i)
		{
			allNodes[i]->m_renderingLevel = i - rootIndex;
		}

		// count number of nodes at each depth
		std::map< int32, int32 > nodeHeightY;
		int maxDepth = 0;
		int minDepth = 0;
		for (auto it : m_nodes)
		{
			const auto node = it.second;
			const auto level = node->m_renderingLevel;

			// take max of vertical sizes
			nodeHeightY[level] = std::max(nodeHeightY[level], node->m_size.y);

			// compute min/max level
			minDepth = std::min(minDepth, level);
			maxDepth = std::max(maxDepth, level);
		}

		// compute the vertical placement of the rows
		std::map< int32, int32 > nodePlacementY;
		nodePlacementY[0] = 0; // ROOT level is aways at 0

		// place the nodes with depth > 0
		for (int32 depth = 1; depth <= maxDepth; ++depth)
		{
			const int height = (nodeHeightY[depth - 1] + marginY);
			nodePlacementY[depth] = nodePlacementY[depth - 1] - height;
		}

		// place the nodes with depth < 0
		for (int32 depth = -1; depth >= minDepth; --depth)
		{
			const int height = (nodeHeightY[depth] + marginY);
			nodePlacementY[depth] = nodePlacementY[depth + 1] + height;
		}

		// place the nodes according to their depth (Y) and position
		for (auto it : m_nodes)
		{
			auto node = it.second;
			const auto level = node->m_renderingLevel;

			int offsetY = node->m_position.y - node->m_basePositionY;
			node->m_basePositionY = nodePlacementY[level];
			node->m_position.y = node->m_basePositionY + offsetY;
		}
	}

	void TimeMachineView::PrepareLayout()
	{
		// get the dirty nodes
		auto nodes = std::move(m_dirtyNodes);
		if (nodes.empty())
			return;

		// style
		const uint32 slotSeparation = 20;
		const uint32 innerMarginX = 30;
		const uint32 innerMarginY = 40;

		// refresh the size of the dirty nodes
		for (auto it : nodes)
		{
			// measure size of the text
			const wxSize textSize = canvas::CanvasPanel::GetTextExtent(it->m_caption);

			// measure the required size of the block, based on the number of inputs/outputs and the values
			uint32 inputRegionSize = 0;
			for (const auto& jt : it->m_inputs)
			{
				const wxSize textSize = canvas::CanvasPanel::GetTextExtent(jt->m_name);
				if (inputRegionSize) inputRegionSize += slotSeparation; // 
				inputRegionSize += textSize.x;
			}

			// output slots
			uint32 outputRegionSize = 0;
			for (const auto& jt : it->m_outputs)
			{
				const wxSize textSize = canvas::CanvasPanel::GetTextExtent(jt->m_name);
				if (outputRegionSize) outputRegionSize += slotSeparation; // 
				outputRegionSize += textSize.x;
			}

			// get the block size ( the max of the input/output/text )
			const uint32 blockSizeX = wxMax(wxMax(inputRegionSize, outputRegionSize), textSize.x);

			// compute node size and text placement
			it->m_size.x = blockSizeX + innerMarginX;
			it->m_size.y = textSize.y + innerMarginY;
			it->m_textOffset.x = it->m_size.x / 2;
			it->m_textOffset.y = it->m_size.y / 2;
			it->m_traceTextOffset.x = it->m_size.x;
			it->m_traceTextOffset.y = it->m_size.y + 2;
			it->m_addrTextOffset.x = it->m_size.x;
			it->m_addrTextOffset.y = it->m_size.y + 14;

			// input slots are placed on the top, output at the bottom of the node
			const uint32 inputY = 0;
			const uint32 outputY = it->m_size.y;

			// values
			uint32 valueY = 5;

			// place the input slots
			{
				uint32 slotX = innerMarginX / 2 + (blockSizeX - inputRegionSize) / 2;
				for (uint32 i = 0; i < it->m_inputs.size(); ++i)
				{
					const wxSize textSize = canvas::CanvasPanel::GetTextExtent(it->m_inputs[i]->m_name);

					it->m_inputs[i]->m_offset.x = slotX + textSize.x / 2;
					it->m_inputs[i]->m_offset.y = inputY;

					it->m_inputs[i]->m_textOffset.x = slotX + textSize.x / 2;
					it->m_inputs[i]->m_textOffset.y = inputY + 10;

					if (it->m_inputs[i]->m_value != "")
					{
						it->m_inputs[i]->m_valueOffset.x = it->m_size.x + 5;
						it->m_inputs[i]->m_valueOffset.y = valueY;
						valueY += 12;
					}

					slotX += textSize.x + slotSeparation;
				}
			}

			// place the output slots
			{
				uint32 slotX = innerMarginX / 2 + (blockSizeX - outputRegionSize) / 2;
				for (uint32 i = 0; i < it->m_outputs.size(); ++i)
				{
					const wxSize textSize = canvas::CanvasPanel::GetTextExtent(it->m_outputs[i]->m_name);

					it->m_outputs[i]->m_offset.x = slotX + textSize.x / 2;
					it->m_outputs[i]->m_offset.y = outputY;

					it->m_outputs[i]->m_textOffset.x = slotX + textSize.x / 2;
					it->m_outputs[i]->m_textOffset.y = outputY - 10;

					if (it->m_outputs[i]->m_value != "")
					{
						it->m_outputs[i]->m_valueOffset.x = it->m_size.x + 5;
						it->m_outputs[i]->m_valueOffset.y = valueY;
						valueY += 12;
					}

					slotX += textSize.x + slotSeparation;
				}
			}
		}

		// calculate graph depth
		PrepareNodePlacement();
	}

	void TimeMachineView::OnPaintCanvas(int width, int height)
	{
		// background
		Clear(wxColour(80, 80, 80));

		// layout change
		if (m_requestedLayoutChange == 1)
		{
			DoShowRootNode(width, height);
			m_requestedLayoutChange = 0;
		}
		else if (m_requestedLayoutChange == 2)
		{
			DoShowRootNode(width, height);
			m_requestedLayoutChange = 0;
		}

		// prepare node layout
		PrepareLayout();

		// style
		const uint32 nodeRectBevel = 8;

		// draw connections
		for (auto it : m_nodes)
		{
			const auto* node = it.second;

			// draw connections from inputs only
			for (auto jt : node->m_inputs)
			{
				const auto rect = jt->GetRect();

				// draw connections
				for (auto kt : jt->m_connections)
				{
					const auto otherRect = kt->GetRect();

					// draw connection
					DrawLine(RectCenter(rect), RectCenter(otherRect), wxColour(32, 32, 32), 2.0f);
				}
			}
		}

		// draw nodes
		for (auto it : m_nodes)
		{
			const auto* node = it.second;

			// SHAPE
			{
				// node background
				const bool isSelected = IsSelected(it.first);
				FillRoundedRect(node->GetRect(), wxColour(128, 128, 128), nodeRectBevel);
				DrawRoundedRect(node->GetRect(), isSelected ? wxColour(255, 255, 128) : wxColour(0, 0, 0), nodeRectBevel, isSelected ? 3.0f : 1.0f);

				// draw input slots
				for (auto jt : node->m_inputs)
				{
					const auto rect = jt->GetRect();

					// slot is not connected
					if (!jt->HasConnections())
					{
						const bool isHighligted = (jt == m_highlightSlot);
						const auto boxRect = jt->GetUnconnectedBox();
						DrawLine(RectCenter(rect), RectCenter(boxRect), wxColour(0, 0, 0), 1.0f);
						FillCircle(boxRect, wxColour(128, 128, 128));
						DrawCircle(boxRect, isHighligted ? wxColour(255, 255, 0) : wxColour(0, 0, 0));
					}

					// slot shape
					FillCircle(rect, wxColour(200, 128, 128));
					DrawCircle(rect, wxColour(0, 0, 0));
				}

				// draw output slots
				for (auto jt : node->m_outputs)
				{
					const auto rect = jt->GetRect();

					// slot is not connected
					if (!jt->HasConnections())
					{
						const bool isHighligted = (jt == m_highlightSlot);
						const auto boxRect = jt->GetUnconnectedBox();
						DrawLine(RectCenter(rect), RectCenter(boxRect), wxColour(0, 0, 0), 1.0f);
						FillCircle(boxRect, wxColour(128, 128, 128));
						DrawCircle(boxRect, isHighligted ? wxColour(255, 255, 0) : wxColour(0, 0, 0));
					}

					// slot shape
					FillCircle(rect, wxColour(128, 200, 128));
					DrawCircle(rect, wxColour(0, 0, 0));
				}
			}

			// TEXT
			{
				// block caption
				// TODO: instruction class should determine the color
				DrawText(node->GetTextPlacement() + wxPoint(1, 1), canvas::eFontType_Bold, node->m_caption.c_str().AsChar(), wxColour(0, 0, 0), canvas::eVerticalAlign_Center, canvas::eHorizontalAlign_Center);
				DrawText(node->GetTextPlacement(), canvas::eFontType_Bold, node->m_caption.c_str().AsChar(), wxColour(255, 255, 255), canvas::eVerticalAlign_Center, canvas::eHorizontalAlign_Center);

				// draw input captions
				for (auto jt : node->m_inputs)
				{
					DrawText(jt->GetTextPlacement(), canvas::eFontType_Normal, jt->m_name.c_str().AsChar(), wxColour(0, 0, 0), canvas::eVerticalAlign_Center, canvas::eHorizontalAlign_Center);

					// slot is not connected
					if (!jt->HasConnections())
					{
						const auto point = jt->GetUnconnectedTextPlacement();
						DrawText(point, canvas::eFontType_Normal, "?", wxColour(0, 0, 0), canvas::eVerticalAlign_Center, canvas::eHorizontalAlign_Center);
					}
				}

				// output captions
				for (auto jt : node->m_outputs)
				{
					DrawText(jt->GetTextPlacement(), canvas::eFontType_Normal, jt->m_name.c_str().AsChar(), wxColour(0, 0, 0), canvas::eVerticalAlign_Center, canvas::eHorizontalAlign_Center);

					// slot is not connected
					if (!jt->HasConnections())
					{
						const auto point = jt->GetUnconnectedTextPlacement();
						DrawText(point, canvas::eFontType_Normal, "?", wxColour(0, 0, 0), canvas::eVerticalAlign_Center, canvas::eHorizontalAlign_Center);
					}
				}
			}

			// VALUES
			if (IsSelected(node->m_entry))
			{
				for (auto jt : node->m_inputs)
				{
					if (!jt->m_value.empty())
					{
						wxString str;
						str += jt->m_name;
						str += " <-- ";
						str += jt->m_value;

						DrawText(jt->GetValueTextPlacement(), canvas::eFontType_Normal, str.c_str().AsChar(), wxColour(0, 0, 0), canvas::eVerticalAlign_Center, canvas::eHorizontalAlign_Left);
					}
				}

				// output captions
				for (auto jt : node->m_outputs)
				{
					if (!jt->m_value.empty())
					{
						wxString str;
						str += jt->m_name;
						str += " --> ";
						str += jt->m_value;

						DrawText(jt->GetValueTextPlacement(), canvas::eFontType_Normal, str.c_str().AsChar(), wxColour(0, 0, 0), canvas::eVerticalAlign_Center, canvas::eHorizontalAlign_Left);
					}
				}
			}

			// Node time order
			if (node->m_entry->GetTraceIndex() >= 0)
			{
				char traceIndex[64];
				sprintf_s(traceIndex, "#%05u", node->m_entry->GetTraceIndex());

				DrawText(node->GetTraceTextPlacement(), canvas::eFontType_Normal, traceIndex, wxColour(0, 0, 0), canvas::eVerticalAlign_Top, canvas::eHorizontalAlign_Right);
			}

			// Node code addr
			if (node->m_entry->GetCodeAddres() != 0)
			{
				char codeAddress[64];
				sprintf_s(codeAddress, "0x%08X", node->m_entry->GetCodeAddres());

				DrawText(node->GetAddrTextPlacement(), canvas::eFontType_Normal, codeAddress, wxColour(0, 0, 0), canvas::eVerticalAlign_Top, canvas::eHorizontalAlign_Right);
			}

		}
	}

	void TimeMachineView::OnMouseClick(wxMouseEvent& event)
	{
		if (m_mouseMode == 0)
		{
			if (event.LeftDown())
			{
				// remove previous selection
				if (!event.ShiftDown())
					DeselectAll();

				// find entry at point
				const auto* node = GetEntryAtPos(ClientToCanvas(event.GetPosition()));
				if (node != nullptr)
				{
					Select(node, !IsSelected(node));

					// start node movement 
					if (IsSelected(node))
					{
						CaptureMouse(canvas::eMouseCapture_Capture);
						m_mouseMode = 2;
					}
				}
				else
				{
					// start background scrolling
					CaptureMouse(canvas::eMouseCapture_Capture);
					m_mouseMode = 1;
				}
			}

			if (event.LeftDClick())
			{
				auto* slot = GetUnconnectedSlotAtPos(ClientToCanvas(event.GetPosition()));
				if (slot != nullptr)
				{
					ExploreSlot(slot);
				}
				else
				{
					const auto* node = GetEntryAtPos(ClientToCanvas(event.GetPosition()));
					if (node != nullptr)
					{
						//wxTheFrame->NavigateToTraceEntry(node->GetTraceIndex());
					}
				}
			}
		}
		else if (m_mouseMode == 1 && event.LeftUp())
		{
			CaptureMouse(canvas::eMouseCapture_None);
			m_mouseMode = 0;
		}
		else if (m_mouseMode == 2 && event.LeftUp())
		{
			CaptureMouse(canvas::eMouseCapture_None);
			m_mouseMode = 0;
		}
	}

	void TimeMachineView::OnMouseMove(wxMouseEvent& event, wxPoint delta)
	{
		if (m_mouseMode == 1)
		{
			Scroll(delta.x, delta.y, true);
			Repaint();
		}
		else if (m_mouseMode == 2)
		{
			for (auto* entry : m_selection)
			{
				LayoutInfo* layout = GetLayoutNode(entry);
				if (layout)
				{
					layout->m_position += delta;
				}
			}

			if (!m_selection.empty())
				Repaint();
		}
	}

	void TimeMachineView::UpdateHover(const wxPoint& mousePoint)
	{
		bool refresh = false;

		// update slot hover
		{
			auto newSlot = GetUnconnectedSlotAtPos(mousePoint);
			refresh |= (newSlot != m_highlightSlot);
			m_highlightSlot = newSlot;
		}

		// update node hover
		{
			auto newNode = GetEntryAtPos(mousePoint);
			refresh |= (newNode != m_highlightNode);
			m_highlightNode = newNode;
		}

		// redraw
		if (refresh)
		{
			Repaint();
		}
	}

	void TimeMachineView::OnMouseTrack(wxMouseEvent& event)
	{
		UpdateHover(ClientToCanvas(event.GetPosition()));
	}

} // tools