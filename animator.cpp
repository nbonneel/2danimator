#include "animator.h"

wxIMPLEMENT_APP_CONSOLE(AnimatorApp);

BEGIN_EVENT_TABLE(AnimatorPanel, wxPanel)
EVT_PAINT(AnimatorPanel::paintEvent)
EVT_MOTION(AnimatorPanel::mouseMoved)
EVT_LEFT_DOWN(AnimatorPanel::mouseDown)
EVT_RIGHT_DOWN(AnimatorPanel::mouseDown)
EVT_LEFT_UP(AnimatorPanel::mouseUp)
EVT_RIGHT_UP(AnimatorPanel::mouseUp)
EVT_MOUSEWHEEL(AnimatorPanel::mouseWheel)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AnimatorApp, wxApp)
EVT_IDLE(AnimatorApp::OnIdle)
END_EVENT_TABLE()

int Shape::numShapes = 0;

AnimatorFrame::AnimatorFrame() : wxFrame(NULL, wxID_ANY, wxT("2D Animator by N.Bonneel"),
	wxPoint(10, 100)) {
	wxMenu *file_menu = new wxMenu();
	file_menu->Append(200, wxT("&Export image sequence..."));
	Connect(200, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorFrame::ExportSequence), NULL, this);

	wxMenuBar* menu_bar = new wxMenuBar();
	menu_bar->Append(file_menu, wxT("&File"));
	//menu_bar->Append(info_menu, wxT("&Info"));

	SetMenuBar(menu_bar);

	CreateStatusBar();

	Centre();
}

