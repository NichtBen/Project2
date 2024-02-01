#include <GL/freeglut.h>

float angle = 0.0f;

void renderFunction() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -5.0f);  // Move the cube away from the camera
    glRotatef(angle, 1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    glVertex3f(-0.5f, -0.5f, 0.f);
    glVertex3f(0.5f, -0.5f, 0.f);
    glVertex3f(0.5f, 0.5f, 0.f);
    glVertex3f(-0.5f, 0.5f, 0.f);
    glVertex3f(-0.5f, 0.5f, 1.0f);
    glVertex3f(0.5f, 0.5f, 1.0f);
    glVertex3f(0.5f, -0.5f, 1.0f);
    glVertex3f(-0.5f, -0.5f, 1.0f);
    glEnd();

    glPopMatrix();

    glutSwapBuffers();
}

void update(int value) {
    angle += 2.0f;

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspectRatio, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutCreateWindow("FreeGLUT Test");

    glEnable(GL_DEPTH_TEST);  // Enable depth testing

    glutDisplayFunc(renderFunction);
    glutReshapeFunc(reshape);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glutTimerFunc(16, update, 0);

    glutMainLoop();

    return 0;
}
