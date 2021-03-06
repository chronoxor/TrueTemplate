/*==========================================================================*/
/*
@@INCLUDE:$\templates\source\header-base@@
*/
/*--------------------------------------------------------------------------*/
/*
@@INCLUDE:$\templates\source\header-gnu@@
*/
/*--------------------------------------------------------------------------*/
/*
@@INCLUDE:$\templates\source\header-cvs@@
*/
/*==========================================================================*/
#ifndef __@@UPPERFILE_NAME@@_@@UPPERFILE_EXT@@__
#define __@@UPPERFILE_NAME@@_@@UPPERFILE_EXT@@__
/*==========================================================================*/
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
/*==========================================================================*/
double angleX = 0.0;
double angleY = 0.0;
double angleZ = 0.0;
/*==========================================================================*/
void init(void)
{
	// Select clearing color
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_SMOOTH);
}
/*--------------------------------------------------------------------------*/
void display(void)
{
	// Clear all pixels
	glClear(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Clear the matrix
	glLoadIdentity();

	// Viewing transformation
	gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	// Do a simple rotations
	glRotated(angleX, 1.0, 0.0, 0.0);
	glRotated(angleY, 0.0, 1.0, 0.0);
	glRotated(angleZ, 0.0, 0.0, 1.0);

	// Render triangle
	glBegin(GL_TRIANGLES);
	{
		glColor3f(1.0, 0.0, 0.0);
		glVertex2f(1.0, 1.0);
		glColor3f(0.0, 1.0, 0.0);
		glVertex2f(0.0, 1.0);
		glColor3f(0.0, 0.0, 1.0);
		glVertex2f(1.0, 0.0);
	}
	glEnd();

	// Start processing buffered OpenGL routines
	glFlush();
	glutSwapBuffers();
}
/*--------------------------------------------------------------------------*/
void reshape(int w, int h)
{
	glViewport(0,0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 1.5, 20.0);
	glMatrixMode(GL_MODELVIEW);
}
/*--------------------------------------------------------------------------*/
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 'w':
		case 'W':
		{
			angleX += 5.0;
			glutPostRedisplay();
			break;
		}
		case 's':
		case 'S':
		{
			angleX -= 5.0;
			glutPostRedisplay();
			break;
		}
		case 'a':
		case 'A':
		{
			angleY += 5.0;
			glutPostRedisplay();
			break;
		}
		case 'd':
		case 'D':
		{
			angleY -= 5.0;
			glutPostRedisplay();
			break;
		}
		case 'q':
		case 'Q':
		{
			angleZ += 5.0;
			glutPostRedisplay();
			break;
		}
		case 'e':
		case 'E':
		{
			angleZ -= 5.0;
			glutPostRedisplay();
			break;
		}
		case 27:
		{
			exit(0);
			break;
		}
		default:
			break;
	}
}
/*--------------------------------------------------------------------------*/
void mouse(int button, int state, int x, int y)
{
	switch (button)
	{
		case GLUT_LEFT_BUTTON:
		{
			if (state == GLUT_DOWN)
			{
				angleZ += 5.0;
				glutPostRedisplay();
			}
			break;
		}
		case GLUT_RIGHT_BUTTON:
		{
			if (state == GLUT_DOWN)
			{
				angleZ -= 5.0;
				glutPostRedisplay();
			}
			break;
		}
		default:
			break;
	}
}
/*--------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
	@@HERE@@glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ACCUM | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("The Hello Program");
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMainLoop();
	return 0;
}
/*==========================================================================*/
#endif
