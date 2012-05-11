#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

const GLdouble FRUSTDIM = 100.f;

/*
** Create a single component texture map
*/
GLfloat *make_texture(int maxs, int maxt)
{
    int s, t;
    static GLfloat *texture;

    texture = (GLfloat *)malloc(maxs * maxt * sizeof(GLfloat));
    for(t = 0; t < maxt; t++) {
	for(s = 0; s < maxs; s++) {
	    texture[s + maxs * t] = ((s >> 4) & 0x1) ^ ((t >> 4) & 0x1);
	}
    }
    return texture;
}

enum {SPHERE = 1, CONE};

void
render(void)
{
    /* material properties for objects in scene */
    static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};

    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    /*
    ** Note: wall verticies are ordered so they are all front facing
    ** this lets me do back face culling to speed things up.
    */
 
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

    /* floor */
    /* make the floor textured */
    glEnable(GL_TEXTURE_2D);

    /*
    ** Since we want to turn texturing on for floor only, we have to
    ** make floor a separate glBegin()/glEnd() sequence. You can't
    ** turn texturing on and off between begin and end calls
    */
    glBegin(GL_QUADS);
    glNormal3f(0.f, 1.f, 0.f);
    glTexCoord2i(0, 0);
    glVertex3f(-100.f, -100.f, -320.f);
    glTexCoord2i(1, 0);
    glVertex3f( 100.f, -100.f, -320.f);
    glTexCoord2i(1, 1);
    glVertex3f( 100.f, -100.f, -520.f);
    glTexCoord2i(0, 1);
    glVertex3f(-100.f, -100.f, -520.f);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    /* walls */

    glBegin(GL_QUADS);
    /* left wall */
    glNormal3f(1.f, 0.f, 0.f);
    glVertex3f(-100.f, -100.f, -320.f);
    glVertex3f(-100.f, -100.f, -520.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glVertex3f(-100.f,  100.f, -320.f);

    /* right wall */
    glNormal3f(-1.f, 0.f, 0.f);
    glVertex3f( 100.f, -100.f, -320.f);
    glVertex3f( 100.f,  100.f, -320.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f( 100.f, -100.f, -520.f);

    /* ceiling */
    glNormal3f(0.f, -1.f, 0.f);
    glVertex3f(-100.f,  100.f, -320.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f( 100.f,  100.f, -320.f);

    /* back wall */
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-100.f, -100.f, -520.f);
    glVertex3f( 100.f, -100.f, -520.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glEnd();


    glPushMatrix();
    glTranslatef(-80.f, -60.f, -420.f);
    glCallList(SPHERE);
    glPopMatrix();


    glPushMatrix();
    glTranslatef(-20.f, -80.f, -500.f);
    glCallList(CONE);
    glPopMatrix();

    if(glGetError()) /* to catch programming errors; should never happen */
	printf("Oops! I screwed up my OpenGL calls somewhere\n");

    glFlush(); /* high end machines may need this */
}

/* compute scale factor for window->object space transform */
/* could use gluUnProject(), but probably too much trouble */
void
computescale(GLfloat *sx, GLfloat *sy)
{
    enum {XORG, YORG, WID, HT};
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    *sx = 2 * FRUSTDIM/viewport[WID];
    *sy = 2 * FRUSTDIM/viewport[WID];
}

enum {NONE, AA};

int rendermode = NONE;

void
menu(int selection)
{
    rendermode = selection;
    glutPostRedisplay();
}

/* Called when window needs to be redrawn */
void redraw(void)
{
    int i, j;
    int min, max;
    int count;
    GLfloat invx, invy;
    GLfloat scale, dx, dy;

    switch(rendermode) {
    case NONE:
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-FRUSTDIM, FRUSTDIM, -FRUSTDIM, FRUSTDIM, 320., 640.); 
	glMatrixMode(GL_MODELVIEW);
	render();
	break;
    case AA:
	min = -2;
	max = -min + 1;
	count = -2 * min + 1;
	count *= count;

	/* uniform scaling, less than one pixel wide */
	scale = -.9f/min;

	computescale(&invx, &invy);

	glutSetCursor(GLUT_CURSOR_WAIT);

	glClear(GL_ACCUM_BUFFER_BIT);

	for(j = min; j < max; j++) {
	    for(i = min; i < max; i++) {
		printf("pass %d of %d\n",
		       (j-min)*(max-min)+i-min+1,(max-min)*(max-min));
		dx = invx * scale * i;
		dy = invy * scale * j;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-FRUSTDIM + dx, 
			  FRUSTDIM + dy, 
			  -FRUSTDIM + dx, 
			  FRUSTDIM + dy, 
			  320., 640.); 
		glMatrixMode(GL_MODELVIEW);
		render();
		glAccum(GL_ACCUM, 1.f/count);
	    }
	}
	glAccum(GL_RETURN, 1.f);
	glutSetCursor(GLUT_CURSOR_INHERIT);
	break;
    }

    glutSwapBuffers();
}

void key(unsigned char key, int x, int y)
{
    if(key == '\033')
	exit(0);
}


const int TEXDIM = 256;
/* Parse arguments, and set up interface between OpenGL and window system */
main(int argc, char *argv[])
{
    GLfloat *tex;
    static GLfloat lightpos[] = {50.f, 50.f, -320.f, 1.f};
    static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};
    GLUquadricObj *sphere, *cone, *base;

    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_ACCUM|GLUT_DOUBLE);
    (void)glutCreateWindow("Anti-aliasing with Accum");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);

    glutCreateMenu(menu);
    glutAddMenuEntry("Aliased View", NONE);
    glutAddMenuEntry("AntiAliased", AA);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-FRUSTDIM, FRUSTDIM, -FRUSTDIM, FRUSTDIM, 320., 640.); 
    glMatrixMode(GL_MODELVIEW);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glNewList(SPHERE, GL_COMPILE);
    /* make display lists for sphere and cone; for efficiency */
    sphere = gluNewQuadric();
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
    gluSphere(sphere, 20.f, 20, 20);
    gluDeleteQuadric(sphere);
    glEndList();

    glNewList(CONE, GL_COMPILE);
    cone = gluNewQuadric();
    base = gluNewQuadric();
    glRotatef(-90.f, 1.f, 0.f, 0.f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
    gluDisk(base, 0., 20., 20, 1);
    gluCylinder(cone, 20., 0., 60., 20, 20);
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glEndList();

    /* load pattern for current 2d texture */
    tex = make_texture(TEXDIM, TEXDIM);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
    free(tex);

    glReadBuffer(GL_BACK); /* input to accum buffer */

    glutMainLoop();

    return 0;
}
