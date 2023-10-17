

#include "baseHeader.h"

#ifndef OBJ_MODEL_H
#define OBJ_MODEL_H

struct Txcoord
{
	float p[2];

	bool operator==(Txcoord& src)
	{
		return p[0] == src.p[0] && p[1] == src.p[1];
	}
};

struct Vertex
{
	float p[3];

	bool operator==(Vertex& src)
	{
		return p[0] == src.p[0] && p[1] == src.p[1] && p[2] == src.p[2];
	}
};

struct Norms
{
	float n[3];

	void inverse()
	{
		for (int i = 0; i < 3; i++)
			n[i]=-n[i];
	}

	bool operator==(Norms& src)
	{
		return n[0] == src.n[0] && n[1] == src.n[1] && n[2] == src.n[2];
	}
};

struct CVertex
{
	Vertex v;
	Txcoord t;
	Norms n;

	bool operator==(CVertex& src)
	{
		return v == src.v && t == src.t && n == src.n;
	}
};

struct Tfacet
{
	int v[3], t[3], n[3];

	void minus()
	{
		for (int i = 0; i < 3; i++)
		{
			v[i]--;
			t[i]--;
			n[i]--;
		}
	}
};

class ObjModel
{
private:
	bool isHidden=false;

	int n_Tri, n_Ind;
	float offset_x, offset_y;

	GLint list;
	GLfloat* pVerts, * pTxd, * pNorm;
	GLushort* pInd;
	GLuint VBO[4] = { 0 };

	ObjModel() :isHidden(false), n_Tri(0), offset_x(0.0), offset_y(0.0), list(0), pVerts(nullptr), pNorm(nullptr), pInd(nullptr) {}
	bool LoadObj(const char* pPathname, float scale_x, float scale_y, float scale_z);
	void InitVBO();

public:
	ObjModel(const char* path, float ox = 0.0, float oy = 0.0, float scale_x = 1.0, float scale_y = 1.0, float scale_z = 1.0);
	~ObjModel();
	void draw(int mode = 4, int drawmode = 0, GLint model_view_loc = -1);
	void hide();
	void show();
	bool isShowing();
};

#endif