/*
  Author: Emanuele Rodol√ 
*/

#include <QtGui>
#include <QtOpenGL>

#include <math.h>
#include <cvlab/mesh.h>
#include <cvlab/cloud3d.h>
#include <iostream>

#include "glscene.h"

using cvlab::point3d;
using cvlab::point2d;
using cvlab::matrix;
using glscene::item;
using glscene::item_id;

QGLScene::QGLScene(QWidget *parent)
      : QGLWidget(parent)
      , center_x(0.)
      , center_y(0.)
      , center_z(0.)
      , selected_item(-1)
      , center_item(-1)
      , picking_buf( new GLuint[64] )
      , xRot(16*180)
      , yRot()
      , zRot(0)
      , dx(0.)
      , dy(0.)
{
   view_volume.left = -0.15;
   view_volume.right = 0.15;
   view_volume.bottom = 0.15;
   view_volume.top = -0.15;
   view_volume.near = -40.0;
   view_volume.far = 15.0;

   setFocusPolicy(Qt::StrongFocus); // for grabbing keyboard events

   memset(picking_buf,0,64);
}

QGLScene::~QGLScene() {
   makeCurrent(); // enable this widget as the current opengl rendering context

   for (uint32_t k=0; k<items.size(); ++k)
      glDeleteLists(items[k].list_id, items[k].n_lists);

   delete[] picking_buf;
}

void QGLScene::setXRotation(int angle) {
   normalizeAngle(&angle);
   if (angle != xRot) {
      xRot = angle;
      updateGL();
   }
}

void QGLScene::setYRotation(int angle) {
   normalizeAngle(&angle);
   if (angle != yRot) {
      yRot = angle;
      updateGL();
   }
}

void QGLScene::setZRotation(int angle) {
   normalizeAngle(&angle);
   if (angle != zRot) {
      zRot = angle;
      updateGL();
   }
}

void QGLScene::zoom(double delta) {
   view_volume.left -= delta;
   view_volume.right += delta;
   view_volume.bottom += delta;
   view_volume.top -= delta;
   view_volume.near -= delta;
   view_volume.far += delta;
   updateGL();
}

/**
 * Given a bounding box, this function calculates its center and a
zoom level to use for fitting
 * the view on the box.
 *
 * @param mx       Min x-coord of the box.
 * @param Mx       Max x-coord of the box.
 * @param my       Min y-coord of the box.
 * @param My       Max y-coord of the box.
 * @param mz       Min z-coord of the box.
 * @param Mz       Max z-coord of the box.
 * @param center   The calculated box center.
 * @param zoom_fit The zoom value to pass to the zoom function for a
best fit of the view to the given box.
 * @sa zoom()
 */
/*void QGLScene::fit_view(double mx, double Mx, double my, double My,
    double mz, double Mz, point3d& center, double& zoom_fit) const
{
   center.x = (Mx + mx) / 2.;
   center.y = (My + my) / 2.;
   center.z = (Mz + mz) / 2.;

   const double boxw = Mx-mx;
   const double boxh = My-my;
   if (boxw > boxh)
      zoom_fit = (boxw - (view_volume.right - view_volume.left)) / 2.;
   else
      zoom_fit = (boxh - (view_volume.bottom - view_volume.top)) / 2.;
}*/

void QGLScene::set_view_volume(const ortho_volume& v)
{
   view_volume = v;
   updateGL();
}

void QGLScene::move_object(double deltax, double deltay) {
   dx += deltax;
   dy += deltay;
   updateGL();
}

/**
 * Add a camera object to the scene. A camera object is a pyramid whose apex
 * is the projection center and with a given image attached to the base.
 *
 * @param image     Image to be attached to the base of the pyramid.
 * @param focal_len Focal length of the camera (in pixels).
 * @param R         Rotation matrix.
 * @param T         Translation vector.
 * @return An item id for the added camera object is returned.
 */
