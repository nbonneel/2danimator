#pragma once

#include "wx/wx.h"

#include <wx/filepicker.h>

class ClickableFilePicker : public wxFilePickerCtrl {
public:
	ClickableFilePicker(wxWindow *parent,
		wxWindowID id,
		const wxString& path = wxEmptyString,
		const wxString& message = wxFileSelectorPromptStr,
		const wxString& wildcard = wxFileSelectorDefaultWildcardStr,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxFLP_DEFAULT_STYLE,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxFilePickerCtrlNameStr) : wxFilePickerCtrl(parent, id, path, message, wildcard, pos, size, style, validator, name) {

		m_text->Bind(wxEVT_RIGHT_UP, wxMouseEventHandler(ClickableFilePicker::MouseUpEvents), this);
				
	};
	virtual ~ClickableFilePicker(){}

	virtual void MouseUpEvents(wxMouseEvent &evt) {
		wxMouseEvent event(evt.LeftUp() ? wxEVT_LEFT_UP : wxEVT_RIGHT_UP);
		event.SetEventObject(this);
		event.SetPosition(evt.GetPosition());
		GetEventHandler()->ProcessEvent(event);
		ReleaseCapture();
	}
};