#pragma once

#include "Vector.h"
#include "Canvas.h"
#include "network_simplex_simple.h"
#include "TextToLines.h"

using namespace lemon;

static inline void drawBox(Canvas& canvas, int x1, int y1, int x2, int y2) {
	if (y2 < 0) return;
	if (x2 < 0) return;
	if (x1 > canvas.W - 1) return;
	if (y1 > canvas.H - 1) return;

	if (y1 >= 0) {
		for (int i = std::max(0, x1); i < std::min(canvas.W, x2); i++) {
			canvas(i, y1, 0) = 0;
			canvas(i, y1, 1) = 0;
			canvas(i, y1, 2) = 0;
		}
	}
	if (y2 <= canvas.H - 1) {
		for (int i = std::max(0, x1); i < std::min(canvas.W, x2); i++) {
			canvas(i, y2, 0) = 0;
			canvas(i, y2, 1) = 0;
			canvas(i, y2, 2) = 0;
		}
	}
	if (x1 >= 0) {
		for (int i = std::max(0, y1); i < std::min(canvas.H, y2); i++) {
			canvas(x1, i, 0) = 0;
			canvas(x1, i, 1) = 0;
			canvas(x1, i, 2) = 0;
		}
	}
	if (x2 <= canvas.W - 1) {
		for (int i = std::max(0, y1); i < std::min(canvas.H, y2); i++) {
			canvas(x2, i, 0) = 0;
			canvas(x2, i, 1) = 0;
			canvas(x2, i, 2) = 0;
		}
	}
}

static inline void drawSquare(Canvas& canvas, int x, int y, int size) {
	drawBox(canvas, x - size / 2, y - size / 2, x + size / 2, y + size / 2);
}

// https://rosettacode.org/wiki/Xiaolin_Wu%27s_line_algorithm#C.2B.2B
static inline void drawLine(Canvas& canvas, int x0, int y0, int x1, int y1, const Vec3u& color = Vec3u(0,0,0), float wd = 1) {
	auto setPixelColor = [&canvas, &color](int x, int y, float mixing) -> void {
		if (x<0 || y<0 || x>canvas.W - 1 || y>canvas.H - 1) return;  
		canvas(x, y, 0) = (unsigned char)(canvas(x, y, 0)*(mixing) + color[0]*(1.f - mixing));
		canvas(x, y, 1) = (unsigned char)(canvas(x, y, 1)*(mixing) + color[1] * (1.f - mixing));
		canvas(x, y, 2) = (unsigned char)(canvas(x, y, 2)*(mixing) + color[2] * (1.f - mixing));
	};
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx - dy, e2, x2, y2;                          /* error value e_xy */
	float ed = dx + dy == 0 ? 1 : sqrt((float)dx*dx + (float)dy*dy);

	for (wd = (wd + 1) / 2; ; ) {                                   /* pixel loop */
		setPixelColor(x0, y0, std::max(0.f,abs(err - dx + dy) / ed - wd + 1));
		e2 = err; x2 = x0;
		if (2 * e2 >= -dx) {                                           /* x step */
			for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
				setPixelColor(x0, y2 += sy, std::max(0.f, abs(e2) / ed - wd + 1));
			if (x0 == x1) break;
			e2 = err; err -= dy; x0 += sx;
		}
		if (2 * e2 <= dy) {                                            /* y step */
			for (e2 = dx - e2; e2 < ed*wd && (x1 != x2 || dx < dy); e2 += dy)
				setPixelColor(x2 += sx, y0,  std::max(0.f, abs(e2) / ed - wd + 1));
			if (y0 == y1) break;
			err += dx; y0 += sy;
		}
	}

	/*auto ipart = [](float x) -> int {return int(std::floor(x)); };
	auto round = [](float x) -> float {return std::round(x); };
	auto fpart = [](float x) -> float {return x - std::floor(x); };
	auto rfpart = [=](float x) -> float {return 1 - fpart(x); };
	auto plot = [&canvas](int x, int y, float val) -> void {if (x<0 || y<0 || x>canvas.W - 1 || y>canvas.H - 1) return;  canvas(x, y, 0) = val; canvas(x, y, 1) = val;  canvas(x, y, 2) = val; };

	const bool steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	const float dx = x1 - x0;
	const float dy = y1 - y0;
	const float gradient = (dx == 0) ? 1 : dy / dx;

	int xpx11;
	float intery;
	{
		const float xend = round(x0);
		const float yend = y0 + gradient * (xend - x0);
		const float xgap = rfpart(x0 + 0.5);
		xpx11 = int(xend);
		const int ypx11 = ipart(yend);
		if (steep) {
			plot(ypx11, xpx11, rfpart(yend) * xgap);
			plot(ypx11 + 1, xpx11, fpart(yend) * xgap);
		} else {
			plot(xpx11, ypx11, rfpart(yend) * xgap);
			plot(xpx11, ypx11 + 1, fpart(yend) * xgap);
		}
		intery = yend + gradient;
	}

	int xpx12;
	{
		const float xend = round(x1);
		const float yend = y1 + gradient * (xend - x1);
		const float xgap = rfpart(x1 + 0.5);
		xpx12 = int(xend);
		const int ypx12 = ipart(yend);
		if (steep) {
			plot(ypx12, xpx12, rfpart(yend) * xgap);
			plot(ypx12 + 1, xpx12, fpart(yend) * xgap);
		} else {
			plot(xpx12, ypx12, rfpart(yend) * xgap);
			plot(xpx12, ypx12 + 1, fpart(yend) * xgap);
		}
	}

	if (steep) {
		for (int x = xpx11 + 1; x < xpx12; x++) {
			plot(ipart(intery), x, rfpart(intery));
			plot(ipart(intery) + 1, x, fpart(intery));
			intery += gradient;
		}
	} else {
		for (int x = xpx11 + 1; x < xpx12; x++) {
			plot(x, ipart(intery), rfpart(intery));
			plot(x, ipart(intery) + 1, fpart(intery));
			intery += gradient;
		}
	}*/
}

// border is the normalized outwards half vector with the previous/next edge
static inline void drawLineForInsidePolyline(Canvas& canvas, int x0, int y0, int x1, int y1, const FastVec2f& border1dir, const FastVec2f& border2dir, const Vec3u& color = Vec3u(0, 0, 0), float wd = 1) {

	float x0b = x0;
	float y0b = y0;
	float x1b = x1;
	float y1b = y1;
	for (int i = 0; i < wd; i++) {
		drawLine(canvas, x0b, y0b, x1b, y1b, color, 1);
		x0b -= border1dir[0];
		y0b -= border1dir[1];
		x1b -= border2dir[0];
		y1b -= border2dir[1];
	}

}


class Shape {
public:
	Shape(bool visible = true):visible(visible) {
		this->visible.name = "Visible"; this->visible.encodedTypes = "B";
		parameters.push_back((Keyable*)&this->visible);
		id = numShapes;
		numShapes++;
	};
	Shape(const Shape& b) {
		visible = b.visible;
		parameters.push_back((Keyable*)&this->visible);
	}
	virtual void Draw(Canvas& canvas, float time, bool displayControls) const = 0;
	virtual bool Contains(const Vec2f& coord, float time) const = 0;
	virtual Shape* Clone() = 0;
	virtual void SetPosition(const Vec2f& coord) = 0;
	virtual Vec2f GetPosition(float time) = 0;
	virtual void SetScale(float value) = 0;
	virtual float GetScale(float time) = 0;
	std::vector<Keyable*> parameters;
	Bool visible;
	int id;
	static int numShapes;
};