item_id QGLScene::add_camera(

      const QImage&         image,
            double          focal_len,
      const matrix<double>& R,
      const point3d&        _T

      ) {
   const point3d T(_T.x, _T.y, -_T.z);

   const int width = image.width();
   const int height = image.height();
   const point2d c(width/2., height/2.);

   point3d top_left(-c.x / focal_len, -c.y / focal_len, 1);
   point3d top_right((width-c.x) / focal_len, top_left.y, 1);
   point3d botm_left(top_left.x, (height-c.y) / focal_len, 1);
   point3d botm_right(top_right.x, botm_left.y, 1);

   rotate_translate(top_left, R, T);
   rotate_translate(top_right, R, T);
   rotate_translate(botm_left, R, T);
   rotate_translate(botm_right, R, T);

   /*point3d pc(R(0,2), R(1,2), R(2,2)); // projection center
   pc += T;*/
   const point3d pc(T);

   // the objects are not added to the scene, but rather kept in video
   // memory for further assemblage with a hierachical display list.

   item_id line1 = add_line(pc, top_left, false);
   item_id line2 = add_line(pc, top_right, false);
   item_id line3 = add_line(pc, botm_left, false);
   item_id line4 = add_line(pc, botm_right, false);
   item_id image_id = add_floating_image(
         top_left, top_right,
         botm_left, botm_right,
         QImage(image.scaled(QSize(320,200), Qt::KeepAspectRatio, Qt::SmoothTransformation)),
         false);

   GLuint list = glGenLists(1); // this list assembles the created objects

   glNewList(list, GL_COMPILE); {

      // this way we can delete or replace a child list, call the parent list,
      // and see changes that were made

      glCallList(image_id);
      glCallList(line1);
      glCallList(line2);
      glCallList(line3);
      glCallList(line4);

      // the pyramid base
      glDisable(GL_LIGHTING);
      glBegin(GL_LINE_LOOP);
         glColor3f(1,1,1);
         glVertex3f(top_left.x, top_left.y, top_left.z);
         glVertex3f(top_right.x, top_right.y, top_right.z);
         glVertex3f(botm_right.x, botm_right.y, botm_right.z);
         glVertex3f(botm_left.x, botm_left.y, botm_left.z);
      glEnd();
      glEnable(GL_LIGHTING);

   } glEndList();

   items.push_back( item(list,1) );
   return items.size()-1;
}

