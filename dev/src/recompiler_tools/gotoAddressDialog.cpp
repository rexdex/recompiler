#include "build.h"
#include "project.h"
#include "memoryView.h"
#include "widgetHelpers.h"
#include "gotoAddressDialog.h"

#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/image.h"
#include "projectImage.h"

namespace tools
{
	BEGIN_EVENT_TABLE(GotoAddressDialog, wxDialog)
		EVT_LISTBOX(XRCID("ModeList"), GotoAddressDialog::OnReferenceChanged)
		EVT_LISTBOX_DCLICK(XRCID("ModeList"), GotoAddressDialog::OnReferenceDblClick)
		EVT_LISTBOX_DCLICK(XRCID("RecentList"), GotoAddressDialog::OnHistoryDblClick)
		EVT_BUTTON(XRCID("Go"), GotoAddressDialog::OnGoTo)
		EVT_BUTTON(XRCID("Cancel"), GotoAddressDialog::OnCancel)
		EVT_TOGGLEBUTTON(XRCID("DecHex"), GotoAddressDialog::OnAddressTypeChanged)
		EVT_TIMER(12345, GotoAddressDialog::OnNavigationTimer)
	END_EVENT_TABLE()

	GotoAddressDialog::GotoAddressDialog(wxWindow* parent, const std::shared_ptr<ProjectImage>& project, MemoryView* view)
		: m_image(project)
		, m_view(view)
		, m_address(0)
		, m_navigateIndex(0)
		, m_navigateTimer(this, 12345)
	{
		// load the dialog
		wxXmlResource::Get()->LoadDialog(this, parent, wxT("GotoAddress"));

		// initialize custom helpers
		m_decHex = new ToggleButtonAuto(this, "DecHex", "Hex", "Dec", true);
		m_addressErrorHint = new TextBoxErrorHint(this, "RelativeAddress");

		// set the inital address from the view (view is not required!)
		if (view != nullptr)
			m_address = view->GetCurrentRVA();

		// update the address list and everything
		UpdateReferenceList();
		UpdateHistoryList();
		UpdateRelativeAddress();

		// set focus to the text control
		XRCCTRL(*this, "RelativeAddress", wxTextCtrl)->SetFocus();
		XRCCTRL(*this, "RelativeAddress", wxTextCtrl)->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(GotoAddressDialog::OnTextKeyDown), NULL, this);

