#include "build.h"
#include "progressDialog.h"
#include "callstackFrame.h"
#include "callstackFunctionTree.h"
#include "project.h"

#include "../recompiler_core/traceCalltree.h"
#include "../recompiler_core/decodingNameMap.h"
#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/decodingContext.h"

namespace tools
{

	BEGIN_EVENT_TABLE(CallstackFunctionTree, wxTreeListCtrl)
		EVT_TREELIST_ITEM_ACTIVATED(wxID_ANY, CallstackFunctionTree::OnItemDoubleClick)
		END_EVENT_TABLE();

	CallstackFunctionTree::CallstackFunctionTree(wxWindow* parent)
		: wxTreeListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE)
	{
		// create layout
		AppendColumn(wxT("Function"), 300);
		AppendColumn(wxT("Address"), 100);
		AppendColumn(wxT("TraceEntry"), 70);
		AppendColumn(wxT("TraceExit"), 70);
		AppendColumn(wxT("#Visits"), 40);

		// icons
		//wxImageList* images = new wxImageList(16,16);
		//images->Add( LoadWXIcon( "tree_expand16" ) );
		//images->Add( LoadWXIcon( "tree_collapse16" ) );
		//GetDataView()->GetModel()->( images );
	}

	CallstackFunctionTree::~CallstackFunctionTree()
	{
	}

	namespace Helper
	{
		// trace range info
		class RangeInfo : public wxClientData
		{
		public:
			uint32	m_traceStart;
			uint32	m_traceEnd;

		public:
			RangeInfo(const uint32 start)
				: m_traceStart(start)
				, m_traceEnd(0xFFFFFFFF)
			{}
		};

		class FunctionNameCache
		{
		public:
			FunctionNameCache(const decoding::NameMap* nameMap, const bool unmangleFunctionNames)
				: m_nameMap(nameMap)
				, m_unmangle(unmangleFunctionNames)
			{
			}

			void GetFunctionName(const uint32 address, std::string& outName)
			{
				// get cached result
				const auto it = m_functionNames.find(address);
				if (it != m_functionNames.end())
				{
					outName = (*it).second;
					return;
				}

				// use the name map to access function name
				if (m_nameMap)
				{
					const char* storedName = m_nameMap->GetName(address);
					if (storedName)
					{
						/*// try to unmangle
						if ( m_unmangle )
						{
							char tempBuffer[ 2048 ];
							if ( wxTheFontList-> GetPlatform()->UnmangleFunctionName( storedName, tempBuffer, ARRAYSIZE(tempBuffer), true, false ) )
							{
								m_functionNames[ address ] = tempBuffer;
								outName = tempBuffer;
								return;
							}
						}*/

						m_functionNames[address] = storedName;
						outName = storedName;
						return;
					}
				}

				// use default name
				char buffer[64];
				sprintf(buffer, "Function 0x%08Xh", address);
				m_functionNames[address] = buffer;
				outName = buffer;
			}

		private:
			const decoding::NameMap*				m_nameMap;
			std::map< uint32, std::string >		m_functionNames;
			bool								m_unmangle;
		};

		// stack walker
		class StackWalker : public trace::CallTree::IStackWalker
		{
		public:
			StackWalker(wxTreeListCtrl* list, wxTreeListItem rootItem, Helper::FunctionNameCache& nameCache)
				: m_list(list)
				, m_nameCache(&nameCache)
			{
				m_stack.push_back(rootItem);
			}

			virtual void OnEnterFunction(const uint32 address, const uint32 trace) override
			{
				// get function name
				std::string name;
				m_nameCache->GetFunctionName(address, name);

				// add new entry
				wxTreeListItem parent = m_stack.back();
				wxTreeListItem id = m_list->AppendItem(parent, name.c_str(), -1, -1, new RangeInfo(trace));

				// setup address
				char buffer[64];
				sprintf(buffer, "%08Xh", address);
				m_list->SetItemText(id, 1, buffer);

				// setup trace entry
				sprintf(buffer, "#%06u", trace);
				m_list->SetItemText(id, 2, buffer);
				m_list->SetItemText(id, 3, "---");

				// push onto stacks
				m_stack.push_back(id);
			}

			virtual void OnLeaveFunction(const uint32 address, const uint32 trace) override
			{
				if (m_stack.size() > 1)
				{
					// pop from the stack
					wxTreeListItem id = m_stack.back();
					m_stack.pop_back();

					// update leaving address
					char buffer[64];
					sprintf(buffer, "#%06u", trace);
					m_list->SetItemText(id, 3, buffer);

					// update range info
					RangeInfo* range = static_cast<RangeInfo*>(m_list->GetItemData(id));
					if (range)
					{
						range->m_traceEnd = trace;
					}
				}
			}

		private:
			wxTreeListCtrl*					m_list;
			std::vector< wxTreeListItem >	m_stack;
			Helper::FunctionNameCache*		m_nameCache;
		};

	} // Helpers

	wxTreeListItem CallstackFunctionTree::FindItemForRange(wxTreeListItem parent, const uint32 traceIndex)
	{
		if (!parent.IsOk())
			return parent;

		// scan children
		wxTreeListItem child = GetFirstChild(parent);
		while (child.IsOk())
		{
			// check range
			auto range = static_cast<const Helper::RangeInfo*>(GetItemData(child));
			if (range)
			{
				if (traceIndex >= range->m_traceStart && traceIndex < range->m_traceEnd)
				{
					return child;
				}
			}

			// go to next
			child = GetNextSibling(child);
		}

		// nothing found
		return wxTreeListItem();
	}

	void CallstackFunctionTree::Highlight(const uint32 traceEntry)
	{
		// find most nested function
		wxTreeListItem lastValid;
		wxTreeListItem cur = GetRootItem();
		while (cur.IsOk())
		{
			lastValid = cur;
			cur = FindItemForRange(cur, traceEntry);
		}

		// valid ?
		if (lastValid.IsOk() && lastValid != GetRootItem())
		{
			Select(lastValid);
			m_view->EnsureVisible(wxDataViewItem(lastValid));
			Refresh();
		}
	}

	void CallstackFunctionTree::Rebuild(const class trace::CallTree* callstackData, const uint32 maxRecursion, const bool unmangleFunctionNames)
	{
		// begin update
		Freeze();
		DeleteAllItems();

		// build from data
		if (callstackData)
		{
			// get the name map to use
			const auto nameMap = (decoding::NameMap*)nullptr;// &wxTheFrame->GetProject()->GetEnv().GetDecodingContext()->GetNameMap();
			Helper::FunctionNameCache nameCache(nameMap, unmangleFunctionNames);
			Helper::StackWalker stackWalker(this, GetRootItem(), nameCache);

			// walk the stack
			callstackData->WalkStack(0, stackWalker, maxRecursion);

			// enable user to play with this stuff
			Enable(true);
		}
		else
		{
			// no data, do not allow user to mess around
			Enable(false);
		}

		// end
		Thaw();
		Refresh();
	}

	void CallstackFunctionTree::OnItemDoubleClick(wxTreeListEvent& event)
	{
		wxTreeListItem item = event.GetItem();
		if (item.IsOk())
		{
			// update range info
			Helper::RangeInfo* range = static_cast<Helper::RangeInfo*>(GetItemData(item));
			if (range)
			{
				if (wxGetKeyState(WXK_SHIFT))
				{
					if (range->m_traceEnd > range->m_traceStart)
					{
						//wxTheFrame->NavigateToTraceEntry(range->m_traceEnd);
					}
					else
					{
						wxBell();
					}
				}
				else
				{
					//wxTheFrame->NavigateToTraceEntry(range->m_traceStart);
				}
			}
		}
	}

} // tools