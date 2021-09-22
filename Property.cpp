#include "Property.h"
#include "animator.h"
#include "wx/event.h"

void PositionProperty::CreateWidgets(float time) {
	wxBoxSizer * property_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* spacing_text = new wxStaticText(myApp->propertiesPanel, controlID, "Position");
	property_sizer->Add(spacing_text, 0, wxEXPAND);
	controlID++;


	Vec2s s = position.getDisplayValue();
	double v1 = ceval_result2(replace_variable(s[0], time));
	double v2 = ceval_result2(replace_variable(s[1], time));
	propFloat1 = new SpinRegex(myApp->propertiesPanel, controlID, s[0], wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, v1, 0.05);
	propFloat2 = new SpinRegex(myApp->propertiesPanel, controlID + 1, s[1], wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, v2, 0.05);
	propFloat1->dim_id = 0;
	propFloat1->dim = 2;
	propFloat2->dim_id = 1;
	propFloat2->dim = 2;
	propFloat1->Bind(wxEVT_SPINCTRLDOUBLE, &AnimatorPanel::updatePropertiesFromControls, myApp->animatorPanel);
	propFloat1->Bind(wxEVT_LEFT_UP, &AnimatorPanel::spinMouseUp, myApp->animatorPanel);
	propFloat1->Bind(wxEVT_RIGHT_UP, &AnimatorPanel::spinMouseUp, myApp->animatorPanel);

	propFloat2->Bind(wxEVT_SPINCTRLDOUBLE, &AnimatorPanel::updatePropertiesFromControls, myApp->animatorPanel);
	propFloat2->Bind(wxEVT_LEFT_UP, &AnimatorPanel::spinMouseUp, myApp->animatorPanel);
	propFloat2->Bind(wxEVT_RIGHT_UP, &AnimatorPanel::spinMouseUp, myApp->animatorPanel);

	propFloat1->Bind(wxEVT_MOUSE_CAPTURE_LOST, &AnimatorPanel::OnSpinMouseCaptureLost, myApp->animatorPanel);
	propFloat2->Bind(wxEVT_MOUSE_CAPTURE_LOST, &AnimatorPanel::OnSpinMouseCaptureLost, myApp->animatorPanel);

	myApp->animatorPanel->widgetToProperty[propFloat1] = this;
	myApp->animatorPanel->widgetToProperty[propFloat2] = this;

	/*floatstringcontrols[propFloat] = s;
	floatstringcontrolsKeyables[propFloat] = shape->parameters[i];
	controlIds.push_back(controlID);*/
	controlID += 2;
	property_sizer->Add(propFloat1, 1, wxEXPAND);
	property_sizer->Add(propFloat2, 1, wxEXPAND);

	property_sizer->Layout();
	myApp->panelProperties_sizer->Add(property_sizer, 0, wxEXPAND);
}

void FloatProperty::CreateWidgets(float time) {
	wxBoxSizer * property_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* spacing_text = new wxStaticText(myApp->propertiesPanel, controlID, name.c_str());
	property_sizer->Add(spacing_text, 0, wxEXPAND);
	controlID++;


	Expr s = value.getDisplayValue();
	double v = ceval_result2(replace_variable(s[0], time));
	propFloat1 = new SpinRegex(myApp->propertiesPanel, controlID, s[0], wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, v, 0.05);
	propFloat1->dim_id = 0;
	propFloat1->dim = 1;
	propFloat1->Bind(wxEVT_SPINCTRLDOUBLE, &AnimatorPanel::updatePropertiesFromControls, myApp->animatorPanel);
	propFloat1->Bind(wxEVT_LEFT_UP, &AnimatorPanel::spinMouseUp, myApp->animatorPanel);
	propFloat1->Bind(wxEVT_RIGHT_UP, &AnimatorPanel::spinMouseUp, myApp->animatorPanel);
	propFloat1->Bind(wxEVT_MOUSE_CAPTURE_LOST, &AnimatorPanel::OnSpinMouseCaptureLost, myApp->animatorPanel);
	myApp->animatorPanel->widgetToProperty[propFloat1] = this;
	controlID++;
	property_sizer->Add(propFloat1, 1, wxEXPAND);

	property_sizer->Layout();
	myApp->panelProperties_sizer->Add(property_sizer, 0, wxEXPAND);
}