class Disk : public Shape {
public: 
	Disk(const Vec2s& position, std::string radius, const Vec3u &color, const Vec3u &colorEdge, std::string thicknessEdge, bool visible = true):Shape(visible), pos(position), radius(radius), color(color), colorEdge(colorEdge), thicknessEdge(thicknessEdge) {
		this->pos.name = "Center"; this->pos.encodedTypes = "SS";
		this->radius.name = "Radius"; this->radius.encodedTypes = "S";
		this->color.name = "Color"; this->color.encodedTypes = "C";
		this->colorEdge.name = "Border Color"; this->colorEdge.encodedTypes = "C";
		this->thicknessEdge.name = "Border Thickness"; this->thicknessEdge.encodedTypes = "S";

		parameters.push_back((Keyable*)&this->pos);
		parameters.push_back((Keyable*)&this->radius);
		parameters.push_back((Keyable*)&this->color);
		parameters.push_back((Keyable*)&this->thicknessEdge);
		parameters.push_back((Keyable*)&this->colorEdge);
	}

	Disk(const Disk& d) : Shape(d.visible[0]), pos(d.pos), radius(d.radius), color(d.color), colorEdge(d.colorEdge),thicknessEdge(d.thicknessEdge) {
		this->pos.name = "Center"; this->pos.encodedTypes = "SS";
		this->radius.name = "Radius"; this->radius.encodedTypes = "S";
		this->color.name = "Color"; this->color.encodedTypes = "C";
		this->colorEdge.name = "Border Color"; this->colorEdge.encodedTypes = "C";
		this->thicknessEdge.name = "Border Thickness"; this->thicknessEdge.encodedTypes = "S";
		
		parameters.push_back((Keyable*)&this->pos);
		parameters.push_back((Keyable*)&this->radius);
		parameters.push_back((Keyable*)&this->color);
		parameters.push_back((Keyable*)&this->thicknessEdge);
		parameters.push_back((Keyable*)&this->colorEdge);
		
	}

	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {
		float r = radius.eval1(time);
		Vec2f p = pos.eval(time);
		float th = thicknessEdge.eval1(time);
		float r2 = sqr(r);
		float r2i = sqr(r - th);
		float r2ib = sqr(r - th -1);
		float r2b = sqr(r +1);
		if (visible[0]) {
			int startj = std::max(0, (int)(p[0] - r - 1));
			int endj = std::min(canvas.W, (int)(p[0] + r + 2));
#pragma omp parallel for
			for (int i = std::max(0, (int)(p[1] - r - 1)); i < std::min(canvas.H, (int)(p[1] + r + 2)); i++) {
				for (int j = startj; j < endj; j++) {
					float rad = sqr(i - p[1]) + sqr(j - p[0]);
					if (rad <= r2) {
						if (rad <= r2i) {
							if (rad <= r2ib) {
								canvas(j, i, 0) = color[0];
								canvas(j, i, 1) = color[1];
								canvas(j, i, 2) = color[2];
							} else {
								float dist = sqrt(rad) - r + th + 1;
								canvas(j, i, 0) = colorEdge[0] * (dist)+color[0] * (1 - dist);
								canvas(j, i, 1) = colorEdge[1] * (dist)+color[1] * (1 - dist);
								canvas(j, i, 2) = colorEdge[2] * (dist)+color[2] * (1 - dist);
							}
						} else {
							canvas(j, i, 0) = colorEdge[0];
							canvas(j, i, 1) = colorEdge[1];
							canvas(j, i, 2) = colorEdge[2];
						}
					} else {
						if (rad <= r2b) {
							float dist = sqrt(rad) - r;
							if (th > 0) {
								canvas(j, i, 0) = canvas(j, i, 0)*(dist)+colorEdge[0] * (1 - dist);
								canvas(j, i, 1) = canvas(j, i, 1)*(dist)+colorEdge[1] * (1 - dist);
								canvas(j, i, 2) = canvas(j, i, 2)*(dist)+colorEdge[2] * (1 - dist);
							} else {
								canvas(j, i, 0) = canvas(j, i, 0)*(dist)+color[0] * (1 - dist);
								canvas(j, i, 1) = canvas(j, i, 1)*(dist)+color[1] * (1 - dist);
								canvas(j, i, 2) = canvas(j, i, 2)*(dist)+color[2] * (1 - dist);
							}
						}
					}
				}
			}
		}

		if (displayControls) { // draw Bbox
			drawBox(canvas, p[0] - r, p[1] - r, p[0] + r, p[1] + r);
		}
	}

	virtual Shape* Clone() {
		return new Disk(*this);
	}

	virtual bool Contains(const Vec2f& coord, float time) const {
		float r = radius.eval1(time);
		Vec2f p = pos.eval(time);
		if (sqr(coord[1] - p[1]) + sqr(coord[0] - p[0]) <= sqr(r)) {
			return true;
		}
		return false;
	}

	virtual void SetPosition(const Vec2f& coord) {
		//pos[0] = coord[0];  // Beware: not pos = coord, since this would erase the name of the property "position"/"center"/....
		//pos[1] = coord[1];
		double lval;
		for (int i = 0; i < 2; i++) {
			if (wxString(pos[i]).ToDouble(&lval)) {
				pos[i] = std::to_string(coord[i]);
			}
		}
	}
	virtual Vec2f GetPosition(float time) {
		return pos.eval(time);
	}

	virtual void SetScale(float value) {
		//radius[0] = value;  
		double lval;
		if (wxString(radius[0]).ToDouble(&lval)) {
			radius[0] = std::to_string(value);
		}
	}
	virtual float GetScale(float time) {
		return radius.eval1(time);
	}
	Vec2s pos;
	Vec3u color, colorEdge;
	Expr radius, thicknessEdge;
};

class PolygonShape : public Shape { // Polygon already taken in wingdi.h (5127)...
public:
	PolygonShape(const Vec2s& position, std::string scale, std::string angle, const Vec3u &color, const Vec3u &edgeColor, std::string edgeThickness, bool visible = true):Shape(visible), pos(position), scale(scale), color(color), angle(angle), edgeColor(edgeColor), edgeThickness(edgeThickness) {
		
		this->pos.name = "Center"; this->pos.encodedTypes = "SS";
		this->scale.name = "Scale"; this->scale.encodedTypes = "S";
		this->color.name = "Color"; this->color.encodedTypes = "C";
		this->edgeColor.name = "Edge Color"; this->edgeColor.encodedTypes = "C";
		this->edgeThickness.name = "Edge Thickness"; this->edgeThickness.encodedTypes = "S";
		this->angle.name = "Angle"; this->angle.encodedTypes = "S";
		this->vertices.name = "Vertices"; this->vertices.encodedTypes = "L" + std::to_string(this->vertices.vertices.size());

		parameters.push_back((Keyable*)&this->pos);
		parameters.push_back((Keyable*)&this->scale);
		parameters.push_back((Keyable*)&this->angle);
		parameters.push_back((Keyable*)&this->color);
		parameters.push_back((Keyable*)&this->edgeColor);
		parameters.push_back((Keyable*)&this->edgeThickness);
		parameters.push_back((Keyable*)&this->vertices);
	}