void AnimatorFrame::ExportSequence(wxCommandEvent &evt) {
	wxFileDialog
		saveFileDialog(this, _("Save Image file"), "", "",
			"Image files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveFileDialog.ShowModal() == wxID_CANCEL)
		return;

	myApp->render_animation(saveFileDialog.GetPath());
}

void AnimatorApp::render_animation(const char* filename) {
	std::string sfilename(filename);
	std::string ext = sfilename.substr(sfilename.find_last_of(".") + 1); // without '.'
	std::string name = sfilename.substr(0, sfilename.find_last_of(".")); // without '.'
	Canvas c(animatorPanel->cur_img.W, animatorPanel->cur_img.H);
	for (int i = 0; i < time_slider->GetMax(); i++) {
		std::string s = name + std::to_string(i) + "." + ext;
		float t = i / (float)time_slider->GetMax();
		animatorPanel->scene->currentTime = t;
		animatorPanel->scene->Draw(c, false);
		c.saveToFile(s.c_str());
	}
}

void AnimatorPanel::mouseDown(wxMouseEvent& event) {
	mouse_init_x = event.m_x; // coordinates before dragging the mouse
	mouse_init_y = event.m_y;
	moveVtxId = -1;
	left_mouse_down = event.LeftIsDown();
	
	if (event.LeftIsDown()) {
		Shape* prevShape = scene->currentShape;
		int sel = scene->SelectedShape(Vec2f(event.m_x, event.m_y));
		if (sel >= 0) {
			scene->currentShape = scene->shapes[sel];
			scene->oldPos = scene->currentShape->GetPosition(scene->currentTime);
			if (scene->currentShape != prevShape) {
				scene->previousShape = prevShape;
				needPanelUpdate = true;
			}				
		}
		if (addingMorph && scene->currentShape && prevShape) {
			PolygonShape* s1 = dynamic_cast<PolygonShape*>(prevShape);
			PolygonShape* s2 = dynamic_cast<PolygonShape*>(scene->currentShape);
			if (s1&&s2) {
				PolygonMorph* p = new PolygonMorph(s1, s2, "0.5");
				scene->Add(p);
				addingMorph = false;
			}
		}
	}

}

void AnimatorPanel::mouseUp(wxMouseEvent& event) {
	moveVtxId = -1;
	moveCtrlId = -1;
	if (needPanelUpdate) {
		animatorApp->setupPanelProperties(scene->currentShape);
		needPanelUpdate = false;
	}
	updateControlValues();
	left_mouse_down = false;
	//scene->currentShape = NULL;
	if (event.RightUp()) {

		PolygonShape* p = dynamic_cast<PolygonShape*>(scene->currentShape);
		if (p) {
			PopupMenu(&polygonMenu);
		}
		Spline* s = dynamic_cast<Spline*>(scene->currentShape);
		if (s) {
			PopupMenu(&splineMenu);
		}
	}
}

void AnimatorPanel::udate_time_values(wxCommandEvent& event) {

	scene->currentTime = dynamic_cast<wxSlider*>(event.GetEventObject())->GetValue() / 100.;
	for (std::map<SpinRegex*, float* >::iterator it = animatorApp->floatcontrols.begin(); it != animatorApp->floatcontrols.end(); ++it) {
		it->first->tval = scene->currentTime;
	}
	updatePropertiesFromControls(event);
	paintNow();
}

void AnimatorPanel::updatePropertiesFromControls(wxCommandEvent& event) {

	for (std::map<SpinRegex*, float* >::iterator it = animatorApp->floatcontrols.begin(); it != animatorApp->floatcontrols.end(); ++it) {
		*(it->second) = it->first->GetValue();
	}

	for (std::map<SpinRegex*, std::string* >::iterator it = animatorApp->floatstringcontrols.begin(); it != animatorApp->floatstringcontrols.end(); ++it) {
		*(it->second) = it->first->GetText()->GetValue();
	}
	
	for (std::map<wxCheckBox*, bool* >::iterator it = animatorApp->boolcontrols.begin(); it != animatorApp->boolcontrols.end(); ++it) {
		*(it->second) = it->first->GetValue();
	}

	for (std::map<wxColourPickerCtrl*, unsigned char* >::iterator it = animatorApp->colorcontrols.begin(); it != animatorApp->colorcontrols.end(); ++it) {
		wxColor col = it->first->GetColour();
		*(it->second) = col.Red();
		*(it->second+1) = col.Green();
		*(it->second+2) = col.Blue();
	}	
};

void AnimatorPanel::editedVertex(wxListEvent& event) {
	int itemIndex = event.GetIndex();
	wxListCtrl *ctrl = (wxListCtrl*)event.GetEventObject();
	wxListItem item = event.GetItem();
	float f1, f2;
	float* f = animatorApp->verticescontrols[ctrl] + (2 + sizeof(Keyable) / 4)*itemIndex;
	sscanf(item.GetText().c_str(), "(%f, %f)", &f1, &f2);
	*f = f1;
	*(f + 1) = f2;
}

void AnimatorPanel::updateControlValues() {

	for (std::map<SpinRegex*, float* >::iterator it = animatorApp->floatcontrols.begin(); it != animatorApp->floatcontrols.end(); ++it) {
		it->first->SetValue((double)*(it->second));
	}
	for (std::map<SpinRegex*, std::string* >::iterator it = animatorApp->floatstringcontrols.begin(); it != animatorApp->floatstringcontrols.end(); ++it) {
		it->first->GetText()->SetValue(*(it->second));
	}

	for (std::map<wxCheckBox*, bool* >::iterator it = animatorApp->boolcontrols.begin(); it != animatorApp->boolcontrols.end(); ++it) {
		it->first->SetValue(*(it->second));
	}
	for (std::map<wxColourPickerCtrl*, unsigned char* >::iterator it = animatorApp->colorcontrols.begin(); it != animatorApp->colorcontrols.end(); ++it) {
		it->first->SetColour(wxColour(*(it->second), *(it->second+1), *(it->second+2)));
	}
	for (std::map<wxListCtrl*, float* >::iterator it = animatorApp->verticescontrols.begin(); it != animatorApp->verticescontrols.end(); ++it) {
		int N = 0;
		PolygonShape* ps = dynamic_cast<PolygonShape*>(scene->currentShape);
		if (ps) {
			N = ps->vertices.vertices.size();
		} else {
			Spline* ss = dynamic_cast<Spline*>(scene->currentShape);
			if (ss) {
				N = ss->controlPoints.vertices.size();
			}
		}
		for (int i = 0; i < N; i++) {
			float* coord = it->second + i * (2+sizeof(Keyable)/4);
			std::string txt = "("+std::to_string(*(coord))+", "+ std::to_string(*(coord +1))+")"; // *3 = 2 floats (coords) + 1 address (sizeof(firstParam))
			wxListItem item;
			item.SetId(i);
			long index = it->first->InsertItem(i, item);
			it->first->SetItem(index, 0, txt, -1);			
		}
	}
};

void AnimatorPanel::OnChangePolygonToTextPopupClick(wxCommandEvent &evt) {
	AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
	PolygonShape* p = dynamic_cast<PolygonShape*>(curPanel->scene->currentShape);

	wxTextEntryDialog dialog(this, "Text", wxGetTextFromUserPromptStr, "text");
	if (dialog.ShowModal()) {
		std::string text = dialog.GetValue().ToStdString();
		p->setText(text.c_str());
	}
}

void AnimatorPanel::mouseMoved(wxMouseEvent& event) {
	mouse_x = event.m_x;
	mouse_y = event.m_y;
	animatorApp->infoBar->SetLabelText(std::to_string(mouse_x) + "," + std::to_string(mouse_y));

	if ((event.LeftIsDown() || event.RightIsDown() || event.MiddleIsDown()) && event.Dragging()) {

		if (scene->currentShape && moveVtxId < 0 && moveCtrlId < 0) {
			if (mouse_x > W) return;
			if (mouse_y > H) return;
			if (mouse_x < 0) return;
			if (mouse_y < 0) return;
			scene->currentShape->SetPosition(scene->currentShape->GetPosition(scene->currentTime) + Vec2f(mouse_x, mouse_y) - Vec2f(mouse_init_x, mouse_init_y));
			mouse_init_x = mouse_x;
			mouse_init_y = mouse_y;
		}
	}
	left_mouse_down = event.LeftIsDown();

	if (scene->currentShape && moveVtxId >= 0) {
		PolygonShape* poly = dynamic_cast<PolygonShape*>(scene->currentShape);
		Vec2f movedV(mouse_x, mouse_y);
		float t = scene->currentTime;
		float s = poly->GetScale(t);
		float a = poly->angle.eval1(t);
		Vec2f trans = poly->GetPosition(t);
		poly->vertices.vertices[moveVtxId] = rotate((movedV - trans) / s, -a);
		mouse_init_x = mouse_x;
		mouse_init_y = mouse_y;
	}

	if (scene->currentShape && moveCtrlId >= 0) {
		Spline* poly = dynamic_cast<Spline*>(scene->currentShape);
		Vec2f movedV(mouse_x, mouse_y);
		float t = scene->currentTime;
		float s = poly->GetScale(t);
		//float a = poly->angle.eval1(t);
		Vec2f trans = poly->GetPosition(t);
		poly->controlPoints.vertices[moveCtrlId] = /*rotate*/((movedV - trans) / s/*, -a*/);
		mouse_init_x = mouse_x;
		mouse_init_y = mouse_y;
	}

	paintNow();
}

void AnimatorPanel::spinMouseUp(wxMouseEvent &evt) {
	if (!evt.GetEventObject()) {
		evt.Skip();
		return;
	}
	SpinRegex* obj = dynamic_cast<SpinRegex*>(evt.GetEventObject());
	//this->ProcessEvent(evt);
	Spline* spline = dynamic_cast<Spline*>(myApp->animatorPanel->scene->currentShape);
	if (!spline) {
		evt.Skip();
		return;
	}
	std::string val = "curvea(" + std::to_string(spline->id) + ",t)";
	int n = myApp->floatstringcontrolsKeyables[obj]->encodedTypes.size();
	if (n == 2) {
		if (myApp->floatstringcontrols[obj] == myApp->floatstringcontrolsKeyables[obj]->firstParam) {
			val = "curvex(" + std::to_string(spline->id) + ",t)";
		} else {
			val = "curvey(" + std::to_string(spline->id) + ",t)";
		}
	}
	obj->SetValue(val);
	*(myApp->floatstringcontrols[obj]) = val;
	myApp->infoBar->SetLabelText(std::to_string(evt.GetX()) + "    " + std::to_string(evt.GetY()));	
	myApp->animatorPanel->needPanelUpdate = false;
	spline->SetPosition(myApp->animatorPanel->scene->oldPos);
	myApp->animatorPanel->scene->currentShape = myApp->animatorPanel->scene->previousShape;
	updateControlValues();
	paintNow();
}

void AnimatorPanel::OnSpinMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
	SpinRegex* obj = dynamic_cast<SpinRegex*>(evt.GetEventObject());
	if (obj->HasCapture())
		obj->ReleaseMouse();
	//ReleaseCapture();
}

