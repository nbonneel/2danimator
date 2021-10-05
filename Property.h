#pragma once


#include "wx/wx.h"

#include "wx/notebook.h"
#include <wx/clrpicker.h>
#include <wx/colordlg.h>
#include "ClickableColourPicker.h"
#include "ClickableFilePicker.h"
#include "wx/filepicker.h"
#include <wx/spinctrl.h>
#include "wx/listctrl.h"
#include "spinRegex.h"
#include "wx/event.h"
#include <map>
#include "Vector.h"
#include "wx/combobox.h"
#include <fstream>

static int controlID = 500;

void RGB2LAB(int R_value, int G_value, int B_value, double *Lab);
void LAB2RGB(double* Lab, unsigned char& R, unsigned char& G, unsigned char& B);

class PositionProperty;
class FloatProperty;
class IntProperty;
class ColorProperty;
class VerticesListProperty;
class BoolProperty;
class ExprProperty;
class FilenameProperty;
class PointSetProperty;

class Points {
public:
	Points() : dim(0){}
	void clear() { coords.clear(); }
	void reserve(size_t n) { coords.reserve(n); }
	int npoints() const { return coords.size() / dim; };
	std::vector<float> coords;
	int dim;

	friend std::ostream& operator<<(std::ostream& os, const Points& v) {
		os << v.dim << std::endl;
		os << v.coords.size() << std::endl;
		for (int i=0; i< v.coords.size(); i++)
			os << v.coords[i] << " ";
		return os;
	}
	friend std::istream& operator>>(std::istream& is, Points& v) {
		is >> v.dim;
		int s;
		is >> s;
		v.coords.resize(s);
		for (int i = 0; i < s; i++) {
			is >> v.coords[i];
		}
		return is;
	}
};



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
	friend std::ostream& operator<<(std::ostream& os, const Interpolator<T>& v) {
		os << v.values.size() << std::endl;
		for (typename std::map<float, T>::const_iterator it = v.values.begin(); it != v.values.cend(); ++it)
			os << it->first << " " << it->second << std::endl;
		os << v.displayValue;
		return os;
	}
	friend std::istream& operator>>(std::istream& is, Interpolator<T>& ve) {
		int nvalues;
		is >> nvalues;
		for (int i = 0; i < nvalues; i++) {
			float fl;
			T v;
			is >> fl;
			is >> v;
			ve.values[fl] = v;
		}
		is >> ve.displayValue;
		return is;
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

template<>
const VerticesList Interpolator<VerticesList>::getValue(float time) const {
	if (values.size() == 0) return displayValue;
	typename std::map<float, VerticesList>::const_iterator it = find_last_key(time);
	if (time <= it->first) return it->second;
	typename std::map<float, VerticesList>::const_iterator nextit = next(it);
	if (nextit == values.end()) {
		return it->second;
	}
	if (it != values.end()) {
		VerticesList e = it->second;
		VerticesList ne = nextit->second;
		VerticesList res;
		float f = (time - it->first) / (nextit->first - it->first);
		for (int i = 0; i < std::min(e.size(), ne.size()); i++) {
			res.vertices.push_back(e.vertices[i]*(1.f-f) + ne.vertices[i]*f);
		}
		res.contourList = e.contourList;
		return res;
	}
};

template<>
const Points Interpolator<Points>::getValue(float time) const {
	if (values.size() == 0) return displayValue;
	typename std::map<float, Points>::const_iterator it = find_last_key(time);
	if (time <= it->first) return it->second;
	typename std::map<float, Points>::const_iterator nextit = next(it);
	if (nextit == values.end()) {
		return it->second;
	}
	if (it != values.end()) {
		Points e = it->second;
		Points ne = nextit->second;
		Points res;
		float f = (time - it->first) / (nextit->first - it->first);
		res.dim = std::min(e.dim, ne.dim);
		int npts = std::min(e.npoints(), ne.npoints());
		res.reserve(res.dim*npts);
		for (int i = 0; i < npts; i++) {
			for (int j = 0; j < res.dim; j++) {
				res.coords.push_back(e.coords[i*e.dim+j] * (1.f - f) + ne.coords[i*ne.dim+j] * f);
			}
		}		
		return res;
	}
};

class Property {
public:
	Property(std::string type = "") :propType(type) {};
	virtual void CreateWidgets(float time) = 0;
	virtual void UpdateWidgets(float time) = 0;
	virtual void UpdateParameterFromWidget(float time) = 0;
	virtual void UpdateInternalTime(float time) {};
	virtual void addKeyframe(float time) = 0;
	virtual void SetWidgetsNull() = 0;
	virtual void print(std::ostream& os) const = 0;
	virtual void read(std::istream& is) = 0;

	friend std::istream& operator>>(std::istream& is, Property& v) {
		v.read(is);
		return is;
	}

	friend std::ostream& operator<<(std::ostream& os, const Property& v) {
		v.print(os);
		return os;
	}
	std::string propType;
};

std::ostream& operator<<(std::ostream& os, const Property* v);
std::istream& operator>>(std::istream& is, Property* &v);

class PositionProperty : public Property {
public :
	PositionProperty(Vec2s defaultPosition = Vec2s("0", "0")) : position(defaultPosition), name("Position"), Property("PositionProperty"){};
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
	virtual void print(std::ostream& os) const {
		os << name << std::endl;
		os << position;		
	}
	virtual void read(std::istream& is) {
		char line[255];
		do {
			is.getline(line, 255);
			name = std::string(line);
		} while (name == "");
		is >> position;
	}

	Interpolator<Vec2s> position;
	SpinRegex *propFloat1, *propFloat2;
	std::string name;
};

class FloatProperty : public Property {
public:
	FloatProperty(Expr defaultValue = Expr("1")) : value(defaultValue), minVal(-100.f), maxVal(100.f), step(0.05f), name("Scale"), Property("FloatProperty") {};
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
	virtual void print(std::ostream& os) const {
		os << name << std::endl;
		os << value << std::endl;
		os << minVal<<" "<<maxVal<<" "<<step;
	}
	virtual void read(std::istream& is) {
		char line[255];
		do {
			is.getline(line, 255);
			name = std::string(line);
		} while (name == "");
		is >> value;
		is >> minVal;
		is >> maxVal; 
		is >> step;
	}
	Interpolator<Expr> value;
	SpinRegex *propFloat1;
	std::string name;
	float minVal, maxVal, step;
};

class IntProperty : public Property {
public:
	IntProperty(int defaultValue = 0) : value(defaultValue), minVal(0), maxVal(100), name("Nb X ticks"), Property("IntProperty") {};
	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		int v = value.getDisplayValue();
		propInt->SetValue(v);
		if (value.hasKeyframe(time))
			propInt->SetBackgroundColour(wxColour(255, 0, 0));
		else
			propInt->SetBackgroundColour(wxColour(255, 255, 255));
	};
	virtual void UpdateParameterFromWidget(float time) {
		value.setDisplayValue(propInt->GetValue());
	}
	virtual void UpdateInternalTime(float time) {
		value.setDisplayValue(value.getValue(time));
	}
	//float eval(float time) const { return eval_string(value.getValue(time).coords[0], time); }
	void setValue(float time, int val) { return value.setValue(time, val); }
	void setDefaults(std::string name, int minVal, int maxVal, int curVal = 0) { this->minVal = minVal; this->maxVal = maxVal; this->name = name;  };

	virtual void addKeyframe(float time) {
		value.setValue(time, getDisplayValue(time));
	}
	int getDisplayValue(float time) const {
		return value.getDisplayValue();
	}
	void setDisplayValue(int val) {
		return value.setDisplayValue(val);
	}
	void SetWidgetsNull() {
		propInt = NULL;
	}
	virtual void print(std::ostream& os) const {
		os << name << std::endl;
		os << value << std::endl;
		os << minVal << " " << maxVal;
	}
	virtual void read(std::istream& is) {
		char line[255];
		do {
			is.getline(line, 255);
			name = std::string(line);
		} while (name == "");
		is >> value;
		is >> minVal;
		is >> maxVal;
	}
	Interpolator<int> value;
	wxSpinCtrl *propInt;
	std::string name;
	int minVal, maxVal;
};


