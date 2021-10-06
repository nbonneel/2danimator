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
EVT_CHAR_HOOK(AnimatorPanel::keydown)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AnimatorApp, wxApp)
EVT_IDLE(AnimatorApp::OnIdle)
END_EVENT_TABLE()

int Shape::numShapes = 0;

AnimatorFrame::AnimatorFrame() : wxFrame(NULL, wxID_ANY, wxT("2D Animator by N.Bonneel"),
	wxPoint(10, 100)) {
	wxMenu *file_menu = new wxMenu();
	
	file_menu->Append(202, wxT("&Load scene..."));
	file_menu->Append(201, wxT("&Save scene..."));
	file_menu->Append(200, wxT("&Export image sequence..."));

	Connect(200, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorFrame::ExportSequence), NULL, this);
	Connect(201, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorFrame::SaveScene), NULL, this);
	Connect(202, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorFrame::LoadScene), NULL, this);

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
void AnimatorFrame::SaveScene(wxCommandEvent &evt) {
	wxFileDialog
		saveFileDialog(this, _("Save Scene file"), "", "",
			"Scene files (*.anm)|*.anm", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveFileDialog.ShowModal() == wxID_CANCEL)
		return;

	std::ofstream f(saveFileDialog.GetPath().ToStdString());
	f << *(myApp->animatorPanel->scene) << std::endl;
	f.close();
}

void AnimatorFrame::LoadScene(wxCommandEvent &evt) {
	wxFileDialog
		loadFileDialog(this, _("Load Scene file"), "", "",
			"Scene files (*.anm)|*.anm", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (loadFileDialog.ShowModal() == wxID_CANCEL)
		return;
	ClearScene();
	std::ifstream f(loadFileDialog.GetPath().ToStdString());
	f >> *(myApp->animatorPanel->scene);
	f.close();
}

void AnimatorFrame::ClearScene() {
	myApp->animatorPanel->scene->Clear();
	myApp->animatorPanel->widgetToProperty.clear();
	myApp->animatorPanel->colorwidgetToProperty.clear();
	myApp->animatorPanel->filenamewidgetToProperty.clear();
	myApp->animatorPanel->textwidgetToProperty.clear();
}


void AnimatorApp::render_animation(const char* filename) {
	std::string sfilename(filename);
	std::string ext = sfilename.substr(sfilename.find_last_of(".") + 1); // without '.'
	std::string name = sfilename.substr(0, sfilename.find_last_of(".")); // without '.'
	Canvas c(myApp->animatorPanel->cur_img.W, myApp->animatorPanel->cur_img.H);
	for (int i = 0; i < time_slider->GetMax(); i++) {
		std::string s = name + std::to_string(i) + "." + ext;
		float t = i / (float)time_slider->GetMax();
		myApp->animatorPanel->scene->currentTime = t;
		myApp->time_slider->SetValue(t*100.);
		wxCommandEvent null;
		myApp->animatorPanel->update_time_values(null);
		myApp->animatorPanel->scene->Draw(c, false);
		c.saveToFile(s.c_str());
	}
}

void AnimatorPanel::mouseDown(wxMouseEvent& event) {
	mouse_init_x = event.m_x; // coordinates before dragging the mouse
	mouse_init_y = event.m_y;
	moveVtxId = -1;
	left_mouse_down = event.LeftIsDown();
	event.Skip();
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
	event.Skip();
	dragging = false;
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
			return;
		}
		Spline* s = dynamic_cast<Spline*>(scene->currentShape);
		if (s) {
			PopupMenu(&splineMenu);
			return;
		}
	}
}

void AnimatorPanel::keydown(wxKeyEvent& event) {
	event.Skip();
	if (dynamic_cast<wxTextCtrl*>(myApp->animatorPanel->FindFocus())) return;

	if (event.GetUnicodeKey() == wxChar('s') || event.GetUnicodeKey() == wxChar('S'))
		if (scene->currentShape) {
			for (int i = 0; i < scene->currentShape->parameters.size(); i++) {
				scene->currentShape->parameters[i]->addKeyframe(scene->currentTime);
				scene->currentShape->parameters[i]->UpdateWidgets(scene->currentTime);
			}
		}
}

void AnimatorPanel::addShape(wxCommandEvent& event) {
	AddObjectParam* param = (AddObjectParam*)event.GetEventUserData();
	Shape* s = param->shapeType->Clone();
	PolygonShape* p = dynamic_cast<PolygonShape*>(s);
	/*if (p)
		p->setText("Liris");*/
	param->scene->Add(s);
	scene->currentShape = s;
	myApp->setupPanelProperties(scene->currentShape);
};