void AnimatorPanel::OnBSplineSubdivPopupClick(wxCommandEvent &evt) {
	AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
	PolygonShape* p = dynamic_cast<PolygonShape*>(curPanel->scene->currentShape);	
	float t = curPanel->scene->currentTime;
	float s = p->GetScale(t);
	float a = p->angle.eval1(t);
	Vec2f trans = p->GetPosition(t);

	VerticesList newList;
	newList.Clear();
	for (int ct = 0; ct < p->vertices.contourList.size(); ct++) {

		int N;
		int startV, endV = p->vertices.contourList[ct];
		if (ct == 0) {
			startV = 0;
		} else {
			startV = p->vertices.contourList[ct - 1]+1;
		}
		N = endV - startV + 1;

		Spline sp(Vec2s("0", "0"), "1", Vec3u(0, 0, 0));
		sp.controlPoints.addVertex(p->vertices.vertices[endV]);
		for (int i = 0; i < N; i++) {
			sp.controlPoints.addVertex(p->vertices.vertices[startV+i]);
		}
		sp.controlPoints.addVertex(p->vertices.vertices[startV]);
		sp.controlPoints.addVertex(p->vertices.vertices[startV+1]);
		
		for (int i = 0; i < 2 * (N); i++) {
			float tparam = i / (2.0*N);  // in [0, 1[
			Vec2f pp = sp.evalBSpline(tparam*(N + 1.) / (N + 3.));
			newList.addVertex(pp, i==0);
		}
	}
	if (newList.contourList.size() >= 2) {
		if (newList.contourList[newList.contourList.size() - 1] == newList.contourList[newList.contourList.size() - 2]) {
			newList.contourList.resize(newList.contourList.size() - 1);
		}
	}
	p->vertices.Clear();
	p->vertices = newList;
}

