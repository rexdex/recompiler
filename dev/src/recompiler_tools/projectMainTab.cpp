#include "build.h"
#include "app.h"
#include "project.h"
#include "projectImage.h"
#include "projectWindow.h"
#include "projectMainTab.h"
#include "../recompiler_core/platformDefinition.h"
#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/image.h"
#include "buildDialog.h"
#include "../recompiler_core/platformDecompilation.h"
#include "../recompiler_core/output.h"
#include "../recompiler_core/traceRawReader.h"
#include "progressDialog.h"
#include "../recompiler_core/traceDataFile.h"

namespace tools
{

	BEGIN_EVENT_TABLE(ProjectMainTab, ProjectTab)
		EVT_BUTTON(XRCID("ImageAdd"), ProjectMainTab::OnAddImage)
		EVT_BUTTON(XRCID("ImageAttach"), ProjectMainTab::OnAddExistingImage)
		EVT_BUTTON(XRCID("ImageRemove"), ProjectMainTab::OnRemoveImages)
		EVT_BUTTON(XRCID("CopyLaunchCommandLine"), ProjectMainTab::OnCopyStartCommandline)
		EVT_LIST_ITEM_ACTIVATED(XRCID("ImageList"), ProjectMainTab::OnShowImageDetails)
		EVT_TOOL(XRCID("openLog"), ProjectMainTab::OnShowLog)
		EVT_TOOL(XRCID("codeBuild"), ProjectMainTab::OnCodeBuild)
		EVT_TOOL(XRCID("codeRunDebug"), ProjectMainTab::OnCodeRunDebug)
		EVT_TOOL(XRCID("codeRunNormal"), ProjectMainTab::OnCodeRunNormal)
		EVT_TOOL(XRCID("codeRunTrace"), ProjectMainTab::OnCodeRunTrace)
		EVT_TOOL(XRCID("kill"), ProjectMainTab::OnKill)
		EVT_TOOL(XRCID("traceLoadFile"), ProjectMainTab::OnLoadTrace)
		EVT_TOOL(XRCID("traceImportFile"), ProjectMainTab::OnImportTrace)
		EVT_TIMER(wxID_ANY, ProjectMainTab::OnRefreshTimer)
	END_EVENT_TABLE()

	ProjectMainTab::ProjectMainTab(ProjectWindow* parent, wxWindow* tabs)
		: ProjectTab(parent, tabs, ProjectTabType::Main)
		, m_imageList(nullptr)
		, m_refreshTimer(this)
		, m_activeProjectInstance(0)
	{
		// load the ui
		wxXmlResource::Get()->LoadPanel(this, tabs, wxT("ProjectTab"));

		// prepare the control
		m_imageList = XRCCTRL(*this, "ImageList", wxListCtrl);
		m_imageList->DeleteAllColumns();
		m_imageList->AppendColumn(wxT("Name"), wxLIST_FORMAT_LEFT, 400);
		m_imageList->AppendColumn(wxT("Load address"), wxLIST_FORMAT_LEFT, 150);
		m_imageList->AppendColumn(wxT("End address"), wxLIST_FORMAT_LEFT, 150);
		m_imageList->AppendColumn(wxT("Entry address"), wxLIST_FORMAT_LEFT, 150);
		m_imageList->AppendColumn(wxT("Type"), wxLIST_FORMAT_LEFT, 100);
		m_imageList->AppendColumn(wxT("Status"), wxLIST_FORMAT_LEFT, 120);
		m_imageList->AppendColumn(wxT("Load path"), wxLIST_FORMAT_LEFT, 350);

		// refresh the UI from time to time
		m_refreshTimer.Start(100);

		// refresh the list of images
		RefreshImageList();
		RefreshUI();
	}

