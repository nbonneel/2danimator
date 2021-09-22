#pragma once


#include "wx/wx.h"

#include "wx/notebook.h"
#include <wx/clrpicker.h>
#include <wx/colordlg.h>
#include "ClickableColourPicker.h"
#include <wx/spinctrl.h>
#include "wx/listctrl.h"
#include "spinRegex.h"
#include "wx/event.h"
#include <map>
#include "Vector.h"


static int controlID = 500;

void RGB2LAB(int R_value, int G_value, int B_value, double *Lab);
void LAB2RGB(double* Lab, unsigned char& R, unsigned char& G, unsigned char& B);

template<typename T>
class Interpolator {
public:
	Interpolator(const T &defaultValue = T()) {	
		displayValue = defaultValue;
	}
	typename std::map<float, T>::const_iterator find_last_key(float time) const {
		if (time <= values.begin()->first) return values.begin();
		typename std::map<float, T>::const_iterator prev_it = values.begin();
		typename std::map<float, T>::const_iterator it = values.begin(); ++it;
		for (; it != values.end(); ++it) {
			if (it->first > time) {
				return prev_it;
			}
			prev_it = it;
		}
		return prev_it;
	}

	const T getValue(float time) const {
		/// .... TODO		
		if (values.size() == 0) return displayValue;
		
		typename std::map<float, T>::const_iterator it = find_last_key(time);
		if (time <= it->first) return it->second;
		typename std::map<float, T>::const_iterator nextit = next(it);
		if (nextit == values.end()) {
			return it->second;
		}
		if (it != values.end()) {
			return it->second;
			//float f = (time - it->first) / (nextit->first - it->first);
			//return nextit->second*f + it->second*(1.f-f);
		} 
	};
	/*T& getValue(float time) {
		/// .... TODO
		if (values.size() == 0) return displayValue;

		typename std::map<float, T>::iterator it = values.find(time);
		if (it != values.end()) {
			return it->second;
		} 
	};*/
	const T getDisplayValue() const {
		return displayValue;
	}
	T& getDisplayValue() {
		return displayValue;
	}
	void setValue(float time, const T& value) {
		values[time] = value;
	}
	void setDisplayValue(const T& value) {
		displayValue = value;
	}
	bool hasKeyframe(float time) const {
		return (values.find(time) != values.end());
	}
	std::map<float, T> values;
	T displayValue;
};

template<>
const Expr Interpolator<Expr>::getValue(float time) const {
	/// .... TODO		
	if (values.size() == 0) return displayValue;

	typename std::map<float, Expr>::const_iterator it = find_last_key(time);
	if (time <= it->first) return it->second;
	typename std::map<float, Expr>::const_iterator nextit = next(it);
	if (nextit == values.end()) {
		return it->second;
	}
	if (it != values.end()) {
		Expr e = it->second;
		double lval;
		if (!wxString(e[0]).ToDouble(&lval)) {
			return e;
		} else {
			float lval2 = eval_string(nextit->second[0], time);
			float f = (time - it->first) / (nextit->first - it->first);
			return Expr(std::to_string(lval2 *f + lval * (1.f - f)));
		}

	}
};

template<>
const Vec2s Interpolator<Vec2s>::getValue(float time) const {
	/// .... TODO		
	if (values.size() == 0) return displayValue;
	typename std::map<float, Vec2s>::const_iterator it = find_last_key(time);
	if (time <= it->first) return it->second;
	typename std::map<float, Vec2s>::const_iterator nextit = next(it);
	if (nextit == values.end()) {
		return it->second;
	}
	if (it != values.end()) {
		Vec2s e = it->second;
		double lvalx;
		double lvaly;
		bool isFloatX = wxString(e[0]).ToDouble(&lvalx);
		bool isFloatY = wxString(e[1]).ToDouble(&lvaly);
		if (!isFloatX || !isFloatY) {
			return e;
		} else {
			float lval2x = eval_string(nextit->second[0], time);
			float lval2y = eval_string(nextit->second[1], time);
			float f = (time - it->first) / (nextit->first - it->first);
			std::string s1 = std::to_string(lval2x *f + lvalx * (1.f - f));
			std::string s2 = std::to_string(lval2y *f + lvaly * (1.f - f));
			return Vec2s(s1, s2);
		}

	}
};

