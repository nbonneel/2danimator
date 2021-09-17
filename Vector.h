#pragma once
#include "ceval.h"
#ifndef M_PI
#define M_PI 3.1415926535897932
#endif

static inline std::string replace_variable(const std::string& s, float t) {
	std::string newstring; newstring.reserve(s.size());
	for (int j = 0; j < s.size(); j++) {
		if (s[j] == 't') {
			newstring = newstring + "(" + std::to_string(t) + ")";
		} else {
			newstring = newstring + s[j];
		}
	}
	return newstring;
}
template<typename T> T sqr(T x) {
	return x * x;
};

class Keyable {
public:
	Keyable(void* firstParam = NULL, std::string name = "?") : firstParam(firstParam), name(name) {};
	
	//virtual template<typename T> T operator[](int i) const = 0;
	//virtual template<typename T> T& operator[](int i) = 0;
	std::string name, encodedTypes;
	void* firstParam;
};




template<typename T, int DIM>
class Vector : public Keyable {
public:
	Vector():Keyable(&coords[0]) { };
	Vector(T x) : Keyable(&coords[0]) {
		for (int i = 0; i < DIM; i++) 
			coords[i] = x; 
	}
	Vector(T x, T y) :Keyable(&coords[0]) {
		coords[0] = x; 
		coords[1] = y; 
	}
	Vector(T x, T y, T z) :Keyable(&coords[0]) {
		coords[0] = x; 
		coords[1] = y; 
		coords[2] = z; 
	}
	Vector(const Vector& v) :Keyable(&coords[0]) {
		for (int i = 0; i < DIM; i++)
			coords[i] = v[i];
	}
	T operator[](int i) const { return coords[i];}
	T& operator[](int i) { return coords[i]; }


//private:
	T coords[DIM];
};

template<typename T, int DIM>
class FastVector {
public:
	FastVector() {};
	FastVector(const Vector<T,DIM>& v) {
		for (int i = 0; i < DIM; i++)
			coords[i] = v[i];
	}
	FastVector(T x) {
		for (int i = 0; i < DIM; i++)
			coords[i] = x;
	}
	FastVector(T x, T y) {
		coords[0] = x;
		coords[1] = y;
	}
	FastVector(T x, T y, T z) {
		coords[0] = x;
		coords[1] = y;
		coords[2] = z;
	}
	FastVector(const FastVector<T,DIM>& v) {
		for (int i = 0; i < DIM; i++)
			coords[i] = v[i];
	}
	T operator[](int i) const { return coords[i]; }
	T& operator[](int i) { return coords[i]; }
	T coords[DIM];
};

template<typename T, int DIM>
T norm2(const Vector<T, DIM>& v) {
	T n(0);
	for (int i = 0; i < DIM; i++) {
		n += v[i] * v[i];
	}
	return n;
}
template<typename T, int DIM>
T norm2(const FastVector<T, DIM>& v) {
	T n(0);
	for (int i = 0; i < DIM; i++) {
		n += v[i] * v[i];
	}
	return n;
}
template<typename T, int DIM>
T norm(const Vector<T, DIM>& v) {
	return sqrt(norm2(v));
}
template<typename T, int DIM>
T norm(const FastVector<T, DIM>& v) {
	return sqrt(norm2(v));
}
template<typename T, int DIM>
Vector<T, DIM> getNormalized(const Vector<T, DIM>& v) {
	T n = norm(v);
	if (n == 0) return v;
	return v / n;
}
template<typename T, int DIM>
FastVector<T, DIM> getNormalized(const FastVector<T, DIM>& v) {
	T n = norm(v);
	if (n == 0) return v;
	return v/n;
}
template<typename T, int DIM>
T dot(const Vector<T, DIM>& v1, const Vector<T, DIM>& v2) {
	T d(0);
	for (int i = 0; i < DIM; i++) {
		d += v1[i] * v2[i];
	}
	return d;
}
template<typename T, int DIM>
T dot(const FastVector<T, DIM>& v1, const FastVector<T, DIM>& v2) {
	T d(0);
	for (int i = 0; i < DIM; i++) {
		d += v1[i] * v2[i];
	}
	return d;
}
template<typename T, int DIM>
Vector<T, DIM> operator+(const Vector<T, DIM>& a, const Vector<T, DIM>& b) {
	Vector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a[i] + b[i];
	}
	return result;
}
template<typename T, int DIM>
FastVector<T, DIM> operator+(const FastVector<T, DIM>& a, const FastVector<T, DIM>& b) {
	FastVector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a[i] + b[i];
	}
	return result;
}
template<typename T, int DIM>
Vector<T, DIM> operator-(const Vector<T, DIM>& a, const Vector<T, DIM>& b) {
	Vector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a[i] - b[i];
	}
	return result;
}
template<typename T, int DIM>
FastVector<T, DIM> operator-(const FastVector<T, DIM>& a, const FastVector<T, DIM>& b) {
	FastVector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a[i] - b[i];
	}
	return result;
}
template<typename T, int DIM>
Vector<T, DIM> operator-(const Vector<T, DIM>& a) {
	Vector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = -a[i];
	}
	return result;
}
template<typename T, int DIM>
FastVector<T, DIM> operator-(const FastVector<T, DIM>& a) {
	FastVector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = -a[i];
	}
	return result;
}
template<typename T, int DIM>
Vector<T, DIM> operator*(const Vector<T, DIM>& a, T b) {
	Vector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a[i] * b;
	}
	return result;
}
template<typename T, int DIM>
FastVector<T, DIM> operator*(const FastVector<T, DIM>& a, T b) {
	FastVector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a[i] * b;
	}
	return result;
}
template<typename T, int DIM>
Vector<T, DIM> operator/(const Vector<T, DIM>& a, T b) {
	Vector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a[i] / b;
	}
	return result;
}
template<typename T, int DIM>
FastVector<T, DIM> operator/(const FastVector<T, DIM>& a, T b) {
	FastVector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a[i] / b;
	}
	return result;
}
template<typename T, int DIM>
Vector<T, DIM> operator*(T a, const Vector<T, DIM>& b) {
	Vector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a * b[i];
	}
	return result;
}
template<typename T, int DIM>
FastVector<T, DIM> operator*(T a, const FastVector<T, DIM>& b) {
	FastVector<T, DIM> result;
	for (int i = 0; i < DIM; i++) {
		result[i] = a * b[i];
	}
	return result;
}

