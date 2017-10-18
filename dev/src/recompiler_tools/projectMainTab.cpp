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

namespace tools
{

	BEGIN_EVENT_TABLE(ProjectMainTab, ProjectTab)
		EVT_BUTTON(XRCID("ImageAdd"), ProjectMainTab::OnAddImage)
		EVT_BUTTON(XRCID("ImageRemove"), ProjectMainTab::OnRemoveImages)
		EVT_LIST_ITEM_ACTIVATED(XRCID("ImageList"), ProjectMainTab::OnShowImageDetails)
		EVT_TOOL(XRCID("openLog"), ProjectMainTab::OnShowLog)
		EVT_TOOL(XRCID("codeBuild"), ProjectMainTab::OnCodeBuild)
		EVT_TOOL(XRCID("codeRunDebug"), ProjectMainTab::OnCodeRunDebug)
		EVT_TOOL(XRCID("codeRunNormal"), ProjectMainTab::OnCodeRunNormal)
		EVT_TOOL(XRCID("codeRunTrace"), ProjectMainTab::OnCodeRunTrace)
		EVT_TOOL(XRCID("kill"), ProjectMainTab::OnKill)
		EVT_TOOL(XRCID("traceLoadFile"), ProjectMainTab::OnLoadTrace)
		EVT_TIMER(wxID_ANY, ProjectMainTab::OnRefreshTimer)
	END_EVENT_TABLE()

	ProjectMainTab::ProjectMainTab(ProjectWindow* parent, wxWindow* tabs)
		: ProjectTab(parent, tabs, ProjectTabType::Main)
		, m_imageList(nullptr)
		, m_imageOpenDialog("ImageImport")
		, m_refreshTimer(this)
		, m_activeProjectInstance(0)
	{
		// load the ui
		wxXmlResource::Get()->LoadPanel(this, tabs, wxT("ProjectTab"));

		// fill the image extensions
		{
			std::vector< platform::FileFormat > executableFormats;
			GetProjectWindow()->GetProject()->GetPlatform()->EnumerateImageFormats(executableFormats);

			// setup file formats for images
			for (const auto& f : executableFormats)
				m_imageOpenDialog.AddFormat(FileFormat(f.m_name, f.m_extension));
			m_imageOpenDialog.SetMultiselection(false);
		}

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
		// select image
		if (!m_imageOpenDialog.DoOpen(this))
			return;

		// import image
		const auto imagePath = m_imageOpenDialog.GetFile();
		GetProjectWindow()->ImportImage(imagePath.wc_str());
	}

	void ProjectMainTab::OnRemoveImages(wxCommandEvent& evt)
	{
	}

	void ProjectMainTab::RefreshImageList()
	{
		// clear existing list
		m_imageList->Freeze();
		m_imageList->DeleteAllItems();

		// add image informations
		const auto& images = GetProjectWindow()->GetProject()->GetImages();
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
		}

		// done
		m_imageList->Thaw();
	}

	void ProjectMainTab::OnShowImageDetails(wxListEvent& event)
	{
		const auto& images = GetProjectWindow()->GetProject()->GetImages();
		const auto& img = images[event.GetItem().GetId()];
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
		const auto& images = GetProjectWindow()->GetProject()->GetImages();
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
		const auto& images = GetProjectWindow()->GetProject()->GetImages();
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

			const auto& images = GetProjectWindow()->GetProject()->GetImages();
			for (const auto& img : images)
			{
				if (img->GetSettings().m_emulationEnabled)
				{
					const auto filePath = img->GetFullPathToCompiledBinary();
					if (!wxFileExists(filePath))
					{
						wxTheApp->GetLogWindow().Log("Project: Compiled binary for '%hs' does not exist", img->GetDisplayName().c_str());
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

		// get the project launcher from the platform
		const auto launcherPath = GetProjectLauncherExecutable(*GetProjectWindow()->GetProject()->GetPlatform());
		if (!wxFileExists(launcherPath))
		{
			wxMessageBox(wxT("Unable to locate launcher for current platform"), wxT("Run project"), wxICON_ERROR, 0);
			wxTheApp->GetLogWindow().Error("Project: Launcher '%ls' not found", launcherPath.wc_str());
			return false;
		}

		// format the command line
		wxString commandLine = launcherPath;
		commandLine += extraCommandLine;
		commandLine += " ";

		// bind file system
		const auto projectDirectory = GetProjectWindow()->GetProject()->GetProjectDirectory();
		commandLine += "-fsroot=\"" + EscapePath(projectDirectory) += "\" ";

		// add the images to run
		bool hasAppImages = false;
		const auto& images = GetProjectWindow()->GetProject()->GetImages();
		for (const auto& img : images)
		{
			if (!img->CanRun())
			{
				wxTheApp->GetLogWindow().Log("Project: Image '%hs' will not be loaded", img->GetDisplayName().c_str());
				continue;
			}

			commandLine += "-image=\"" + EscapePath(img->GetFullPathToCompiledBinary()) + "\" ";

			if (img->GetSettings().m_imageType == ProjectImageType::Application)
			{ 
				wxTheApp->GetLogWindow().Log("Project: Image '%hs' will be loaded as application", img->GetDisplayName().c_str());
				hasAppImages = true;
			}
			else
			{
				wxTheApp->GetLogWindow().Log("Project: Image '%hs' will be loaded as dynamic library", img->GetDisplayName().c_str());
			}
		}

		// no apps to run
		if (!hasAppImages)
		{
			wxMessageBox(wxT("There are no applications to run in current project"), wxT("Run project"), wxICON_ERROR, 0);
			wxTheApp->GetLogWindow().Error("Project: No applications to run in current project");
			return false;
		}

		// start the application executable
		const auto pid = wxExecute(commandLine, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER, nullptr, nullptr);
		if (0 == pid)
		{
			wxMessageBox(wxT("Failed to start project launcher"), wxT("Run project"), wxICON_ERROR, 0);
			wxTheApp->GetLogWindow().Error("Project: Failed to start project launcher");
			return false;
		}

		// started
		wxTheApp->GetLogWindow().Log("Project: Project started, local PID=%u", pid);
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

		// run the project, we are not attaching anything in trace mode
		wxString extraCommandLine = " -trace";
		StartProject(extraCommandLine);
	}

	void ProjectMainTab::OnLoadTrace(wxCommandEvent& evt)
	{
		// TODO
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
			if (!wxProcess::Exists(m_activeProjectInstance))
			{
				GetProjectWindow()->GetApp()->GetLogWindow().Log("Project: Project instance (PID %u) was closed", m_activeProjectInstance);
				m_activeProjectInstance = 0;
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

} // tools