	void ProjectMainTab::OnAddImage(wxCommandEvent& evt)
	{
		// fill the image extensions
		std::vector< platform::FileFormat > executableFormats;
		GetProject()->GetPlatform()->EnumerateImageFormats(executableFormats);

		// build the file enumeration string
		wxString filterString;
		for (const auto& f : executableFormats)
		{
			if (!filterString.empty())
				filterString += "|";

			filterString += f.m_name;
			filterString += " (*.";
			filterString += f.m_extension;
			filterString += ")|*.";
			filterString += f.m_extension;
		}

		// ask user for the file name
		wxFileDialog loadDialog(this, "Import image into project", "", wxEmptyString, filterString, wxFD_OPEN);
		if (loadDialog.ShowModal() == wxID_CANCEL)
			return;

		// import image
		const auto imagePath = loadDialog.GetPath();
		ImportImage(imagePath.wc_str());
	}

	bool ProjectMainTab::ImportImage(const std::wstring& imageImportPath)
	{
		std::shared_ptr<ProjectImage> importedImage;

		// load the image
		{
			auto* projectOwner = GetProject().get();

			ProgressDialog progress(this, GetProjectWindow()->GetApp()->GetLogWindow(), false);
			progress.RunLongTask([&importedImage, imageImportPath, projectOwner](ILogOutput& log)
			{
				// load the image
				importedImage = ProjectImage::Create(log, projectOwner, imageImportPath);
				return 0;
			});
		}

		// image not imported
		if (!importedImage)
		{
			wxMessageBox(wxT("Failed to import image from selected file. Inspect log for details."), wxT("Import image"), wxICON_ERROR, this);
			return false;
		}

		// add to project
		GetProject()->AddImage(importedImage);
		RefreshImageList();

		// done
		return true;
	}

	void ProjectMainTab::OnAddExistingImage(wxCommandEvent& evt)
	{
		// ask user for the file name
		const char* filterString = "Recompiler project image (*.pdi)|*.pdi";
		wxFileDialog loadDialog(this, "Import existing image into project", "", wxEmptyString, filterString, wxFD_OPEN);
		if (loadDialog.ShowModal() == wxID_CANCEL)
			return;

		auto importPath = loadDialog.GetPath();
		ImportExistingImage(importPath.wc_str());
	}

	bool ProjectMainTab::ImportExistingImage(const std::wstring& imageImportPath)
	{
		// load the image
		std::shared_ptr<ProjectImage> projectImage;
		{
			auto* projectOwner = GetProject().get();

			ProgressDialog progress(this, GetProjectWindow()->GetApp()->GetLogWindow(), false);
			progress.RunLongTask([&projectImage, projectOwner, imageImportPath](ILogOutput& log)
			{
				// load the image
				projectImage = ProjectImage::Load(log, projectOwner, imageImportPath);
				return 0;
			});
		}

		// image not imported
		if (!projectImage)
		{
			wxMessageBox(wxT("Failed to import image from selected file. Inspect log for details."), wxT("Import image"), wxICON_ERROR, this);
			return false;
		}

		// check if image with that name already exists
		for (const auto& curImage : GetProject()->GetImages())
		{
			if (curImage->GetDisplayName() == projectImage->GetDisplayName())
			{
				wxMessageBox(wxT("Image existing in project with the same display name '") + curImage->GetDisplayName() + wxT("'. Duplicate?"), wxT("Import image"), wxICON_ERROR, this);
				return false;
			}
			if (curImage->GetLoadPath() == projectImage->GetLoadPath())
			{
				wxMessageBox(wxT("Image existing in project with the same relative path '") + curImage->GetLoadPath() + wxT("'. Duplicate?"), wxT("Import image"), wxICON_ERROR, this);
				return false;
			}
		}

		// add to project
		GetProject()->AddImage(projectImage);

		// refresh the list
		RefreshImageList();
		return true;
	}

	bool ProjectMainTab::RemoveImages(const std::vector<std::shared_ptr<ProjectImage>>& images)
	{
		for (const auto& img : images)
			GetProject()->RemoveImage(img);

		RefreshImageList();
		return true;
	}