	PolygonShape(const PolygonShape& d) : Shape(d.visible[0]), pos(d.pos), scale(d.scale), color(d.color),angle(d.angle), vertices(d.vertices), edgeColor(d.edgeColor), edgeThickness(d.edgeThickness) {
		this->pos.name = "Center"; this->pos.encodedTypes = "SS";
		this->scale.name = "Scale"; this->scale.encodedTypes = "S";
		this->color.name = "Color"; this->color.encodedTypes = "C";
		this->edgeColor.name = "Edge Color"; this->edgeColor.encodedTypes = "C";
		this->edgeThickness.name = "Edge Thickness"; this->edgeThickness.encodedTypes = "S";		
		this->angle.name = "Angle"; this->angle.encodedTypes = "S";
		this->vertices.name = "Vertices"; this->vertices.encodedTypes = "L"+std::to_string(d.vertices.vertices.size());
		parameters.push_back((Keyable*)&this->pos);
		parameters.push_back((Keyable*)&this->scale);
		parameters.push_back((Keyable*)&this->angle);
		parameters.push_back((Keyable*)&this->color);
		parameters.push_back((Keyable*)&this->edgeColor);
		parameters.push_back((Keyable*)&this->edgeThickness);
		parameters.push_back((Keyable*)&this->vertices);
		this->vertices.firstParam = &this->vertices.vertices[0].coords[0];
	}

	void addVertex(const Vec2f& v, bool createNewContour = false) {
		vertices.addVertex(v, createNewContour);
		this->vertices.encodedTypes = "L" + std::to_string(this->vertices.vertices.size());
		this->vertices.firstParam = &this->vertices.vertices[0].coords[0];		
	}
	void insertVertex(const Vec2f& v, int prevVertex) {
		vertices.insertVertex(v, prevVertex);
		this->vertices.encodedTypes = "L" + std::to_string(this->vertices.vertices.size());
		this->vertices.firstParam = &this->vertices.vertices[0].coords[0];
	}
	int getComponentVtxCount(int component) {
		if (component >= this->vertices.contourList.size()) return 0;
		int start = 0;
		if (component > 0) {
			start = this->vertices.contourList[component - 1]+1;
		}
		return this->vertices.contourList[component] - start + 1;
	}

	virtual Shape* Clone() {
		return new PolygonShape(*this);
	}

	void setText(const char* text) {
		TextToLine t(text);
		std::vector<float> co;
		std::vector<int> contours;
		t.convert(co, contours);
		vertices.Clear();
		int curContour = 0;
		for (int i = 0; i < co.size() / 2; i++) {
			if (i == contours[curContour]) {
				vertices.addVertex(Vec2f(co[i * 2], co[i * 2 + 1]), true);
				curContour++;
			} else {
				vertices.addVertex(Vec2f(co[i * 2], co[i * 2 + 1]), false);
			}
		}
		vertices.contourList = contours;

	}


	virtual bool Contains(const Vec2f& initPoint, float s, float a, const Vec2f &p, float time) const {
		const Vec2f *pg1, *pg2, *pgend;
		Vec2f point = rotate((initPoint  - p) / s, -a);

		const int X = 0;
		const int Y = 1;
		int j, yflag0, yflag1, inside_flag, xflag0;
		float ty, tx;
		const Vec2f *vtx0, *vtx1;
		const Vec2f* pgon = &vertices.vertices[0];
		int numverts = vertices.vertices.size();

		tx = point[X];
		ty = point[Y];

		//vtx0 = &pgon[numverts - 1].coords[0];
		vtx0 = &pgon[vertices.contourList[0]];
		/* get test bit for above/below X axis */
		yflag0 = ((*vtx0)[Y] >= ty);
		vtx1 = &pgon[0];

		inside_flag = 0;
		int vtxId = 0;
		int numContour = 0;
		for (j = numverts + 1; --j; ) {

			yflag1 = ((*vtx1)[Y] >= ty);
			/* check if edge's endpoints straddle (are on opposite sides) of X axis
			 * (i.e. the Y's differ); if so, +X ray could intersect this edge.
			 */
			if (yflag0 != yflag1) {
				xflag0 = ((*vtx0)[X] >= tx);
				/* check if endpoints are on same side of the Y axis (i.e. X's
				 * are the same); if so, it's easy to test if edge hits or misses.
				 */
				if (xflag0 == ((*vtx1)[X] >= tx)) {

					/* if edge's X values both right of the point, must hit */
					if (xflag0) inside_flag = !inside_flag;
				} else {
					/* compute intersection of polygon edge with +X ray, note
					 * if >= point's X; if so, the ray hits it.
					 */
					if (((*vtx1)[X] - ((*vtx1)[Y] - ty) *
						((*vtx0)[X] - (*vtx1)[X]) / ((*vtx0)[Y] - (*vtx1)[Y])) >= tx) {
						inside_flag = !inside_flag;
					}
				}
				/* small improvement over original code: update yflag0 only when
				 * we know it differs from yflag1.
				 */
				yflag0 = yflag1;
			}

			/* move to next pair of vertices, retaining info as possible */
			vtx0 = vtx1;
			vtx1 ++;

			if (vtxId == vertices.contourList[numContour]) {

				//vtx0 = vtx1;
				//vtx1 += 2;
				if (numContour + 1 <= vertices.contourList.size() - 1)
					vtx0 = &pgon[vertices.contourList[numContour + 1]];
				else
					vtx0 = &pgon[vertices.contourList[numContour]];
				//vtxId++;
				yflag0 = ((*vtx0)[Y] >= ty);
				numContour++;
				//continue;
			}
			vtxId++;
		}

		return(inside_flag);


		#if 0
		float tx, ty, u0, u1, u2, v0, v1, vx0, vy0, alpha, beta, denom;
		int inside_flag;
		const int X = 0;
		const int Y = 1;
		const Vec2f* pgon = &vertices.vertices[0];
		int numverts = vertices.vertices.size();
		tx = point[X];
		ty = point[Y];
		vx0 = pgon[0][X];
		vy0 = pgon[0][Y];
		u0 = tx - vx0;
		v0 = ty - vy0;

		inside_flag = 0;
		pgend = &pgon[numverts - 1];
		for (pg1 = &pgon[1], pg2 = &pgon[2]; pg1 != pgend; pg1 += 1, pg2 += 1) {

			u1 = (*pg1)[X] - vx0;
			if (u1 == 0.0) {

				/* 0 and 1 vertices have same X value */

				/* zero area test - can be removed for convex testing */
				u2 = (*pg2)[X] - vx0;
				if ((u2 == 0.0) ||

					/* compute beta and check bounds */
					/* we use "<= 0.0" so that points on the shared interior
					 * edge will (generally) be inside only one polygon.
					 */
					((beta = u0 / u2) <= 0.0) ||
					(beta > 1.0) ||

					/* zero area test - remove for convex testing */
					((v1 = (*pg1)[Y] - vy0) == 0.0) ||

					/* compute alpha and check bounds */
					((alpha = (v0 - beta *
					((*pg2)[Y] - vy0)) / v1) < 0.0)) {

					/* whew! missed! */
					goto NextTri;
				}

			} else {
				/* 0 and 1 vertices have different X value */

				/* compute denom and check for zero area triangle - check
				 * is not needed for convex polygon testing
				 */
				u2 = (*pg2)[X] - vx0;
				v1 = (*pg1)[Y] - vy0;
				denom = ((*pg2)[Y] - vy0) * u1 - u2 * v1;
				if ((denom == 0.0) ||

					/* compute beta and check bounds */
					/* we use "<= 0.0" so that points on the shared interior
					 * edge will (generally) be inside only one polygon.
					 */
					((beta = (v0 * u1 - u0 * v1) / denom) <= 0.0) ||
					(beta > 1.0) ||

					/* compute alpha & check bounds */
					((alpha = (u0 - beta * u2) / u1) < 0.0)) {

					/* whew! missed! */
					goto NextTri;
				}
			}

			/* check gamma */
			if (alpha + beta <= 1.0) {
				/* survived */
				inside_flag = !inside_flag;
			}

		NextTri:;
		}
		return(inside_flag);
#endif
	}

