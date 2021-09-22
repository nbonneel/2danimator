#pragma once

#include "wx/wx.h"
#include "wx/notebook.h"
#include <wx/clrpicker.h>
#include <wx/colordlg.h>
#include <wx/spinctrl.h>
#include "SpinRegex.h"
#include "wx/listctrl.h"
#include "Shape.h"
#include "Scene.h"
#include <map>

class AnimatorFrame;
class AnimatorApp;

extern AnimatorApp* myApp;

class AnimatorFrame : public wxFrame  // the main window
{
public:
	AnimatorFrame();
	void ExportSequence(wxCommandEvent &evt);	

private:

	//wxDECLARE_EVENT_TABLE();
};




struct AddObjectParam {
	AddObjectParam(Scene* scene, Shape* shapeType) :scene(scene), shapeType(shapeType) {};
	Scene* scene;
	Shape* shapeType;
};

class AnimatorPanel : public wxPanel
{
public:

	AnimatorPanel(AnimatorFrame* parent, AnimatorApp* app, int W, int H) :wxPanel(parent) {
		Connect(wxEVT_PAINT, wxPaintEventHandler(AnimatorPanel::paintEvent));
		
		scene = new Scene();
		animatorApp = app;
		cur_img = Canvas(W, H);
		this->W = W;
		this->H = H;		

		addingMorph = false;
		left_mouse_down = false;
		dragging = false;
		needPanelUpdate = false;
		moveVtxId = -1;
		moveCtrlId = -1;

		polygonMenu.Append(51, "Add Vertex Here");
		polygonMenu.Append(52, "Move Vertex");
		polygonMenu.Append(53, "BSpline Subdivide");
		changeShapeMenu.Append(54, "Text...");
		polygonMenu.AppendSubMenu(&changeShapeMenu, "Change Shape to...");

		splineMenu.Append(55, "Add Control Point Here");
		splineMenu.Append(56, "Move Control Point");

		spinCtrlMenu.Append(57, "Add Keyframe");
		colorCtrlMenu.Append(58, "Add Keyframe");
		

		//polygonMenu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorPanel::OnAddVtxPopupClick), this, this);
		Connect(51, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorPanel::OnAddVtxPopupClick), this, this);
		Connect(52, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorPanel::OnMoveVtxPopupClick), this, this);
		Connect(53, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorPanel::OnBSplineSubdivPopupClick), this, this);
		Connect(54, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorPanel::OnChangePolygonToTextPopupClick), this, this);

		Connect(55, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorPanel::OnAddCtrlPointPopupClick), this, this);
		Connect(56, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorPanel::OnMoveCtrlPointPopupClick), this, this);

		Connect(57, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorPanel::OnAddKeyframePointPopupClick), this, this);
		Connect(58, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(AnimatorPanel::OnAddColorKeyframePointPopupClick), this, this);
		
		//s.Add(new Disk(Vec2f(0, 0), 20, Vec3u(0, 0, 255)));
		start_display_thread();
	}

	void paintNow() {
		wxClientDC dc(this);
		render(dc);
	}
	void paintEvent(wxPaintEvent& evt) {	
		wxPaintDC dc(this);
		render(dc);
	}

	void render(wxDC& dc);

	void start_display_thread() {
		//compute_thread = CreateThread(NULL, 0, doRender, (void*)this, 0, NULL);
		//SetThreadPriority(compute_thread, THREAD_PRIORITY_HIGHEST);				
		//compute_thread2 = std::thread(doDisplay, (void*)this);
	}

	void addShape(wxCommandEvent& event);

	void addMorphPolygon(wxCommandEvent& event) {
		addingMorph = true;
	}
	void OnAddVtxPopupClick(wxCommandEvent &evt) {
		AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
		PolygonShape* p = dynamic_cast<PolygonShape*>(curPanel->scene->currentShape);
		Vec2f pos(curPanel->mouse_x, curPanel->mouse_y);
		float t = curPanel->scene->currentTime;
		float s = p->GetScale(t);
		float a = p->angle.getDisplayValue(t);
		Vec2f trans = p->GetPosition(t);
		int id_prev = p->nearestSegment(pos, s, a, trans, t);		
		p->insertVertex(t, rotate((pos - trans) / s, -a) , id_prev);
		moveVtxId = id_prev + 1;
	}

	void OnAddCtrlPointPopupClick(wxCommandEvent &evt) {
		AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
		Spline* p = dynamic_cast<Spline*>(curPanel->scene->currentShape);
		Vec2f pos(curPanel->mouse_x, curPanel->mouse_y);
		float t = curPanel->scene->currentTime;
		float s = p->GetScale(t);
		//float a = p->angle.eval1(t);
		Vec2f trans = p->GetPosition(t);
		int id_prev = p->nearestSegment(pos, s, 0., trans, t);
		p->insertControlPoint(t, rotate((pos - trans) / s, -0), id_prev);
		moveCtrlId = id_prev + 1;
	}
	void OnMoveVtxPopupClick(wxCommandEvent &evt) {
		AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
		PolygonShape* p = dynamic_cast<PolygonShape*>(curPanel->scene->currentShape);
		Vec2f pos(curPanel->mouse_x, curPanel->mouse_y);
		float t = curPanel->scene->currentTime;
		float s = p->GetScale(t);
		float a = p->angle.getDisplayValue(t);
		Vec2f trans = p->GetPosition(t);
		curPanel->moveVtxId = p->nearestVertex(pos, s, a, trans, 10, t);
	}
	void OnMoveCtrlPointPopupClick(wxCommandEvent &evt) {
		AnimatorPanel* curPanel = (AnimatorPanel*)evt.GetEventUserData();
		Spline* p = dynamic_cast<Spline*>(curPanel->scene->currentShape);
		Vec2f pos(curPanel->mouse_x, curPanel->mouse_y);
		float t = curPanel->scene->currentTime;
		float s = p->GetScale(t);
		//float a = p->angle.eval1(t);
		Vec2f trans = p->GetPosition(t);
		curPanel->moveCtrlId = p->nearestControlPoint(pos, s, 0, trans, 10, t);
	}
	void OnAddKeyframePointPopupClick(wxCommandEvent &evt);

	void OnAddColorKeyframePointPopupClick(wxCommandEvent &evt);

	void OnBSplineSubdivPopupClick(wxCommandEvent &evt);

	void OnChangePolygonToTextPopupClick(wxCommandEvent &evt);

	void update_time_values(wxCommandEvent& event);

	void updatePropertiesFromControls(wxCommandEvent& event);

	void editedVertex(wxListEvent& event);

	void updateControlValues();

	
	void OnSpinMouseCaptureLost(wxMouseCaptureLostEvent& event);

	void mouseMoved(wxMouseEvent& event);

	void mouseDown(wxMouseEvent& event);

	void mouseUp(wxMouseEvent& event);

	void spinMouseUp(wxMouseEvent &evt);

	void colourMouseUp(wxMouseEvent &evt);

	void mouseWheel(wxMouseEvent& event) {

		if (scene->currentShape) {
			scene->currentShape->SetScale(scene->currentTime,((event.GetWheelRotation() > 0) ? 1.1 : (1 / 1.1)) * scene->currentShape->GetScale(scene->currentTime));
			updateControlValues();
		}

	}

	void keydown(wxKeyEvent& event);

	//std::thread compute_thread2;

	bool left_mouse_down, addingMorph, needPanelUpdate, dragging;
	Canvas cur_img;
	wxImage screenImage;
	wxBitmap bmpBuf;
	int W, H, mouse_x, mouse_y, mouse_init_x, mouse_init_y;
	int moveVtxId, moveCtrlId;
	Scene* scene;
	AnimatorApp* animatorApp;
	wxMenu polygonMenu, changeShapeMenu, splineMenu, spinCtrlMenu, colorCtrlMenu;
	std::map<SpinRegex*, Property*> widgetToProperty;
	std::map<ClickableColourPicker*, Property*> colorwidgetToProperty;

	DECLARE_EVENT_TABLE();
};



class AnimatorApp : public wxApp {
public:
	virtual bool OnInit();

	void OnIdle(wxIdleEvent& evt) {
		if (render_loop_on) {
			animatorPanel->paintNow();
			evt.RequestMore(); // render continuously, not only once on idle				
		}
	}
	void OnEraseBackGround(wxEraseEvent& event) {
		//wxDC * TheDC = event.GetDC();		
		event.Skip(); // don't call -- not needed, we already erased the background
	}

	void activateRenderLoop(bool on);
	wxControl* addObjectButton(const char* name, Shape* s, wxPanel* panel, wxPanel* parent);
	void setupPanelProperties(Shape* shape);

	void render_animation(const char* filename);

	void DestroySizer(const wxSizerItemList &toremove);

	bool render_loop_on;
	AnimatorPanel* animatorPanel;
	AnimatorFrame *animatorFrame;
	wxSlider* time_slider;
	wxNotebook* m_bookCtrl;
	wxScrolled<wxPanel>* propertiesPanel;
	wxBoxSizer * panelProperties_sizer;

	wxStaticText* infoBar;

	DECLARE_EVENT_TABLE();
};




