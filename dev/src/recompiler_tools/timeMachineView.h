#pragma once

//-----------------------------------------------------------------------------

#include "canvas.h"
#include "../recompiler_core/timemachine.h"

//-----------------------------------------------------------------------------

namespace tools
{

	/// time machine navigation helper
	class ITimeMachineViewNavigationHelper
	{
	public:
		virtual ~ITimeMachineViewNavigationHelper() {};

		/// navigate to frame
		virtual bool NavigateToFrame(const TraceFrameID id) = 0;
	};

	/// Displays code casual history
	class TimeMachineView : public canvas::CanvasPanel
	{
	public:
		TimeMachineView(wxWindow* parent, class timemachine::Trace* trace, ITimeMachineViewNavigationHelper* navigator);
		virtual ~TimeMachineView();

		// Set trace data index
		const TraceFrameID GetRootTraceIndex() const;

		// Focus on root node
		void ShowRootNode();

		// Show whole graph
		void ShowAllNodes();

	private:
		ITimeMachineViewNavigationHelper* m_navigator;

		timemachine::Trace*		m_trace;

		struct LayoutInfo;

		// Slot information
		struct SlotInfo
		{
			typedef std::vector< const SlotInfo* > TConnections;

			wxString				m_name;
			wxPoint					m_offset; // in parent block
			wxPoint					m_textOffset; // in parent block
			TConnections			m_connections;
			LayoutInfo*				m_node;

			wxString				m_value;
			wxPoint					m_valueOffset; // in parent block

			const timemachine::Entry::AbstractDependency*		m_abstractDep;
			const timemachine::Entry::AbstractSource*			m_abstractSource;

			SlotInfo(timemachine::Trace* trace, LayoutInfo* node, const timemachine::Entry::AbstractDependency* dependency);
			SlotInfo(timemachine::Trace* trace, LayoutInfo* node, const timemachine::Entry::AbstractSource* source);
			~SlotInfo();

			inline const bool HasConnections() const
			{
				return !m_connections.empty();
			}

			inline const bool IsConnected(const SlotInfo* other) const
			{
				return std::find(m_connections.begin(), m_connections.end(), other) != m_connections.end();
			}

			inline const bool ConnectOneSide(const SlotInfo* other)
			{
				if (other && !IsConnected(other))
				{
					m_connections.push_back(other);
					return true;
				}

				return false;
			}

			inline const wxRect GetRect() const
			{
				const uint32 slotSize = 6;
				return wxRect(m_node->m_position.x + m_offset.x - slotSize / 2, m_node->m_position.y + m_offset.y - slotSize / 2, slotSize, slotSize);
			}

			inline const wxPoint GetTextPlacement() const
			{
				return m_node->m_position + m_textOffset;
			}

			inline const wxPoint GetValueTextPlacement() const
			{
				return m_node->m_position + m_valueOffset;
			}

			inline const wxRect GetUnconnectedBox() const
			{
				const int dirY = (m_offset.y > (m_node->m_size.y / 2)) ? 1 : -1;
				const int dist = 30 * dirY;
				const int size = 18;
				return wxRect(m_node->m_position.x + m_offset.x - size / 2, m_node->m_position.y + m_offset.y + dist, size, size);
			}

			inline const wxPoint GetUnconnectedTextPlacement() const
			{
				const auto rect = GetUnconnectedBox();
				return wxPoint(rect.GetLeft() + 9, rect.GetTop() + 9);
			}
		};

		// Layout information
		struct LayoutInfo
		{
			wxString					m_caption;
			const timemachine::Entry*	m_entry;
			wxSize						m_size;
			wxPoint						m_position;
			int							m_basePositionY;
			wxPoint						m_textOffset;
			wxPoint						m_traceTextOffset;
			wxPoint						m_addrTextOffset;
			std::vector< SlotInfo* >	m_inputs;
			std::vector< SlotInfo* >	m_outputs;
			int32						m_orderingIndex;
			int32						m_renderingLevel;

			LayoutInfo(timemachine::Trace* trace, const timemachine::Entry* entry);
			~LayoutInfo();

			inline const wxPoint GetTextPlacement() const
			{
				return m_position + m_textOffset;
			}

			inline const wxPoint GetTraceTextPlacement() const
			{
				return m_position + m_traceTextOffset;
			}

			inline const wxPoint GetAddrTextPlacement() const
			{
				return m_position + m_addrTextOffset;
			}

			inline const wxRect GetRect() const
			{
				return wxRect(m_position.x, m_position.y, m_size.x, m_size.y);
			}

			static inline bool SortByOrderingInfo(const LayoutInfo* a, const LayoutInfo* b)
			{
				return a->m_orderingIndex > b->m_orderingIndex;
			}
		};

		// Layout entries for time machine nodes
		typedef std::map< const timemachine::Entry*, LayoutInfo* >		TLayout;
		typedef std::vector< LayoutInfo* >								TNewNodes;
		typedef std::vector< const timemachine::Entry* >				TSelection;
		TLayout							m_nodes;
		TNewNodes						m_dirtyNodes;
		TSelection						m_selection;

		// Highlight
		const SlotInfo*					m_highlightSlot;
		const timemachine::Entry*		m_highlightNode;

		// Requested layout change
		int32							m_requestedLayoutChange;

		// Mouse mode
		int32							m_mouseMode;

		// Clear all nodes
		void ClearNodes();

		// Prepare layout
		void PrepareLayout();

		// Recopmpute depth of each node (determines it's position)
		void PrepareNodePlacement();

		// Finalize layout
		void DoShowRootNode(const int w, const int h);
		void DoShowAllNodes(const int w, const int h);

		// Selection
		void DeselectAll();
		void Select(const timemachine::Entry* entry, const bool isSelected);
		const bool IsSelected(const timemachine::Entry* entry) const;

		// Create/Get layout for node
		LayoutInfo* GetLayoutNode(const timemachine::Entry* entry);
		SlotInfo* GetLayoutSlot(const timemachine::Entry::AbstractSource* source);
		SlotInfo* GetLayoutSlot(const timemachine::Entry::AbstractDependency* dep);

		// Get entry at given location, mouse position is in canvas space
		const timemachine::Entry* GetEntryAtPos(const wxPoint mousePos);

		// Get unconnected entry at given mouse position (in canvas space)
		SlotInfo* GetUnconnectedSlotAtPos(const wxPoint mousePos);

		// Explore slot
		void ExploreSlot(SlotInfo* slot);

		// Update hover stuff
		void UpdateHover(const wxPoint& mousePoint);

		// CanvasPanel interface
		virtual void OnPaintCanvas(int width, int height) override;
		virtual void OnMouseClick(wxMouseEvent& event) override;
		virtual void OnMouseMove(wxMouseEvent& event, wxPoint delta) override;
		virtual void OnMouseTrack(wxMouseEvent& event) override;
	};

} // tools