class ColorProperty : public Property {
public:
	ColorProperty(const Vec3u& defaultColor = Vec3u(0,0,0), std::string propertyName = "Color"):color(defaultColor), propertyName(propertyName), Property("ColorProperty") {};
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
	virtual void print(std::ostream& os) const {
		os << propertyName << std::endl;
		os << color;
	}
	virtual void read(std::istream& is) {
		char line[255];
		do {
			is.getline(line, 255);
			propertyName = std::string(line);
		} while (propertyName == "");
		is >> color;
	}
	Interpolator<Vec3u> color;
	std::string propertyName;
	ClickableColourPicker *colorPicker;
};


class VerticesListProperty : public Property {
public:
	VerticesListProperty(): Property("VerticesListProperty"){};
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
		for (int i = 0; i < propListVertices->GetItemCount(); i++) {
			wxString s = propListVertices->GetItemText(i, 0);
			float f1, f2;
			sscanf(s.c_str(), "(%f, %f)", &f1, &f2);
			vertices.displayValue.vertices[i] = Vec2f(f1, f2);
		}		
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
	virtual void UpdateInternalTime(float time) {
		vertices.setDisplayValue(vertices.getValue(time));
	}
	virtual void print(std::ostream& os) const {
		os << vertices;
	}
	virtual void read(std::istream& is) {
		is >> vertices;
	}
	Interpolator<VerticesList> vertices;
	wxListCtrl* propListVertices;
};

