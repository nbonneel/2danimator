#pragma once

#include "wx/wx.h"

#include <wx/clrpicker.h>
#include <wx/colordlg.h>

class ClickableColourPicker : public wxColourPickerCtrl {
public:
	ClickableColourPicker(wxWindow *parent, wxWindowID id,
		const wxColour& col = *wxBLACK, const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = wxCLRP_DEFAULT_STYLE,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxColourPickerCtrlNameStr) : wxColourPickerCtrl(parent, id, col, pos, size, style, validator, name) {

		// https://learnui.design/tools/data-color-picker.html#divergent
		m_text->Bind(wxEVT_RIGHT_UP, wxMouseEventHandler(ClickableColourPicker::MouseUpEvents), this);
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(0, wxColour("#003f5c"));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(1, wxColour("#2f4b7c"));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(2, wxColour("#665191"));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(3, wxColour("#a05195"));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(4, wxColour("#d45087"));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(5, wxColour("#f95d6a"));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(6, wxColour("#ff7c43"));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(7, wxColour("#ffa600"));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(8, wxColour(0,135,108));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(9, wxColour(87,161,139));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(10, wxColour(140,188,172));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(11, wxColour(190,214,206));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(12, wxColour(241,198,198));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(13, wxColour(236,156,157));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(14, wxColour(226,112,118));
		((wxColourPickerWidget *)m_picker)->GetColourData()->SetCustomColour(15, wxColour(212,61,81));					
	};
	virtual ~ClickableColourPicker(){}

	virtual void MouseUpEvents(wxMouseEvent &evt) {
		wxMouseEvent event(evt.LeftUp() ? wxEVT_LEFT_UP : wxEVT_RIGHT_UP);
		event.SetEventObject(this);
		event.SetPosition(evt.GetPosition());
		GetEventHandler()->ProcessEvent(event);
		ReleaseCapture();
	}
};