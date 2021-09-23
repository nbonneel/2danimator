#include "Shape.h"

void PolygonShape::BSplineSubdivide(float time) {
	float s = GetScale(time);
	float a = angle.getDisplayValue(time);
	Vec2f trans = GetPosition(time);
	VerticesList verticesEval = vertices.getDisplayValue();

	VerticesList newList;
	newList.Clear();
	for (int ct = 0; ct < verticesEval.contourList.size(); ct++) {

		int N;
		int startV, endV = verticesEval.contourList[ct];
		if (ct == 0) {
			startV = 0;
		} else {
			startV = verticesEval.contourList[ct - 1] + 1;
		}
		N = endV - startV + 1;

		Spline sp(Vec2s("0", "0"), "1", Vec3u(0, 0, 0));
		sp.controlPoints.addVertex(time, verticesEval.vertices[endV]);
		for (int i = 0; i < N; i++) {
			sp.controlPoints.addVertex(time, verticesEval.vertices[startV + i]);
		}
		sp.controlPoints.addVertex(time, verticesEval.vertices[startV]);
		sp.controlPoints.addVertex(time, verticesEval.vertices[startV + 1]);

		for (int i = 0; i < 2 * (N); i++) {
			float tparam = i / (2.0*N);  // in [0, 1[			
			Vec2f pp = sp.evalBSpline(tparam*(N + 1.) / (N + 3.), sp.controlPoints.getDisplayValue());
			newList.addVertex(pp, i == 0);
		}
	}
	if (newList.contourList.size() >= 2) {
		if (newList.contourList[newList.contourList.size() - 1] == newList.contourList[newList.contourList.size() - 2]) {
			newList.contourList.resize(newList.contourList.size() - 1);
		}
	}
	//vertices.Clear();
	//p->vertices = newList;
	vertices.setDisplayValue(newList);
}