void QGLScene::put_some_objects_in_the_scene() {

   point3d barycenter(0,0,0);

   //cvlab::mesh model;
   //model.load_range_from_ply("../GTRegistration/data/bunny/bun000.ply", 0.008);

   /////-------------------------------------------------------------------------------------------
   /////                            TEST
   /////-------------------------------------------------------------------------------------------
   //model.test_spin();
   //model.test_spin_fine();

   //barycenter = model.get_barycenter();

   // The center axes frame is always the first item
   items.push_back( add_center_axes() );
   center_item = 0;

   if (true){
cvlab::matrix<double> r_0(3,3);
r_0(0,0) = 9.9883080428e-01;
r_0(0,1) = -1.1725533085e-02;
r_0(0,2) = 4.6899214311e-02;
r_0(1,0) = 6.7932354224e-03;
r_0(1,1) = 9.9455656105e-01;
r_0(1,2) = 1.0397643399e-01;
r_0(2,0) = -4.7863100418e-02;
r_0(2,1) = -1.0353626779e-01;
r_0(2,2) = 9.9347338408e-01;

point3d t_0(1.7713500383e+00,1.4698441518e+00,-3.9937280036e+00);

add_camera(QImage("IMG_0149.jpg"),5.3200000000e+02,r_0,t_0);


cvlab::matrix<double> r_1(3,3);
r_1(0,0) = 9.8638013040e-01;
r_1(0,1) = -7.2994937660e-02;
r_1(0,2) = 1.4739734541e-01;
r_1(1,0) = 6.1292760651e-02;
r_1(1,1) = 9.9470977566e-01;
r_1(1,2) = 8.2435791414e-02;
r_1(2,0) = -1.5263497584e-01;
r_1(2,1) = -7.2278636472e-02;
r_1(2,2) = 9.8563601946e-01;

point3d t_1(6.6500576957e-01,1.3933886951e+00,-3.2191751719e+00);

add_camera(QImage("IMG_0150.jpg"),5.3200000000e+02,r_1,t_1);


cvlab::matrix<double> r_2(3,3);
r_2(0,0) = 9.9120804924e-01;
r_2(0,1) = -5.5669889863e-02;
r_2(0,2) = 1.2003110632e-01;
r_2(1,0) = 4.7078087241e-02;
r_2(1,1) = 9.9620065434e-01;
r_2(1,2) = 7.3266022134e-02;
r_2(2,0) = -1.2365377804e-01;
r_2(2,1) = -6.6971035980e-02;
r_2(2,2) = 9.9006293917e-01;

point3d t_2(3.4901605987e-01,1.2991789178e+00,-2.9423928091e+00);

add_camera(QImage("IMG_0151.jpg"),5.3200000000e+02,r_2,t_2);


cvlab::matrix<double> r_3(3,3);
r_3(0,0) = 9.9374915482e-01;
r_3(0,1) = -3.6902262544e-02;
r_3(0,2) = 1.0536052543e-01;
r_3(1,0) = 2.9108666206e-02;
r_3(1,1) = 9.9679058056e-01;
r_3(1,2) = 7.4573615052e-02;
r_3(2,0) = -1.0777431443e-01;
r_3(2,1) = -7.1040562564e-02;
r_3(2,2) = 9.9163397260e-01;

point3d t_3(-1.9582673901e-02,1.1514267424e+00,-2.6075178907e+00);

add_camera(QImage("IMG_0152.jpg"),5.3200000000e+02,r_3,t_3);


cvlab::matrix<double> r_4(3,3);
r_4(0,0) = 9.9369084211e-01;
r_4(0,1) = -2.9939496436e-02;
r_4(0,2) = 1.0808393431e-01;
r_4(1,0) = 2.4810125938e-02;
r_4(1,1) = 9.9851528030e-01;
r_4(1,2) = 4.8494253777e-02;
r_4(2,0) = -1.0937535350e-01;
r_4(2,1) = -4.5506719851e-02;
r_4(2,2) = 9.9295829242e-01;

point3d t_4(3.8511889975e-01,1.0407501339e+00,-2.0849220405e+00);

add_camera(QImage("IMG_0153.jpg"),5.3200000000e+02,r_4,t_4);


cvlab::matrix<double> r_5(3,3);
r_5(0,0) = 9.9903813967e-01;
r_5(0,1) = 1.5712056724e-02;
r_5(0,2) = 4.0938084505e-02;
r_5(1,0) = -1.7603304983e-02;
r_5(1,1) = 9.9877457194e-01;
r_5(1,2) = 4.6254492769e-02;
r_5(2,0) = -4.0161164614e-02;
r_5(2,1) = -4.6930647994e-02;
r_5(2,2) = 9.9809047442e-01;

point3d t_5(9.7707823824e-01,7.6689962311e-01,-1.4672112455e+00);

add_camera(QImage("IMG_0154.jpg"),5.3200000000e+02,r_5,t_5);


cvlab::matrix<double> r_6(3,3);
r_6(0,0) = 9.9720488725e-01;
r_6(0,1) = 7.4405102405e-02;
r_6(0,2) = 6.8039380598e-03;
r_6(1,0) = -7.4284452281e-02;
r_6(1,1) = 9.9709999005e-01;
r_6(1,2) = -1.6535718789e-02;
r_6(2,0) = -8.0145484216e-03;
r_6(2,1) = 1.5984072778e-02;
r_6(2,2) = 9.9984012544e-01;

point3d t_6(1.3755992726e+00,6.0488713297e-01,-9.3648977966e-01);

add_camera(QImage("IMG_0155.jpg"),5.3200000000e+02,r_6,t_6);


cvlab::matrix<double> r_7(3,3);
r_7(0,0) = 9.9723117117e-01;
r_7(0,1) = -3.9392674053e-02;
r_7(0,2) = 6.3073040854e-02;
r_7(1,0) = 3.9233858817e-02;
r_7(1,1) = 9.9922300030e-01;
r_7(1,2) = 3.7549954155e-03;
r_7(2,0) = -6.3171952430e-02;
r_7(2,1) = -1.2699996959e-03;
r_7(2,2) = 9.9800184946e-01;

point3d t_7(5.4205192052e-01,5.6112291278e-01,-5.9253578491e-01);

add_camera(QImage("IMG_0156.jpg"),5.3200000000e+02,r_7,t_7);


cvlab::matrix<double> r_8(3,3);
r_8(0,0) = 9.9822819459e-01;
r_8(0,1) = -2.5005397979e-02;
r_8(0,2) = 5.3992606943e-02;
r_8(1,0) = 2.6821637665e-02;
r_8(1,1) = 9.9908942195e-01;
r_8(1,2) = -3.3180215427e-02;
r_8(2,0) = -5.3113757969e-02;
r_8(2,1) = 3.4569596682e-02;
r_8(2,2) = 9.9798991563e-01;

point3d t_8(-2.4778467500e-02,3.5777040098e-01,-1.3112717464e-01);

add_camera(QImage("IMG_0157.jpg"),5.3200000000e+02,r_8,t_8);


cvlab::matrix<double> r_9(3,3);
r_9(0,0) = 9.9902986384e-01;
r_9(0,1) = -2.2354476674e-02;
r_9(0,2) = 3.7942173496e-02;
r_9(1,0) = 2.4771328155e-02;
r_9(1,1) = 9.9761203039e-01;
r_9(1,2) = -6.4471839786e-02;
r_9(2,0) = -3.6410334500e-02;
r_9(2,1) = 6.5349171354e-02;
r_9(2,2) = 9.9719796096e-01;

point3d t_9(-5.9576911572e-01,9.8144374701e-02,2.9885720992e-01);

add_camera(QImage("IMG_0158.jpg"),5.3200000000e+02,r_9,t_9);


cvlab::matrix<double> r_10(3,3);
r_10(0,0) = 9.9815447682e-01;
r_10(0,1) = 6.8885215800e-03;
r_10(0,2) = 6.0333976101e-02;
r_10(1,0) = -4.1970547588e-03;
r_10(1,1) = 9.9899507609e-01;
r_10(1,2) = -4.4623118134e-02;
r_10(2,0) = -6.0580732359e-02;
r_10(2,1) = 4.4287540134e-02;
r_10(2,2) = 9.9718031903e-01;

point3d t_10(2.4903818794e-01,-1.7510485345e-01,-1.7509436775e-01);

add_camera(QImage("IMG_0159.jpg"),6.7132867000e+02,r_10,t_10);


cvlab::matrix<double> r_11(3,3);
r_11(0,0) = 9.9481709557e-01;
r_11(0,1) = 1.3457892038e-02;
r_11(0,2) = 1.0078606805e-01;
r_11(1,0) = -7.2074843674e-03;
r_11(1,1) = 9.9804230119e-01;
r_11(1,2) = -6.2125817537e-02;
r_11(2,0) = -1.0142484183e-01;
r_11(2,1) = 6.1077411352e-02;
r_11(2,2) = 9.9296654087e-01;

point3d t_11(1.0590943376e+00,-4.7343655109e-01,4.0519220977e-01);

add_camera(QImage("IMG_0160.jpg"),6.7132867000e+02,r_11,t_11);


cvlab::matrix<double> r_12(3,3);
r_12(0,0) = 9.9746468484e-01;
r_12(0,1) = -4.2087286406e-04;
r_12(0,2) = 7.1161965638e-02;
r_12(1,0) = 5.8706782304e-03;
r_12(1,1) = 9.9706062863e-01;
r_12(1,2) = -7.6391347538e-02;
r_12(2,0) = -7.0920643148e-02;
r_12(2,1) = 7.6615440399e-02;
r_12(2,2) = 9.9453523651e-01;

point3d t_12(2.5281685618e-01,-6.1212782377e-01,1.5153452126e+00);

add_camera(QImage("IMG_0161.jpg"),5.3200000000e+02,r_12,t_12);

barycenter /= 13.;

}

   //item_id mesh_id = add_mesh(model);

   //QImage qimg("./test.jpg");
   //item_id img_item = add_floating_image(point3d(0,0,0),point3d(0.3,0,0),point3d(0,0.3,0),point3d(0.3,0.3,0),qimg);

   // the latest added item is automatically selected
   //selected_item = img_item;

   //cvlab::cloud3d cloud;
   //cloud.load_from_ply("kermit.ply");
   //barycenter = cloud.get_barycenter();
   //object = add_cloud(cloud);

   // the camera looks at the last object inserted
   center_x = -barycenter.x;
   center_y = -barycenter.y;
   center_z = -barycenter.z;
}