	virtual bool Contains(const Vec2f& initPoint, float time) const {
		return Contains(initPoint, scale.eval1(time), angle.eval1(time), pos.eval(time), time);
	}

	virtual bool ContainsForDraw(const FastVec2f& point) const {
#if 0
		const FastVec2f *pg1, *pg2, *pgend;
		float tx, ty, u0, u1, u2, v0, v1, vx0, vy0, alpha, beta, denom;
		int inside_flag;
		const int X = 0;
		const int Y = 1;
		const FastVec2f* pgon = &fastTransformedVertices[0];
		int numverts = fastTransformedVertices.size();
		tx = point[X];
		ty = point[Y];
		vx0 = pgon[0][X];
		vy0 = pgon[0][Y];
		u0 = tx - vx0;
		v0 = ty - vy0;

		inside_flag = 0;
		pgend = &pgon[numverts - 1];
		for (pg1 = &pgon[1], pg2 = &pgon[2]; pg1 != pgend; pg1 += 1, pg2 += 1) {

			u1 = (*pg1)[X] - vx0;
			if (u1 == 0.0) {

				/* 0 and 1 vertices have same X value */

				/* zero area test - can be removed for convex testing */
				u2 = (*pg2)[X] - vx0;
				if ((u2 == 0.0) ||

					/* compute beta and check bounds */
					/* we use "<= 0.0" so that points on the shared interior
					 * edge will (generally) be inside only one polygon.
					 */
					((beta = u0 / u2) <= 0.0) ||
					(beta > 1.0) ||

					/* zero area test - remove for convex testing */
					((v1 = (*pg1)[Y] - vy0) == 0.0) ||

					/* compute alpha and check bounds */
					((alpha = (v0 - beta *
					((*pg2)[Y] - vy0)) / v1) < 0.0)) {

					/* whew! missed! */
					goto NextTri;
				}

			} else {
				/* 0 and 1 vertices have different X value */

				/* compute denom and check for zero area triangle - check
				 * is not needed for convex polygon testing
				 */
				u2 = (*pg2)[X] - vx0;
				v1 = (*pg1)[Y] - vy0;
				denom = ((*pg2)[Y] - vy0) * u1 - u2 * v1;
				if ((denom == 0.0) ||

					/* compute beta and check bounds */
					/* we use "<= 0.0" so that points on the shared interior
					 * edge will (generally) be inside only one polygon.
					 */
					((beta = (v0 * u1 - u0 * v1) / denom) <= 0.0) ||
					(beta > 1.0) ||

					/* compute alpha & check bounds */
					((alpha = (u0 - beta * u2) / u1) < 0.0)) {

					/* whew! missed! */
					goto NextTri;
				}
			}

			/* check gamma */
			if (alpha + beta <= 1.0) {
				/* survived */
				inside_flag = !inside_flag;
			}

		NextTri:;
		}
		return(inside_flag);
#endif

		const int X = 0; 
		const int Y = 1;
		int j, yflag0, yflag1, inside_flag, xflag0;
		float ty, tx;
		const float *vtx0, *vtx1;
		const FastVec2f* pgon = &fastTransformedVertices[0];
		int numverts = fastTransformedVertices.size();

		tx = point[X];
		ty = point[Y];

		//vtx0 = &pgon[numverts - 1].coords[0];
		vtx0 = &pgon[vertices.contourList[0]].coords[0];
		/* get test bit for above/below X axis */
		yflag0 = (vtx0[Y] >= ty);
		vtx1 = &pgon[0].coords[0];

		inside_flag = 0;
		int vtxId = 0;
		int numContour = 0;
		for (j = numverts + 1; --j; ) {

			yflag1 = (vtx1[Y] >= ty);
			/* check if edge's endpoints straddle (are on opposite sides) of X axis
			 * (i.e. the Y's differ); if so, +X ray could intersect this edge.
			 */
			if (yflag0 != yflag1) {
				xflag0 = (vtx0[X] >= tx);
				/* check if endpoints are on same side of the Y axis (i.e. X's
				 * are the same); if so, it's easy to test if edge hits or misses.
				 */
				if (xflag0 == (vtx1[X] >= tx)) {

					/* if edge's X values both right of the point, must hit */
					if (xflag0) inside_flag = !inside_flag;
				} else {
					/* compute intersection of polygon edge with +X ray, note
					 * if >= point's X; if so, the ray hits it.
					 */
					if ((vtx1[X] - (vtx1[Y] - ty) *
						(vtx0[X] - vtx1[X]) / (vtx0[Y] - vtx1[Y])) >= tx) {
						inside_flag = !inside_flag;
					}
				}
				/* small improvement over original code: update yflag0 only when
				 * we know it differs from yflag1.
				 */
				yflag0 = yflag1;
			}

			/* move to next pair of vertices, retaining info as possible */
			vtx0 = vtx1;
			vtx1 +=2;

			if (vtxId == vertices.contourList[numContour]) {

				//vtx0 = vtx1;
				//vtx1 += 2;
				if (numContour + 1 <= vertices.contourList.size() - 1)
					vtx0 = &pgon[vertices.contourList[numContour + 1]].coords[0];
				else
					vtx0 = &pgon[vertices.contourList[numContour]].coords[0];
				//vtxId++;
				yflag0 = (vtx0[Y] >= ty);
				numContour++;
				//continue;
			}
			vtxId++;
		}

			   return(inside_flag);
	}

	float distFromNearestEdgeForDraw(const FastVec2f& v) const {  // s = evaluated scale		
		float d = 1E9;
		int N = fastTransformedVertices.size();
		int curContour = 0;
		for (int i = 0; i < N; i++) {
			int nextPoint = i + 1;
			if (i == vertices.contourList[curContour]) {
				if (curContour == 0) {
					nextPoint = 0;
				} else {
					nextPoint = vertices.contourList[curContour - 1] + 1;
				}
				curContour++;
			}
			const FastVec2f &v1 = fastTransformedVertices[nextPoint];
			const FastVec2f &v2 = fastTransformedVertices[i];
			float absciProj = dot(v - v1, v2 - v1);
			if (absciProj <= 0) {  // dist point point
				d = std::min(d, norm2(v - v1));
			} else {
				absciProj /= norm2(v2 - v1);
				if (absciProj >= 1) {
					d = std::min(d, norm2(v - v2));
				} else { // dist point line
					FastVec2f p = v1 + absciProj * (v2 - v1);
					d = std::min(d, norm2(p - v));
				}
			}
		}
		return sqrt(d);
	}

	Vec2f nearestPoint(const Vec2f& initPoint, float s, float a, const Vec2f &p) const {  // s = evaluated scale
		Vec2f v = rotate((initPoint - p) / s, -a);
		float d = 1E9;
		int N = vertices.vertices.size();
		Vec2f result;
		int curContour = 0;
		for (int i = 0; i < N; i++) {
			int nextPoint = i + 1;
			if (i == vertices.contourList[curContour]) {
				if (curContour == 0) {
					nextPoint = 0;
				} else {
					nextPoint = vertices.contourList[curContour - 1] + 1;
				}
				curContour++;
			}
			Vec2f v1 = vertices.vertices[nextPoint];
			Vec2f v2 = vertices.vertices[i];
			float absciProj = dot(v - v1, v2 - v1) / norm2(v2 - v1);
			float d1 = norm(v - v1);
			float d2 = norm(v - v2);
			if (absciProj <= 0) {  // dist point point
				if (d1 < d) {
					d = d1;
					result = v1;
				}
			} else {
				if (absciProj >= 1) {
					if (d2 < d) {
						d = d2;
						result = v2;
					}
				} else { // dist point line
					Vec2f p = v1 + absciProj * (v2 - v1);
					float dp = norm(p - v);
					if (dp < d) {
						d = dp;
						result = p;
					}
				}
			}
		}
		return result;
	}

