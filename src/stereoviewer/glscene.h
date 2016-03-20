/*
  Author: Emanuele Rodol√  <rodola@dsi.unive.it>
*/

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <vector>

#undef near
#undef far

struct ortho_volume {
   GLdouble left, right, bottom, top, near, far;
};

namespace cvlab {
   class mesh;
   class cloud3d;

   template <typename T>
   class vec3d;

   template <typename T>
   class matrix;
}

class QGLScene;

namespace glscene {

   struct color {
      uint8_t r, g, b;
      explicit color(uint8_t R, uint8_t G, uint8_t B) : r(R), g(G), b(B) {}
   };

/**
 * This class represents an item in a QGLScene; thus it can be a mesh, or a line,
 * a collection of points, one point, and so on.
 * If it is a mesh then the representation also accounts for its wireframe version,
 * its points version and so on.
 *
 * An item is actually a continuous group of display lists.
 */
class item {

   friend class ::QGLScene; // otherwise glscene::QGLScene is my friend

   GLuint list_id; // the first list in the group
   GLsizei n_lists; // # of lists in the group
   GLuint cur_list; // the current list (the one to be displayed)
   bool visible;

   public:
      item(GLuint id, GLsizei n) : list_id(id), n_lists(n), cur_list(id), visible(true) {}

      void set_visible(bool enabled) { visible = enabled; }
      void set_pickable(bool /*enabled*/) { /* to implement */ }

      bool is_visible() const { return visible; }
      bool is_pickable() const { /* to implement */ return false; }
};

typedef uint32_t item_id; // an index in the items vector

}

/**
 *
 */
class QGLScene : public QGLWidget
{
    Q_OBJECT;

public:
    typedef cvlab::vec3d<double> point3d;

    QGLScene(QWidget *parent = 0);
    ~QGLScene();

    void put_some_objects_in_the_scene();
    glscene::item_id add_camera(const QImage& image,double focal_len,const cvlab::matrix<double>& R,const point3d& T);

    glscene::item_id add_mesh(const cvlab::mesh&);
    glscene::item_id add_cloud(const cvlab::cloud3d&);
    glscene::item_id add_point(const point3d& p, bool add_to_scene=true);
    glscene::item_id add_point(const point3d& p, const glscene::color& color, bool add_to_scene=true);
    glscene::item_id add_point_set(const std::vector<point3d >& pts, bool add_to_scene=true);
    glscene::item_id add_point_set(const std::vector<point3d>& pts, const std::vector<glscene::color>& colors, bool add_to_scene=true);
    glscene::item_id add_line(const point3d& p, const point3d& q, bool add_to_scene=true);
    glscene::item_id add_floating_image(const point3d& top_left,const point3d& top_right,const point3d& botm_left,const point3d& botm_right,const QImage& image,bool add_to_scene=true);

    void zoom(double delta);
    void set_view_volume(const ortho_volume& v);
    //void fit_view(double mx, double Mx, double my, double My, double mz, double Mz, point3d& center, double& zoom_fit) const;

    void clear();

    void look_at(const point3d& p);

    void show_center() {
       items.at(center_item).set_visible(true);
       updateGL();
    }

    void hide_center() {
       items.at(center_item).set_visible(false);
       updateGL();
    }

    virtual QSize minimumSizeHint() const { return QSize(100, 100); }
    virtual QSize sizeHint() const { return QSize(800, 600); }

protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void keyPressEvent(QKeyEvent*);

private:
    double center_x, center_y, center_z;
    std::vector<glscene::item> items;
    int selected_item; // -1 or index into the items vector
    int center_item;
    GLuint* picking_buf; // used in selection mode for picking

    glscene::item add_center_axes();

    void start_picking(int cursorX, int cursorY);
    void stop_picking();
    void gl_select(int x, int y);
    void list_hits(GLint hits, GLuint *names);

    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void move_object(double dx, double dy);

    void normalizeAngle(int *angle);

    ortho_volume view_volume;

    int xRot, yRot, zRot;
    GLdouble dx, dy;
    QPoint lastPos;
};

#endif