void QGLScene::initializeGL() {

   qglClearColor(QColor(Qt::black));

   //put_some_objects_in_the_scene();

   glShadeModel(GL_FLAT);

   GLfloat lmodel_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
   glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
   glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
}

void QGLScene::look_at(const point3d& p) {
   center_x = -p.x;
   center_y = -p.y;
   center_z = -p.z;
   updateGL();
}

/**
 * Clear up the current scene. All the objects are discarded, thus
 * all previously returned id's become invalid!
 */
void QGLScene::clear() {
   uint32_t nitems = items.size();
   for (uint32_t k=0; k<nitems; ++k)
      glDeleteLists(items[k].list_id, items[k].n_lists);
}

void QGLScene::paintGL()
{
   if (glGetError() == GL_OUT_OF_MEMORY)
      throw std::runtime_error("OpenGl: Out Of Memory");

   if (glGetError() == GL_INVALID_VALUE || glGetError() == GL_INVALID_OPERATION || glGetError() == GL_STACK_OVERFLOW || glGetError() == GL_STACK_UNDERFLOW)
      throw std::runtime_error("OpenGl: Selection mode error");


   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();

   glTranslated(center_x, center_y, center_z);
   glRotated(xRot / 16.0, 1.0, 0.0, 0.0);
   glRotated(yRot / 16.0, 0.0, 1.0, 0.0);
   glRotated(zRot / 16.0, 0.0, 0.0, 1.0);

   for (uint32_t k=0; k<items.size(); ++k) {
      if (items[k].is_visible()) {
         glCallList(items[k].cur_list);
      }
   }

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(view_volume.left, view_volume.right, view_volume.bottom, view_volume.top, view_volume.near, view_volume.far);
   glMatrixMode(GL_MODELVIEW);
}