template<>
const Vec3u Interpolator<Vec3u>::getValue(float time) const {
	if (values.size() == 0) return displayValue;
	typename std::map<float, Vec3u>::const_iterator it = find_last_key(time);
	if (time <= it->first) return it->second;
	typename std::map<float, Vec3u>::const_iterator nextit = next(it);
	if (nextit == values.end()) {
		return it->second;
	}
	if (it != values.end()) {
		Vec3u e = it->second;
		Vec3u ne = nextit->second;
		float f = (time - it->first) / (nextit->first - it->first);
		double Lab1[3], Lab2[3], Labi[3];
		RGB2LAB(e[0], e[1], e[2], Lab1);
		RGB2LAB(ne[0], ne[1], ne[2], Lab2);
		Labi[0] = Lab2[0] * f + (1. - f)*Lab1[0];
		Labi[1] = Lab2[1] * f + (1. - f)*Lab1[1];
		Labi[2] = Lab2[2] * f + (1. - f)*Lab1[2];
		Vec3u res;
		LAB2RGB(Labi, res[0], res[1], res[2]);
		return res;
	}
};

class Property {
public:
	virtual void CreateWidgets(float time) = 0;
	virtual void UpdateWidgets(float time) = 0;
	virtual void UpdateParameterFromWidget(float time) = 0;
	virtual void UpdateInternalTime(float time) {};
	virtual void addKeyframe(float time) = 0;
	virtual void SetWidgetsNull() = 0;
};

class PositionProperty : public Property {
public :
	PositionProperty(Vec2s defaultPosition = Vec2s("0", "0")) : position(defaultPosition){};
	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		Vec2s v = position.getDisplayValue();
		if (propFloat1) {
			propFloat1->tval = time;
			propFloat1->SetValue(v[0]);
			propFloat1->GetText()->SetValue(v[0]);
			if (position.hasKeyframe(time)) {
				propFloat1->SetBackgroundColour(wxColour(255, 0, 0));
			} else {
				propFloat1->SetBackgroundColour(wxColour(255, 255, 255));
			}
		}
		if (propFloat2) {
			propFloat2->tval = time;
			propFloat2->SetValue(v[1]);
			propFloat2->GetText()->SetValue(v[1]);
			if (position.hasKeyframe(time)) {				
				propFloat2->SetBackgroundColour(wxColour(255, 0, 0));
			} else {				
				propFloat2->SetBackgroundColour(wxColour(255, 255, 255));
			}
		}
	}; 
	virtual void UpdateParameterFromWidget(float time) {
		position.setDisplayValue(Vec2s(propFloat1->GetText()->GetValue().ToStdString(), propFloat2->GetText()->GetValue().ToStdString()));
	}
	virtual void UpdateInternalTime(float time) {
		if (propFloat1) propFloat1->tval = time;
		if (propFloat2) propFloat2->tval = time;
		position.setDisplayValue(position.getValue(time));
	}

	//Vec2f eval(float time) const { Vec2s v = position.getValue(time);  return Vec2f(eval_string(v[0], time), eval_string(v[1], time)); }
	void setValue(float time, const Vec2s &value) { 
		position.setValue(time, value); 
	}
	void setValue(float time, int dim, const Expr &value) { 
		Vec2s e = position.getValue(time);
		e[dim] = value.coords[0];
		position.setValue(time, e); 
	}
	void addKeyframe(float time, int dim, std::string val) {
		setValue(time, dim, val);
	}
	void addKeyframe(float time, const Vec2s &val) {
		setValue(time, val);
	}
	virtual void addKeyframe(float time) {
		setValue(time, getDisplayText());
	}
	Vec2f getDisplayValue(float time) const {
		double lab[3];
		unsigned char rgb[3];
		Vec2s v = position.getDisplayValue();
		return Vec2f(eval_string(v[0], time), eval_string(v[1], time));
	}
	Vec2s getDisplayText() const {
		Vec2s v = position.getDisplayValue();
		return v;
	}
	void setDisplayValue(const Vec2s &val) {
		return position.setDisplayValue(val);
	}
	void SetWidgetsNull() {
		propFloat1 = NULL;
		propFloat2 = NULL;
	}
	Interpolator<Vec2s> position;
	SpinRegex *propFloat1, *propFloat2;
};

