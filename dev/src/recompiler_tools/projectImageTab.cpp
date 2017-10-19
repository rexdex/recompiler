#include "build.h"
#include "project.h"
#include "projectImage.h"
#include "projectWindow.h"
#include "projectImageTab.h"
#include "projectMemoryView.h"
#include "gotoAddressDialog.h"

#include "../recompiler_core/codeGenerator.h"
#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/image.h"
#include "../recompiler_core/decodingContext.h"
#include "../recompiler_core/decodingMemoryMap.h"
#include "../recompiler_core/decodingNameMap.h"
#include "buildDialog.h"

namespace tools
{
	BEGIN_EVENT_TABLE(ProjectImageTab, ProjectTab)
		EVT_MENU(XRCID("navigateGoTo"), ProjectImageTab::OnGotoAddress)
		EVT_MENU(XRCID("navigatePrevious"), ProjectImageTab::OnPrevAddress)
		EVT_MENU(XRCID("navigateNext"), ProjectImageTab::OnNextAddress)
		EVT_MENU(XRCID("navigateEntryPoint"), ProjectImageTab::OnGotoEntryAddress)
		EVT_MENU(XRCID("codeBuild"), ProjectImageTab::OnBuildCode)
		EVT_CHOICE(XRCID("ImageRecompilationTool"), ProjectImageTab::OnSettingsChanged)
		EVT_CHOICE(XRCID("ImageRecompilationMode"), ProjectImageTab::OnSettingsChanged)
		EVT_CHOICE(XRCID("ImageTypeChoice"), ProjectImageTab::OnSettingsChanged)
		EVT_CHECKBOX(XRCID("ImageLoad"), ProjectImageTab::OnSettingsChanged)
		EVT_CHECKBOX(XRCID("ImageRecompiled"), ProjectImageTab::OnSettingsChanged)
		EVT_CHECKBOX(XRCID("ImageIgnoreRelocations"), ProjectImageTab::OnSettingsChanged)
		EVT_TEXT(XRCID("ImageBaseAddress"), ProjectImageTab::OnSettingsChanged)
		EVT_TEXT(XRCID("ImageEntryAddress"), ProjectImageTab::OnSettingsChanged)
		EVT_LIST_ITEM_ACTIVATED(XRCID("ImportList"), ProjectImageTab::OnImportSelected)
		EVT_LIST_ITEM_ACTIVATED(XRCID("ExportList"), ProjectImageTab::OnExportSelected)
		EVT_LIST_ITEM_ACTIVATED(XRCID("SymbolList"), ProjectImageTab::OnSymbolSelected)
		EVT_TEXT(XRCID("ImportFilter"), ProjectImageTab::OnImportSearch)
		EVT_TEXT(XRCID("ExportFilter"), ProjectImageTab::OnExportSearch)
		EVT_TEXT(XRCID("SymbolFilter"), ProjectImageTab::OnSymbolSearch)
		EVT_TIMER(10000, ProjectImageTab::OnImportSearchTimer)
		EVT_TIMER(10001, ProjectImageTab::OnExportSearchTimer)
		EVT_TIMER(10002, ProjectImageTab::OnSymbolSearchTimer)
		EVT_LIST_ITEM_ACTIVATED(XRCID("FunctionList"), ProjectImageTab::OnSelectFunction)
		EVT_BUTTON(XRCID("RefreshFunctionList"), ProjectImageTab::OnRefreshFunctionList)
	END_EVENT_TABLE()

	ProjectImageTab::ProjectImageTab(ProjectWindow* parent, wxWindow* tabs, const std::shared_ptr<ProjectImage>& img)
		: ProjectTab(parent, tabs, ProjectTabType::Image)
		, m_disassemblyView(new ImageMemoryView(img, this))
		, m_disassemblyPanel(nullptr)
		, m_importSearchTimer(this, 10000)
		, m_exportSearchTimer(this, 10001)
		, m_symbolSearchTimer(this, 10002)
		, m_image(img)
	{
		// load the ui
		wxXmlResource::Get()->LoadPanel(this, tabs, wxT("ImageTab"));

		// create the disassembly window
		{
			auto* panel = XRCCTRL(*this, "DisasmPanel", wxPanel);
			m_disassemblyPanel = new MemoryView(panel);
			m_disassemblyPanel->SetDataView(m_disassemblyView);
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_disassemblyPanel, 1, wxEXPAND, 0);
		}