void AnimatorPanel::update_time_values(wxCommandEvent& event) {

	scene->currentTime = myApp->time_slider->GetValue() / 100.;
	for (int i = 0; i < scene->shapes.size(); i++)
		for (int j = 0; j < scene->shapes[i]->parameters.size(); j++) {
			scene->shapes[i]->parameters[j]->UpdateInternalTime(scene->currentTime);
		}

	//updatePropertiesFromControls(event);
	//updateControlValues();
	//paintNow();
	this->CallAfter(&AnimatorPanel::updateControlValues);
}

void AnimatorPanel::updatePropertiesFromControls(wxCommandEvent& event) {
	if (!scene->currentShape) return;

	for (int i = 0; i < scene->currentShape->parameters.size(); i++) {
		scene->currentShape->parameters[i]->UpdateParameterFromWidget(scene->currentTime);
	}	
};

void AnimatorPanel::editedVertex(wxListEvent& event) {
	event.Skip();
	int itemIndex = event.GetIndex();
	wxListCtrl *ctrl = (wxListCtrl*)event.GetEventObject();
	wxListItem item = event.GetItem();
	float f1, f2;							
		//animatorApp->verticescontrols[ctrl] + (2 + sizeof(Keyable) / 4)*itemIndex;
	sscanf(item.GetText().c_str(), "(%f, %f)", &f1, &f2);
	dynamic_cast<PolygonShape*>(scene->currentShape)->vertices.editVertex(scene->currentTime, itemIndex, Vec2f(f1, f2));
}

void AnimatorPanel::updateControlValues() {
	if (!myApp->animatorPanel->scene->currentShape) return;
	for (int i = 0; i < myApp->animatorPanel->scene->currentShape->parameters.size(); i++) {
		myApp->animatorPanel->scene->currentShape->parameters[i]->UpdateWidgets(myApp->animatorPanel->scene->currentTime);
	}
	myApp->animatorPanel->paintNow();
};

void AnimatorPanel::OnChangePolygonToTextPopupClick(wxCommandEvent &evt) {
	evt.Skip();
	AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
	PolygonShape* p = dynamic_cast<PolygonShape*>(curPanel->scene->currentShape);

	wxTextEntryDialog dialog(this, "Text", wxGetTextFromUserPromptStr, "text");
	if (dialog.ShowModal()) {
		std::string text = dialog.GetValue().ToStdString();
		p->setText(scene->currentTime, text.c_str());
	}
}

void AnimatorPanel::mouseMoved(wxMouseEvent& event) {
	event.Skip();
	mouse_x = event.m_x;
	mouse_y = event.m_y;
	animatorApp->infoBar->SetLabelText(std::to_string(mouse_x) + "," + std::to_string(mouse_y));

	if ((event.LeftIsDown() || event.RightIsDown() || event.MiddleIsDown()) && event.Dragging()) {

		if (scene->currentShape && moveVtxId < 0 && moveCtrlId < 0) {
			if (mouse_x > W) return;
			if (mouse_y > H) return;
			if (mouse_x < 0) return;
			if (mouse_y < 0) return;
			scene->currentShape->SetPosition(scene->currentTime, scene->currentShape->GetPosition(scene->currentTime) + Vec2f(mouse_x, mouse_y) - Vec2f(mouse_init_x, mouse_init_y));
			mouse_init_x = mouse_x;
			mouse_init_y = mouse_y;
			dragging = true;
		}
	}
	left_mouse_down = event.LeftIsDown();

	if (scene->currentShape && moveVtxId >= 0) {
		PolygonShape* poly = dynamic_cast<PolygonShape*>(scene->currentShape);
		Vec2f movedV(mouse_x, mouse_y);
		float t = scene->currentTime;
		float s = poly->GetScale(t);
		float a = poly->angle.getDisplayValue(t);
		Vec2f trans = poly->GetPosition(t);
		poly->vertices.editVertex(t, moveVtxId, rotate((movedV - trans) / s, -a));		
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
		//poly->controlPoints.vertices[moveCtrlId] = /*rotate*/((movedV - trans) / s/*, -a*/);
		poly->controlPoints.editVertex(t, moveCtrlId, /*rotate(*/(movedV - trans) / s/*, -a)*/);
		mouse_init_x = mouse_x;
		mouse_init_y = mouse_y;
	}

	paintNow();
}

void AnimatorPanel::colourMouseUp(wxMouseEvent &evt) {
	if (!evt.GetEventObject()) {
		evt.Skip();
		return;
	}
	evt.Skip();
	if (evt.RightUp()) {
		colorCtrlMenu.SetClientData(evt.GetEventObject());
		PopupMenu(&colorCtrlMenu);
		return;
	}
}