	void ProjectMainTab::GetSelectedImages(std::vector<std::shared_ptr<ProjectImage>>& outImages)
	{
		const auto& images = GetProject()->GetImages();

		// not the most convenient interface :)
		long item = -1;
		for (;;)
		{
			item = m_imageList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			if (item == -1)
				break;

			const auto imgIndex = (int)m_imageList->GetItemData(item);
			outImages.push_back(images[imgIndex]);
		}
	}

	void ProjectMainTab::OnRemoveImages(wxCommandEvent& evt)
	{
		std::vector<std::shared_ptr<ProjectImage>> images;
		GetSelectedImages(images);

		if (images.empty())
		{
			wxMessageBox(wxT("Please select images to remove"), wxT("Remove images"), wxICON_ERROR, this);
		}
		else
		{
			if (wxNO == wxMessageBox(wxT("Sure to remove images from the project? They won't be deleted from disk, just detached"), wxT("Remove images"), wxICON_QUESTION | wxYES_NO, this))
				return;

			RemoveImages(images);
		}
	}

	void ProjectMainTab::RefreshImageList()
	{
		// clear existing list
		m_imageList->Freeze();
		m_imageList->DeleteAllItems();

		// add image informations
		const auto& images = GetProject()->GetImages();
		for (uint32 i = 0; i < images.size(); ++i)
		{
			const auto image = images[i];
			const auto binary = image->GetEnvironment().GetImage();

			const auto index = m_imageList->GetItemCount();
			m_imageList->InsertItem(index, image->GetDisplayName());

			const auto* type = "Unknown";
			switch (image->GetSettings().m_imageType)
			{
				case ProjectImageType::Application: type = "Application"; break;
				case ProjectImageType::DynamicLibrary: type = "DynamicLibrary"; break;
			}

			const auto* status = "Unknown";

			m_imageList->SetItem(index, 1, wxString::Format("0x%08llX", (uint64)binary->GetBaseAddress()));
			m_imageList->SetItem(index, 2, wxString::Format("0x%08llX", (uint64)binary->GetBaseAddress() + binary->GetMemorySize()));
			m_imageList->SetItem(index, 3, wxString::Format("0x%08llX", (uint64)binary->GetEntryAddress()));
			m_imageList->SetItem(index, 4, type);
			m_imageList->SetItem(index, 5, status);
			m_imageList->SetItem(index, 6, image->GetLoadPath());
			m_imageList->SetItemData(index, index);
		}

		// done
		m_imageList->Thaw();
	}

	void ProjectMainTab::OnShowImageDetails(wxListEvent& event)
	{
		const auto& images = GetProject()->GetImages();
		const auto& img = images[(int)event.GetItem().GetData()];
		GetProjectWindow()->GetImageTab(img, true, true);
	}

	void ProjectMainTab::OnShowLog(wxCommandEvent& evt)
	{
		wxTheApp->ShowLog();
	}

	void ProjectMainTab::OnCodeBuild(wxCommandEvent& evt)
	{
		// kill existing project
		if (m_activeProjectInstance != 0)
		{
			if (wxNO == wxMessageBox("There is another instance of the project running, it will be killed before continuing. Proceed?", "Build project", wxICON_QUESTION | wxYES_NO, this))
				return;

			if (!KillProject())
				return;
		}

		// check for images
		if (m_imageList->GetItemCount() == 0)
		{
			wxMessageBox(wxT("Nothing to build, there are no images in the project"), wxT("Build project"), wxICON_WARNING, this);
			return;
		}

		// collect dirty images
		std::vector<std::shared_ptr<ProjectImage>> dirtyImages;
		const auto& images = GetProject()->GetImages();
		for (const auto& img : images)
		{
			if (img->CanCompile() && img->RequiresRecompilation())
			{
				dirtyImages.push_back(img);
			}
		}

		// nothing to recompile
		if (dirtyImages.empty())
		{
			if (wxNO == wxMessageBox(wxT("All images are up to date, force recompilation?"), wxT("Build project"), wxICON_INFORMATION | wxYES_NO, this))
				return;
			dirtyImages = images; // recompile all images
		}

		// collect tasks
		std::vector<BuildTask> buildTasks;
		for (const auto& img : dirtyImages)
			img->GetCompilationTasks(buildTasks);

		// run build process
		BuildDialog dlg(this);
		dlg.RunCommands(buildTasks);
	}