class BoolProperty : public Property {
public:
	BoolProperty(bool defaultValue = true) : value(defaultValue), name("Visible"), Property("BoolProperty") {};

	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		bool v = value.getDisplayValue();
		if (propBool) 
			propBool->SetValue(v);
	}; 
	virtual void UpdateParameterFromWidget(float time) {
		if (propBool)
			value.setDisplayValue(propBool->GetValue());
	}
	bool getDisplayValue() const {
		return value.getDisplayValue();
	}
	void setValue(float time, const bool &value) { this->value.setValue(time, value); }

	virtual void addKeyframe(float time) {
		setValue(time, getDisplayValue());
	}
	//bool eval(float time) const { return visible.getValue(time); }
	virtual void UpdateInternalTime(float time) {
		value.setDisplayValue(value.getValue(time));
	}

	void SetWidgetsNull() {
		propBool = NULL;
	}
	virtual void print(std::ostream& os) const {
		os << name << std::endl;
		os << value;
	}
	virtual void read(std::istream& is) {
		char line[255];
		do {
			is.getline(line, 255);
			name = std::string(line);
		} while (name == "");
		is >> value;
	}
	Interpolator<bool> value;
	std::string name;
	wxCheckBox* propBool;
};


class ExprProperty : public Property {
public:
	ExprProperty(std::string defaultValue = "x") : value(defaultValue), Property("ExprProperty") {};
	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		std::string v = value.getDisplayValue();
		propString->SetValue(v);
		if (value.hasKeyframe(time))
			propString->SetBackgroundColour(wxColour(255, 0, 0));
		else
			propString->SetBackgroundColour(wxColour(255, 255, 255));
	};
	virtual void UpdateParameterFromWidget(float time) {
		value.setDisplayValue(propString->GetValue().ToStdString());
	}
	virtual void UpdateInternalTime(float time) {
		value.setDisplayValue(value.getValue(time));
	}
	//float eval(float time) const { return eval_string(value.getValue(time).coords[0], time); }
	void setValue(float time, const std::string &val) { return value.setValue(time, val); }
	
	void addKeyframe(float time, std::string val) {
		value.setValue(time, val);
	}
	virtual void addKeyframe(float time) {
		value.setValue(time, getDisplayValue(time));
	}
	std::string getDisplayValue(float time) const {
		return value.getDisplayValue();
	}

	void setDisplayValue(const std::string &val) {
		return value.setDisplayValue(val);
	}
	void SetWidgetsNull() {
		propString = NULL;
	}
	virtual void print(std::ostream& os) const {
		os << value;
	}
	virtual void read(std::istream& is) {
		is >> value;
	}
	Interpolator<std::string> value;
	wxTextCtrl *propString;
};

class FilenameProperty : public Property {
public:
	FilenameProperty(std::string defaultValue = "", std::string fileName = "File") :filename(defaultValue), propertyName(propertyName), Property("FilenameProperty") {};
	void setName(std::string propertyName) { this->propertyName = propertyName; }
	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		if (!filePicker) return;
		std::string fil = filename.getDisplayValue();
		filePicker->SetFileName(wxFileName(fil));
		if (filename.hasKeyframe(time))
			filePicker->SetBackgroundColour(wxColour(255, 0, 0));
		else
			filePicker->SetBackgroundColour(wxColour(255, 255, 255));
	};

	//Vec3u eval(float time) const { return color.getValue(time); }
	void setValue(float time, const std::string &value) { filename.setValue(time, value); }

	virtual void UpdateParameterFromWidget(float time) {
		std::string name = filePicker->GetFileName().GetFullPath().ToStdString();
		filename.setDisplayValue(name);
	}
	std::string getDisplayValue(float time) const {
		return filename.getDisplayValue();
	}
	void setDisplayValue(const std::string &val) {
		return filename.setDisplayValue(val);
	}
	void addKeyframe(float time, const std::string &val) {
		setValue(time, val);
	}
	virtual void addKeyframe(float time) {
		setValue(time, getDisplayValue(time));
	}
	virtual void UpdateInternalTime(float time) {
		filename.setDisplayValue(filename.getValue(time));
	}
	void SetWidgetsNull() {
		filePicker = NULL;
	}
	virtual void print(std::ostream& os) const {
		os << propertyName << std::endl;
		os << filename;
	}
	virtual void read(std::istream& is) {
		char line[255];
		do {
			is.getline(line, 255);
			propertyName = std::string(line);
		} while (propertyName == "");
		is >> filename;
	}
	Interpolator<std::string> filename;
	std::string propertyName;
	ClickableFilePicker  *filePicker;
};

