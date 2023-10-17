#include "ObjModel.h"

#include <fstream>
#include <vector>
#include <math.h>
#include <iostream>

using namespace std;

ObjModel::ObjModel(const char* path, float ox, float oy, float scale_x, float scale_y, float scale_z) : offset_x(ox), offset_y(oy), list(0), pVerts(0)
{
	LoadObj(path, scale_x, scale_y, scale_z);
	InitVBO();
}

bool ObjModel::LoadObj(const char* pPathname, float scale_x, float scale_y, float scale_z)
{
	delete[] pVerts, pTxd, pNorm, pInd;

	try
	{
		if (!pPathname)
			throw "File not found!";

		ifstream fin;
		fin.open(pPathname, ios::in);

		if (!fin.good())
			throw "Error loading file!";

		const int strLen = 512;
		char str[strLen];

		fin.getline(str, strLen);
		if (!fin.good())
			throw "Error loading file!";
		
		//read to the start of the first obj
		while (!fin.eof())
		{
			fin.getline(str, strLen);
			if (str[0] == 'o' && str[1] == ' ')
				break;
		}
		
		std::vector<Vertex> vertices;
		std::vector<Txcoord> txcoords;
		std::vector<Norms> norms;
		std::vector<Tfacet> faces;

		cout << "Start loading "<< pPathname << endl;
		while (!fin.eof())
		{
			fin.getline(str, strLen);

			if (fin.eof()) break;

			if (strncmp(str, "v ", 2) == 0)
			{
				Vertex n_ver;
				sscanf(str, "v %f %f %f", &n_ver.p[0], &n_ver.p[1], &n_ver.p[2]);

				n_ver.p[0] *= scale_x;
				n_ver.p[1] *= scale_y;
				n_ver.p[2] *= scale_z;

				vertices.push_back(n_ver);
			}
			else if (strncmp(str, "vt ", 3) == 0)
			{
				Txcoord n_txc;
				sscanf(str, "vt %f %f", &n_txc.p[0], &n_txc.p[1]);
				txcoords.push_back(n_txc);
			}
			else if (strncmp(str, "vn ", 3) == 0)
			{
				Norms n_nor;
				sscanf(str, "vn %f %f %f", &n_nor.n[0], &n_nor.n[1], &n_nor.n[2]);

				n_nor.n[0] *= scale_x;
				n_nor.n[1] *= scale_y;
				n_nor.n[2] *= scale_z;

				n_nor.inverse();
				norms.push_back(n_nor);
			}
			else if (strncmp(str, "f ", 2) == 0)
			{
				Tfacet face;
				sscanf(str, "f %d/%d/%d %d/%d/%d %d/%d/%d", &face.v[0], &face.t[0], &face.n[0], &face.v[1], &face.t[1], &face.n[1], &face.v[2], &face.t[2], &face.n[2]);
				face.minus();
				faces.push_back(face);
			}
			else if (str[0] == 'o' && str[1] == ' ')
				break;
		}
		fin.clear();
		fin.close();
		
		cout << "Totally " << vertices.size() << " vertices, " << txcoords.size() << " txcoords, " << norms.size()<<" norms, " << faces.size() << " faces."<<endl;

		std::vector<CVertex> cvertices;
		std::vector<int> indices;

		for (int i = 0; i < faces.size(); i++)	//go through the faces
		{
			CVertex cvt[3];

			for (int j = 0; j < 3; j++)	//three vertices per face
			{

				if (faces[i].v[j] >= vertices.size())
					cout << "face overflow : " << faces[i].v[j] << "  " << vertices.size() << endl;
				else
					cvt[j].v = vertices[faces[i].v[j]];

				if (faces[i].t[j] >= txcoords.size())
					cout << "txd overflow : " << faces[i].t[j] << "  " << txcoords.size() << endl;
				else
					cvt[j].t = txcoords[faces[i].t[j]];

				if (faces[i].n[j] >= norms.size())
					cout << "norms overflow : " << faces[i].n[j] << "  " << norms.size() << endl;
				else
					cvt[j].n = norms[faces[i].n[j]];
				
				bool repeated = false;
				
				for (int k = 0; k < cvertices.size() && !repeated; k++)
					if (cvt[j] == cvertices[k])
					{
						indices.push_back(k);
						repeated = true;
					}
				if (repeated) continue;
				
				cvertices.push_back(cvt[j]);
				indices.push_back(cvertices.size()-1);
			}
		}

		n_Tri = cvertices.size();
		n_Ind = indices.size();
		
		pVerts = new GLfloat[cvertices.size() * 3];
		pTxd = new GLfloat[cvertices.size() * 2];
		pNorm = new GLfloat[cvertices.size() * 3];
		pInd = new GLushort[indices.size()];

		for (int i = 0; i < indices.size(); i++)
		{
			pInd[i] = indices[i];
		}

		for (int i = 0; i < cvertices.size(); i++)
		{
			for (int j = 0; j < 3; j++)
			{
				pVerts[i * 3 + j] = cvertices[i].v.p[j];
				pNorm[i * 3 + j] = cvertices[i].n.n[j];
			}
			for (int j = 0; j < 2; j++)
				pTxd[i * 2 + j] = cvertices[i].t.p[j];
		}

		vector<Vertex>().swap(vertices);
		vector<Txcoord>().swap(txcoords);
		vector<Norms>().swap(norms);
		vector<Tfacet>().swap(faces);
		vector<CVertex>().swap(cvertices);
		vector<int>().swap(indices);
	}
	catch (exception *e)
	{
		cout << e->what();
	}
	catch (char* str)
	{
		cout << str;
	}
	return 0;
}