		// setup columns in exports table
		if (m_image->GetEnvironment().GetDecodingContext()->GetImage()->GetNumExports() > 0)
		{
			auto* box = XRCCTRL(*this, "ExportList", wxListCtrl);
			box->AppendColumn("Ordinal", wxLIST_FORMAT_LEFT, 80);
			box->AppendColumn("ExportName", wxLIST_FORMAT_LEFT, 400);
			box->AppendColumn("Address", wxLIST_FORMAT_LEFT, 200);
		}
		else
		{
			DeleteTab("Exports");
		}

		// setup columns in import table
		if (m_image->GetEnvironment().GetDecodingContext()->GetImage()->GetNumImports() > 0)
		{
			auto* box = XRCCTRL(*this, "ImportList", wxListCtrl);
			box->AppendColumn("Ordinal", wxLIST_FORMAT_LEFT ,80);
			box->AppendColumn("Type", wxLIST_FORMAT_LEFT, 120);
			box->AppendColumn("LibraryName", wxLIST_FORMAT_LEFT, 200);
			box->AppendColumn("ExportName", wxLIST_FORMAT_LEFT, 400);
			box->AppendColumn("ExportOrginal", wxLIST_FORMAT_LEFT, 120);
			box->AppendColumn("Stub Address", wxLIST_FORMAT_LEFT, 200);
		}
		else
		{
			DeleteTab("Imports");
		}

		// setup columns in symbols
		if (m_image->GetEnvironment().GetDecodingContext()->GetImage()->GetNumSymbols() > 0)
		{
			auto* box = XRCCTRL(*this, "SymbolList", wxListCtrl);
			box->AppendColumn("Type", wxLIST_FORMAT_LEFT, 120);
			box->AppendColumn("Address", wxLIST_FORMAT_LEFT, 120);
			box->AppendColumn("Name", wxLIST_FORMAT_LEFT, 600);
		}
		else
		{
			DeleteTab("Symbols");
		}

		// function list
		{
			auto* box = XRCCTRL(*this, "FunctionList", wxListCtrl);
			box->AppendColumn("Address", wxLIST_FORMAT_LEFT, 120);
			box->AppendColumn("Name", wxLIST_FORMAT_LEFT, 600);
		}

		// go to the address from address history
		NavigateToAddress(img->GetAddressHistory().GetCurrentAddress(), false);

		// refresh ui
		RefreshSectionList();
		RefreshSettings();
		RefreshImportList();
		RefreshExportList();
		RefreshSymbolList();
		RefreshFunctionList();

