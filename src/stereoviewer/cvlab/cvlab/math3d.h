#ifndef CVLAB_MATH3D_H
#define CVLAB_MATH3D_H

#include <iostream>
#include <cmath>
#include <vector>
#include <stdexcept>
#include <QTextStream>

#ifndef M_PI
#define M_PI (3.14159265358979323846f)
#endif

namespace cvlab {

template<typename T>
struct vec2d {

    T x,y;

    explicit vec2d() : x(0), y(0) {}

    vec2d(const T& x_, const T& y_) : x(x_), y(y_) {}

    template <typename S>
    vec2d(const vec2d<S>& o) : x(static_cast<T>(o.x)), y(static_cast<T>(o.y)) {}

    bool operator==(const vec2d& o) const { return x==o.x && y==o.y; }
    bool operator!=(const vec2d& o) const { return !(*this==o); }
};

typedef vec2d<double> point2d;


template <typename T>
struct vec3d {

    T x,y,z;

    explicit vec3d() : x(0), y(0), z(0) {}

    vec3d(const T& x_, const T& y_, const T& z_) : x(x_), y(y_), z(z_) {}

    template <typename S>
    vec3d(const vec3d<S>& s) : x(static_cast<T>(s.x)), y(static_cast<T>(s.y)), z(static_cast<T>(s.z)) {}

    vec3d<T> operator+(const vec3d<T>& p) const {
        return vec3d<T>(x + p.x, y + p.y, z + p.z);
    }

    vec3d<T> operator-(const vec3d<T>& p) const {
        return vec3d<T>(x - p.x, y - p.y, z - p.z);
    }

    vec3d<T> operator-() const {
       return vec3d<T>(-x, -y, -z);
    }

    template <typename S>
    vec3d<T>& operator+=(const vec3d<S>& p) {
        x += static_cast<T>(p.x);
        y += static_cast<T>(p.y);
        z += static_cast<T>(p.z);
        return *this;
    }

    // rules for partial ordering of function templates say that the new overload
    // is a better match, when it matches

    vec3d<T>& operator+=(const vec3d<T>& p) {
        x += p.x;
        y += p.y;
        z += p.z;
        return *this;
    }

    template <typename S>
    vec3d<T>& operator-=(const vec3d<S>& p) {
        x -= static_cast<T>(p.x);
        y -= static_cast<T>(p.y);
        z -= static_cast<T>(p.z);
        return *this;
    }

    vec3d<T>& operator-=(const vec3d<T>& p) {
        x -= p.x;
        y -= p.y;
        z -= p.z;
        return *this;
    }

    template <typename Scalar>
    vec3d<T>& operator/=(const Scalar& s) {
       x /= static_cast<T>(s);
       y /= static_cast<T>(s);
       z /= static_cast<T>(s);
       return *this;
    }

    template <typename Scalar>
    vec3d<T>& operator *=(const Scalar& s) {
       x *= static_cast<T>(s);
       y *= static_cast<T>(s);
       z *= static_cast<T>(s);
       return *this;
    }

    T mod() const
    {
        return sqrt(x*x + y*y + z*z);
    }

    T distance(const vec3d<T>& w) const
    {
        return operator-(w).mod();
    }

    bool operator==(const vec3d& o) const { return x == o.x && y == o.y && z == o.z; }

    template <typename S>
    bool operator==(const vec3d<S>& o) const {
       return (x == static_cast<T>(o.x) && y == static_cast<T>(o.y) && z == static_cast<T>(o.z));
    }

    bool operator!=(const vec3d& o) const { return !(*this==o); }

    template <typename S>
    bool operator!=(const vec3d<S>& o) const {
       return !(*this == o);
    }

    template <typename Scalar>
    friend vec3d<T> operator*(const vec3d<T>& p, const Scalar& s) {
        return vec3d<T>(s * p.x, s * p.y, s * p.z);
    }

    template <typename Scalar>
    friend vec3d<T> operator*(const Scalar& s, const vec3d<T>& p) {
        return p*s;
    }

    template <typename Scalar>
    friend vec3d<T> operator/(const vec3d<T>& p, const Scalar& s) {
       return vec3d<T>(p.x / static_cast<T>(s), p.y / static_cast<T>(s), p.z / static_cast<T>(s));
    }

    friend std::ostream& operator<<(std::ostream& os, const vec3d<T>& p) {
       os << p.x << " " << p.y << " " << p.z;
       return os;
    }

    friend QTextStream& operator<<(QTextStream& os, const vec3d<T>& p) {
       os << p.x << " " << p.y << " " << p.z;
       return os;
    }
};

typedef vec3d<double> normal3d;
typedef vec3d<double> point3d;

class oriented_point3d : public point3d {
   public:
      explicit oriented_point3d() : point3d() {}
      explicit oriented_point3d(const oriented_point3d& p) : point3d(p), n(p.n) {}
      explicit oriented_point3d(const point3d& p) : point3d(p) {}
      normal3d n;
};

struct triangle {
   oriented_point3d p0, p1, p2;
   normal3d n;
   triangle() {}
   triangle(const oriented_point3d& p0_, const oriented_point3d& p1_, const oriented_point3d& p2_, const normal3d& n_)
         : p0(p0_), p1(p1_), p2(p2_), n(n_) {}
   triangle(const point3d& p0_, const point3d& p1_, const point3d& p2_, const normal3d& n_)
         : p0(p0_), p1(p1_), p2(p2_), n(n_) {}
};

/**
 *
 */
template<class T>
class matrix : private std::vector<T>
{
protected:
    unsigned int width_, height_;

public:
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;
    typedef T value_type;