	const bool ProjectMainTab::CheckDebugProjects()
	{
		// check if have a non-debug configurations
		wxString nonDebugProjects;
		const auto& images = GetProject()->GetImages();
		for (const auto& img : images)
		{
			if (!img->GetSettings().m_emulationEnabled)
				continue;

			if (img->GetSettings().m_recompilationMode != ProjectRecompilationMode::Debug)
			{
				nonDebugProjects += img->GetDisplayName();
				nonDebugProjects += wxTextFile::GetEOL();
			}
		}

		// in case we have a non-debug images selected for running we should warn user that debugging may NOT work as expected
		if (!nonDebugProjects.empty())
		{
			wxString message = "Following projects are compiled without debugging features:";
			message += wxTextFile::GetEOL();
			message += wxTextFile::GetEOL();
			message += nonDebugProjects;
			message += wxTextFile::GetEOL();
			message += "Continue ?";

			if (wxNO == wxMessageBox(message, wxT("Debug project"), wxICON_QUESTION | wxYES_NO, this))
				return false;
		}

		// we can run debug builds
		return true;
	}

	static wxString GetProjectLauncherExecutable(const platform::Definition& platform)
	{
		wxFileName fileName(wxStandardPaths::Get().GetExecutablePath());
		fileName.SetName(platform.GetDecompilationInterface()->GetLauncherName());
		return fileName.GetFullPath();
	}

	static wxString EscapePath(const wxString path)
	{
		if (wxFileName::GetPathSeparator() == '\\')
		{
			wxString temp(path);
			temp.Replace("\\", "\\\\");
			return temp;
		}
		else
		{
			return path;
		}
	}

	const bool ProjectMainTab::GetStartCommandline(wxString& outCommandLine)
	{
		// get the project launcher from the platform
		const auto launcherPath = GetProjectLauncherExecutable(*GetProject()->GetPlatform());
		if (!wxFileExists(launcherPath))
		{
			wxMessageBox(wxT("Unable to locate launcher for current platform"), wxT("Run project"), wxICON_ERROR, 0);
			GetProjectWindow()->GetApp()->GetLogWindow().Error("Project: Launcher '%ls' not found", launcherPath.wc_str());
			return false;
		}

		// format the command line
		wxString commandLine = launcherPath;
		commandLine += " ";

		// bind file system
		const auto projectDirectory = GetProject()->GetProjectDirectory();
		commandLine += "-fsroot=\"" + EscapePath(projectDirectory) += "\" ";

		// add the images to run
		bool hasAppImages = false;
		const auto& images = GetProject()->GetImages();
		for (const auto& img : images)
		{
			if (!img->CanRun())
			{
				GetProjectWindow()->GetApp()->GetLogWindow().Log("Project: Image '%hs' will not be loaded", img->GetDisplayName().c_str());
				continue;
			}

			commandLine += "-image=\"" + EscapePath(img->GetFullPathToCompiledBinary()) + "\" ";

			if (img->GetSettings().m_imageType == ProjectImageType::Application)
			{
				GetProjectWindow()->GetApp()->GetLogWindow().Log("Project: Image '%hs' will be loaded as application", img->GetDisplayName().c_str());
				hasAppImages = true;
			}
			else
			{
				GetProjectWindow()->GetApp()->GetLogWindow().Log("Project: Image '%hs' will be loaded as dynamic library", img->GetDisplayName().c_str());
			}
		}

		// no apps to run
		if (!hasAppImages)
		{
			wxMessageBox(wxT("There are no applications to run in current project"), wxT("Run project"), wxICON_ERROR, 0);
			GetProjectWindow()->GetApp()->GetLogWindow().Error("Project: No applications to run in current project");
			return false;
		}

		// valid command line
		outCommandLine = commandLine;
		return true;
	}