wxControl* AnimatorApp::addObjectButton(const char* name, Shape* s, wxPanel* panel, wxPanel* parent) {

	static int objId = 1000;
	wxButton* objectButton = new wxButton(panel, objId, name, wxDefaultPosition, wxDefaultSize);
	
	Connect(objId, wxEVT_BUTTON, wxCommandEventHandler(AnimatorPanel::addShape), (wxObject*)new AddObjectParam(animatorPanel->scene, s), parent);
	objId++;
	return objectButton;
}

void AnimatorApp::setupPanelProperties(Shape* shape) {
	int controlID = 10000;
	
	for (int i = 0; i < controlIds.size(); i++) {
		wxWindow* w = propertiesPanel->FindWindowById(controlIds[i]);
		delete w;
	}
	for (int i = 0; i < property_sizers.size(); i++) {
		panelProperties_sizer->Remove(property_sizers[i]);
		//delete property_sizers[i]; // already done in Remove
	}
	floatcontrols.clear();
	colorcontrols.clear();
	boolcontrols.clear();
	property_sizers.clear();
	verticescontrols.clear();
	floatstringcontrols.clear();
	floatcontrolsKeyables.clear();
	colorcontrolsKeyables.clear();
	boolcontrolsKeyables.clear();	
	verticescontrolsKeyables.clear();
	floatstringcontrolsKeyables.clear();
	controlIds.clear();
	for (int i = 0; i < shape->parameters.size(); i++) {
		wxBoxSizer * property_sizer = new wxBoxSizer(wxHORIZONTAL);

		wxStaticText* spacing_text = new wxStaticText(propertiesPanel, controlID, shape->parameters[i]->name);
		controlIds.push_back(controlID);
		controlID++;
		
		property_sizer->Add(spacing_text, 0, wxEXPAND);
		int num_param = 0;
		for (int j = 0; j < shape->parameters[i]->encodedTypes.size(); j++, num_param++) {
			unsigned char type = shape->parameters[i]->encodedTypes[j];

			if (type == 'B') {

				bool* b = (bool*)shape->parameters[i]->firstParam + num_param;
				wxCheckBox* propBool = new wxCheckBox(propertiesPanel, controlID, "", wxDefaultPosition, wxDefaultSize);
				Connect(controlID, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(AnimatorPanel::updatePropertiesFromControls), NULL, animatorPanel);
				boolcontrols[propBool] = b;
				boolcontrolsKeyables[propBool] = shape->parameters[i];
				controlIds.push_back(controlID);
				controlID++;
				property_sizer->Add(propBool, 1, wxEXPAND);
			}

			if (type == 'F' || type == 't') {
				
				float* f = (float*)shape->parameters[i]->firstParam+ num_param;
				SpinRegex* propFloat = new SpinRegex(propertiesPanel, controlID, wxString::Format(wxT("%f"), *f), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, *f, (type == 't')?0.05:1.0);
				Connect(controlID, wxEVT_SPINCTRLDOUBLE, wxCommandEventHandler(AnimatorPanel::updatePropertiesFromControls), NULL, animatorPanel);
				floatcontrols[propFloat] = f;
				floatcontrolsKeyables[propFloat] = shape->parameters[i];
				controlIds.push_back(controlID);
				controlID++;
				property_sizer->Add(propFloat, 1, wxEXPAND);
			}
			if (type == 'S' || type == 'T') {

				std::string* s = (std::string*)shape->parameters[i]->firstParam + num_param;
				double v = ceval_result2(replace_variable(*s, animatorPanel->scene->currentTime));
				SpinRegex* propFloat = new SpinRegex(propertiesPanel, controlID, *s, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, v, (type == 'T') ? 0.05 : 1.0);
				//propFloat->CaptureMouse();
				Connect(controlID, wxEVT_SPINCTRLDOUBLE, wxCommandEventHandler(AnimatorPanel::updatePropertiesFromControls), NULL, animatorPanel);
				propFloat->Bind(wxEVT_LEFT_UP, &AnimatorPanel::spinMouseUp, animatorPanel);
				
				Connect(controlID, wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(AnimatorPanel::OnSpinMouseCaptureLost), NULL, animatorPanel);
				
				floatstringcontrols[propFloat] = s;
				floatstringcontrolsKeyables[propFloat] = shape->parameters[i];
				controlIds.push_back(controlID);
				controlID++;
				property_sizer->Add(propFloat, 1, wxEXPAND);
			}
			if (type == 'C') {

				unsigned char* u = (unsigned char*)shape->parameters[i]->firstParam + num_param;
				wxColourPickerCtrl *colorPicker = new wxColourPickerCtrl(propertiesPanel, controlID, wxColour(*u,*(u+1),*(u+2)), wxDefaultPosition, wxDefaultSize, wxCLRP_USE_TEXTCTRL | wxCLRP_SHOW_LABEL);
				Connect(controlID, wxEVT_COLOURPICKER_CHANGED, wxCommandEventHandler(AnimatorPanel::updatePropertiesFromControls), NULL, animatorPanel);
				colorcontrols[colorPicker] = u;
				colorcontrolsKeyables[colorPicker] = shape->parameters[i];
				controlIds.push_back(controlID);
				controlID++;
				property_sizer->Add(colorPicker, 1, wxEXPAND);
			}

			if (type == 'L') {
				std::string numV;
				int k = j + 1;
				for ( ; k < shape->parameters[i]->encodedTypes.size(); k++) {
					if (shape->parameters[i]->encodedTypes[k] >= '0' && shape->parameters[i]->encodedTypes[k] <= '9') {
						numV = numV + shape->parameters[i]->encodedTypes[k];
					} else break;
				}
				j = k-1;
				int numP = std::stoi(numV);

				float* f = (float*)shape->parameters[i]->firstParam + num_param;
				wxListCtrl* propListVertices = new wxListCtrl(propertiesPanel, controlID, wxDefaultPosition, wxSize(-1, 100), wxLC_REPORT| wxLC_EDIT_LABELS);
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
				Connect(controlID, wxEVT_LIST_END_LABEL_EDIT, wxListEventHandler(AnimatorPanel::editedVertex), NULL, animatorPanel);
				verticescontrols[propListVertices] = f;
				verticescontrolsKeyables[propListVertices] = shape->parameters[i];
				controlIds.push_back(controlID);
				controlID++;
				property_sizer->Add(propListVertices, 1, wxEXPAND);
			}
		}
		property_sizer->Layout();
		panelProperties_sizer->Add(property_sizer, 0, wxEXPAND);
		property_sizers.push_back(property_sizer);
	}
	panelProperties_sizer->Layout();
}