	int nearestVertex(const Vec2f& initPoint, float s, float a, const Vec2f &p, float searchRadius) const {  // s = evaluated scale ; segment = [result;result+1]
		Vec2f v = rotate((initPoint - p) / s, -a);
		float d = 1E9;
		int N = vertices.vertices.size();
		int result = -1;
		for (int i = 0; i < N; i++) {
			Vec2f v1 = vertices.vertices[i];
			float d1 = norm(v - v1);
			if (d1 < d && d1< searchRadius) {
				d = d1;
				result = i;
			}
		}
		return result;
	}

	int nearestSegment(const Vec2f& initPoint, float s, float a, const Vec2f &p) const {  // s = evaluated scale ; segment = [result;result+1]
		Vec2f v = rotate((initPoint - p) / s, -a);
		float d = 1E9;
		int N = vertices.vertices.size();
		int result;
		int curContour = 0;
		for (int i = 0; i < N; i++) {
			int nextPoint = i + 1;
			if (i == vertices.contourList[curContour]) {
				if (curContour == 0) {
					nextPoint = 0;
				} else {
					nextPoint = vertices.contourList[curContour - 1] + 1;
				}
				curContour++;
			}
			Vec2f v1 = vertices.vertices[nextPoint];
			Vec2f v2 = vertices.vertices[i];
			float absciProj = dot(v - v1, v2 - v1) / norm2(v2 - v1);
			float d1 = norm(v - v1);
			float d2 = norm(v - v2);			
			if (absciProj <= 0) {  // dist point point
				if (d1 < d) {
					d = d1;
					result = i;
				}
			} else {
				if (absciProj >= 1) {
					if (d2 < d) {
						d = d2;
						result = (i + 1) % N;
					}
				} else { // dist point line
					Vec2f p = v1 + absciProj * (v2 - v1);
					float dp = norm(v - p);
					if (dp < d) {
						d = dp;
						result = i;
					}
				}
			}
		}
		return result;
	}

	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {
		float s = scale.eval1(time);
		float a = angle.eval1(time);
		FastVec2f p(pos.eval(time));
		int minX=1E9, maxX = -1E9, minY = 1E9, maxY = -1E9;
		int N = vertices.vertices.size();
		fastTransformedVertices.resize(N);
		FastVec2f center(0, 0);
		for (int i = 0; i < N; i++) {
			FastVec2f v = rotate(FastVec2f(vertices.vertices[i]), a);
			fastTransformedVertices[i] = v * s + p;
			center = center+fastTransformedVertices[i];
			minX = std::min(minX, (int)(fastTransformedVertices[i][0]));
			maxX = std::max(maxX, (int)(fastTransformedVertices[i][0]));
			minY = std::min(minY, (int)(fastTransformedVertices[i][1]));
			maxY = std::max(maxY, (int)(fastTransformedVertices[i][1]));
		}
		center = center / (float)N;
		int ominX = minX, ominY = minY, omaxX = maxX, omaxY = maxY;
		minX = std::max(minX-1, 0);
		maxX = std::min(maxX-1, canvas.W - 1);
		minY = std::max(minY+1, 0);
		maxY = std::min(maxY+1, canvas.H - 1);
		float edgeThick = edgeThickness.eval1(time);
		const unsigned char* cc = &color.coords[0];
		if (visible[0]) {
#pragma omp parallel for
			for (int i = minY; i <= maxY; i++) {
				for (int j = minX; j <= maxX; j++) {
					if (ContainsForDraw(Vec2f(j, i))) {
						/*float dist = distFromNearestEdgeForDraw(Vec2f(j, i));
						if (dist < edgeThick) {
							canvas(j, i, 0) = edgeColor[0];
							canvas(j, i, 1) = edgeColor[1];
							canvas(j, i, 2) = edgeColor[2];
						} else {*/
							/*canvas(j, i, 0) = color[0];
							canvas(j, i, 1) = color[1];
							canvas(j, i, 2) = color[2];*/
						memcpy(&canvas(j, i, 0), cc, 3 * sizeof(char));
						//}
					} else {
						float dist = distFromNearestEdgeForDraw(Vec2f(j, i));
						if (dist < 1) {
							if (edgeThick > 0) {
								canvas(j, i, 0) = canvas(j, i, 0)*(dist)+edgeColor[0] * (1 - dist);
								canvas(j, i, 1) = canvas(j, i, 1)*(dist)+edgeColor[1] * (1 - dist);
								canvas(j, i, 2) = canvas(j, i, 2)*(dist)+edgeColor[2] * (1 - dist);
							} else {
								canvas(j, i, 0) = canvas(j, i, 0)*(dist)+color[0] * (1 - dist);
								canvas(j, i, 1) = canvas(j, i, 1)*(dist)+color[1] * (1 - dist);
								canvas(j, i, 2) = canvas(j, i, 2)*(dist)+color[2] * (1 - dist);
							}
						}
					}
				}
			}

			int curContour = 0;
			for (int i = 0; i < fastTransformedVertices.size(); i++) {

				int nextPoint = i + 1;
				if (i == vertices.contourList[curContour]) {
					if (curContour == 0) {
						nextPoint = 0;
					} else {
						nextPoint = vertices.contourList[curContour - 1]+1;
					}
				}
				//drawLine(canvas, fastTransformedVertices[i][0], fastTransformedVertices[i][1], fastTransformedVertices[nextPoint][0], fastTransformedVertices[nextPoint][1], edgeColor, edgeThick);


				//drawLine(canvas, fastTransformedVertices[i][0], fastTransformedVertices[i][1], fastTransformedVertices[(i + 1) % N][0], fastTransformedVertices[(i + 1) % N][1], edgeColor, edgeThick);
				int prevPoint = i - 1; 
				if (prevPoint == -1) 
					prevPoint = vertices.contourList[0];
				else 
					if (curContour > 0 && prevPoint <= vertices.contourList[curContour - 1]) {
						prevPoint = vertices.contourList[curContour];
					}
				int nextnextPoint = nextPoint + 1;
				if (nextnextPoint > vertices.contourList[curContour]) {
					if (curContour>=1)
						nextnextPoint = vertices.contourList[curContour - 1] + 1;
					else
						nextnextPoint = 1;
				}

				//FastVec2f v1 = -getNormalized(getNormalized(fastTransformedVertices[nextPoint] - fastTransformedVertices[i]) + getNormalized(fastTransformedVertices[prevPoint] - fastTransformedVertices[i]));
				//FastVec2f v2 = -getNormalized(getNormalized(fastTransformedVertices[nextnextPoint] - fastTransformedVertices[nextPoint]) + getNormalized(fastTransformedVertices[i] - fastTransformedVertices[nextPoint]));
				FastVec2f perpprev(-fastTransformedVertices[i][1] + fastTransformedVertices[prevPoint][1], fastTransformedVertices[i][0] - fastTransformedVertices[prevPoint][0]);
				FastVec2f perp(-fastTransformedVertices[nextPoint][1] + fastTransformedVertices[i][1], fastTransformedVertices[nextPoint][0] - fastTransformedVertices[i][0]);
				FastVec2f perpnext(-fastTransformedVertices[nextnextPoint][1] + fastTransformedVertices[nextPoint][1], fastTransformedVertices[nextnextPoint][0] - fastTransformedVertices[nextPoint][0]);
				FastVec2f v1 = getNormalized(perpprev + perp);
				FastVec2f v2 = getNormalized(perpnext + perp);
				//if (dot(perp2, v2 - v1) < 1E-5) perp = center - v1;
				//if (dot(v1, perp) < 0) v1 = -v1;
				//if (dot(v2, perp2) < 0) v2 = -v2;

				drawLineForInsidePolyline(canvas, fastTransformedVertices[i][0], fastTransformedVertices[i][1], fastTransformedVertices[nextPoint][0], fastTransformedVertices[nextPoint][1], v1, v2, edgeColor, edgeThick*s);
				if (i == vertices.contourList[curContour]) {
					curContour++;
				}
			}
		}

		if (displayControls) { // draw Bbox
			drawBox(canvas, ominX, ominY, omaxX, omaxY);
			for (int i = 0; i < vertices.vertices.size(); i++) {
				const FastVec2f &pp = fastTransformedVertices[i];
				drawSquare(canvas, pp[0], pp[1], 5);
			}
		}


	}

