#pragma once

#include "..\backend\traceCalltree.h"

namespace decoding
{
	class NameMap;
}

class CCallstackFunctionTree : public wxTreeListCtrl
{
	DECLARE_EVENT_TABLE();

public:
	enum EIcon
	{
		eIcon_Closed,
		eIcon_Opened,
	};

	CCallstackFunctionTree( wxWindow* parent );
	~CCallstackFunctionTree();

	// clear data
	void Clear();

	// highlight trace index
	void Highlight( const uint32 traceEntry );

	// rebuild tree
	void Rebuild( const class trace::CallTree* callstackData, const uint32 maxRecursion, const bool unmangleFunctionNames );

private:
	// range search
	wxTreeListItem FindItemForRange( wxTreeListItem parent, const uint32 traceIndex );

	// create child functions
	void CreateChildItems(wxTreeListItem item);

	// events
	void OnItemDoubleClick( wxTreeListEvent& event );
	void OnItemExpanded(wxTreeListEvent& event);

	// last selected
	wxTreeListItem m_highlighted;

	// cache for function names
	class FunctionNameCache
	{
	public:
		FunctionNameCache(const decoding::NameMap* nameMap)
			: m_nameMap(nameMap)
		{}

		void GetFunctionName(const uint32 address, std::string& outName);

	private:
		const decoding::NameMap*				m_nameMap;
		std::map< uint32, std::string >		m_functionNames;
		bool								m_unmangle;
	};


	// name cache for functions
	FunctionNameCache* m_nameCache;

	// stack element
	struct FunctionInfo
	{
		uint32 m_address;
		uint32 m_enterTrace;
		uint32 m_leaveTrace;
		FunctionInfo* m_parent;
		FunctionInfo* m_children;
		FunctionInfo* m_next;
		uint32 m_numChildren;

		inline FunctionInfo(const uint32 address, const uint32 trace)
			: m_address(address)
			, m_enterTrace(trace)
			, m_leaveTrace(0)
			, m_parent(nullptr)
			, m_next(nullptr)
			, m_children(nullptr)
			, m_numChildren(0)
		{}

		inline ~FunctionInfo()
		{
			FunctionInfo* next = nullptr;
			for (auto* cur = m_children; cur; cur = next)
			{
				next = cur->m_next;
				delete cur;
			}
		}

		inline const FunctionInfo* FindForTraceIndex(const uint32 trace) const
		{
			if (trace >= m_enterTrace && trace <= m_leaveTrace)
			{
				for (auto* cur = m_children; cur; cur = cur->m_next)
				{
					auto* ret = cur->FindForTraceIndex(trace);
					if (ret)
						return ret;
				}

				return this;
			}

			return nullptr;
		}
	};

	// function client data
	class FunctionInfoClientData : public wxClientData
	{
	public:
		inline FunctionInfoClientData(const FunctionInfo* func)
			: m_func(func)
		{}

		inline const FunctionInfo* GetFunctionInfo() const
		{
			return m_func;
		}

	private:
		const FunctionInfo* m_func;
	};

	// stack walker
	class StackWalker : public trace::CallTree::IStackWalker
	{
	public:
		StackWalker(std::vector<FunctionInfo*>& outRoots);
		virtual void OnEnterFunction(const uint32 address, const uint32 trace) override;
		virtual void OnLeaveFunction(const uint32 address, const uint32 trace) override;

	private:
		struct StackEntry
		{
			FunctionInfo* m_func;
			FunctionInfo** m_lastChild;

			inline StackEntry()
				: m_func(nullptr)
				, m_lastChild(nullptr)
			{}

			inline StackEntry(FunctionInfo* info)
				: m_func(info)
				, m_lastChild(&info->m_children)
			{}

			inline void AddChild(FunctionInfo* info)
			{
				info->m_parent = m_func;
				info->m_next = nullptr;
				m_func->m_numChildren += 1;
				*m_lastChild = info;
				m_lastChild = &info->m_next;
			}
		};

		std::vector<StackEntry> m_stack;
		std::vector<FunctionInfo*>& m_outRoots;
	};

	// root of the function tree
	std::vector<FunctionInfo*> m_roots;

	// create item
	wxTreeListItem CreateItem(wxTreeListItem parent, const FunctionInfo* info);
};