void QGLScene::resizeGL(int width, int height)
{
   int side = qMax(width, height);
   glViewport((width - side) / 2, (height - side) / 2, side, side);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-0.25, +0.25, +0.15, -0.15, -40.0, 15.0);

   glMatrixMode(GL_MODELVIEW);
}

void QGLScene::keyPressEvent(QKeyEvent* event)
{
   if (event->key() == Qt::Key_C)
      items[center_item].set_visible(!items[center_item].is_visible());

   else if (event->key() == Qt::Key_Z)
      clear();
 /*
   else if (event->key() == Qt::Key_W)
      object = wireframe;

   else if(event->key() == Qt::Key_N)
      object = normals;

   else if(event->key() == Qt::Key_S)
      object = surface;

   else if(event->key() == Qt::Key_P)
      object = points;
*/
   updateGL();
}

void QGLScene::mousePressEvent(QMouseEvent *event)
{
   lastPos = event->pos();
   //gl_select(event->x(), event->y()); // FIXME
}

void QGLScene::start_picking(int cursorX, int cursorY) {

   GLint viewport[4];

   glSelectBuffer(64,picking_buf); // estabilish a buffer for selection mode values
   glRenderMode(GL_SELECT);

   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();

   glGetIntegerv(GL_VIEWPORT,viewport); // get current (x,y,width,height) of the viewport

   // 5x5 picking region around (x,y). It gets multiplied by the current projection matrix.
   gluPickMatrix(cursorX,viewport[3]-cursorY,5,5,viewport);
   glOrtho(view_volume.left, view_volume.right, view_volume.bottom, view_volume.top, -40.0, 15.0); // multiplied by the pick matrix

   glMatrixMode(GL_MODELVIEW);
   glInitNames(); // initialize the name stack to the empty state
}

void QGLScene::stop_picking() {

   int hits;

   // restoring the original projection matrix
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glFlush();

   // returning to normal rendering mode
   hits = glRenderMode(GL_RENDER);
   std::cout << "hits: " << hits << std::endl;

   // if there are hits process the selection buffer
   if (hits != 0)
      list_hits(hits,picking_buf);
}

void QGLScene::gl_select(int x, int y) {

   start_picking(x,y);

   glPushName(2);

   /*glBegin(GL_TRIANGLES);
      glVertex3d(0, 0, 0);
      glVertex3d(1, 0, 0);
      glVertex3d(0, 1, 0);
   glEnd();*/

   glPopName();

   stop_picking();
}

void QGLScene::list_hits(GLint hits, GLuint *names) {
   int i;

   /*
      For each hit in the buffer are allocated 4 bytes:
      1. Number of hits selected (always one,
                           beacuse when we draw each object
                           we use glLoadName, so we replace the
                           prevous name in the stack)
      2. Min Z
      3. Max Z
      4. Name of the hit (glLoadName)
   */

   printf("%d hits:\n", hits);

   for (i = 0; i < hits; i++)
      printf(	"Number: %d\n"
            "Min Z: %d\n"
            "Max Z: %d\n"
            "Name on stack: %d\n",
            (GLubyte)names[i * 4],
            (GLubyte)names[i * 4 + 1],
            (GLubyte)names[i * 4 + 2],
            (GLubyte)names[i * 4 + 3]
            );

   printf("\n");
 }