	const bool ProjectMainTab::StartProject(const wxString& extraCommandLine)
	{
		// kill existing project
		if (m_activeProjectInstance != 0)
		{
			if (wxNO == wxMessageBox("There is another instance of the project running, it will be killed before continuing. Proceed?", "Run project", wxICON_QUESTION | wxYES_NO, this))
				return false;

			if (!KillProject())
				return false;
		}

		// collect images that are not compiled and ask for compilation before running
		{
			std::vector<BuildTask> buildTasks;

			const auto& images = GetProject()->GetImages();
			for (const auto& img : images)
			{
				if (img->GetSettings().m_emulationEnabled)
				{
					const auto filePath = img->GetFullPathToCompiledBinary();
					if (!wxFileExists(filePath))
					{
						GetProjectWindow()->GetApp()->GetLogWindow().Log("Project: Compiled binary for '%hs' does not exist", img->GetDisplayName().c_str());
						img->GetCompilationTasks(buildTasks);
					}
				}
			}

			if (!buildTasks.empty())
			{
				if (wxNO == wxMessageBox(wxT("Some project are not compiled, run the build process first?"), wxT("Run project"), wxICON_QUESTION | wxYES_NO))
					return false;

				BuildDialog dlg(this);
				if (!dlg.RunCommands(buildTasks))
					return false;
			}
		}

		// get the commandline to execute
		wxString commandLine;
		if (!GetStartCommandline(commandLine))
			return false;

		// add extra options
		if (!extraCommandLine.empty())
		{
			commandLine += " ";
			commandLine += extraCommandLine;
		}

		// start the application executable
		const auto pid = wxExecute(commandLine, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER, nullptr, nullptr);
		if (0 == pid)
		{
			wxMessageBox(wxT("Failed to start project launcher"), wxT("Run project"), wxICON_ERROR, 0);
			GetProjectWindow()->GetApp()->GetLogWindow().Error("Project: Failed to start project launcher");
			return false;
		}

		// started
		GetProjectWindow()->GetApp()->GetLogWindow().Log("Project: Project started, local PID=%u", pid);
		m_activeProjectInstance = pid;
		RefreshUI();
		return true;
	}

	void ProjectMainTab::OnCodeRunNormal(wxCommandEvent& evt)
	{
		wxString extraCommandLine;
		StartProject(extraCommandLine);
	}

	void ProjectMainTab::OnCodeRunDebug(wxCommandEvent& evt)
	{
		// make sure we are running the debug builds
		if (!CheckDebugProjects())
			return;

		// start project, this may fail
		const wxString extraCommandLine = " -debug";
		if (StartProject(extraCommandLine))
		{
			// try to attach to debugger
			//wxTheApp->Attach			
		}
	}