void ColorProperty::CreateWidgets(float time) {
	wxBoxSizer * property_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* spacing_text = new wxStaticText(myApp->propertiesPanel, controlID, propertyName);
	property_sizer->Add(spacing_text, 0, wxEXPAND);
	controlID++;

	//unsigned char* u = (unsigned char*)shape->parameters[i]->firstParam + num_param;
	Vec3u v = color.getValue(time);
	colorPicker = new ClickableColourPicker(myApp->propertiesPanel, controlID, wxColour(v[0], v[1], v[2]), wxDefaultPosition, wxDefaultSize, wxCLRP_USE_TEXTCTRL | wxCLRP_SHOW_LABEL);
	colorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &AnimatorPanel::updatePropertiesFromControls, myApp->animatorPanel);
	colorPicker->Bind(wxEVT_RIGHT_UP, &AnimatorPanel::colourMouseUp, myApp->animatorPanel);

	myApp->animatorPanel->colorwidgetToProperty[colorPicker] = this;

	controlID++;
	property_sizer->Add(colorPicker, 1, wxEXPAND);

	property_sizer->Layout();
	myApp->panelProperties_sizer->Add(property_sizer, 0, wxEXPAND);
}

void VerticesListProperty::CreateWidgets(float time) {
	wxBoxSizer * property_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* spacing_text = new wxStaticText(myApp->propertiesPanel, controlID, "Vertices");
	property_sizer->Add(spacing_text, 0, wxEXPAND);
	controlID++;

	VerticesList vl = vertices.getValue(time);
	int numP = vl.size();

	//float* f = (float*)shape->parameters[i]->firstParam + num_param;
	propListVertices = new wxListCtrl(myApp->propertiesPanel, controlID, wxDefaultPosition, wxSize(-1, 100), wxLC_REPORT | wxLC_EDIT_LABELS);
	wxListItem itCol;
	itCol.SetId(0);
	itCol.SetText("Vertex");
	itCol.SetWidth(200);
	wxListItem itCol2;
	itCol2.SetId(1);
	itCol2.SetText("Color");
	itCol2.SetWidth(200);
	propListVertices->InsertColumn(0, itCol);
	propListVertices->InsertColumn(1, itCol2);
	propListVertices->Bind(wxEVT_LIST_END_LABEL_EDIT, &AnimatorPanel::editedVertex, myApp->animatorPanel);
	//verticescontrols[propListVertices] = f;
	//verticescontrolsKeyables[propListVertices] = shape->parameters[i];
	controlID++;
	property_sizer->Add(propListVertices, 1, wxEXPAND);

	property_sizer->Layout();
	myApp->panelProperties_sizer->Add(property_sizer, 0, wxEXPAND);
}


void VisibleProperty::CreateWidgets(float time) {
	wxBoxSizer * property_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* spacing_text = new wxStaticText(myApp->propertiesPanel, controlID, "Visible");
	property_sizer->Add(spacing_text, 0, wxEXPAND);
	controlID++;

	//bool* b = (bool*)shape->parameters[i]->firstParam + num_param;
	bool b = visible.getValue(time);
	propBool = new wxCheckBox(myApp->propertiesPanel, controlID, "", wxDefaultPosition, wxDefaultSize);
	propBool->SetValue(b);
	propBool->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &AnimatorPanel::updatePropertiesFromControls, myApp->animatorPanel);
	//boolcontrols[propBool] = b;
	//boolcontrolsKeyables[propBool] = shape->parameters[i];
	//controlIds.push_back(controlID);
	controlID++;
	property_sizer->Add(propBool, 1, wxEXPAND);

	property_sizer->Layout();
	myApp->panelProperties_sizer->Add(property_sizer, 0, wxEXPAND);
}