void QGLScene::mouseMoveEvent(QMouseEvent *event)
{
   int dx = event->x() - lastPos.x();
   int dy = event->y() - lastPos.y();

   if (event->buttons() & Qt::LeftButton) { // rotate
      setXRotation(xRot - 8 * dy);
      setYRotation(yRot + 8 * dx);
   }

   else if (event->buttons() & Qt::RightButton) { // zoom in-out
      zoom(dy / 200.0);
   }

   else if (event->buttons() & Qt::MidButton) { // move
      //move_object(dx / 1000., dy / 1000.);
      //move_object(event->x()
   }

   lastPos = event->pos();
}

/**
 * Add frame axes at the (0,0,0) of the current scene.
 */
item QGLScene::add_center_axes() {

   GLuint list = glGenLists(1);
   glNewList(list, GL_COMPILE);

   glDisable(GL_LIGHTING); // so that we can assign colors to the axes

   glColor3f(1,1,1);
   gluSphere(gluNewQuadric(),0.001,10,10);

   glBegin(GL_LINES); {

      // X (red)
      glColor3f(1,0,0);
      glVertex3d(0,0,0);
      glVertex3d(0.05,0,0);

      // Y (green)
      glColor3f(0,1,0);
      glVertex3d(0,0,0);
      glVertex3d(0,0.05,0);

      // Z (blue)
      glColor3f(0,0,1);
      glVertex3d(0,0,0);
      glVertex3d(0,0,0.05);

   } glEnd();

   glEnable(GL_LIGHTING);

   glEndList();

   return item(list,1);
}

/**
 * Add a set of 3D points to the current scene, or simply put it into video memory.
 *
 * @param pts The set of 3D points to be added to the scene.
 * @param add_to_scene
 * @return If add_to_scene is TRUE, the index to the items vector is returned. Otherwise, the display list name is returned.
 */
item_id QGLScene::add_point_set(const std::vector<point3d>& pts, bool add_to_scene) {

   GLuint list = glGenLists(1);

   glNewList(list, GL_COMPILE);

   glDisable(GL_LIGHTING); // so that we can assign colors to the points
   glBegin(GL_POINTS); {
      glColor3f(1,1,1);
      uint32_t npts = pts.size();
      for (uint32_t k=0; k<npts; ++k)
         glVertex3d(pts[k].x, pts[k].y, pts[k].z);
   } glEnd();
   glEnable(GL_LIGHTING);

   glEndList();

   if (add_to_scene) {
      items.push_back( item(list,1) );
      return items.size()-1;
   }

   return list;
}

/**
 * Add a set of colored 3D points to the current scene, or simply put it into video memory.
 *
 * @param pts The set of 3D points to be added to the scene.
 * @param colors The set of colors to use with the given points.
 * @param add_to_scene
 * @return If add_to_scene is TRUE, the index to the items vector is returned. Otherwise, the display list name is returned.
 * @throw std::invalid_argument
 */
item_id QGLScene::add_point_set(const std::vector<point3d>& pts, const std::vector<glscene::color>& colors, bool add_to_scene) {

   if (pts.size() != colors.size() || pts.empty() || colors.empty())
      throw std::invalid_argument("Invalid vector of 3d points and colors");

   GLuint list = glGenLists(1);

   glNewList(list, GL_COMPILE);

   glDisable(GL_LIGHTING); // so that we can assign colors to the points
   glBegin(GL_POINTS); {
      uint32_t npts = pts.size();
      for (uint32_t k=0; k<npts; ++k) {
         glColor3ub(colors[k].r, colors[k].g, colors[k].b);
         glVertex3d(pts[k].x, pts[k].y, pts[k].z);
      }
   } glEnd();
   glEnable(GL_LIGHTING);

   glEndList();

   if (add_to_scene) {
      items.push_back( item(list,1) );
      return items.size()-1;
   }

   return list;
}

/**
 * Add a single 3D point to the current scene, or simply put it into video memory.
 *
 * @param p The 3D point to be added to the scene.
 * @param add_to_scene
 * @return If add_to_scene is TRUE, the index to the items vector is returned. Otherwise, the display list name is returned.
 */