void AnimatorPanel::filenameMouseUp(wxMouseEvent &evt) {
	if (!evt.GetEventObject()) {
		evt.Skip();
		return;
	}
	evt.Skip();
	if (evt.RightUp()) {
		filenameCtrlMenu.SetClientData(evt.GetEventObject());
		PopupMenu(&filenameCtrlMenu);
		return;
	}
}



void AnimatorPanel::textMouseUp(wxMouseEvent &evt) {
	if (!evt.GetEventObject()) {
		evt.Skip();
		return;
	}
	evt.Skip();
	if (evt.RightUp()) {
		textCtrlMenu.SetClientData(evt.GetEventObject());
		PopupMenu(&textCtrlMenu);
		return;
	}
}


void AnimatorPanel::spinMouseUp(wxMouseEvent &evt) {
	if (!evt.GetEventObject()) {
		evt.Skip();
		return;
	}
	

	if (evt.RightUp()) {
		spinCtrlMenu.SetClientData(evt.GetEventObject());
		PopupMenu(&spinCtrlMenu);
		evt.Skip();
		return;
	}
	if (!dragging) {
		dragging = false;
		evt.Skip();
		return;
	}
	dragging = false;
	SpinRegex* obj = dynamic_cast<SpinRegex*>(evt.GetEventObject());
	//this->ProcessEvent(evt);
	Spline* spline = dynamic_cast<Spline*>(myApp->animatorPanel->scene->currentShape);
	if (spline) {

		std::string val;
		if (obj->dim == 1) {
			val = "curvea(" + std::to_string(spline->id) + ",t)";
			dynamic_cast<FloatProperty*>(myApp->animatorPanel->widgetToProperty[obj])->setDisplayValue(val);
		} else {
			if (obj->dim_id == 0) {
				val = "curvex(" + std::to_string(spline->id) + ",t)";
			} else	if (obj->dim_id == 1) {
				val = "curvey(" + std::to_string(spline->id) + ",t)";
			}
			dynamic_cast<PositionProperty*>(myApp->animatorPanel->widgetToProperty[obj])->setValue(scene->currentTime, obj->dim_id, val);
		}
		obj->SetValue(val);

		//*(myApp->floatstringcontrols[obj]) = val;

		myApp->infoBar->SetLabelText(std::to_string(evt.GetX()) + "    " + std::to_string(evt.GetY()));
		myApp->animatorPanel->needPanelUpdate = false;
		spline->SetPosition(scene->currentTime, myApp->animatorPanel->scene->oldPos);
		myApp->animatorPanel->scene->currentShape = myApp->animatorPanel->scene->previousShape;	
	}

	Plotter1D* plotter = dynamic_cast<Plotter1D*>(myApp->animatorPanel->scene->currentShape);
	if (plotter) {

		std::string val;
		if (obj->dim == 1) {
			val = "curvea(" + std::to_string(plotter->id) + ",t)";
			dynamic_cast<FloatProperty*>(myApp->animatorPanel->widgetToProperty[obj])->setDisplayValue(val);
		} else {
			if (obj->dim_id == 0) {
				val = "curvex(" + std::to_string(plotter->id) + ",t)";
			} else	if (obj->dim_id == 1) {
				val = "curvey(" + std::to_string(plotter->id) + ",t)";
			}
			dynamic_cast<PositionProperty*>(myApp->animatorPanel->widgetToProperty[obj])->setValue(scene->currentTime, obj->dim_id, val);
		}
		obj->SetValue(val);

		//*(myApp->floatstringcontrols[obj]) = val;

		myApp->infoBar->SetLabelText(std::to_string(evt.GetX()) + "    " + std::to_string(evt.GetY()));
		myApp->animatorPanel->needPanelUpdate = false;
		plotter->SetPosition(scene->currentTime, myApp->animatorPanel->scene->oldPos);
		myApp->animatorPanel->scene->currentShape = myApp->animatorPanel->scene->previousShape;
	}

	updateControlValues();
	evt.Skip();
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
	p->BSplineSubdivide(curPanel->scene->currentTime);
}