class FloatProperty : public Property {
public:
	FloatProperty(Expr defaultValue = Expr("1")) : value(defaultValue), minVal(-100.f), maxVal(100.f), step(0.05f), name("Scale") {};
	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		Expr v = value.getDisplayValue();
		propFloat1->tval = time;
		propFloat1->SetValue(v[0]);
		propFloat1->GetText()->SetValue(v[0]);
		if (value.hasKeyframe(time)) 
			propFloat1->SetBackgroundColour(wxColour(255,0,0));
		else
			propFloat1->SetBackgroundColour(wxColour(255,255,255));
	};
	virtual void UpdateParameterFromWidget(float time) {
		value.setDisplayValue(propFloat1->GetText()->GetValue().ToStdString());
	}
	virtual void UpdateInternalTime(float time) {
		if (propFloat1)
			propFloat1->tval = time;
		value.setDisplayValue(value.getValue(time));
	}
	//float eval(float time) const { return eval_string(value.getValue(time).coords[0], time); }
	void setValue(float time, const Expr &val) { return value.setValue(time, val); }
	void setDefaults(std::string name, float minVal, float maxVal, float step) { this->minVal = minVal; this->maxVal = maxVal; this->name = name; this->step = step; };
	void addKeyframe(float time, float val) {
		value.setValue(time, std::to_string(val));
	}
	void addKeyframe(float time, std::string val) {
		value.setValue(time, val);
	}
	virtual void addKeyframe(float time) {
		value.setValue(time, Expr(getDisplayText(time)));
	}
	float getDisplayValue(float time) const {
		return eval_string(value.getDisplayValue().coords[0], time);
	}
	std::string getDisplayText(float time) const {
		return value.getDisplayValue().coords[0];
	}
	void setDisplayValue(const Expr &val) { 
		return value.setDisplayValue(val); 
	}
	void SetWidgetsNull() {
		propFloat1 = NULL;
	}

	Interpolator<Expr> value;
	SpinRegex *propFloat1;
	std::string name;
	float minVal, maxVal, step;
};

class ColorProperty : public Property {
public:
	ColorProperty(const Vec3u& defaultColor = Vec3u(0,0,0), std::string propertyName = "Color"):color(defaultColor), propertyName(propertyName){};
	void setName(std::string propertyName) { this->propertyName = propertyName; }
	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		if (!colorPicker) return;
		Vec3u col = color.getDisplayValue();
		colorPicker->SetColour(wxColour(col[0], col[1], col[2]));
		if (color.hasKeyframe(time))
			colorPicker->SetBackgroundColour(wxColour(255, 0, 0));
		else
			colorPicker->SetBackgroundColour(wxColour(255, 255, 255));
	}; 

	//Vec3u eval(float time) const { return color.getValue(time); }
	void setValue(float time, const Vec3u &value) { color.setValue(time, value); }

	virtual void UpdateParameterFromWidget(float time) {
		wxColor col = colorPicker->GetColour();
		color.setDisplayValue( Vec3u(col.Red(), col.Green(), col.Blue()));
	}
	Vec3u getDisplayValue(float time) const {
		return color.getDisplayValue();
	}
	void setDisplayValue(const Vec3u &val) {
		return color.setDisplayValue(val);
	}
	void addKeyframe(float time, const Vec3u &val) {
		setValue(time, val);
	}
	virtual void addKeyframe(float time) {
		setValue(time, getDisplayValue(time));
	}
	virtual void UpdateInternalTime(float time) {
		color.setDisplayValue(color.getValue(time));
	}
	void SetWidgetsNull() {
		colorPicker = NULL;
	}

	Interpolator<Vec3u> color;
	std::string propertyName;
	ClickableColourPicker *colorPicker;
};


