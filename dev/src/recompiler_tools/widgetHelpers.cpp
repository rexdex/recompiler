#include "build.h"
#include "widgetHelpers.h"

namespace tools
{

	//---------------------------------------------------------------------------

	ListBoxUpdate::ListBoxUpdate(wxWindow* parent, const char* name, bool allowEmpty /*=false*/, bool issueEvent /*= true*/)
		: m_allowEmpty(allowEmpty)
		, m_issueEvent(issueEvent)
		, m_selectedIndex(-1)
		, m_list(nullptr)
	{
		// get the list box
		wxWindow* window = parent->FindWindow(XRCID(name));
		m_list = wxDynamicCast(window, wxListBox);

		// begin update
		if (m_list != nullptr)
		{
			// prevent selection events from being send
			m_list->Connect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(ListBoxUpdate::OnSelectionChanged), NULL, this);

			// clear the list
			m_list->Freeze();
			m_list->Clear();
		}
	}

	ListBoxUpdate::~ListBoxUpdate()
	{
		// finalize
		if (m_list != nullptr)
		{
			// elements
			if (m_list->GetCount() == 0 && !m_allowEmpty)
			{
				m_list->Append("(Empty)");
				m_list->SetSelection(0);
				m_list->Enable(false);
			}
			else
			{
				m_list->Enable(true);
			}

			// select final item
			if (m_selectedIndex != -1)
			{
				m_list->SetSelection(m_selectedIndex);
			}

			// disconnect the fake event handler
			m_list->Disconnect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(ListBoxUpdate::OnSelectionChanged), NULL, this);

			// thaw
			m_list->Thaw();
			m_list->Refresh();

			// send fake change event
			if (m_issueEvent && m_selectedIndex != -1)
			{
				wxCommandEvent fakeEvent(wxEVT_COMMAND_LISTBOX_SELECTED, m_list->GetId());
				m_list->ProcessCommand(fakeEvent);
			}
		}
	}

	void ListBoxUpdate::AddItem(bool isSelected, const char* txt, ...)
	{
		if (m_list != nullptr)
		{
			char buffer[512];
			va_list args;

			// format the text to insert in the list box
			va_start(args, txt);
			vsprintf_s(buffer, sizeof(buffer), txt, args);
			va_end(args);

			// insert in the list box
			const int index = m_list->Append(buffer);
			if (isSelected)
			{
				m_selectedIndex = index;
			}
		}
	}

	void ListBoxUpdate::AddItemWithData(wxClientData* data, bool isSelected, const char* txt, ...)
	{
		if (m_list != nullptr)
		{
			char buffer[512];
			va_list args;

			// format the text to insert in the list box
			va_start(args, txt);
			vsprintf_s(buffer, sizeof(buffer), txt, args);
			va_end(args);

			// insert in the list box
			const int index = m_list->Append(buffer, data);
			if (isSelected)
			{
				m_selectedIndex = index;
			}
		}
		else
		{
			delete data;
		}
	}

	void ListBoxUpdate::OnSelectionChanged(wxCommandEvent& event)
	{
		// do nothing, just consume the event
	}

	//---------------------------------------------------------------------------

	TextBoxUpdate::TextBoxUpdate(wxWindow* parent, const char* name)
		: m_changed(false)
		, m_textBox(nullptr)
	{
		// get the list box
		wxWindow* window = parent->FindWindow(XRCID(name));
		m_textBox = wxDynamicCast(window, wxTextCtrl);

		// begin update
		if (m_textBox != nullptr)
		{
			// prevent selection events from being send
			m_textBox->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(TextBoxUpdate::OnTextChanged), NULL, this);

			// clear the list
			m_textBox->Freeze();
		}
	}

	TextBoxUpdate::~TextBoxUpdate()
	{
		// finalize
		if (m_textBox != nullptr)
		{
			// disconnect the fake event handler
			m_textBox->Disconnect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(TextBoxUpdate::OnTextChanged), NULL, this);

			// thaw
			m_textBox->Thaw();

			// redraw only when needed
			if (m_changed)
			{
				m_textBox->Refresh();
			}
		}
	}

	TextBoxUpdate& TextBoxUpdate::SetText(const char* txt, ...)
	{
		if (m_textBox != nullptr)
		{
			char buffer[512];
			va_list args;

			// format the text to insert in the list box
			va_start(args, txt);
			vsprintf_s(buffer, sizeof(buffer), txt, args);
			va_end(args);

			// set the text, only if changed
			const wxString currentText = m_textBox->GetValue();
			if (currentText != buffer)
			{
				// set new text value
				m_changed = true;
				m_textBox->SetValue(buffer);
			}
		}

		return *this;
	}

	TextBoxUpdate& TextBoxUpdate::SelectText()
	{
		if (m_textBox != nullptr)
		{
			m_textBox->SetSelection(-1, -1);
			m_textBox->SetFocus();
		}

		return *this;
	}

	void TextBoxUpdate::OnTextChanged(wxCommandEvent& event)
	{
		// do nothing, just eat the event
	}

	//---------------------------------------------------------------------------

	TextBoxReader::TextBoxReader(wxWindow* parent, const char* name)
	{
		// get the list box
		wxWindow* window = parent->FindWindow(XRCID(name));
		m_textBox = wxDynamicCast(window, wxTextCtrl);
	}

	TextBoxReader::~TextBoxReader()
	{
	}

	const wxString TextBoxReader::GetString(const char* defaultText /*= ""*/) const
	{
		if (m_textBox != nullptr)
		{
			return m_textBox->GetValue();
		}

		return defaultText ? wxString(defaultText) : wxEmptyString;
	}

	const bool TextBoxReader::GetInt(wxLongLong_t& outValue, const bool hex /*= false*/) const
	{
		if (m_textBox != nullptr)
		{
			// get text
			const wxString valueString = m_textBox->GetValue();
			const char* valueText = valueString.c_str().AsChar();

			// extract sign
			int sign = 0; // +
			const char* cur = valueText;
			if (*cur == '+')
			{
				sign = 0; // +
				cur += 1;
			}
			else if (*cur == '-')
			{
				sign = 1; // -
				cur += 1;
			}

			// parse the text, exact parsing to check for errors
			char digits[32];
			int numDigits = 0;
			bool error = false;
			while (*cur != 0 && numDigits < sizeof(digits))
			{
				// is valid digit ?
				if (*cur >= '0' && *cur <= '9')
				{
					digits[numDigits++] = *cur - '0';
					cur += 1;
					continue;
				}

				// hex
				if (hex)
				{
					if (*cur == 'A' || *cur == 'a')
					{
						digits[numDigits++] = 10;
						cur += 1;
						continue;
					}
					else if (*cur == 'B' || *cur == 'b')
					{
						digits[numDigits++] = 11;
						cur += 1;
						continue;
					}
					else if (*cur == 'C' || *cur == 'c')
					{
						digits[numDigits++] = 12;
						cur += 1;
						continue;
					}
					else if (*cur == 'D' || *cur == 'd')
					{
						digits[numDigits++] = 13;
						cur += 1;
						continue;
					}
					else if (*cur == 'E' || *cur == 'e')
					{
						digits[numDigits++] = 14;
						cur += 1;
						continue;
					}
					else if (*cur == 'F' || *cur == 'f')
					{
						digits[numDigits++] = 15;
						cur += 1;
						continue;
					}
				}

				// invalid digit
				error = true;
				break;
			}

			// valid number parsed, process
			if (!error)
			{
				wxLongLong_t finalValue = 0;

				int mul = 1;
				int base = hex ? 16 : 10;
				for (int i = 0; i < numDigits; ++i)
				{
					finalValue *= base;
					finalValue += digits[i];
				}

				// sign 
				if (sign)
				{
					finalValue = -finalValue;
				}

				// return the value
				outValue = finalValue;
				return true;
			}
		}

		// errors
		return false;
	}

	//---------------------------------------------------------------------------

	TextBoxErrorHint::TextBoxErrorHint(wxWindow* parent, const char* name)
		: m_textBox(nullptr)
		, m_normalColor(255, 255, 255)
		, m_errorColor(255, 200, 200)
	{
		// get the list box
		wxWindow* window = parent->FindWindow(XRCID(name));
		m_textBox = wxDynamicCast(window, wxTextCtrl);

		// setup
		if (m_textBox != nullptr)
		{
			// get default background color
			m_normalColor = m_textBox->GetBackgroundColour();

			// connect the event
			m_textBox->Connect(wxEVT_TEXT, wxCommandEventHandler(TextBoxErrorHint::OnTextChanged), NULL, this);
		}
	}

	TextBoxErrorHint::~TextBoxErrorHint()
	{
		// disconnect
		if (m_textBox != nullptr)
		{
			m_textBox->Disconnect(wxEVT_TEXT, wxCommandEventHandler(TextBoxErrorHint::OnTextChanged), NULL, this);
		}
	}

	void TextBoxErrorHint::SetErrorHint(const bool hasError /*= true*/)
	{
		if (m_hasError != hasError)
		{
			m_hasError = hasError;

			// update background color
			if (m_textBox != nullptr)
			{
				m_textBox->SetBackgroundColour(m_hasError ? m_errorColor : m_normalColor);
				m_textBox->Refresh();
			}
		}
	}

	void TextBoxErrorHint::ResetErrorHint()
	{
		SetErrorHint(false);
	}

	void TextBoxErrorHint::OnTextChanged(wxCommandEvent& event)
	{
		ResetErrorHint();
		event.Skip();
	}

	//---------------------------------------------------------------------------

	ToggleButtonAuto::ToggleButtonAuto(wxWindow* parent, const char* name, const char* customOnText /*= nullptr*/, const char* customOffText /*= nullptr*/, const bool defaultValue /*= false*/)
		: m_toggleButton(nullptr)
		, m_value(defaultValue)
	{
		// get the button
		wxWindow* window = parent->FindWindow(XRCID(name));
		m_toggleButton = wxDynamicCast(window, wxToggleButton);

		// inital update
		if (m_toggleButton != nullptr)
		{
			// get the on/off strings
			m_offString = customOffText ? customOffText : m_toggleButton->GetLabel();
			m_onString = customOnText ? customOnText : m_toggleButton->GetLabel();

			// set the button state
			if (m_toggleButton->GetValue() != m_value)
			{
				m_toggleButton->SetValue(m_value);
				m_toggleButton->SetLabel(m_value ? m_onString : m_offString);
			}

			// connect the event to track the button value
			m_toggleButton->Connect(wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(ToggleButtonAuto::OnButtonToggled), NULL, this);
		}
	}

	ToggleButtonAuto::~ToggleButtonAuto()
	{
		if (m_toggleButton != nullptr)
		{
			// remove the event helper
			m_toggleButton->Disconnect(wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(ToggleButtonAuto::OnButtonToggled), NULL, this);
		}
	}

	const bool ToggleButtonAuto::GetValue() const
	{
		return m_toggleButton ? m_toggleButton->GetValue() : m_value;
	}

	void ToggleButtonAuto::SetValue(const bool isToggled)
	{
		if (m_value != isToggled)
		{
			m_value = isToggled;

			if (m_toggleButton != nullptr)
			{
				m_toggleButton->SetValue(m_value);
				m_toggleButton->SetLabel(m_value ? m_onString : m_offString);
			}
		}
	}

	void ToggleButtonAuto::OnButtonToggled(wxCommandEvent& event)
	{
		if (m_toggleButton != nullptr)
		{
			m_value = m_toggleButton->GetValue();
			m_toggleButton->SetLabel(m_value ? m_onString : m_offString);
		}

		// allow the event to propagate
		event.Skip();
	}

	//---------------------------------------------------------------------------

	bool ParseHexValue64(const char* start, uint64& outValue)
	{
		// decode number
		const char* txt = start + strlen(start) - 1;
		uint64 ret = 0;
		uint64 b = 1;
		while (txt >= start)
		{
			const char ch = *txt--;

			// get char value
			uint64 c = 0;
			if (ch == 'A' || ch == 'a') c = 10;
			else if (ch == 'B' || ch == 'b') c = 11;
			else if (ch == 'C' || ch == 'c') c = 12;
			else if (ch == 'D' || ch == 'd') c = 13;
			else if (ch == 'E' || ch == 'e') c = 14;
			else if (ch == 'F' || ch == 'f') c = 15;
			else if (ch >= '0' && ch <= '9') c = (ch - '0');
			else
			{
				return false;
			}

			// accumulate
			ret += b * c;
			b <<= 4;
		}

		// done
		outValue = ret;
		return true;
	}

	bool ParseAddress(const wxString& str, uint64& outValue)
	{
		auto text = str;
		text.Trim();

		if (text.EndsWith("h"))
			text = text.Mid(0, text.Length() - 1);
		else if (text.StartsWith("0x"))
			text = text.Mid(2);

		return ParseHexValue64(text.c_str().AsChar(), outValue);
	}

	//---------------------------------------------------------------------------

} // tools