	virtual void SetPosition(const Vec2f& coord) {
		//pos[0] = coord[0];
		//pos[1] = coord[1];
		double lval;
		for (int i = 0; i < 2; i++) {
			if (wxString(pos[i]).ToDouble(&lval)) {
				pos[i] = std::to_string(coord[i]);
			}
		}
	}
	virtual Vec2f GetPosition(float time) {
		return pos.eval(time);
	}
	virtual void SetScale(float value) {
		double lval;
		if (wxString(scale[0]).ToDouble(&lval)) {
			scale[0] = std::to_string(value);
		}
	}
	virtual float GetScale(float time) {
		return scale.eval1(time);
	}

	Vec2s pos;
	Vec3u color, edgeColor;
	Expr scale, angle, edgeThickness;
	VerticesList vertices;
	mutable std::vector<FastVec2f> fastTransformedVertices;
};

class PolygonMorph : public Shape {
public:
	PolygonMorph(PolygonShape* s1 = NULL, PolygonShape* s2 = NULL, std::string t = std::string("0"), bool visible = true) : Shape(visible), s1(s1), s2(s2), t(t) {
		this->t.name = "Interp. time"; this->t.encodedTypes = "T";
		parameters.push_back((Keyable*)&this->t);
	};
	PolygonMorph(const PolygonMorph& d) : Shape(d.visible[0]), t(d.t), s1(d.s1), s2(d.s2) {
		this->t.name = "Interp. time"; this->t.encodedTypes = "T";
		parameters.push_back((Keyable*)&this->t);
	}

	PolygonShape interpolatedShape(float time) const {
		float tval = t.eval1(time);
		float scale1 = s1->GetScale(time);
		float scale2 = s2->GetScale(time);
		Vec2f pos1 = s1->GetPosition(time);
		Vec2f pos2 = s2->GetPosition(time);
		float angle1 = s1->angle.eval1(time);
		float angle2 = s2->angle.eval1(time);
		Vec2f pos = (1.f - tval)*pos1 + tval * pos2;
		float scale = (1.f - tval)*scale1 + tval * scale2;
		float angle = (1.f - tval)*angle1 + tval * angle2;
		float thickness = (1.f - tval)*s1->edgeThickness.eval1(time) + tval * s2->edgeThickness.eval1(time);
		Vec3u col;
		col[0] = (1.f - tval)*s1->color[0] + tval * s2->color[0];
		col[1] = (1.f - tval)*s1->color[1] + tval * s2->color[1];
		col[2] = (1.f - tval)*s1->color[2] + tval * s2->color[2];
		Vec3u edgecol;
		edgecol[0] = (1.f - tval)*s1->edgeColor[0] + tval * s2->edgeColor[0];
		edgecol[1] = (1.f - tval)*s1->edgeColor[1] + tval * s2->edgeColor[1];
		edgecol[2] = (1.f - tval)*s1->edgeColor[2] + tval * s2->edgeColor[2];
		PolygonShape s(Vec2s(std::to_string(pos[0]), std::to_string(pos[1])), std::to_string(scale), std::to_string(angle), col, edgecol, std::to_string(thickness));

		typedef FullBipartiteDigraph Digraph;
		DIGRAPH_TYPEDEFS(FullBipartiteDigraph);

		int Nct = std::max(s1->vertices.contourList.size(), s2->vertices.contourList.size());
		for (int ct = 0; ct < Nct; ct++) {
			int n1 = s1->getComponentVtxCount(ct);
			int n2 = s2->getComponentVtxCount(ct);
			std::vector<Vec2f> v1; v1.reserve(n1 + 3);
			std::vector<Vec2f> v2; v2.reserve(n2 + 3);
			if (n1 > 0) {
				int start = 0;
				if (ct > 0) {
					start = s1->vertices.contourList[ct - 1] + 1;
				}
				int end = s1->vertices.contourList[ct];
				for (int i = start; i <= end; i++) {
					v1.push_back(s1->vertices.vertices[i]);
				}
			} else {
				Vec2f center(0, 0);
				int start = 0;
				if (ct > 0) {
					start = s2->vertices.contourList[ct - 1] + 1;
				}
				int end = s2->vertices.contourList[ct];
				for (int i = start; i <= end; i++) {
					center = center + s2->vertices.vertices[i];
				}
				center = center / (float)(end - start + 1.f);
				v1.push_back(center + Vec2f(0.001f, 0.001f));
				v1.push_back(center + Vec2f(-0.001f, 0.001f));
				v1.push_back(center + Vec2f(0.001f, -0.001f));
				n1 = 3;
			}
			if (n2 > 0) {
				int start = 0;
				if (ct > 0) {
					start = s2->vertices.contourList[ct - 1] + 1;
				}
				int end = s2->vertices.contourList[ct];
				for (int i = start; i <= end; i++) {
					v2.push_back(s2->vertices.vertices[i]);
				}
			} else {
				Vec2f center(0, 0);
				int start = 0;
				if (ct > 0) {
					start = s1->vertices.contourList[ct - 1] + 1;
				}
				int end = s1->vertices.contourList[ct];
				for (int i = start; i <= end; i++) {
					center = center + s1->vertices.vertices[i];
				}
				center = center / (float)(end - start + 1.f);
				v2.push_back(center + Vec2f(0.001f, 0.001f));
				v2.push_back(center + Vec2f(-0.001f, 0.001f));
				v2.push_back(center + Vec2f(0.001f, -0.001f));
				n2 = 3;
			}
			Digraph di(n1, n2);
			NetworkSimplexSimple<Digraph, double, double, int> net(di, true, n1 + n2, n1*n2);

			int idarc = 0;
			for (int i = 0; i < n1; i++) {
				Vec2f pt1 = rotate((v1[i] - pos1) / scale1, -angle1);
				for (int j = 0; j < n2; j++) {
					double d = norm2(  pt1 - rotate((v2[j] - pos2) / scale2, -angle2));
					net.setCost(di.arcFromId(idarc), d);
					idarc++;
				}
			}

			std::vector<double> weights1(n1, 1. / n1);
			std::vector<double> weights2(n2, -1. / n2);
			net.supplyMap(&weights1[0], n1, &weights2[0], n2);
			int ret = net.run();

			if (n1 > n2) {
				for (int i = 0; i < n1; i++) {
					Vec2f dest(0.f, 0.f);
					float sumAmount = 1;
					float prevAmount = -1;
					for (int j = 0; j < n2; j++) {
						float amount = net.flow(di.arcFromId(i*n2 + j));
						/*if (fabs(amount) > 1E-7) {
							dest = dest + amount * v2[j];
							sumAmount += amount;
						}*/
						if (amount > prevAmount) {
							prevAmount = amount;
							dest = v2[j];
						}
					}
					s.addVertex((1.f - tval)*v1[i] + tval * dest / sumAmount, i==0);
				}
			} else {
				for (int i = 0; i < n2; i++) {
					Vec2f dest(0.f, 0.f);
					float sumAmount = 1;
					float prevAmount = -1;
					for (int j = 0; j < n1; j++) {
						float amount = net.flow(di.arcFromId(j*n2 + i));
						/*if (fabs(amount) > 1E-7) {
							dest = dest + amount * v1[j];
							sumAmount += amount;
						}*/
						if (amount > prevAmount) {
							prevAmount = amount;
							dest = v1[j];
						}
					}
					s.addVertex((1.f - tval)*dest / sumAmount + tval * v2[i], i==0);
				}
			}

		}

		/*for (int i = 0; i < std::min(s1->vertices.vertices.size(), s2->vertices.vertices.size()); i++) {
			s.addVertex((1.f - tval)*s1->vertices.vertices[i] + tval * s2->vertices.vertices[i]);
		}*/
		return s;
	}

	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {
		if (visible[0]) {
			PolygonShape s(interpolatedShape(time));

			s.Draw(canvas, time, displayControls);
		}
	}

