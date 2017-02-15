#pragma once

//---------------------------------------------------------------------------

struct CGlobalConfig : public CAutoConfig
{
public:
	bool		m_undecorateSymbols;
	bool		m_fullModulePath;

public:
	CGlobalConfig();
};

//---------------------------------------------------------------------------

class ISyncUpdate
{
public:
	ISyncUpdate();
	virtual ~ISyncUpdate();

	//! Receive sync update, called during OnIdle
	virtual void OnSyncUpdate() = 0;
};

//---------------------------------------------------------------------------

class CToolsApp : public wxApp
{
private:
	// application data path
	wxString				m_editorPath;
	wxString				m_binaryPath;

	// configuration manager
	CConfigurationManager	m_config;

	// global application config
	CGlobalConfig			m_globalConfig;

	// global event dispatcher
	class CEventDispatcher*	m_events;

	// sync update lists
	typedef std::vector< ISyncUpdate* >		TSyncUpdates;
	TSyncUpdates			m_syncUpdate;

	// GDI+ token
	ULONG_PTR				m_gdiplusToken;

public:
	CToolsApp();
	~CToolsApp();

	void RegisterSyncUpdate( ISyncUpdate* syncUpdate );
	void UnregisterSyncUpdate( ISyncUpdate* syncUpdate );

	inline CConfigurationManager& GetConfig() { return m_config; }

	inline const CGlobalConfig& GetGlobalConfig() const { return m_globalConfig; }
	inline CGlobalConfig& GetGlobalConfig() { return m_globalConfig; }

	inline class CEventDispatcher* GetEvents() const { return m_events; }

	inline const wxString& GetEditorPath() const { return m_editorPath; }

	virtual bool OnInit();
	virtual int OnExit();
    //virtual bool OnInitGui();
	virtual void OnIdle(wxIdleEvent& event);

	void NavigateToAddress( const uint32 address );
};

#ifdef wxTheApp
	#undef wxTheApp
#endif

#define wxTheApp wx_static_cast( CToolsApp*, wxApp::GetInstance() )

#define wxTheEvents (wxTheApp->GetEvents())