AnimatorApp* myApp = NULL;
bool AnimatorApp::OnInit() {
	myApp = this;
	if (!wxApp::OnInit())
		return false;

	AnimatorFrame* frame = new AnimatorFrame();	
	animatorFrame = frame;
	frame->SetSize(1200, 1000);
	animatorPanel = new AnimatorPanel(frame, this, 512, 512);

	wxBoxSizer * statusbar_sizer = new wxBoxSizer(wxHORIZONTAL);
	infoBar = new wxStaticText(frame->GetStatusBar(), wxID_ANY, " ");
	statusbar_sizer->Add(infoBar);
	frame->GetStatusBar()->SetSizer(statusbar_sizer);

	m_bookCtrl = new wxNotebook(frame, wxID_ANY, wxDefaultPosition, wxSize(200, 100), 0);

	wxScrolled<wxPanel> *panelObject = new wxScrolled<wxPanel>(m_bookCtrl);
	panelObject->EnableScrolling(true, true);
	panelObject->SetScrollbars(5, 10, 20, 60);

	wxBoxSizer * panelObject_sizer = new wxBoxSizer(wxVERTICAL);
	
	panelObject_sizer->Add(addObjectButton("Disk", new Disk(Vec2s("50", "50"), "30", Vec3u(255, 0, 0), Vec3u(0,0,255), "5"), panelObject, animatorPanel) , 0, wxEXPAND);
	PolygonShape* defaultPolygon = new PolygonShape(Vec2s("175", "125"), "1", "0", Vec3u(255, 0, 0), Vec3u(0,0,0), "2");
	//PolygonShape* defaultPolygon = new PolygonShape(Vec2s("0", "0"), "1", "0", Vec3u(255, 0, 0), Vec3u(0, 0, 0), "2");
	defaultPolygon->addVertex(Vec2f(150-175, 50 - 125));
	defaultPolygon->addVertex(Vec2f(200 - 175, 90- 125));
	defaultPolygon->addVertex(Vec2f(200 - 175, 150 - 125));
	defaultPolygon->addVertex(Vec2f(150 - 175, 200 - 125));
	defaultPolygon->addVertex(Vec2f(100 - 175, 150 - 125));
	defaultPolygon->addVertex(Vec2f(100 - 175, 90 - 125));	
	panelObject_sizer->Add(addObjectButton("Polygon", defaultPolygon, panelObject, animatorPanel), 0, wxEXPAND);


	wxButton* morphPolygonsButton = new wxButton(panelObject, 1100, "Morph Polygons", wxDefaultPosition, wxDefaultSize);
	Connect(1100, wxEVT_BUTTON, wxCommandEventHandler(AnimatorPanel::addMorphPolygon), NULL, animatorPanel);
	panelObject_sizer->Add(morphPolygonsButton, 0, wxEXPAND);
	
	Spline* defaultSpline = new Spline(Vec2s("0", "0"), "1", Vec3u(255, 0, 0));
	defaultSpline->addVertex(Vec2f(150, 50));
	defaultSpline->addVertex(Vec2f(200, 90));
	defaultSpline->addVertex(Vec2f(200, 150));
	defaultSpline->addVertex(Vec2f(150, 200));
	defaultSpline->addVertex(Vec2f(100, 150));
	defaultSpline->addVertex(Vec2f(100, 90));
	panelObject_sizer->Add(addObjectButton("BSpline", defaultSpline, panelObject, animatorPanel), 0, wxEXPAND);

	panelObject->SetSizer(panelObject_sizer);

	m_bookCtrl->AddPage(panelObject, wxT("Add Objects"), false);


	propertiesPanel = new wxScrolled<wxPanel>(m_bookCtrl);
	m_bookCtrl->AddPage(propertiesPanel, wxT("Properties"), false);
	propertiesPanel->EnableScrolling(true, true);
	propertiesPanel->SetScrollbars(5, 10, 20, 60);
	panelProperties_sizer = new wxBoxSizer(wxVERTICAL);
	
	propertiesPanel->SetSizer(panelProperties_sizer);

	wxBoxSizer * renderSuperSizer_sizer = new wxBoxSizer(wxVERTICAL);
	renderSuperSizer_sizer->Add(animatorPanel, 5, wxEXPAND);
	
	wxBoxSizer * time_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* time_text = new wxStaticText(frame, 9999, "timeline ");
	time_slider = new wxSlider(frame, 123, 0, 0, 100, wxDefaultPosition, wxSize(600, 32), wxSL_HORIZONTAL | wxSL_LABELS);
	Connect(123, wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(AnimatorPanel::udate_time_values), NULL, animatorPanel);
	time_sizer->Add(time_text, 0, wxEXPAND);
	time_sizer->Add(time_slider, 1, wxEXPAND);
	renderSuperSizer_sizer->Add(time_sizer, 5, wxEXPAND);

	wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
	frame->SetSizer(sizer);
	sizer->Add(renderSuperSizer_sizer, 2, wxEXPAND);
	sizer->Add(m_bookCtrl, 1, wxEXPAND);

	activateRenderLoop(true);
	frame->Show();
	return true;
}