void ObjModel::InitVBO()
{
	printf("Initializing vertex buffer object:");

	try
	{
		glGenBuffers(4, VBO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * n_Tri * 3 * 3, pVerts, GL_STATIC_DRAW);
		printf("vertices,");

		glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * n_Tri * 3 * 3, pNorm, GL_STATIC_DRAW);
		printf("norm,");

		glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * n_Tri * 3 * 2, pTxd, GL_STATIC_DRAW);
		printf("txd,");

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[3]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * n_Ind, pInd, GL_STATIC_DRAW);
		printf("ind");
		printf("...Done!\n");
	}
	catch (...)
	{
		printf("...Failed!\n");
	}

}

ObjModel::~ObjModel()
{
	delete[] pVerts, pTxd, pNorm, pInd;
}

void ObjModel::draw(int mode, int drawmode, GLint model_view_loc)
{
	if (isHidden)return;

	// temporarily enforce the behavior
	mode=4;

	if (model_view_loc>=0)
	{
		M3DMatrix44f model_view;
		glGetFloatv(GL_MODELVIEW_MATRIX, model_view);
		glUniformMatrix4fv(model_view_loc, 1, GL_FALSE, &model_view[0]);
	}

	switch (mode)
	{

	case 0://Normal for loop
		glBegin(GL_TRIANGLES);
		for (int i = 0; i < n_Ind;)
		{
			glNormal3fv(pNorm + pInd[i]*3);
			glVertex3fv(pVerts + pInd[i]*3);
			glTexCoord2fv(pTxd + pInd[i]*2);i++;
			glNormal3fv(pNorm + pInd[i]*3);
			glVertex3fv(pVerts + pInd[i]*3);
			glTexCoord2fv(pTxd + pInd[i]*2);i++;
			glNormal3fv(pNorm + pInd[i]*3);
			glVertex3fv(pVerts + pInd[i]*3);
			glTexCoord2fv(pTxd + pInd[i]*2);i++;
		}
		glEnd();
		break;

	case 1://Display List
		/*glBegin(drawmode);
		glCallList(list);
		glEnd();*/
		break;

	case 2://VAO
	case 3:
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, pVerts);

		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, pNorm);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, pTxd);

		glDrawElements(drawmode, n_Ind, GL_UNSIGNED_SHORT, pInd);
		glDisableClientState(GL_VERTEX_ARRAY);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		break;

	case 4://VBO
		glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, NULL);
		
		glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[3]);
		glDrawElements(drawmode, n_Ind, GL_UNSIGNED_SHORT, NULL);
		glDisableClientState(GL_VERTEX_ARRAY);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		break;

	default:
		break;
	}
}

void ObjModel::hide()
{
	isHidden = true;
}

void ObjModel::show()
{
	isHidden = false;
}

bool ObjModel::isShowing()
{
	return !isHidden;
}