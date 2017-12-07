/************************************************************************
\link	www.twinklingstar.cn
\author Twinkling Star
\date	2014/04/06
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include "SrVector3.h"
#include "SrAlgorithmQuickHull3D.h"
#include "SrAlgorithmCreateOBB.h"

#pragma comment (lib,"glew32.lib")
//#pragma comment (lib,"freeglut.lib")

bool		gMoveScene = false;
bool		gStopTimer = false;
bool		gDisplayMesh = false;

void outputInfo()
{
	printf("************************************\n");
	printf("S:	Stop or start timer        *\n");
	printf("C:	Display the mesh or not    *\n");
	printf("************************************\n");
}

void timerMessage(int id)
{
	glutPostRedisplay();
	if (!gStopTimer)
		glutTimerFunc(200, timerMessage, 0);
}

void setNormalLight(const SrVector3& lightPosition = SrVector3(0, 0, 0))
{
	SrVector3 lightColor = SrVector3(0.5, 0.5f, 0.5f);
	SrVector3 white = SrVector3(1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition.get());
	glLightfv(GL_LIGHT1, GL_AMBIENT, (lightColor / 5.0f).get());
	glLightfv(GL_LIGHT1, GL_DIFFUSE, (lightColor).get());
	glLightfv(GL_LIGHT1, GL_SPECULAR, white.get());
}

void drawRectangle(const SrPoint3D& p0, const SrPoint3D& p1, const SrPoint3D& p2, const SrPoint3D& p3)
{
	glBegin(GL_LINE_STRIP);
	glVertex3f(p0.x, p0.y, p0.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glVertex3f(p3.x, p3.y, p3.z);
	glEnd();
}

void drawOBB(SrVector3* axis, SrVector3& halfLength, SrPoint3D& center)
{
	int i;
	glColor3f(1.0f, 1.0f, 1.0f);
	SrVector3 ori, u, v;
	for (i = 0; i < 3; i++)
	{
		ori = center + axis[i] * halfLength[i];
		u = axis[(i + 1) % 3] * halfLength[(i + 1) % 3];
		v = axis[(i + 2) % 3] * halfLength[(i + 2) % 3];
		drawRectangle(ori - u - v, ori + u - v, ori + u + v, ori - u + v);
		ori = center - axis[i] * halfLength[i];
		drawRectangle(ori - u - v, ori + u - v, ori + u + v, ori - u + v);
	}
}

void renderMessage()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glTranslatef(-5.0f, -5.0f, -50.0f);

	setNormalLight();
	SrAlgorithmQuickHull3D hull3d;

	int numPoint = 100, i;

	tHull gHull;//凸包
	SrPoint3D* point = new SrPoint3D[numPoint];//点数据

	for (i = 0; i < numPoint; i++)//生成点数据
	{
		point[i].x = (float)(rand() % 100) / 10.0f;
		point[i].y = (float)(rand() % 100) / 10.0f;
		point[i].z = (float)(rand() % 100) / 10.0f;
	}

	if (hull3d.quickHull(point, numPoint, &gHull))
	{
		int i, j;
		for (i = 0; i < gHull.numFacet; i++)
		{
			if (!gDisplayMesh)
			{
				glColor3f((float)(rand() % 100) / 100.0f, (float)(rand() % 100) / 100.0f, (float)(rand() % 100) / 100.0f);
				glBegin(GL_TRIANGLES);
				for (j = 0; j < 3; j++)
				{
					glVertex3f(gHull.vertex[gHull.facet[i].vertexIndex[j]].x,
						gHull.vertex[gHull.facet[i].vertexIndex[j]].y,
						gHull.vertex[gHull.facet[i].vertexIndex[j]].z);
				}

				glEnd();
			}
			else
			{
				glColor3f(1.0f, 1.0f, 1.0f);
				glBegin(GL_LINES);
				for (j = 0; j < 3; j++)
				{
					glVertex3f(gHull.vertex[gHull.facet[i].vertexIndex[j]].x,
						gHull.vertex[gHull.facet[i].vertexIndex[j]].y,
						gHull.vertex[gHull.facet[i].vertexIndex[j]].z);
					glVertex3f(gHull.vertex[gHull.facet[i].vertexIndex[(j + 1) % 3]].x,
						gHull.vertex[gHull.facet[i].vertexIndex[(j + 1) % 3]].y,
						gHull.vertex[gHull.facet[i].vertexIndex[(j + 1) % 3]].z);
				}
				glEnd();
			}
		}
		SrVector3 axis[3], halfLength, center;

		SrAlgorithmCreateOBB::createOBB(&gHull, axis, halfLength, center);

		drawOBB(axis, halfLength, center);

		delete[] gHull.facet;
		delete[] gHull.vertex;
	}

	delete[]point;

	glFlush();
}

void init()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	float MatAmb[] = { 0.4f, 0.4f, 0.4f, 1.0f };				// Material - Ambient Values
	float MatDif[] = { 0.2f, 0.6f, 0.9f, 1.0f };				// Material - Diffuse Values
	float MatSpc[] = { 0.5f, 0.5f, 0.5f, 1.0f };				// Material - Specular Values
	float MatShn[] = { 1.0f };
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glMaterialfv(GL_FRONT, GL_AMBIENT, MatAmb);			// Set Material Ambience
	glMaterialfv(GL_FRONT, GL_DIFFUSE, MatDif);			// Set Material Diffuse
	glMaterialfv(GL_FRONT, GL_SPECULAR, MatSpc);		// Set Material Specular
	glMaterialfv(GL_FRONT, GL_SHININESS, MatShn);		// Set Material Shininess
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT1);
}

void reshapeMessage(GLsizei w, GLsizei h)
{
	glViewport(0, 0, w, h);
	float pinf[4][4];
	pinf[0][1] = pinf[0][2] = pinf[0][3] = pinf[1][0] =
		pinf[1][2] = pinf[1][3] = pinf[3][0] = pinf[3][1] =
		pinf[3][3] = 0;
	float nearPlane = 1.0f;
	float farPlane = 10000.0f;
	float fovy = 45;
	float top = nearPlane*tan(3.1415926*fovy / 360.0f);
	float bott = -top;
	float right = top*(float)w / (float)h;
	float left = -right;
	pinf[0][0] = 2 * nearPlane / (right - left);
	pinf[1][1] = 2 * nearPlane / (top - bott);
	pinf[2][0] = (right + left) / (right - left);
	pinf[2][1] = (top + bott) / (top - bott);
	pinf[2][2] = pinf[2][3] = -1;
	pinf[3][2] = -2 * nearPlane;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glLoadMatrixf(&pinf[0][0]);
	glMatrixMode(GL_MODELVIEW);
}

void keyMessage(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 's':
		case 'S':
			gStopTimer = !gStopTimer;
			if (!gStopTimer)
			{
				glutTimerFunc(200, timerMessage, 0);
			}
			break;
		case 'C':
		case 'c':
			gDisplayMesh = !gDisplayMesh;
			glutPostRedisplay();
			break;
	}
}

int main(int argc, char ** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(400, 400);

	glutCreateWindow("Create OBB");

	init();
	outputInfo();
	glutReshapeFunc(reshapeMessage);
	glutDisplayFunc(renderMessage);
	glutTimerFunc(200, timerMessage, 0);
	glutKeyboardFunc(keyMessage);

	glutMainLoop();
	return(0);
}