	void ProjectMainTab::OnCodeRunTrace(wxCommandEvent& evt)
	{
		// make sure we are running the debug builds
		if (!CheckDebugProjects())
			return;

		// get the images to run
		std::vector<std::shared_ptr<ProjectImage>> startupImages;
		GetProject()->GetStartupImages(startupImages);

		// build the base name
		wxString sampleFileName;
		sampleFileName += "trace_";
		for (const auto& img : startupImages)
		{
			sampleFileName += img->GetDisplayName();
			sampleFileName += "_";
		}

		// create default file name for a trace file
		auto now = wxDateTime::Now();
		sampleFileName += wxString::Format("[%04u_%hs_%02u][%02u_%02u_%02u]",
			now.GetYear(), now.GetMonthName(now.GetMonth()).c_str().AsChar(), now.GetDay(),
			now.GetHour(), now.GetMinute(), now.GetSecond());
		sampleFileName += ".rawtrace";

		// ask for the actual file name
		wxFileDialog saveFileDialog(this, "Save trace file", "", sampleFileName, "Raw trace files (*.rawtrace)|*.rawtrace", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (saveFileDialog.ShowModal() == wxID_CANCEL)
			return;

		// get the full path to trace file
		const auto traceFullPath = saveFileDialog.GetPath();
		GetProjectWindow()->GetApp()->GetLogWindow().Log("Project: Trace will be saved to file '%ls'", traceFullPath.wc_str());
		m_activeTraceFile = traceFullPath;

		// run the project, we are not attaching anything in trace mode
		wxString extraCommandLine = " -trace=\"";
		extraCommandLine += EscapePath(traceFullPath);
		extraCommandLine += "\" ";
		StartProject(extraCommandLine);
	}


	void ProjectMainTab::OnCopyStartCommandline(wxCommandEvent& evt)
	{
		wxString commandLine;
		if (GetStartCommandline(commandLine))
		{
			wxClipboard *clip = new wxClipboard();
			if (clip->Open())
			{
				clip->Clear();
				clip->SetData(new wxTextDataObject(commandLine));
				clip->Flush();
				clip->Close();
			}
		}
	}

	void ProjectMainTab::OnLoadTrace(wxCommandEvent& evt)
	{
		// ask user for the file name
		wxFileDialog loadDialog(this, "Load compiled trace file", "", wxEmptyString, "Compiled trace files (*.trace)|*.trace", wxFD_OPEN);
		if (loadDialog.ShowModal() == wxID_CANCEL)
			return;

		// load the trace file
		const auto traceFilePath = loadDialog.GetPath();
		LoadTraceFile(traceFilePath);
	}

	void ProjectMainTab::OnImportTrace(wxCommandEvent& evt)
	{
		wxFileDialog loadDialog(this, "Import raw trace file", "", wxEmptyString, "Raw trace files (*.rawtrace)|*.rawtrace", wxFD_OPEN);
		if (loadDialog.ShowModal() == wxID_CANCEL)
			return;

		// import the trace
		const auto traceFilePath = loadDialog.GetPath();
		ImportTraceFile(traceFilePath);
	}

	void ProjectMainTab::OnRefreshTimer(wxTimerEvent & evt)
	{
		RefreshState();
	}

	void ProjectMainTab::RefreshState()
	{
		// project killed ?
		if (m_activeProjectInstance != 0)
		{
			// project was closed
			if (!wxProcess::Exists(m_activeProjectInstance))
			{
				GetProjectWindow()->GetApp()->GetLogWindow().Log("Project: Project instance (PID %u) was closed", m_activeProjectInstance);
				m_activeProjectInstance = 0;

				// if we were tracing ask to load the file
				if (!m_activeTraceFile.empty())
				{
					if (wxYES == wxMessageBox(wxT("Import created trace file?"), wxT("Trace project"), wxICON_QUESTION | wxYES_NO, this))
					{
						const auto path = m_activeTraceFile;
						m_activeTraceFile.clear();

						ImportTraceFile(path);
					}
				}
			}
		}

		// refresh the ui
		RefreshUI();
	}

	void ProjectMainTab::RefreshUI()
	{		
		auto* toolbar = XRCCTRL(*this, "ToolBar", wxToolBar);
		const bool canRun = (m_activeProjectInstance == 0);
		const bool canKill = (m_activeProjectInstance != 0);
		toolbar->EnableTool(XRCID("codeRunNormal"), canRun);
		toolbar->EnableTool(XRCID("codeRunDebug"), canRun);
		toolbar->EnableTool(XRCID("codeRunTrace"), canRun);
		toolbar->EnableTool(XRCID("kill"), canKill);
	}

	const bool ProjectMainTab::KillProject()
	{
		// no project is running
		if (m_activeProjectInstance == 0)
		{
			wxMessageBox(wxT("No project is running"), wxT("Kill"), wxICON_ERROR, this);
			return true;
		}

		// kill the process
		const auto kill = wxProcess::Kill(m_activeProjectInstance, wxSIGTERM, wxKILL_CHILDREN);
		if (wxKILL_OK != kill && wxKILL_NO_PROCESS != kill)
		{
			wxSleep(1);
			if (wxProcess::Exists(m_activeProjectInstance))
			{
				wxMessageBox(wxT("Failed to kill project"), wxT("Kill"), wxICON_ERROR, this);
				return false;
			}
		}

		// project killed
		GetProjectWindow()->GetApp()->GetLogWindow().Log("Project: Project killed");
		m_activeProjectInstance = 0;
		RefreshUI();

		return true;
	}

	void ProjectMainTab::OnKill(wxCommandEvent& evt)
	{
		KillProject();
	}

	const bool ProjectMainTab::ImportTraceFile(const wxString& traceFilePath)
	{
		// create the non-raw trace file
		wxFileName compiledFileName(traceFilePath);
		compiledFileName.SetExt("trace");

		// ask for the actual file name
		wxFileDialog saveFileDialog(this, "Save compiler trace file", "", compiledFileName.GetFullPath(), "Compiled trace files (*.trace)|*.trace", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (saveFileDialog.ShowModal() == wxID_CANCEL)
			return false;

		// load the source trace file
		const auto rawTraceData = trace::RawTraceReader::Load(GetProjectWindow()->GetApp()->GetLogWindow(), traceFilePath.wc_str());
		if (!rawTraceData)
		{
			wxMessageBox(wxT("Failed to load raw trace"), wxT("Import trace error"), wxICON_ERROR, this);
			return false;
		}

		// compile full trace
		std::unique_ptr<trace::DataFile> traceData;
		{
			auto* project = GetProject().get();
			const auto* cpuInfo = project->GetPlatform()->GetCPU(0);

			ProgressDialog dlg(this, GetProjectWindow()->GetApp()->GetLogWindow(), true);
			dlg.RunLongTask([&traceData, &rawTraceData, cpuInfo, project](ILogOutput& log)
			{
				auto decodingContextFunc = [project](const uint64_t ip)
				{
					return project->GetDecodingContext(ip);
				};

				traceData = trace::DataFile::Build(log, *cpuInfo, *rawTraceData, decodingContextFunc);
				return 0;
			});

			if (!traceData)
			{
				wxMessageBox(wxT("Failed to build trace file"), wxT("Import trace error"), wxICON_ERROR, this);
				return false;
			}
		}

		// save the trace file
		{
			const auto compiledFilePath = saveFileDialog.GetPath();

			ProgressDialog dlg(this, GetProjectWindow()->GetApp()->GetLogWindow(), true);
			const auto ret = dlg.RunLongTask([&traceData, compiledFilePath](ILogOutput& log)
			{
				if (!traceData->Save(log, compiledFilePath.wc_str()))
					return 1;
				return 0;
			});

			if (ret)
			{
				wxMessageBox(wxT("Failed to save compiled trace file. Check for disk space."), wxT("Import trace error"), wxICON_WARNING, this);
			}
		}

		// open the trace tab
		return OpenTraceTab(traceData);
	}

	const bool ProjectMainTab::LoadTraceFile(const wxString& traceFilePath)
	{
		// compile full trace
		std::unique_ptr<trace::DataFile> traceData;
		{
			const auto* cpuInfo = GetProject()->GetPlatform()->GetCPU(0);

			ProgressDialog dlg(this, GetProjectWindow()->GetApp()->GetLogWindow(), true);
			dlg.RunLongTask([&traceData, cpuInfo, traceFilePath](ILogOutput& log)
			{
				traceData = trace::DataFile::Load(log, *cpuInfo, traceFilePath.wc_str());
				return 0;
			});

			if (!traceData)
			{
				wxMessageBox(wxT("Failed to load trace file"), wxT("Load trace error"), wxICON_ERROR, this);
				return false;
			}
		}

		// open the tab
		return OpenTraceTab(traceData);
	}

	const bool ProjectMainTab::OpenTraceTab(std::unique_ptr<trace::DataFile>& traceData)
	{
		const auto* tab = GetProjectWindow()->GetTraceTab(traceData, true);
		return (tab != nullptr);
	}

} // tools