class VerticesListProperty : public Property {
public:
	VerticesListProperty(){};
	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		if (!propListVertices) return;		

		VerticesList vl = vertices.getDisplayValue();
		int N = vl.size();
		if (propListVertices->GetItemCount() != N) {
			propListVertices->DeleteAllItems();			
			for (int i = 0; i < N; i++) {
				float* coord = &vl.vertices[i].coords[0];
				std::string txt = "(" + std::to_string(*(coord)) + ", " + std::to_string(*(coord + 1)) + ")"; // *3 = 2 floats (coords) + 1 address (sizeof(firstParam))
				wxListItem item;
				item.SetId(i);
				long index = propListVertices->InsertItem(i, item);
				propListVertices->SetItem(index, 0, txt, -1);
			}
		} else {
			for (int i = 0; i < N; i++) {
				float* coord = &vl.vertices[i].coords[0];
				std::string txt = "(" + std::to_string(*(coord)) + ", " + std::to_string(*(coord + 1)) + ")"; // *3 = 2 floats (coords) + 1 address (sizeof(firstParam))
				propListVertices->SetItem(i, 0, txt, -1);
			}

		}
	};
	virtual void UpdateParameterFromWidget(float time) {
		/// TODO ??
	}

	//VerticesList eval(float time) const { return vertices.getValue(time); }
	void addVertex(float time, const Vec2f& v, bool createNewContour = false) { 
		VerticesList ev = vertices.getDisplayValue();
		vertices.setDisplayValue( ev.addVertex(v, createNewContour));
	}
	void insertVertex(float time, const Vec2f& v, int prevVertex) { 
		VerticesList ev = vertices.getDisplayValue();
		vertices.setDisplayValue( ev.insertVertex(v, prevVertex));
	}
	void setValue(float time, const VerticesList &value) { vertices.setValue(time, value); }
	void editVertex(float time, int vertexId, const Vec2f& value) {
		VerticesList evalV = vertices.getDisplayValue();
		evalV.vertices[vertexId] = value;
		vertices.setDisplayValue( evalV);
	}
	VerticesList getDisplayValue() const {
		return vertices.getDisplayValue();
	}
	virtual void addKeyframe(float time) {
		setValue(time, getDisplayValue());
	}
	void setDisplayValue(const VerticesList &val) {
		return vertices.setDisplayValue(val);
	}
	void SetWidgetsNull() {
		propListVertices = NULL;
	}
	Interpolator<VerticesList> vertices;
	wxListCtrl* propListVertices;
};

class VisibleProperty : public Property {
public:
	VisibleProperty(bool defaultVisibility = true) : visible(defaultVisibility) {};
	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		bool v = visible.getValue(time);
		if (propBool) 
			propBool->SetValue(v);
	}; 
	virtual void UpdateParameterFromWidget(float time) {
		if (propBool)
			visible.setValue(time, propBool->GetValue());
	}
	bool getDisplayValue() const {
		return visible.getDisplayValue();
	}
	void setValue(float time, const bool &value) { visible.setValue(time, value); }

	virtual void addKeyframe(float time) {
		setValue(time, getDisplayValue());
	}
	bool eval(float time) const { return visible.getValue(time); }

	void SetWidgetsNull() {
		propBool = NULL;
	}
	Interpolator<bool> visible;
	wxCheckBox* propBool;
};