double Hue(double);


//https://www.csie.ntu.edu.tw/~b92069/HWs/rgb2lab.c
void RGB2LAB(int R_value, int G_value, int B_value, double *Lab) {
	double RGB[3];
	double XYZ[3];
	double adapt[3];
	//	double trans[3];
	//	double transf[3];
	//	double newXYZ[3];
	//	double newRGB[3];
	double value;
	//maybe change to global, XYZ[0] = X_value

	adapt[0] = 0.950467;
	adapt[1] = 1.000000;
	adapt[2] = 1.088969;

	RGB[0] = R_value * 0.003922;
	RGB[1] = G_value * 0.003922;
	RGB[2] = B_value * 0.003922;

	XYZ[0] = 0.412424 * RGB[0] + 0.357579 * RGB[1] + 0.180464 * RGB[2];
	XYZ[1] = 0.212656 * RGB[0] + 0.715158 * RGB[1] + 0.0721856 * RGB[2];
	XYZ[2] = 0.0193324 * RGB[0] + 0.119193 * RGB[1] + 0.950444 * RGB[2];

	Lab[0] = 116 * Hue(XYZ[1] / adapt[1]) - 16;
	Lab[1] = 500 * (Hue(XYZ[0] / adapt[0]) - Hue(XYZ[1] / adapt[1]));
	Lab[2] = 200 * (Hue(XYZ[1] / adapt[1]) - Hue(XYZ[2] / adapt[2]));

}

void LAB2RGB(double* Lab, unsigned char& R, unsigned char& G, unsigned char& B) {
	////////////////////////////////////////////////////
		double trans[3];
		double transf[3];
		double adapt[3];
		double newXYZ[3];
		double newRGB[3];
		adapt[0] = 0.950467;
		adapt[1] = 1.000000;
		adapt[2] = 1.088969;

		if ( Lab[0] > 903.3*0.008856 )
			trans[1] = pow ( (Lab[0]+16)*0.00862, 3);
		else
			trans[1] = Lab[0] * 0.001107;

		if ( trans[1] > 0.008856 )
			transf[1] = (Lab[0]+16)*0.00862;
		else
			transf[1] = (903.3*trans[1]+16)*0.00862;

		transf[0] = Lab[1] * 0.002 + transf[1];
		transf[2] = transf[1] - Lab[2] * 0.005;

		if ( pow( transf[0], 3 ) > 0.008856 )
			trans[0] = pow( transf[0], 3 );
		else
			trans[0] =  ((116 * transf[0]) - 16) * 0.001107;

		if ( pow( transf[2], 3 ) > 0.008856 )
			trans[2] = pow( transf[2], 3 );
		else
			trans[2] =  ((116 * transf[2]) - 16) * 0.001107;

		newXYZ[0] = trans[0] * adapt[0];
		newXYZ[1] = trans[1] * adapt[1];
		newXYZ[2] = trans[2] * adapt[2];

		newRGB[0] = 3.24071 * newXYZ[0] + (-1.53726) * newXYZ[1] + (-0.498571) * newXYZ[2];
		newRGB[1] = (-0.969258) * newXYZ[0] + 1.87599 * newXYZ[1] + 0.0415557 * newXYZ[2];
		newRGB[2] = 0.0556352 * newXYZ[0] + (-0.203996) * newXYZ[1] + 1.05707 * newXYZ[2];

		R = (unsigned char)std::min(255., std::max(0., newRGB[0] * 255.));
		G = (unsigned char)std::min(255., std::max(0., newRGB[1] * 255.));
		B = (unsigned char)std::min(255., std::max(0., newRGB[2] * 255.));
		//printf("r=%d g=%d b=%d nr=%.f ng=%.f nb=%.f\n",R_value,G_value,B_value,newRGB[0],newRGB[1],newRGB[2]);	
}

double Hue(double q) {
	double value;
	if (q > 0.008856) {
		value = pow(q, 0.333333);
		return value;
	} else {
		value = 7.787*q + 0.137931;
		return value;
	}
}