	virtual bool Contains(const Vec2f& initPoint, float time) const {
		PolygonShape s(interpolatedShape(time));
		return s.Contains(initPoint, time);
	}

	virtual Shape* Clone() {
		return new PolygonMorph(*this);
	}


	virtual void SetPosition(const Vec2f& coord) {
		//pos[0] = coord[0];
		//pos[1] = coord[1];
	}
	virtual Vec2f GetPosition(float time) {
		float tval = t.eval1(time);
		return (1.f - tval)*s1->GetPosition(time) + tval * s2->GetPosition(time);
	}
	virtual void SetScale(float value) {
		//scale[0] = value;
	}
	virtual float GetScale(float time) {
		float tval = t.eval1(time);
		return (1.f - tval)*s1->GetScale(time) + tval * s2->GetScale(time);
	}

	Expr t;
	PolygonShape *s1, *s2;
};

class Spline : public Shape {
public: 
	Spline(const Vec2s& position, std::string scale, const Vec3u &color, bool visible = true, std::string thickness = "1") : Shape(visible), pos(position), scale(scale), color(color), thickness(thickness){

		this->pos.name = "Center"; this->pos.encodedTypes = "SS";
		this->scale.name = "Scale"; this->scale.encodedTypes = "S";
		this->color.name = "Color"; this->color.encodedTypes = "C";
		this->thickness.name = "Thickness"; this->thickness.encodedTypes = "S";
		this->controlPoints.name = "Control Points"; this->controlPoints.encodedTypes = "L" + std::to_string(this->controlPoints.vertices.size());

		parameters.push_back((Keyable*)&this->pos);
		parameters.push_back((Keyable*)&this->scale);
		parameters.push_back((Keyable*)&this->color);
		parameters.push_back((Keyable*)&this->thickness);
		parameters.push_back((Keyable*)&this->controlPoints);
	}
	Spline(const Spline& d) : Shape(d.visible[0]), pos(d.pos), scale(d.scale), color(d.color), controlPoints(d.controlPoints), thickness(d.thickness){
		this->pos.name = "Center"; this->pos.encodedTypes = "SS";
		this->scale.name = "Scale"; this->scale.encodedTypes = "S";
		this->color.name = "Color"; this->color.encodedTypes = "C";
		this->thickness.name = "Thickness"; this->thickness.encodedTypes = "S";
		this->controlPoints.name = "Control Points"; this->controlPoints.encodedTypes = "L" + std::to_string(d.controlPoints.vertices.size());
		parameters.push_back((Keyable*)&this->pos);
		parameters.push_back((Keyable*)&this->scale);
		parameters.push_back((Keyable*)&this->color);
		parameters.push_back((Keyable*)&this->thickness);
		parameters.push_back((Keyable*)&this->controlPoints);
		this->controlPoints.firstParam = &this->controlPoints.vertices[0].coords[0];
	}

	void addVertex(const Vec2f& v) {
		controlPoints.addVertex(v);
		this->controlPoints.encodedTypes = "L" + std::to_string(this->controlPoints.vertices.size());
		this->controlPoints.firstParam = &this->controlPoints.vertices[0].coords[0];
	}