void AnimatorPanel::OnAddKeyframePointPopupClick(wxCommandEvent &evt) {
	AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
	wxMenu* menu = dynamic_cast<wxMenu*>(evt.GetEventObject());
	SpinRegex* spin = (SpinRegex*)(menu->GetClientData());

	FloatProperty* f = dynamic_cast<FloatProperty*>(curPanel->widgetToProperty[spin]);
	if (f) f->addKeyframe(scene->currentTime, spin->GetText()->GetValue().ToStdString());
	PositionProperty* p = dynamic_cast<PositionProperty*>(curPanel->widgetToProperty[spin]);
	if (p) p->addKeyframe(scene->currentTime,  p->getDisplayText() );// spin->dim_id, spin->GetText()->GetValue().ToStdString());

	
}

void AnimatorPanel::OnAddColorKeyframePointPopupClick(wxCommandEvent &evt) {
	AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
	wxMenu* menu = dynamic_cast<wxMenu*>(evt.GetEventObject());
	ClickableColourPicker* picker = (ClickableColourPicker*)(menu->GetClientData());
	ColorProperty* c = dynamic_cast<ColorProperty*>(curPanel->colorwidgetToProperty[picker]);
	if (c) c->addKeyframe(scene->currentTime, c->getDisplayValue(scene->currentTime));
}

void AnimatorPanel::OnAddTextKeyframePointPopupClick(wxCommandEvent &evt) {
	AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
	wxMenu* menu = dynamic_cast<wxMenu*>(evt.GetEventObject());
	wxTextCtrl* textc = (wxTextCtrl*)(menu->GetClientData());
	ExprProperty* e = dynamic_cast<ExprProperty*>(curPanel->textwidgetToProperty[textc]);
	if (e) e->addKeyframe(scene->currentTime, e->getDisplayValue(scene->currentTime));
}
void AnimatorPanel::OnAddFilenameKeyframePointPopupClick(wxCommandEvent &evt) {
	AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
	wxMenu* menu = dynamic_cast<wxMenu*>(evt.GetEventObject());
	ClickableFilePicker* picker = (ClickableFilePicker*)(menu->GetClientData());
	FilenameProperty* f = dynamic_cast<FilenameProperty*>(curPanel->filenamewidgetToProperty[picker]);
	if (f) f->addKeyframe(scene->currentTime, f->getDisplayValue(scene->currentTime));
}

wxControl* AnimatorApp::addObjectButton(const char* name, Shape* s, wxPanel* panel, wxPanel* parent) {

	static int objId = 1000;
	wxButton* objectButton = new wxButton(panel, objId, name, wxDefaultPosition, wxDefaultSize);
	
	Connect(objId, wxEVT_BUTTON, wxCommandEventHandler(AnimatorPanel::addShape), (wxObject*)new AddObjectParam(animatorPanel->scene, s), parent);
	objId++;
	return objectButton;
}

void AnimatorApp::DestroySizer(wxSizerItemList toremove) {
	//sizer->Clear(true);

	for (int i = 0; i < toremove.size(); i++) {
		wxSizerItem* item = toremove[i];
		item->DeleteWindows();				
	}
	std::vector<Shape*> &shapes = myApp->animatorPanel->scene->shapes;
	for (int i = 0; i < shapes.size(); i++) {
		if (shapes[i] != myApp->animatorPanel->scene->currentShape) {
			for (int j = 0; j < shapes[i]->parameters.size(); j++) {
				shapes[i]->parameters[j]->SetWidgetsNull();
			}
		}
	}
	myApp->panelProperties_sizer->Layout();
}