item_id QGLScene::add_point(const point3d& p, bool add_to_scene) {

   GLuint list = glGenLists(1);

   glNewList(list, GL_COMPILE);

   glDisable(GL_LIGHTING); // so that we can assign colors to the point
   glBegin(GL_POINTS);
      glVertex3d(p.x, p.y, p.z);
   glEnd();
   glEnable(GL_LIGHTING);

   glEndList();

   if (add_to_scene) {
      items.push_back( item(list,1) );
      return items.size()-1;
   }

   return list;
}

/**
 * Add a single 3D point to the current scene, or simply put it into video memory.
 *
 * @param p The 3D point to be added to the scene.
 * @param color The color to use with the given point.
 * @param add_to_scene
 * @return If add_to_scene is TRUE, the index to the items vector is returned. Otherwise, the display list name is returned.
 */
item_id QGLScene::add_point(const point3d& p, const glscene::color& color, bool add_to_scene) {

   GLuint list = glGenLists(1);

   glNewList(list, GL_COMPILE);

   glDisable(GL_LIGHTING); // so that we can assign colors to the point
   glBegin(GL_POINTS);
      glColor3ub(color.r, color.g, color.b);
      glVertex3d(p.x, p.y, p.z);
   glEnd();
   glEnable(GL_LIGHTING);

   glEndList();

   if (add_to_scene) {
      items.push_back( item(list,1) );
      return items.size()-1;
   }

   return list;
}

/**
 * Add a 3D line segment to the current scene, or put it in video memory.
 *
 * @param p One endpoint of the segment.
 * @param q The other endpoint.
 * @param add_to_scene If TRUE, the line segment is added to the scene, otherwise the line is only put in video memory.
 * @return If add_to_scene is TRUE, the index to the items vector is returned. Otherwise, the display list name is returned.
 */
item_id QGLScene::add_line(const point3d& p, const point3d& q, bool add_to_scene) {

   GLuint list = glGenLists(1);

   glNewList(list, GL_COMPILE);

   glDisable(GL_LIGHTING); // so that we can assign colors to the lines
   glBegin(GL_LINES);
      glColor3f(1,1,1);
      glVertex3d(p.x, p.y, p.z);
      glVertex3d(q.x, q.y, q.z);
   glEnd();
   glEnable(GL_LIGHTING);

   glEndList();

   if (add_to_scene) {
      items.push_back( item(list,1) );
      return items.size()-1; // an index into the items vector is returned
   }

   return list; // a display list name is returned
}

/**
 * Put a floating image in the scene.
 * The given image is texturized over a quad with given coordinates.
 *
 * @param top_left     Top-left point in space.
 * @param top_right    Top-right point in space.
 * @param botm_left    Bottom-left point in space.
 * @param botm_right   Bottom-right point in space.
 * @param image        The image to add to the scene.
 * @param add_to_scene
 * @return If add_to_scene is TRUE, the index to the items vector is returned. Otherwise, the display list name is returned.
 */
item_id QGLScene::add_floating_image(

      const point3d& top_left,
      const point3d& top_right,
      const point3d& botm_left,
      const point3d& botm_right,
      const QImage&  image,
            bool     add_to_scene

      ) {

   GLuint list = glGenLists(1);
   glNewList(list, GL_COMPILE);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); // replace lighting color with texture color

   bindTexture(image, GL_TEXTURE_2D, GL_RGB8);
   glEnable(GL_TEXTURE_2D);

   glBegin( GL_QUADS ); {
      glTexCoord2i( 0, 0 );
      glVertex3d( top_left.x, top_left.y, top_left.z );
      glTexCoord2i( 1, 0 );
      glVertex3d( top_right.x, top_right.y, top_right.z );
      glTexCoord2i( 1, 1 );
      glVertex3d( botm_right.x, botm_right.y, botm_right.z );
      glTexCoord2i( 0, 1 );
      glVertex3d( botm_left.x, botm_left.y, botm_left.z );
   } glEnd();

   glDisable(GL_TEXTURE_2D);

   glEndList();

   if (add_to_scene) {
      items.push_back( item(list,1) );
      return items.size()-1;
   }

   return list;
}

/**
 * Adds a 3D point cloud to the current scene.
 *
 * @param cloud The point cloud to be added to the scene.
 */
