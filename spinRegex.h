#pragma once
#include "wx/wx.h"
#include <wx/string.h>
#include <wx/spinctrl.h>
#include "ceval.h"
#include "Vector.h"

class SpinRegex : public wxSpinCtrlDouble {
public:
	SpinRegex(wxWindow *parent, wxWindowID id = -1, const wxString &value = wxEmptyString, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, long style = wxSP_ARROW_KEYS, double min = 0, double max = 100, double initial = 0, double inc = 1, const wxString &name = wxT("wxSpinCtrlDouble")):wxSpinCtrlDouble(parent, id, value, pos, size, wxSP_ARROW_KEYS, min, max, initial, inc, name) {
		tval = 0.5;
		dim = 1;
		dim_id = 0;
		m_textCtrl->Bind(wxEVT_LEFT_UP, wxMouseEventHandler(SpinRegex::MouseUpEvents), this);
		m_textCtrl->Bind(wxEVT_RIGHT_UP, wxMouseEventHandler(SpinRegex::MouseUpEvents), this);
		//m_textCtrl->Bind(wxEVT_LEFT_DOWN, wxMouseEventHandler(SpinRegex::MouseUpEvents), this);
	}
	virtual ~SpinRegex() {}

	virtual void MouseUpEvents(wxMouseEvent &evt) {

		wxMouseEvent event(evt.LeftUp()?wxEVT_LEFT_UP: wxEVT_RIGHT_UP);
		event.SetEventObject(this);
		event.SetPosition(evt.GetPosition());		
		GetEventHandler()->ProcessEvent(event);
		ReleaseCapture();
	}
	
	virtual bool DoTextToValue(const wxString& text, double *val) wxOVERRIDE {	
		double lval;
		if (!text.ToDouble(&lval)) {
			std::string newtext = replace_variable(std::string(text), tval);
			*val = ceval_result2(newtext);

			return true;
		}

		*val = static_cast<double>(lval);


			return true;		
	}
	virtual wxString DoValueToText(double val) wxOVERRIDE {
		double lval;
		wxString txt = m_textCtrl->GetValue();
		if (!txt.ToDouble(&lval)) {
			return txt;
		}
		return wxString::Format("%lf", static_cast<double>(val));
	}


	float tval;
	int dim, dim_id;

protected:

};