void AnimatorApp::setupPanelProperties(Shape* shape) {
	myApp->animatorPanel->widgetToProperty.clear();
	myApp->animatorPanel->colorwidgetToProperty.clear();
	myApp->animatorPanel->filenamewidgetToProperty.clear();
	myApp->animatorPanel->textwidgetToProperty.clear();

	wxSizerItemList sil = panelProperties_sizer->GetChildren();
	CallAfter(&AnimatorApp::DestroySizer, sil);

	for (int i = 0; i < shape->parameters.size(); i++) {
		shape->parameters[i]->CreateWidgets(animatorPanel->scene->currentTime);
	}
	

	/*for (int i = 0; i < animatorPanel->property_sizers.size(); i++) {
		try {
			//animatorPanel->property_sizers[i]->Clear(true);			
			panelProperties_sizer->Remove(animatorPanel->property_sizers[i]);
		} catch (...) {};
	}*/


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
	frame->Bind(wxEVT_CHAR_HOOK, &AnimatorPanel::keydown, animatorPanel);

	wxBoxSizer * statusbar_sizer = new wxBoxSizer(wxHORIZONTAL);
	infoBar = new wxStaticText(frame->GetStatusBar(), wxID_ANY, " ");
	statusbar_sizer->Add(infoBar);
	frame->GetStatusBar()->SetSizer(statusbar_sizer);

	m_bookCtrl = new wxNotebook(frame, wxID_ANY, wxDefaultPosition, wxSize(200, 100), 0);

	wxScrolled<wxPanel> *panelObject = new wxScrolled<wxPanel>(m_bookCtrl);
	panelObject->EnableScrolling(true, true);
	panelObject->SetScrollbars(5, 10, 20, 60);

	wxBoxSizer * panelObject_sizer = new wxBoxSizer(wxVERTICAL);
	
	panelObject_sizer->Add(addObjectButton("Disk", new Disk(Vec2s("50", "50"), "30", Vec3u(241, 198, 198), Vec3u(226,112,118), "5"), panelObject, animatorPanel) , 0, wxEXPAND);
	PolygonShape* defaultPolygon = new PolygonShape(Vec2s("175", "125"), "1", "0", Vec3u(87, 161, 139), Vec3u(0,63,92), "5");
	//PolygonShape* defaultPolygon = new PolygonShape(Vec2s("0", "0"), "1", "0", Vec3u(255, 0, 0), Vec3u(0, 0, 0), "2");
	defaultPolygon->addVertex(-1.f, Vec2f(150-175, 50 - 125));
	defaultPolygon->addVertex(-1.f, Vec2f(200 - 175, 90- 125));
	defaultPolygon->addVertex(-1.f, Vec2f(200 - 175, 150 - 125));
	defaultPolygon->addVertex(-1.f, Vec2f(150 - 175, 200 - 125));
	defaultPolygon->addVertex(-1.f, Vec2f(100 - 175, 150 - 125));
	defaultPolygon->addVertex(-1.f, Vec2f(100 - 175, 90 - 125));
	panelObject_sizer->Add(addObjectButton("Polygon", defaultPolygon, panelObject, animatorPanel), 0, wxEXPAND);


	wxButton* morphPolygonsButton = new wxButton(panelObject, 1100, "Morph Polygons", wxDefaultPosition, wxDefaultSize);
	Connect(1100, wxEVT_BUTTON, wxCommandEventHandler(AnimatorPanel::addMorphPolygon), NULL, animatorPanel);
	panelObject_sizer->Add(morphPolygonsButton, 0, wxEXPAND);
	
	Spline* defaultSpline = new Spline(Vec2s("0", "0"), "1", Vec3u(212, 61, 81), true, "5");
	defaultSpline->addVertex(-1.f, Vec2f(150, 50));
	defaultSpline->addVertex(-1.f, Vec2f(200, 90));
	defaultSpline->addVertex(-1.f, Vec2f(200, 150));
	defaultSpline->addVertex(-1.f, Vec2f(150, 200));
	defaultSpline->addVertex(-1.f, Vec2f(100, 150));
	defaultSpline->addVertex(-1.f, Vec2f(100, 90));
	panelObject_sizer->Add(addObjectButton("BSpline / Polylines", defaultSpline, panelObject, animatorPanel), 0, wxEXPAND);

	panelObject_sizer->Add(addObjectButton("1D Plot", new Plotter1D(Vec2s("200", "200"), "1", Vec3u(241, 198, 198)), panelObject, animatorPanel), 0, wxEXPAND);

	panelObject_sizer->Add(addObjectButton("Grid", new Grid(Vec2s("200", "200"), "1", Vec3u(241, 198, 198)), panelObject, animatorPanel), 0, wxEXPAND);

	panelObject_sizer->Add(addObjectButton("Point Set", new PointSet(Vec2s("200", "200"), "1", Vec3u(212, 61, 81)), panelObject, animatorPanel), 0, wxEXPAND);

	panelObject_sizer->Add(addObjectButton("Latex", new Latex(Vec2s("200", "200"), "1", Vec3u(212, 61, 81)), panelObject, animatorPanel), 0, wxEXPAND);


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
	Connect(123, wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(AnimatorPanel::update_time_values), NULL, animatorPanel);
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

	cur_img.removeAlpha();
	static int nbcalls = -1; nbcalls++;
	if (nbcalls == 0 || screenImage.GetWidth() != W || screenImage.GetHeight() != H) {
		screenImage = wxImage(W, H, &(cur_img.imgNoAlpha[0]), true);
	}

	bmpBuf = wxBitmap(screenImage/*, dc*/);


	double scale_x = 1; // (double)displayW / raytracer.W;
	double scale_y = 1; // (double)displayH / raytracer.H;
	dc.SetUserScale(scale_x, scale_y);
	dc.DrawBitmap(bmpBuf, 0, 0);
	dc.SetUserScale(1.0, 1.0);
}