		// make sure the disassembly tab gets the initial focus
		XRCCTRL(*this, "Tabs", wxNotebook)->SetSelection(1);
		m_disassemblyPanel->SetFocus();
	}

	ProjectImageTab::~ProjectImageTab()
	{
		delete m_disassemblyView;
		m_disassemblyView = nullptr;
	}

	void ProjectImageTab::WriteSettings()
	{
		auto newSettings = m_image->GetSettings();

		// type of image
		{
			auto* box = XRCCTRL(*this, "ImageTypeChoice", wxChoice);
			if (box->GetSelection() != -1)
				newSettings.m_imageType = (ProjectImageType)box->GetSelection();
		}

		// tool to use for recompilation
		{
			auto* box = XRCCTRL(*this, "ImageRecompilationTool", wxChoice);

			std::vector<std::string> generatorNames;
			code::IGenerator::ListGenerators(generatorNames);
			if (box->GetSelection() != -1)
				newSettings.m_recompilationTool = generatorNames[box->GetSelection()];
		}	

		// recompilation mode
		{
			auto* box = XRCCTRL(*this, "ImageRecompilationMode", wxChoice);
			if (box->GetSelection() != -1)
				newSettings.m_recompilationMode = (ProjectRecompilationMode)box->GetSelection();
		}

		// flags
		{
			auto* box = XRCCTRL(*this, "ImageLoad", wxCheckBox);
			newSettings.m_emulationEnabled = box->GetValue();
		}

		// flags
		{
			auto* box = XRCCTRL(*this, "ImageRecompiled", wxCheckBox);
			newSettings.m_recompilationEnabled = box->GetValue();
		}

		// flags
		{
			auto* box = XRCCTRL(*this, "ImageIgnoreRelocations", wxCheckBox);
			newSettings.m_ignoreRelocations = box->GetValue();
		}

		// custom entry address
		{
			auto* box = XRCCTRL(*this, "ImageEntryAddress", wxTextCtrl);
			const auto text = box->GetValue();

			if (text.StartsWith("0x"))
			{
				const auto numText = text.Mid(2);
				uint64 val;
				if (numText.ToULongLong(&val, 16))
				{
					newSettings.m_customEntryAddress = val;
				}
			}
		}

		// custom base address
		{
			auto* box = XRCCTRL(*this, "ImageBaseAddress", wxTextCtrl);
			const auto text = box->GetValue();

			if (text.StartsWith("0x"))
			{
				const auto numText = text.Mid(2);
				uint64 val;
				if (numText.ToULongLong(&val, 16))
				{
					newSettings.m_customLoadAddress = val;
				}
			}
		}

		// store
		m_image->SetSettings(newSettings);
	}

	void ProjectImageTab::RefreshSettings()
	{
		// type of image
		{
			auto* box = XRCCTRL(*this, "ImageTypeChoice", wxChoice);
			box->SetEvtHandlerEnabled(false);
			box->Clear();
			box->AppendString("Application");
			box->AppendString("DynamicLibrary");
			box->SetSelection((int)m_image->GetSettings().m_imageType);
			box->SetEvtHandlerEnabled(true);
		}

		// tool to use for recompilation
		{
			auto* box = XRCCTRL(*this, "ImageRecompilationTool", wxChoice);
			box->SetEvtHandlerEnabled(false);
			box->Clear();

			std::vector<std::string> generatorNames;
			code::IGenerator::ListGenerators(generatorNames);

			for (uint32 i = 0; i < generatorNames.size(); i++)
			{
				const auto& gen = generatorNames[i];
				box->AppendString(gen);

				if (gen == m_image->GetSettings().m_recompilationTool)
					box->SetSelection(i);
			}

			if (box->GetSelection() == -1)
				box->SetSelection(0);
			box->SetEvtHandlerEnabled(true);
		}

		// recompilation mode
		{
			auto* box = XRCCTRL(*this, "ImageRecompilationMode", wxChoice);
			box->SetEvtHandlerEnabled(false);
			box->Clear();
			box->AppendString("Debug");
			box->AppendString("SafeRelease");
			box->AppendString("FastRelease");
			box->SetSelection((int)m_image->GetSettings().m_recompilationMode);
			box->SetEvtHandlerEnabled(true);
		}

		// flags
		{
			auto* box = XRCCTRL(*this, "ImageLoad", wxCheckBox);
			box->SetEvtHandlerEnabled(false);
			box->SetValue(m_image->GetSettings().m_emulationEnabled);
			box->SetEvtHandlerEnabled(true);
		}

		// flags
		{
			auto* box = XRCCTRL(*this, "ImageRecompiled", wxCheckBox);
			box->SetEvtHandlerEnabled(false);
			box->SetValue(m_image->GetSettings().m_recompilationEnabled);
			box->SetEvtHandlerEnabled(true);
		}

		// flags
		{
			auto* box = XRCCTRL(*this, "ImageIgnoreRelocations", wxCheckBox);
			box->SetEvtHandlerEnabled(false);
			box->SetValue(m_image->GetSettings().m_ignoreRelocations);
			box->SetEvtHandlerEnabled(true);
		}

		// custom entry address
		{
			auto* box = XRCCTRL(*this, "ImageEntryAddress", wxTextCtrl);
			box->SetEvtHandlerEnabled(false);
			box->SetValue(wxString::Format("0x%08llX", m_image->GetSettings().m_customEntryAddress));
			box->SetEvtHandlerEnabled(true);
		}

		// custom base address
		{
			auto* box = XRCCTRL(*this, "ImageBaseAddress", wxTextCtrl);
			box->SetEvtHandlerEnabled(false);
			box->SetValue(wxString::Format("0x%08llX", m_image->GetSettings().m_customLoadAddress));
			box->SetEvtHandlerEnabled(true);
		}
	}

	void ProjectImageTab::RefreshSectionList()
	{
		wxChoice* ctrl = XRCCTRL(*this, "sectionList", wxChoice);

		// begin update
		ctrl->Freeze();
		ctrl->Clear();

		// active offset
		const auto mainImage = m_image->GetEnvironment().GetImage();
		const auto activeAddress = m_disassemblyPanel->GetCurrentRVA();
		const auto activeOffset = activeAddress - mainImage->GetBaseAddress();

		// add sections
		int32 newSectionIndex = -1;
		const uint32 numSection = mainImage->GetNumSections();
		for (uint32 i = 0; i < numSection; ++i)
		{
			const auto section = mainImage->GetSection(i);

			// mode
			const char* mode = "";
			if (section->CanExecute())
				mode = "+x";
			else if (section->CanWrite())
				mode = "+w";

			// format section string
			char buffer[128];
			sprintf_s(buffer, sizeof(buffer), "%hs %hs (0x%06llX - 0x%06llX) %1.2fKB",
				section->GetName().c_str(), mode,
				section->GetVirtualOffset() + mainImage->GetBaseAddress(),
				section->GetVirtualOffset() + section->GetVirtualSize() - 1 + mainImage->GetBaseAddress(),
				section->GetVirtualSize() / 1024.0f);
			ctrl->AppendString(buffer);

			// valid section for our new address ?
			if (section->IsValidOffset(activeOffset))
				newSectionIndex = i;
		}

		// no sections
		if (ctrl->GetCount() == 0)
		{
			ctrl->Enable(false);
			ctrl->AppendString("(No sections)");
			ctrl->SetSelection(0);
		}
		else
		{
			ctrl->SetSelection(newSectionIndex >= 0 ? newSectionIndex : 0);
			ctrl->Enable(true);
		}

		// refresh
		ctrl->Thaw();
		ctrl->Refresh();

		// connect to user event
		ctrl->Connect(wxEVT_CHOICE, wxCommandEventHandler(ProjectImageTab::OnSelectSection), nullptr, this);
	}

	bool ProjectImageTab::Navigate(const NavigationType type)
	{
		if (type == NavigationType::Back)
		{
			const auto newAddress = m_image->GetAddressHistory().NavigateBack();
			return NavigateToAddress(newAddress, false);
		}
		else if (type == NavigationType::Advance)
		{
			const auto newAddress = m_image->GetAddressHistory().NavigateForward();
			return NavigateToAddress(newAddress, false);
		}
		else
		{
			return false;
		}
	}

	bool ProjectImageTab::NavigateToAddress(const uint64 address, const bool addToHistory)
	{
		// do not navigate to invalid address
		if (address == INVALID_ADDRESS)
			return false;

		// check if the address if within the limits of the image
		const auto baseAddress = m_image->GetEnvironment().GetImage()->GetBaseAddress();
		const auto endAddress = baseAddress + m_image->GetEnvironment().GetImage()->GetMemorySize();
		if (address < baseAddress || address >= endAddress)
		{
			GetProjectWindow()->GetApp()->GetLogWindow().Error("Image: Trying to navigate to address 0x%08llX that is outside the image boundary", address);
			return false;
		}

		// move to offset
		const auto newOffset = address - baseAddress;
		m_disassemblyPanel->SetActiveOffset(newOffset, addToHistory);
		SelectTab("Disassembly");
		return true;
	}

	void ProjectImageTab::OnSelectSection(wxCommandEvent& evt)
	{
		// get selection id
		wxChoice* ctrl = XRCCTRL(*this, "sectionList", wxChoice);
		const int selectedSectionID = ctrl->GetSelection();

		// get the section
		const auto image = m_image->GetEnvironment().GetImage();
		const auto* section = image->GetSection(selectedSectionID);
		NavigateToAddress(section->GetVirtualOffset() + image->GetBaseAddress(), true);
	}

	void ProjectImageTab::OnGotoAddress(wxCommandEvent& evt)
	{
		GotoAddressDialog dlg(this, m_image, m_disassemblyPanel);
		const uint32 newAddres = dlg.GetNewAddress();
		NavigateToAddress(newAddres, true);
	}

	void ProjectImageTab::OnGotoEntryAddress(wxCommandEvent& evt)
	{
		const auto entryAddress = m_image->GetEnvironment().GetDecodingContext()->GetEntryPointRVA();
		NavigateToAddress(entryAddress, true);
	}

	void ProjectImageTab::OnBuildCode(wxCommandEvent& evt)
	{
		// collect tasks
		std::vector<BuildTask> buildTasks;
		m_image->GetCompilationTasks(buildTasks);

		// run build process
		BuildDialog dlg(this);
		dlg.RunCommands(buildTasks);
	}

	void ProjectImageTab::OnNextAddress(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::Advance))
			wxMessageBox(wxT("Navigation failed"), wxT("Go to next address"), wxICON_WARNING, this);
	}

	void ProjectImageTab::OnPrevAddress(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::Back))
			wxMessageBox(wxT("Navigation failed"), wxT("Go to next address"), wxICON_WARNING, this);
	}

	void ProjectImageTab::OnSettingsChanged(wxCommandEvent& evt)
	{
		WriteSettings();
	}

	void ProjectImageTab::RefreshExportList()
	{
		auto* box = XRCCTRL(*this, "ExportList", wxListCtrl);
		if (!box)
			return;

		auto* searchBox = XRCCTRL(*this, "ImportFilter", wxTextCtrl);
		const std::string pattern = searchBox->GetValue();

		box->Freeze();
		box->DeleteAllItems();

		const auto* image = m_image->GetEnvironment().GetDecodingContext()->GetImage().get();
		const auto numExports = image->GetNumExports();
		for (uint32 i = 0; i < numExports; ++i)
		{
			const auto* dat = image->GetExports(i);

			const auto* name = dat->GetName();
			if (!pattern.empty() && !strstr(name, pattern.c_str()))
				continue;

			const auto index = box->GetItemCount();
			box->InsertItem(index, wxString::Format("%u", dat->GetIndex()));

			box->SetItem(index, 2, dat->GetName());
			box->SetItem(index, 3, wxString::Format("0x%08llX", dat->GetEntryPointAddress()));
			box->SetItemData(index, i);
		}

		box->Thaw();
	}

	void ProjectImageTab::SelectTab(const char* tabName)
	{
		auto* tabs = XRCCTRL(*this, "Tabs", wxNotebook);

		const auto numTabs = tabs->GetPageCount();
		for (uint32 i = 0; i < numTabs; ++i)
		{
			const auto pageName = tabs->GetPageText(i);
			if (pageName == tabName)
			{
				tabs->SetSelection(i);
				break;
			}
		}
	}

	void ProjectImageTab::DeleteTab(const char* tabName)
	{
		auto* tabs = XRCCTRL(*this, "Tabs", wxNotebook);

		const auto numTabs = tabs->GetPageCount();
		for (uint32 i = 0; i < numTabs; ++i)
		{
			const auto pageName = tabs->GetPageText(i);
			if (pageName == tabName)
			{
				tabs->RemovePage(i);
				break;
			}
		}
	}

	void ProjectImageTab::RefreshImportList()
	{
		auto* box = XRCCTRL(*this, "ImportList", wxListCtrl);
		if (!box)
			return;

		auto* searchBox = XRCCTRL(*this, "ImportFilter", wxTextCtrl);
		const std::string pattern = searchBox->GetValue();

		box->Freeze();
		box->DeleteAllItems();

		const auto* image = m_image->GetEnvironment().GetDecodingContext()->GetImage().get();
		const auto numImports = image->GetNumImports();
		for (uint32 i = 0; i < numImports; ++i)
		{
			const auto* imageImport = image->GetImport(i);

			const auto* name = imageImport->GetExportName();
			if (!pattern.empty() && !strstr(name, pattern.c_str()))
				continue;

			const auto index = box->GetItemCount();
			box->InsertItem(index, wxString::Format("%u", i));

			const auto* type = "Unknown";
			switch (imageImport->GetType())
			{
				case image::Import::eImportType_Function: type = "Function"; break;
				case image::Import::eImportType_Data: type = "Data"; break;
			}

			box->SetItem(index, 1, type);
			box->SetItem(index, 2, imageImport->GetExportImageName());
			box->SetItem(index, 3, imageImport->GetExportName());
			box->SetItem(index, 4, wxString::Format("%u", imageImport->GetExportIndex()));
			box->SetItem(index, 5, wxString::Format("0x%08llX", imageImport->GetEntryAddress()));
			box->SetItemData(index, i);
		}

		box->Thaw();
	}

	void ProjectImageTab::RefreshFunctionList()
	{
		auto* box = XRCCTRL(*this, "FunctionList", wxListCtrl);
		if (!box)
			return;

		const auto dc = m_image->GetEnvironment().GetDecodingContext();
		const auto& map = dc->GetMemoryMap();
		const auto& functionList = map.GetFunctionRoots();

		std::vector<uint64> addresses;
		addresses.reserve(functionList.size());
		for (const auto& it : functionList)
			addresses.push_back(it);
		std::sort(addresses.begin(), addresses.end());

		box->Freeze();
		box->DeleteAllItems();

		for (uint32 i=0; i<addresses.size(); ++i)
		{
			const auto addr = addresses[i];

			// base address
			const auto index = box->GetItemCount();
			box->InsertItem(index, wxString::Format("0x%08llX", addr));
			box->SetItemData(index, addr);

			// get name if available
			const auto extraName = dc->GetNameMap().GetName(addr);
			if (extraName && *extraName)
				box->SetItem(index, 1, extraName);
		}

		box->Thaw();
	}

	void ProjectImageTab::RefreshSymbolList()
	{
		auto* box = XRCCTRL(*this, "SymbolList", wxListCtrl);
		if (!box)
			return;

		auto* searchBox = XRCCTRL(*this, "SymbolFilter", wxTextCtrl);
		const std::string pattern = searchBox->GetValue();

		box->Freeze();
		box->DeleteAllItems();

		const auto* image = m_image->GetEnvironment().GetDecodingContext()->GetImage().get();
		const auto num = image->GetNumSymbols();
		for (uint32 i = 0; i < num; ++i)
		{
			const auto* dat = image->GetSymbol(i);

			const auto* name = dat->GetName();
			if (!pattern.empty() && !strstr(name, pattern.c_str()))
				continue;

			const char* type = "Unknown";
			switch (dat->GetType())
			{
				case image::Symbol::eSymbolType_Function: type = "Function"; break;
				case image::Symbol::eSymbolType_Variable: type = "Data"; break;
			}

			const auto index = box->GetItemCount();
			box->InsertItem(index, type);
			box->SetItem(index, 1, wxString::Format("0x%08llX", dat->GetAddress()));
			box->SetItem(index, 2, dat->GetName());
			box->SetItemData(index, i);
		}

		box->Thaw();
	}

	void ProjectImageTab::OnImportSearch(wxCommandEvent& evt)
	{
		m_importSearchTimer.Stop();
		m_importSearchTimer.Start(200, true);
	}

	void ProjectImageTab::OnExportSearch(wxCommandEvent& evt)
	{
		m_exportSearchTimer.Stop();
		m_exportSearchTimer.Start(200, true);
	}

	void ProjectImageTab::OnSymbolSearch(wxCommandEvent& evt)
	{
		m_symbolSearchTimer.Stop();
		m_symbolSearchTimer.Start(200, true);
	}

	void ProjectImageTab::OnImportSearchTimer(wxTimerEvent& evt)
	{
		RefreshImportList();
	}

	void ProjectImageTab::OnExportSearchTimer(wxTimerEvent& evt)
	{
		RefreshExportList();
	}

	void ProjectImageTab::OnSymbolSearchTimer(wxTimerEvent& evt)
	{
		RefreshSymbolList();
	}

	void ProjectImageTab::OnImportSelected(wxListEvent& evt)
	{
		auto* box = XRCCTRL(*this, "ImportList", wxListCtrl);
		if (box)
		{
			const auto id = evt.GetItem().GetId();
			const auto index = (uint32)box->GetItemData(id);

			const auto* image = m_image->GetEnvironment().GetDecodingContext()->GetImage().get();
			if (index < image->GetNumImports())
			{
				const auto* dat = image->GetImport(index);
				NavigateToAddress(dat->GetEntryAddress(), true);
			}
		}
	}

	void ProjectImageTab::OnExportSelected(wxListEvent& evt)
	{
		auto* box = XRCCTRL(*this, "ExportList", wxListCtrl);
		if (box)
		{
			const auto id = evt.GetItem().GetId();
			const auto index = (uint32)box->GetItemData(id);

			const auto* image = m_image->GetEnvironment().GetDecodingContext()->GetImage().get();
			if (index < image->GetNumExports())
			{
				const auto* dat = image->GetExports(index);
				NavigateToAddress(dat->GetEntryPointAddress(), true);
			}
		}
	}

	void ProjectImageTab::OnSymbolSelected(wxListEvent& evt)
	{
		auto* box = XRCCTRL(*this, "SymbolList", wxListCtrl);
		if (box)
		{
			const auto id = evt.GetItem().GetId();
			const auto index = (uint32)box->GetItemData(id);

			const auto* image = m_image->GetEnvironment().GetDecodingContext()->GetImage().get();
			if (index < image->GetNumSymbols())
			{
				const auto* dat = image->GetSymbol(index);
				NavigateToAddress(dat->GetAddress(), true);
			}
		}
	}

	void ProjectImageTab::OnSelectFunction(wxListEvent& evt)
	{
		auto* box = XRCCTRL(*this, "FunctionList", wxListCtrl);
		if (box)
		{
			const auto id = evt.GetItem().GetId();
			const auto addr = (uint32)box->GetItemData(id);
			if (addr != 0)
			{
				NavigateToAddress(addr, true);
			}
		}
	}

	void ProjectImageTab::OnRefreshFunctionList(wxCommandEvent& evt)
	{
		RefreshFunctionList();
	}

} // tools