item_id QGLScene::add_cloud(const cvlab::cloud3d& cloud) {

   GLuint list = glGenLists(1);

   glNewList(list, GL_COMPILE);

   glDisable(GL_LIGHTING); // so that we can assign colors to the points
   glBegin(GL_POINTS); {

      cvlab::cloud3d::const_iterator it = cloud.begin(), it_end = cloud.end();
      for (; it != it_end; ++it) {
         glColor3f(1.0f, 0.0f, 0.0f);
         glVertex3d(it->x, it->y, it->z);
      }

   } glEnd();
   glEnable(GL_LIGHTING);

   glEndList();

   items.push_back( item(list,1) );
   return items.size()-1;
}

/**
 * Adds a mesh to the current scene.
 *
 * @param model The mesh model to be added to the scene.
 * @param barycenter
 */
item_id QGLScene::add_mesh(const cvlab::mesh& model)
{
   GLuint list = glGenLists(4);

   // Triangle (surface) version of the model

   cvlab::mesh::const_iterator_tri it = model.begin_by_triangle(), it_end = model.end_by_triangle();

   glNewList(list, GL_COMPILE);
   glBegin(GL_TRIANGLES); {

      for (; it != it_end; ++it) {
         cvlab::triangle tri = *it;

         glNormal3f(tri.p0.n.x, tri.p0.n.y, tri.p0.n.z);
         glVertex3d(tri.p0.x, tri.p0.y, tri.p0.z);

         glNormal3f(tri.p1.n.x, tri.p1.n.y, tri.p1.n.z);
         glVertex3d(tri.p1.x, tri.p1.y, tri.p1.z);

         glNormal3f(tri.p2.n.x, tri.p2.n.y, tri.p2.n.z);
         glVertex3d(tri.p2.x, tri.p2.y, tri.p2.z);
      }

   } glEnd();
   glEndList();

   // Create the wireframe version of the model

   //TODO try to use glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)

   glNewList(list+1, GL_COMPILE);

   glDisable(GL_LIGHTING); // so that we can assign colors to the lines
   glBegin(GL_LINES); {

      glColor3f(1,1,1);
      it = model.begin_by_triangle();

      for (; it != it_end; ++it) {
         cvlab::triangle tri = *it;

         glVertex3d(tri.p0.x, tri.p0.y, tri.p0.z);
         glVertex3d(tri.p1.x, tri.p1.y, tri.p1.z);

         glVertex3d(tri.p1.x, tri.p1.y, tri.p1.z);
         glVertex3d(tri.p2.x, tri.p2.y, tri.p2.z);

         glVertex3d(tri.p2.x, tri.p2.y, tri.p2.z);
         glVertex3d(tri.p0.x, tri.p0.y, tri.p0.z);
      }

   } glEnd();
   glEnable(GL_LIGHTING);

   glEndList();

   // Create the normals (triangles) version

   glNewList(list+2, GL_COMPILE);
   glBegin(GL_LINES); {

         it = model.begin_by_triangle();

         for (; it != it_end; ++it) {
            cvlab::triangle tri = *it;

            cvlab::point3d centroid( (tri.p1 + tri.p0 + tri.p2) / 3. );
            glVertex3d(centroid.x, centroid.y, centroid.z);

            cvlab::point3d n(centroid + (tri.n * 0.001)); // rescale normals for visualization purposes
            glVertex3d(n.x, n.y, n.z);
         }

   } glEnd();
   glEndList();

   // Create the points version

   glNewList(list+3, GL_COMPILE);

   glDisable(GL_LIGHTING); // so that we can assign colors to the points
   glBegin(GL_POINTS); {

      glColor3f(1,1,1);
      it = model.begin_by_triangle();

      for (; it != it_end; ++it) {
         cvlab::triangle tri = *it;
         glVertex3d(tri.p0.x, tri.p0.y, tri.p0.z);
         glVertex3d(tri.p1.x, tri.p1.y, tri.p1.z);
         glVertex3d(tri.p2.x, tri.p2.y, tri.p2.z);
      }

   } glEnd();
   glEnable(GL_LIGHTING);

   glEndList();


   items.push_back( item(list, 4) );
   return items.size()-1;
}

void QGLScene::normalizeAngle(int *angle)
{
   while (*angle < 0)
      *angle += 360 * 16;
   while (*angle > 360 * 16)
      *angle -= 360 * 16;
}