    explicit matrix() : std::vector<T>(), width_(0), height_(0) {}
    matrix(int w, int h) : std::vector<T>(w*h), width_(w), height_(h) {}

    T& operator() (size_t row, size_t col) { return std::vector<T>::operator[](row*width_+col); }
    const T& operator() (size_t row, size_t col) const { return std::vector<T>::operator[](row*width_+col); }

    T& at(size_t row, size_t col) { return std::vector<T>::at(row*width_+col); }
    const T& at(size_t row, size_t col) const { return std::vector<T>::at(row*width_+col); }

    const unsigned int& width() const { return width_; }
    const unsigned int& height() const { return height_; }

    void resize(int w, int h) {
        std::vector<T>::resize(w*h);
        width_ = w;
        height_ = h;
    }

    iterator begin() { return std::vector<T>::begin(); }
    const_iterator begin() const { return std::vector<T>::begin(); }
    iterator end() { return std::vector<T>::end(); }
    const_iterator end() const { return std::vector<T>::end(); }

    void identity()
    {
        if (width() != height()) throw std::runtime_error("[matrix::set_identity] matrix is non-square");
        std::fill(begin(), end(), 0.0);
        for (size_t i = 0; i < width(); ++i)
        {
            (*this)(i, i) = 1.0;
        }
    }
};


/**
 * NOTE: the minimal information needed for a plane are
 * the normal to the plane (N) and a point lying over it (P).
 */
struct ClipPlane3D {
    point3d N,P;
    point3d top_left,top_right,botm_left,botm_right;
    point3d projection_center;
    point3d principal_point;
};

struct quaternion {
   double w, i, j, k;
   quaternion() : w(0.), i(0.), j(0.), k(0.) {}
   quaternion(double a, double b, double c, double d) : w(a), i(b), j(c), k(d) {}
};

double dot_product(const point2d&,const point2d&);

template <typename T, typename S>
void rotate(vec3d<T>& p, const matrix<S>& rot) {
   T oldx = p.x, oldy = p.y, oldz = p.z;
   p.x = static_cast<T>( oldx*rot(0,0) + oldy*rot(0,1) + oldz*rot(0,2) );
   p.y = static_cast<T>( oldx*rot(1,0) + oldy*rot(1,1) + oldz*rot(1,2) );
   p.z = static_cast<T>( oldx*rot(2,0) + oldy*rot(2,1) + oldz*rot(2,2) );
}

template <typename T, typename S>
void rotate(vec3d<T>& p, const matrix<T>& rot) {
   T oldx = p.x, oldy = p.y, oldz = p.z;
   p.x = oldx*rot(0,0) + oldy*rot(0,1) + oldz*rot(0,2);
   p.y = oldx*rot(1,0) + oldy*rot(1,1) + oldz*rot(1,2);
   p.z = oldx*rot(2,0) + oldy*rot(2,1) + oldz*rot(2,2);
}

template <typename T, typename S>
void rotate_translate(vec3d<T>& p, const matrix<S>& rot, const point3d& trans) {
   rotate(p,rot);
   p += trans;
}

template <typename T>
double normalize(vec3d<T>& p) {
   double n = magnitude(p);
   if (n==0.) {
      p.x = 0.;
      p.y = 0.;
      p.z = 0.;
      return 0.;
   }
   p.x /= n;
   p.y /= n;
   p.z /= n;
   return n;
}

template <typename T>
vec3d<T> get_normalize(const vec3d<T>& p) {
   vec3d<T> q(p);
   normalize(q);
   return q;
}

double dist(const point3d&, const point3d&);
double squared_dist(const point3d&, const point3d&);

template <typename T>
double dot_product(const vec3d<T>& v1, const vec3d<T>& v2) {
   return (v1.x*v2.x) + (v1.y*v2.y) + (v1.z*v2.z);
}

template <typename T, typename S>
double dot_product(const vec3d<T>& v1, const vec3d<S>& v2) {
   return (v1.x*v2.x) + (v1.y*v2.y) + (v1.z*v2.z);
}

template <typename T>
double magnitude(const vec3d<T>& p) {
    return sqrt(dot_product(p,p));
}

template <typename T>
vec3d<T> cross_product(const vec3d<T>& v1, const vec3d<T>& v2) {
   return vec3d<T>(
         (v1.y*v2.z) - (v1.z*v2.y),
         (v1.z*v2.x) - (v1.x*v2.z),
         (v1.x*v2.y) - (v1.y*v2.x)
         );
}

template <typename T>
matrix<T> transpose(const matrix<T>& m)
{
    matrix<T> r(m.height(), m.width());
    for (size_t i = 0; i < m.width(); ++i)
        for (size_t j = 0; j < m.height(); ++j)
            r(j, i) = m(i, j);
    return r;
}

int GCD(int a, int b);
int LCM(int a, int b);

bool findLineTriangleIntersection(
      const point3d& S,
      const point3d& V,
      const point3d& P0,
      const point3d& P1,
      const point3d& P2,
      point3d& intersection);

double findSquareInParallelogram(
      float *A, float *B,
      float *C, float *D);

point2d IntersectLines(
      const point2d& p1,
      const point2d& p2,
      const point2d& c1,
      const point2d& c2);

double IntersectLines(
      const point3d& p1,
      const point3d& p2,
      const point3d& c1,
      const point3d& c2,
      double& X, double& Y, double& Z);

point2d IntersectSegments(
      const point2d& p1,
      const point2d& p2,
      const point2d& p3,
      const point2d& p4);


} // namespace scanner_math


#endif // CVLAB_MATH3D_H