		// layout the dialog
		Layout();
	}

	GotoAddressDialog::~GotoAddressDialog()
	{
	}

	const uint64 GotoAddressDialog::GetNewAddress()
	{
		if (0 != wxDialog::ShowModal())
			return INVALID_ADDRESS;

		// return the final address
		return m_address;
	}

	void GotoAddressDialog::UpdateHistoryList()
	{
		ListBoxUpdate list(this, "RecentList", false, false);

		// get the address history
		std::vector<uint64> fullAddressHistory;
		m_image->GetAddressHistory().GetFullHistory(fullAddressHistory);
		std::sort(fullAddressHistory.begin(), fullAddressHistory.end());

		// get reference address
		const uint32 baseAddressRVA = TListBoxSelection<uint32>(this, "ModeList").GetSelection(0);

		// display the address list
		uint32 prevAddr = 0;
		for (uint32 i = 0; i < fullAddressHistory.size(); ++i)
		{
			const uint32 rva = fullAddressHistory[i];
			if (rva == prevAddr)
			{
				continue;
			}

			prevAddr = rva;
			if (baseAddressRVA == 0)
			{
				list.AddItemWithData(new TClientData< uint32 >(rva), false, "%08Xh", rva);
			}
			else
			{
				wxLongLong_t relative = (wxLongLong_t)rva - (wxLongLong_t)baseAddressRVA;
				if (relative >= 0)
				{
					list.AddItemWithData(new TClientData< uint32 >(rva), false, "%08Xh (base + %04Xh)", rva, (uint32)relative);
				}
				else
				{
					const uint32 absRelative = (uint32)(-relative);
					list.AddItemWithData(new TClientData< uint32 >(rva), false, "%08Xh (base - %04Xh)", rva, (uint32)absRelative);
				}
			}
		}
	}

	void GotoAddressDialog::UpdateReferenceList()
	{
		ListBoxUpdate list(this, "ModeList");

		// absolute memory base
		{
			const uint32 memoryBase = 0;
			list.AddItemWithData(new TClientData<uint32>(0), true, "Absolute address (%08Xh)", 0);
		}

		// we have the image :)
		auto image = m_image->GetEnvironment().GetImage();
		const uint32 imageBase = image->GetBaseAddress();
		list.AddItemWithData(new TClientData<uint32>(imageBase), false, "Base of the image (%08Xh)", imageBase);

		// if we have a memory view we have more options
		if (m_view != nullptr)
		{
			// get the current address
			const uint32 currentRVA = m_view->GetCurrentRVA();

			// get the section base
			{
				// find base section
				const image::Section* section = nullptr;
				const uint32 numSection = image->GetNumSections();
				for (uint32 i = 0; i < numSection; ++i)
				{
					const image::Section* test = image->GetSection(i);
					const uint32 sectionStartRVA = imageBase + test->GetVirtualOffset();
					const uint32 sectionEndRVA = imageBase + test->GetVirtualOffset() + test->GetVirtualSize();

					if (currentRVA >= sectionStartRVA && currentRVA < sectionEndRVA)
					{
						section = test;
						break;
					}
				}

				// relative to section base
				if (nullptr != section)
				{
					const uint32 sectionBase = imageBase + section->GetVirtualOffset();
					list.AddItemWithData(new TClientData<uint32>(sectionBase), false, "Base of the section '%hs' (%08Xh)", section->GetName().c_str(), sectionBase);
				}
			}

			// relative to current address
			list.AddItemWithData(new TClientData<uint32>(currentRVA), false, "Current address (%08Xh)", currentRVA);
		}
	}

	void GotoAddressDialog::UpdateRelativeAddress()
	{
		// get the base address
		const uint32 baseAddress = TListBoxSelection<uint32>(this, "ModeList").GetSelection(0);

		// compute the relative address
		const bool hexBase = m_decHex->GetValue();
		const wxLongLong_t relativeAddress = (wxLongLong_t)m_address - (wxLongLong_t)baseAddress;
		const uint32 absRelativeAddress = relativeAddress > 0 ? (uint32)relativeAddress : (uint32)-relativeAddress;
		TextBoxUpdate(this, "RelativeAddress").SetText(
			(relativeAddress >= 0) ?
			(hexBase ? "%08X" : "%d") :
			(hexBase ? "-%08X" : "-%d"),
			absRelativeAddress);

		// reset the error hint
		m_addressErrorHint->ResetErrorHint();
	}

	bool GotoAddressDialog::GrabCurrentAddress()
	{
		const bool hex = m_decHex->GetValue();
		wxLongLong_t offset = 0;
		if (TextBoxReader(this, "RelativeAddress").GetInt(offset, hex))
		{
			const uint32 baseAddress = TListBoxSelection<uint32>(this, "ModeList").GetSelection(0);
			m_address = (uint32)((wxLongLong_t)baseAddress + offset);
			return true;
		}

		// invalid address
		return false;
	}

	void GotoAddressDialog::OnReferenceChanged(wxCommandEvent& event)
	{
		UpdateHistoryList();
		UpdateRelativeAddress();
	}

	void GotoAddressDialog::OnReferenceDblClick(wxCommandEvent& event)
	{
		const uint32 address = TListBoxSelection<uint32>(this, "ModeList").GetSelection(m_address);
		if (m_image->GetEnvironment().GetImage()->IsValidAddress(address))
		{
			m_address = address;
			EndDialog(0);
		}
	}

	void GotoAddressDialog::OnHistorySelected(wxCommandEvent& event)
	{
		UpdateRelativeAddress();
	}

	void GotoAddressDialog::OnHistoryDblClick(wxCommandEvent& event)
	{
		const uint32 address = TListBoxSelection<uint32>(this, "RecentList").GetSelection(m_address);
		if (m_image->GetEnvironment().GetImage()->IsValidAddress(address))
		{
			m_address = address;
			EndDialog(0);
		}
	}

	void GotoAddressDialog::OnGoTo(wxCommandEvent& event)
	{
		if (GrabCurrentAddress())
		{
			// is the address valid
			if (m_image->GetEnvironment().GetImage()->IsValidAddress(m_address))
			{
				// exit the dialog
				EndDialog(0);
				return;
			}
		}

		// specified address was not valid
		m_addressErrorHint->SetErrorHint(true);
	}

	void GotoAddressDialog::OnCancel(wxCommandEvent& event)
	{
		EndDialog(-1);
	}

	void GotoAddressDialog::OnAddressTypeChanged(wxCommandEvent& event)
	{
		wxLongLong_t offset = 0;
		if (TextBoxReader(this, "RelativeAddress").GetInt(offset, !m_decHex->GetValue()))
		{
			const uint32 baseAddress = TListBoxSelection<uint32>(this, "ModeList").GetSelection(0);
			m_address = (uint32)((wxLongLong_t)baseAddress + offset);
			UpdateRelativeAddress();
		}
	}

	void GotoAddressDialog::OnTextKeyDown(wxKeyEvent& event)
	{
		// fake navigation
		if (event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_DOWN)
		{
			wxListBox* list = XRCCTRL(*this, "ModeList", wxListBox);
			if (list->GetCount() > 1)
			{
				int index = list->GetSelection();
				const int lastIndex = list->GetCount() - 1;
				if (event.GetKeyCode() == WXK_UP)
				{
					m_navigateIndex = (index == 0) ? lastIndex : (index - 1);
					m_navigateTimer.StartOnce(50);
				}
				else if (event.GetKeyCode() == WXK_DOWN)
				{
					m_navigateIndex = (index == lastIndex) ? 0 : index + 1;
					m_navigateTimer.StartOnce(50);
				}
			}

			return;
		}

		// enter
		if (event.GetKeyCode() == WXK_RETURN)
		{
			wxCommandEvent fakeEvent;
			OnGoTo(fakeEvent);

			return;
		}

		// close
		if (event.GetKeyCode() == WXK_ESCAPE)
		{
			EndDialog(-1);
			return;
		}

		// skip rest of the chars
		event.Skip();
	}

	void GotoAddressDialog::OnNavigationTimer(wxTimerEvent& event)
	{
		wxListBox* list = XRCCTRL(*this, "ModeList", wxListBox);
		if (m_navigateIndex != list->GetSelection())
		{
			list->SetFocus();
			list->SetSelection(m_navigateIndex);

			UpdateHistoryList();
			UpdateRelativeAddress();

			XRCCTRL(*this, "RelativeAddress", wxTextCtrl)->SetFocus();
			XRCCTRL(*this, "RelativeAddress", wxTextCtrl)->SetSelection(-1, -1);
		}
	}

} // tools