#pragma once

#include "../recompiler_core/traceDataFile.h"
#include "../recompiler_core/traceUtils.h"
#include "../recompiler_core/platformCPU.h"
#include "treelistctrl.h"

//---------------------------------------------------------------------------

namespace tools
{

	/// view showing register data
	class RegisterView : public wxPanel
	{
		DECLARE_EVENT_TABLE();

	public:
		RegisterView(wxWindow* parent);

		// initialize register list from a CPU definition
		void InitializeRegisters(const platform::CPU& cpuInfo, const platform::CPURegisterType regType);

		// update register values from a trace frame
		void UpdateRegisters(const trace::DataFrame& frame, const trace::DataFrame& nextFrame, const trace::RegDisplayFormat format);

	private:
		wxcode::wxTreeListCtrl*	m_list;

		struct RegInfo
		{
			const platform::CPURegister* m_rootReg;
			const platform::CPURegister* m_reg;
			wxTreeItemId m_item;

			wxString m_curValue;
			wxString m_nextValue;
			bool m_wasModified;

			inline RegInfo()
				: m_wasModified(false)
			{}
		};

		std::vector<RegInfo> m_registers;

		void UpdateRegisterList(const platform::CPU& cpuInfo, const platform::CPURegisterType regType);
		void UpdateRegisterValues(const trace::DataFrame& frame, const trace::DataFrame& nextFrame, const trace::RegDisplayFormat format);

		void CreateRegisterInfo(wxTreeItemId parentItem, const platform::CPURegister* rootReg, const platform::CPURegister* reg);
	};

} // tools

//---------------------------------------------------------------------------
