#include "build.h"
#include "progressDialog.h"
#include "callstackFrame.h"
#include "callstackFunctionTree.h"
#include "project.h"
#include "frame.h"

#include "..\backend\traceCalltree.h"
#include "..\backend\decodingNameMap.h"
#include "..\backend\decodingEnvironment.h"
#include "..\backend\decodingContext.h"

#pragma optimize("",off)

BEGIN_EVENT_TABLE(CCallstackFunctionTree, wxTreeListCtrl)
EVT_TREELIST_ITEM_ACTIVATED(wxID_ANY, CCallstackFunctionTree::OnItemDoubleClick)
EVT_TREELIST_ITEM_EXPANDING(wxID_ANY, CCallstackFunctionTree::OnItemExpanded)
END_EVENT_TABLE();

CCallstackFunctionTree::CCallstackFunctionTree(wxWindow* parent)
	: wxTreeListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE)
	, m_nameCache(nullptr)
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

CCallstackFunctionTree::~CCallstackFunctionTree()
{
	Clear();
}

//--

void CCallstackFunctionTree::FunctionNameCache::GetFunctionName(const uint32 address, std::string& outName)
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

//--

CCallstackFunctionTree::StackWalker::StackWalker(std::vector<FunctionInfo*>& outRoots)
	: m_outRoots(outRoots)
{}

void CCallstackFunctionTree::StackWalker::OnEnterFunction(const uint32 address, const uint32 trace)
{
	// create entry
	auto* entry = new FunctionInfo(address, trace);

	// add to list
	if (m_stack.empty())
		m_outRoots.push_back(entry);
	else
		m_stack.back().AddChild(entry);

	// add to stack
	m_stack.emplace_back(entry);
}

void CCallstackFunctionTree::StackWalker::OnLeaveFunction(const uint32 address, const uint32 trace)
{
	m_stack.back().m_func->m_leaveTrace = trace;
	m_stack.pop_back();
}

//---

void CCallstackFunctionTree::Clear()
{
	// remove all items
	DeleteAllItems();

	// clear
	for (auto* root : m_roots)
		delete root;
	m_roots.clear();

	// delete name cache
	delete m_nameCache;
	m_nameCache = nullptr;

}

wxTreeListItem CCallstackFunctionTree::FindItemForRange(wxTreeListItem parent, const uint32 traceIndex)
{
	if (!parent.IsOk())
		return parent;

	// scan children
	wxTreeListItem child = GetFirstChild(parent);
	while (child.IsOk())
	{
		// check range
		auto* funcInfo = static_cast<const FunctionInfoClientData*>(GetItemData(child));
		if (funcInfo)
		{
			auto* func = funcInfo->GetFunctionInfo();
			if (traceIndex >= func->m_enterTrace && traceIndex <= func->m_leaveTrace)
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

void CCallstackFunctionTree::CreateChildItems(wxTreeListItem item)
{
	if (IsExpanded(item))
		return;

	auto* funcInfo = static_cast<const FunctionInfoClientData*>(GetItemData(item));
	if (!funcInfo)
		return;

	bool isExpanded = false;
	{
		wxTreeListItem child = GetFirstChild(item);
		if (child.IsOk())
		{
			auto* funcInfo = static_cast<const FunctionInfoClientData*>(GetItemData(child));
			if (funcInfo != nullptr)
			{
				isExpanded = true;
			}
		}
	}

	if (isExpanded)
		return;	

	{
		wxTreeListItem child = GetFirstChild(item);
		if (child.IsOk())
			DeleteItem(child);
	}
	
	const auto* func = funcInfo->GetFunctionInfo();
	for (auto* cur = func->m_children; cur; cur = cur->m_next)
		CreateItem(item, cur);
}

void CCallstackFunctionTree::Highlight(const uint32 traceEntry)
{
	// find most nested function
	wxTreeListItem lastValid;
	wxTreeListItem cur = GetRootItem();
	while (cur.IsOk())
	{
		CreateChildItems(cur);
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

wxTreeListItem CCallstackFunctionTree::CreateItem(wxTreeListItem parent, const FunctionInfo* info)
{
	// resolve function name
	std::string name;
	m_nameCache->GetFunctionName(info->m_address, name);

	// add new entry
	wxTreeListItem id = AppendItem(parent, name.c_str(), -1, -1, new FunctionInfoClientData(info));

	// setup address
	char buffer[64];
	sprintf(buffer, "%08Xh", info->m_address);
	SetItemText(id, 1, buffer);

	// setup trace entry
	sprintf(buffer, "#%06u", info->m_enterTrace);
	SetItemText(id, 2, buffer);
	sprintf(buffer, "#%06u", info->m_leaveTrace);
	SetItemText(id, 3, buffer);

	// create fake child element
	if (info->m_children != nullptr)
		AppendItem(id, "__fake", -1, -1, nullptr);

	return id;
}

void CCallstackFunctionTree::Rebuild(const class trace::CallTree* callstackData, const uint32 maxRecursion, const bool unmangleFunctionNames)
{
	// clear items
	Clear();

	// build from data
	if (callstackData)
	{
		// get the name map to use
		const auto nameMap = &wxTheFrame->GetProject()->GetEnv().GetDecodingContext()->GetNameMap();
		m_nameCache = new FunctionNameCache(nameMap);

		// walk the stack creating the items
		StackWalker stackWalker(m_roots);
		callstackData->WalkStack(0, stackWalker, maxRecursion);

		// create the roots
		for (const auto* rootInfo : m_roots)
			CreateItem(GetRootItem(), rootInfo);
	}

	// enable the control based on havid data or not
	Enable(callstackData != nullptr);
}

void CCallstackFunctionTree::OnItemExpanded(wxTreeListEvent& event)
{
	wxTreeListItem item = event.GetItem();
	CreateChildItems(item);
}

void CCallstackFunctionTree::OnItemDoubleClick(wxTreeListEvent& event)
{
	wxTreeListItem item = event.GetItem();
	if (item.IsOk())
	{
		// update range info
		auto* funcInfo = static_cast<const FunctionInfoClientData*>(GetItemData(item));
		if (funcInfo)
		{
			auto* func = funcInfo->GetFunctionInfo();

			if (wxGetKeyState(WXK_SHIFT))
			{
				if (func->m_leaveTrace)
				{
					wxTheFrame->NavigateToTraceEntry(func->m_leaveTrace);
				}
				else
				{
					wxBell();
				}
			}
			else
			{
				wxTheFrame->NavigateToTraceEntry(func->m_enterTrace);
			}
		}
	}
}