	//0 1/6 2/6 3/6 4/6 5/6 6/6
	Vec2f evalBSpline(float t) const { // t in 0..1
		Vec2f result;
		int N = controlPoints.vertices.size();
		int i = t * (N - 1);
		t = t * (N - 1) - i;
		if (i >= 1 && i <= N - 3) {
			result = (-controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i] - 3.f* controlPoints.vertices[i + 1] + controlPoints.vertices[i + 2])*(t*t*t / 6.f)
				+ (3.f*controlPoints.vertices[i - 1] - 6.f* controlPoints.vertices[i] + 3.f* controlPoints.vertices[i + 1])*(t*t / 6.f)
				+ (-3.f*controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i + 1])*(t / 6.f)
				+ (controlPoints.vertices[i - 1] + 4.f* controlPoints.vertices[i] + controlPoints.vertices[i + 1])*(1 / 6.f);
		} else {
			if (i == 0) {
				result = (-controlPoints.vertices[i] + 3.f* controlPoints.vertices[i] - 3.f* controlPoints.vertices[i + 1] + controlPoints.vertices[i + 2])*(t*t*t / 6.f)
					+ (3.f*controlPoints.vertices[i] - 6.f* controlPoints.vertices[i] + 3.f* controlPoints.vertices[i + 1])*(t*t / 6.f)
					+ (-3.f*controlPoints.vertices[i] + 3.f* controlPoints.vertices[i + 1])*(t / 6.f)
					+ (controlPoints.vertices[i] + 4.f* controlPoints.vertices[i] + controlPoints.vertices[i + 1])*(1 / 6.f);
			} else {
				if (i == N - 2) {
					result = (-controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i] - 3.f* controlPoints.vertices[i + 1] + controlPoints.vertices[i + 1])*(t*t*t / 6.f)
						+ (3.f*controlPoints.vertices[i - 1] - 6.f* controlPoints.vertices[i] + 3.f* controlPoints.vertices[i + 1])*(t*t / 6.f)
						+ (-3.f*controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i + 1])*(t / 6.f)
						+ (controlPoints.vertices[i - 1] + 4.f* controlPoints.vertices[i] + controlPoints.vertices[i + 1])*(1 / 6.f);
				} else { // i==N-1
					result = (-controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i] - 3.f* controlPoints.vertices[i] + controlPoints.vertices[i])*(t*t*t / 6.f)
						+ (3.f*controlPoints.vertices[i - 1] - 6.f* controlPoints.vertices[i] + 3.f* controlPoints.vertices[i])*(t*t / 6.f)
						+ (-3.f*controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i])*(t / 6.f)
						+ (controlPoints.vertices[i - 1] + 4.f* controlPoints.vertices[i] + controlPoints.vertices[i])*(1 / 6.f);
				}
			}
		}
		return result;
	}

	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {
		float s = scale.eval1(time);
		Vec2f p = pos.eval(time);
		int minX = 1E9, maxX = -1E9, minY = 1E9, maxY = -1E9;
		for (int i = 0; i < controlPoints.vertices.size(); i++) {
			minX = std::min(minX, (int)((controlPoints.vertices[i][0] * s + p[0])));
			maxX = std::max(maxX, (int)((controlPoints.vertices[i][0] * s + p[0]) + 0.5));
			minY = std::min(minY, (int)((controlPoints.vertices[i][1] * s + p[1])));
			maxY = std::max(maxY, (int)((controlPoints.vertices[i][1] * s + p[1]) + 0.5));
		}
		int ominX = minX, ominY = minY, omaxX = maxX, omaxY = maxY;
		minX = std::max(minX - 1, 0);
		maxX = std::min(maxX - 1, canvas.W - 1);
		minY = std::max(minY + 1, 0);
		maxY = std::min(maxY + 1, canvas.H - 1);

		if (visible[0]) {
			float edgeThick = thickness.eval1(time);
#pragma omp parallel for
			for (int i = 0; i < 100; i++) {
				Vec2f v = evalBSpline(i / 100.f)*s + p;
				if (v[0] >= 0 && v[0] <= canvas.W - 1 && v[1] >= 0 && v[1] <= canvas.H - 1) {
					/*canvas(v[0], v[1], 0) = color[0];
					canvas(v[0], v[1], 1) = color[1];
					canvas(v[0], v[1], 2) = color[2];*/
					Vec2f prevV = evalBSpline((i-1.f) / 100.f)*s + p;
					Vec2f nextV = evalBSpline((i + 1.f) / 100.f)*s + p;
					Vec2f nextnextV = evalBSpline((i + 1.f) / 100.f)*s + p;
					FastVec2f perpprev(-v[1] + prevV[1], v[0] - prevV[0]);
					FastVec2f perp(-nextV[1] + v[1], nextV[0] - v[0]);
					FastVec2f perpnext(-nextnextV[1] + nextV[1], nextnextV[0] - nextV[0]);
					FastVec2f v1 = getNormalized(perpprev + perp);
					FastVec2f v2 = getNormalized(perpnext + perp);
					//if (dot(perp2, v2 - v1) < 1E-5) perp = center - v1;

					drawLineForInsidePolyline(canvas, v[0], v[1], nextV[0], nextV[1], v1, v2, color, edgeThick*s);

				}
			}
		}

		if (displayControls) { // draw Bbox
			drawBox(canvas, ominX, ominY, omaxX, omaxY);
			int N = controlPoints.vertices.size();
			for (int i = 0; i < N-1; i++) {
				FastVec2f pp = controlPoints.vertices[i] * s + p;
				FastVec2f pn = controlPoints.vertices[i+1] * s + p;
				FastVec2f dir1 = getNormalized(pn - pp);
				FastVec2f v1;
				if (i >= 1) {
					FastVec2f pm = controlPoints.vertices[i-1] * s + p;
					v1 = dir1 + getNormalized(pm - pp);
				} else
					v1 = FastVec2f(-dir1[1], dir1[0]);
				v1 = -getNormalized(v1);
				FastVec2f v2;
				if (i < N - 2) {
					FastVec2f ppp = controlPoints.vertices[i +2] * s + p;
					v2 = getNormalized(ppp - pp) + getNormalized(pp - pn);
				} else {
					v2 = FastVec2f(-dir1[1], dir1[0]);
				}
				v2 = -getNormalized(v2);
				drawLineForInsidePolyline(canvas, pp[0], pp[1], pn[0], pn[1], v1, v2, Vec3u(0,0,0), 2);
			}
			for (int i = 0; i < controlPoints.vertices.size(); i++) {
				Vec2f pp = controlPoints.vertices[i] * s + p;
				drawSquare(canvas, pp[0], pp[1], 5);
			}
		}
	}

	virtual Shape* Clone() {
		return new Spline(*this);
	}

	virtual bool Contains(const Vec2f& initPoint, float time) const {
		float s = scale.eval1(time);
		Vec2f p = pos.eval(time);
		float d = 1E9;
		for (int i = 0; i < 50; i++) {
			d = std::min(d, norm2(evalBSpline(i / 50.f)*s+p - initPoint));
		}
		d = sqrt(d);
		if (d < 5) return true;
		return false;
	}

	void insertControlPoint(const Vec2f& v, int prevVertex) {
		controlPoints.insertVertex(v, prevVertex);
		this->controlPoints.encodedTypes = "L" + std::to_string(this->controlPoints.vertices.size());
		this->controlPoints.firstParam = &this->controlPoints.vertices[0].coords[0];
	}

	int nearestSegment(const Vec2f& initPoint, float s, float a, const Vec2f &p) const {  // s = evaluated scale ; segment = [result;result+1]
		Vec2f v = rotate((initPoint - p) / s, -a);
		float d = 1E9;
		int N = controlPoints.vertices.size();
		int result;
		int curContour = 0;
		for (int i = 0; i < N-1; i++) {
			int nextPoint = i + 1;
			if (i == controlPoints.contourList[curContour]) {
				/*if (curContour == 0) {
					nextPoint = 0;
				} else {
					nextPoint = controlPoints.contourList[curContour - 1] + 1;
				}*/
				curContour++;
				continue;
			}
			Vec2f v1 = controlPoints.vertices[nextPoint];
			Vec2f v2 = controlPoints.vertices[i];
			float absciProj = dot(v - v1, v2 - v1) / norm2(v2 - v1);
			float d1 = norm(v - v1);
			float d2 = norm(v - v2);
			if (absciProj <= 0) {  // dist point point
				if (d1 < d) {
					d = d1;
					result = i;
				}
			} else {
				if (absciProj >= 1) {
					if (d2 < d) {
						d = d2;
						result = (i + 1) % N;
					}
				} else { // dist point line
					Vec2f p = v1 + absciProj * (v2 - v1);
					float dp = norm(v - p);
					if (dp < d) {
						d = dp;
						result = i;
					}
				}
			}
		}
		return result;
	}

	int nearestControlPoint(const Vec2f& initPoint, float s, float a, const Vec2f &p, float searchRadius) const {  // s = evaluated scale ; segment = [result;result+1]
		Vec2f v = rotate((initPoint - p) / s, -a);
		float d = 1E9;
		int N = controlPoints.vertices.size();
		int result = -1;
		for (int i = 0; i < N; i++) {
			Vec2f v1 = controlPoints.vertices[i];
			float d1 = norm(v - v1);
			if (d1 < d && d1 < searchRadius) {
				d = d1;
				result = i;
			}
		}
		return result;
	}

	virtual void SetPosition(const Vec2f& coord) {
		//pos[0] = coord[0];
		//pos[1] = coord[1];
		double lval;
		for (int i = 0; i < 2; i++) {
			if (wxString(pos[i]).ToDouble(&lval)) {
				pos[i] = std::to_string(coord[i]);
			}
		}
	}
	virtual Vec2f GetPosition(float time) {
		return pos.eval(time);
	}
	virtual void SetScale(float value) {
		double lval;
		if (wxString(scale[0]).ToDouble(&lval)) {
			scale[0] = std::to_string(value);
		}
	}
	virtual float GetScale(float time) {
		return scale.eval1(time);
	}

	Vec2s pos;
	Vec3u color;
	Expr scale;
	Expr thickness;
	VerticesList controlPoints;
};