class PointSetProperty : public Property {
public:
	PointSetProperty() : Property("PointSetProperty"), propertyName("Point set"){ };
	void setName(std::string propertyName) { this->propertyName = propertyName; }

	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		if (!filePicker) return;
		std::string fil = filename.getDisplayValue();
		filePicker->SetFileName(wxFileName(fil));
		if (filename.hasKeyframe(time))
			filePicker->SetBackgroundColour(wxColour(255, 0, 0));
		else
			filePicker->SetBackgroundColour(wxColour(255, 255, 255));
	};
	
	Points load(std::string fname)  {
		FILE *f = fopen(fname.c_str(), "r+");
		Points pts;
		if (!f) return pts;
		
		char line[4096];
		if (!fgets(line, 4096, f)) return pts;
		int offset;
		char* lineptr = line;
		pts.dim = 0;
		float v;
		int read = 1;
		while (read == 1) {
			read = sscanf(lineptr, "%f%n", &v, &offset);
			lineptr = lineptr + offset;
			if (read == 1)
				pts.dim++;
		}
		fclose(f);
		pts.clear();
		pts.reserve(100 * pts.dim);
		f = fopen(fname.c_str(), "r+");
		while (!feof(f)) {

			char* lineptr = line;
			if (!fgets(line, 255, f)) break;
			float v;
			float v1, v2;
			for (int i = 0; i < pts.dim; i++) {
				int offset;
				int ret = sscanf(lineptr, "%f%n", &v, &offset);
				if (ret==1)
					pts.coords.push_back(v);
				lineptr = lineptr + offset;
			}
		}
		fclose(f);
		return pts;
	}

	void setValue(float time, const std::string &value) { 
		filename.setValue(time, value); 
		points.setValue(time, load(value));
	}

	virtual void UpdateParameterFromWidget(float time) {
		std::string name = filePicker->GetFileName().GetFullPath().ToStdString();
		filename.setDisplayValue(name);
		points.setDisplayValue(load(name));
	}

	Points getDisplayValue(float time) const {
		return points.getDisplayValue();
	}
	void setDisplayValue(const std::string &fname) {
		filename.setDisplayValue(fname);
		return points.setDisplayValue(load(fname));
	}
	void addKeyframe(float time, const std::string &fname) {
		setValue(time, fname);
	}
	virtual void addKeyframe(float time) {
		setValue(time, filename.getDisplayValue());
	}
	virtual void UpdateInternalTime(float time) {
		points.setDisplayValue(points.getValue(time));
		filename.setDisplayValue(filename.getValue(time));
	}
	void SetWidgetsNull() {
		filePicker = NULL;
	}

	virtual void print(std::ostream& os) const {
		os << propertyName << std::endl;
		os << filename << std::endl;
		os << points;
	}
	virtual void read(std::istream& is) {
		char line[255];
		do {
			is.getline(line, 255);
			propertyName = std::string(line);
		} while (propertyName == "");
		is >> filename;
		is >> points;
	}

	Interpolator<std::string> filename;
	Interpolator<Points> points;
	std::string propertyName;
	ClickableFilePicker  *filePicker;
};

class EnumProperty : public Property {
public:
	EnumProperty(int val) : name("options"), Property("EnumProperty"), value(val) {};
	virtual void CreateWidgets(float time);
	virtual void UpdateWidgets(float time) {
		int v = value.getDisplayValue();
		if (v < 0) v = 0;
		propEnum->SetValue(options[v]);
		propEnum->Select(v);

		if (value.hasKeyframe(time))
			propEnum->SetBackgroundColour(wxColour(255, 0, 0));
		else
			propEnum->SetBackgroundColour(wxColour(255, 255, 255));
	};
	virtual void UpdateParameterFromWidget(float time) {
		value.setDisplayValue(propEnum->GetSelection());
	}
	virtual void UpdateInternalTime(float time) {
		value.setDisplayValue(value.getValue(time));
	}
	//float eval(float time) const { return eval_string(value.getValue(time).coords[0], time); }
	void setValue(float time, int val) { return value.setValue(time, val); }
	void setDefaults(std::string name, const std::string* values, int nvalues) { this->name = name; options.clear();  for (int i = 0; i < nvalues; i++) options.push_back(values[i]); };

	virtual void addKeyframe(float time) {
		value.setValue(time, getDisplayValue(time));
	}
	int getDisplayValue(float time) const {
		return value.getDisplayValue();
	}
	void setDisplayValue(int val) {
		return value.setDisplayValue(val);
	}
	void SetWidgetsNull() {
		propEnum = NULL;
	}
	virtual void print(std::ostream& os) const {
		os << name << std::endl;
		os << value << std::endl;
	}
	virtual void read(std::istream& is) {
		char line[255];
		do {
			is.getline(line, 255);
			name = std::string(line);
		} while (name == "");
		is >> value;
	}
	Interpolator<int> value;
	wxComboBox *propEnum;
	std::string name;
	std::vector<std::string> options;
};