typedef  Vector<bool, 1> Bool;
typedef  Vector<float, 2> Vec2f;
typedef  FastVector<float, 2> FastVec2f;
typedef  Vector<unsigned char, 3> Vec3u;
typedef  Vector<float, 1> Float;


template<int DIM>
class Vector<std::string, DIM> : public Keyable {
public:
	Vector() :Keyable(&coords[0]) {};
	Vector(const std::string &x) : Keyable(&coords[0]) {
		for (int i = 0; i < DIM; i++)
			coords[i] = x;
	}
	Vector(const std::string &x, const std::string& y) :Keyable(&coords[0]) {
		coords[0] = x;
		coords[1] = y;
	}
	Vector(const std::string &x, const std::string &y, const std::string &z) :Keyable(&coords[0]) {
		coords[0] = x;
		coords[1] = y;
		coords[2] = z;
	}
	Vector(const Vector& v) :Keyable(&coords[0]) {
		for (int i = 0; i < DIM; i++)
			coords[i] = v[i];
	}
	std::string operator[](int i) const { return coords[i]; }
	std::string& operator[](int i) { return coords[i]; }

	Vector<float, DIM> eval(float t) const{
		Vector<float, DIM> v;
		for (int i = 0; i < DIM; i++) {
			std::string newstring = replace_variable(coords[i], t);
			v[i] = ceval_result2(newstring);
		}
		return v;
	}

	float eval1(float t) const {
		std::string newstring = replace_variable(coords[0], t);
		return  ceval_result2(newstring);
	}

		std::string coords[DIM];
};

typedef  Vector<std::string, 1> Expr;
typedef  Vector<std::string, 2> Vec2s;
typedef  Vector<std::string, 3> Vec3s;

static inline Vec2f rotate(const Vec2f& v, float angle) {
	Vec2f res;
	float sa = sin(angle);
	float ca = cos(angle);
	res[0] = v[0] * ca - v[1] * sa;
	res[1] = v[0] * sa + v[1] * ca;
	return res;
}
static inline FastVec2f rotate(const FastVec2f& v, float angle) {
	FastVec2f res;
	float sa = sin(angle);
	float ca = cos(angle);
	res[0] = v[0] * ca - v[1] * sa;
	res[1] = v[0] * sa + v[1] * ca;
	return res;
}

class VerticesList : public Keyable {
public:
	VerticesList() : Keyable(NULL) {};
	VerticesList(const Vec2f& v1, const Vec2f& v2, const Vec2f& v3) : Keyable(NULL) {
		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		firstParam = &vertices[0];
		contourList.push_back(2);
	};
	VerticesList(const VerticesList& v) {
		vertices = v.vertices;
		contourList = v.contourList;
		firstParam = &vertices[0];
	}
	void Clear() {
		vertices.clear();
		contourList.clear();
		firstParam = NULL;
	}
	void addVertex(const Vec2f& v, bool createNewContour = false) {
		vertices.push_back(v);
		if (contourList.size() == 0 || createNewContour) {
			if (contourList.size() != 0)
				contourList[contourList.size() - 1] = vertices.size() - 2;
			contourList.push_back(vertices.size() - 1);
		} else {
			contourList[contourList.size() - 1] = vertices.size() - 1;
		}

		firstParam = &vertices[0];
	}
	void insertVertex(const Vec2f& v, int prevVtx) {
		for (int i = 0; i < contourList.size(); i++) {
			int start = 0;
			if (i > 0) start = contourList[i - 1] + 1;
			if (prevVtx >= start) {
				contourList[i]++;
				break;
			}
		}
		vertices.insert(vertices.begin() + prevVtx+1, v);
		// contourList ->>>> TODO
		firstParam = &vertices[0];
	}
	std::vector<Vec2f> vertices;
	std::vector<int> contourList; // contours from 0 to contourList[0], then contourList[0]+1 to contourList[1] etc.
	std::vector<Vec3u> colors;
};