void AnimatorApp::activateRenderLoop(bool on) {
#if !defined(__WXOSX__)&&!defined(__WXMAC__)&&!defined(__APPLE__)
	animatorPanel->SetDoubleBuffered(true);
#endif
	if (on && !render_loop_on) {
		Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(AnimatorApp::OnIdle));
		Connect(wxID_ANY, wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(AnimatorApp::OnEraseBackGround));
		render_loop_on = true;
	} else if (!on && render_loop_on) {
		Disconnect(wxEVT_IDLE, wxIdleEventHandler(AnimatorApp::OnIdle));
		render_loop_on = false;
	}
}

void AnimatorPanel::render(wxDC& dc) {
	if (cur_img.pixels.size() == 0) return;
	/*if (animator.stopped) {
		double scale_x = (double)displayW / raytracer.W;
		double scale_y = (double)displayH / raytracer.H;
		dc.SetUserScale(scale_x, scale_y);
		dc.DrawBitmap(bmpBuf, 0, 0);
		dc.SetUserScale(1.0, 1.0);
		return;
	}*/

	//dynamic_cast<Disk*>(s.shapes[0])->pos = Vec2f(mouse_x, mouse_y);

	scene->Draw(cur_img, true);

	static int nbcalls = -1; nbcalls++;
	if (nbcalls == 0 || screenImage.GetWidth() != W || screenImage.GetHeight() != H) {
		screenImage = wxImage(W, H, &(cur_img.pixels[0]), true);
	}

	bmpBuf = wxBitmap(screenImage/*, dc*/);


	double scale_x = 1; // (double)displayW / raytracer.W;
	double scale_y = 1; // (double)displayH / raytracer.H;
	dc.SetUserScale(scale_x, scale_y);
	dc.DrawBitmap(bmpBuf, 0, 0);
	dc.SetUserScale(1.0, 1.0);
}