#pragma once

#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <set>
#include <deque>
#include <map>
#include <string>
#include <algorithm>
#include <functional>
#include <mutex>
#include <atomic>

#include "..\recompiler_core\build.h"

// Widgets
#define WXUSINGDLL
#define wxUSE_EXCEPTIONS 0
#define wxUSE_RICHTEXT 1
#define wxNO_PNG_LIB
#include "wx/wxprec.h"
#include "wx/xrc/xmlres.h"
#include "wx/splitter.h"
#include "wx/aui/aui.h"
#include "wx/aui/auibar.h"
#include "wx/dcbuffer.h"
#include "wx/evtloop.h"
#include "wx/statusbr.h"
#include "wx/menu.h"
#include "wx/gdicmn.h"
#include "wx/treectrl.h"
#include "wx/toolbar.h"
#include "wx/spinctrl.h"
#include "wx/srchctrl.h"
#include "wx/combobox.h"
#include "wx/treectrl.h"
#include "wx/panel.h"
#include "wx/statline.h"
#include "wx/sizer.h"
#include "wx/splitter.h"
#include "wx/frame.h"
#include "wx/aui/aui.h"
#include "wx/artprov.h"
#include "wx/laywin.h"
#include "wx/bmpcbox.h"
#include "wx/xrc/xmlres.h"
#include "wx/image.h"
#include "wx/collpane.h"
#include "wx/dcbuffer.h"
#include "wx/tglbtn.h"
#include "wx/clipbrd.h"
#include "wx/fileconf.h"
#include "wx/datectrl.h"
#include "wx/listbase.h"
#include "wx/listctrl.h"
#include "wx/grid.h"
#include "wx/colordlg.h"
#include "wx/filepicker.h"
#include "wx/numdlg.h"
#include "wx/html/htmlwin.h"
#include "wx/clrpicker.h"
#include "wx/ribbon/toolbar.h"
#include "wx/valnum.h"
#include "wx/treelist.h"
#include "wx/dataview.h"
#include "wx/renderer.h"
#include "wx/richtext/richtextctrl.h"
#include "wx/filename.h"
#include "wx/stdpaths.h"
#include "wx/wfstream.h"
#include "wx/filedlg.h"
#include "wx/process.h"
#include "wx/datetime.h"

// Windows
#include <windows.h>
#include <commctrl.h>
#include <dbghelp.h>

// Widgets require these libs to link with 
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "rpcrt4.lib")
#pragma comment (lib, "gdiplus.lib")
#pragma comment (lib, "dbghelp.lib")

// common shit
class Project;

// Warnings
#pragma warning ( disable: 4275 )

template< typename T >
const T& TemplateMin( const T& a, const T& b )
{
	if ( a < b ) return a;
	return b;
}

template< typename T >
const T& TemplateMax( const T& a, const T& b )
{
	if ( a > b ) return a;
	return b;
}

template< typename T >
const T& TemplateClamp( const T& v, const T& a, const T& b )
{
	if ( v <= a ) return a;
	if ( v >= b ) return b;
	return v;
}

// Min max for GDI+
#ifndef min 
#define min TemplateMin
#endif

#ifndef max
#define max TemplateMax
#endif

// GDI+
#include <GdiPlus.h>

// Undefine
#undef min
#undef max

// Backend
#include "../recompiler_core/build.h"

// Crap
#include "config.h"
#include "app.h"
#include "bitmaps.h"
#include "logWindow.h"

namespace tools
{
	class Project;
	class ProjectImage;

	enum class ValueViewMode
	{
		Auto,
		Hex,
	};

	static const uint64 INVALID_ADDRESS = ~(uint64)0;
}