#include "math3d.h"

namespace cvlab {


// ||p1 - p2||^2
double squared_dist(const point3d& p1, const point3d& p2) {
   double x = p1.x - p2.x;
   double y = p1.y - p2.y;
   double z = p1.z - p2.z;
   return ((x*x) + (y*y) + (z*z));
}

double dist(const point3d& p1, const point3d& p2) {
    return sqrt(squared_dist(p1,p2));
}

double dot_product(const point2d& v1, const point2d& v2) {
   return (v1.x*v2.x) + (v1.y*v2.y);
}

/**
 * Find the intersection point between a line and a triangle.
 *
 * "Mathematics for 3D Game Programming and Computer Graphics"
 * pp.144,145
 *
 * @param S A point lying on the line.
 * @param V Direction of the line.
 * @param P0 triangle vertex
 * @param P1 triangle vertex
 * @param P2 triangle vertex
 * @return TRUE if an intersection was found, FALSE otherwise.
 */
bool findLineTriangleIntersection(
      const point3d& S,
      const point3d& V,
      const point3d& P0,
      const point3d& P1,
      const point3d& P2,
      point3d& intersection) {


   /*std::cout << "S: " << S << std::endl;
   std::cout << "V: " << V << std::endl;
   std::cout << "P0: " << P0 << " , P1: " << P1 << " , P2: " << P2 << std::endl;*/

   point3d Q1(P1-P0), Q2(P2-P0);
   point3d N = cross_product(Q1,Q2);
   double d = -dot_product(N,P0);

   double LdotV = N.x*V.x + N.y*V.y + N.z*V.z;
   if (LdotV==0.)
      return false;

   double LdotS = N.x*S.x + N.y*S.y + N.z*S.z + d;
   double t = - LdotS / LdotV;

   point3d P(S + (t*V));
   point3d R(P-P0);

   double Q1dotQ2 = dot_product(Q1,Q2);
   double Q1_sq_norm = dot_product(Q1,Q1);
   double Q2_sq_norm = dot_product(Q2,Q2);
   double RdotQ1 = dot_product(R,Q1);
   double RdotQ2 = dot_product(R,Q2);

   double den = ((Q1_sq_norm*Q2_sq_norm) - (Q1dotQ2*Q1dotQ2));

   double w1 = (Q2_sq_norm*RdotQ1 - Q1dotQ2*RdotQ2) / den;
   if (w1 < 0.)
      return false;

   double w2 = (-RdotQ1*Q1dotQ2 + Q1_sq_norm*RdotQ2) / den;
   if (w2 < 0.)
      return false;

   if (w1+w2 > 1.)
      return false;

   intersection = P;
   //std::cout << "intersection: " << intersection << std::endl;

   return true;
}

/**
 *	Finds the inscribed square in a given parallelogram.
 *	The input parameters MUST be 2-dimensional float arrays (containing
 *	the x and y coords of the point).
 *
 * @param A Top-left corner of the parallelogram.
 * @param B Top-right corner of the parallelogram.
 * @param C Bottom-left corner of the parallelogram.
 * @param D Bottom-right corner of the parallelogram.
 * @return The inscribed square side is returned.
 */
double findSquareInParallelogram(
      float *A, float *B,
      float *C, float *D) {

    // parallelogram centroid
   point2d p_centroid(IntersectSegments(
         point2d(A[0],A[1]),
         point2d(D[0],D[1]),
         point2d(B[0],B[1]),
         point2d(C[0],C[1])
         ));

   double radius = 1e15;
   point2d P,S;

   P = cvlab::IntersectLines(
         point2d(B[0],B[1]),
         p_centroid,
         point2d(D[0],D[1]),
         point2d(p_centroid.x+1.,p_centroid.y-1.)
         );
   S = cvlab::IntersectLines(
         point2d(A[0],A[1]),
         p_centroid,
         point2d(C[0],C[1]),
         point2d(p_centroid.x+1.,p_centroid.y-1.)
         );
   double radius1 = fabs(P.x-S.x);
   if (radius1<radius) radius=radius1;

   P = cvlab::IntersectLines(
         point2d(B[0],B[1]),
         p_centroid,
         point2d(D[0],D[1]),
         point2d(p_centroid.x+1.,p_centroid.y+1.)
         );
   S = cvlab::IntersectLines(
         point2d(A[0],A[1]),
         p_centroid,
         point2d(C[0],C[1]),
         point2d(p_centroid.x+1.,p_centroid.y+1.)
         );
   double radius2 = fabs(P.x-S.x);
   if (radius2<radius) radius=radius2;

   P = cvlab::IntersectLines(
         point2d(B[0],B[1]),
         p_centroid,
         point2d(A[0],A[1]),
         point2d(p_centroid.x+1.,p_centroid.y-1.)
         );
   S = cvlab::IntersectLines(
         point2d(D[0],D[1]),
         p_centroid,
         point2d(C[0],C[1]),
         point2d(p_centroid.x+1.,p_centroid.y-1.)
         );
   double radius3 = fabs(P.x-S.x);
   if (radius3<radius) radius=radius3;

   P = cvlab::IntersectLines(
         point2d(B[0],B[1]),
         p_centroid,
         point2d(A[0],A[1]),
         point2d(p_centroid.x+1.,p_centroid.y+1.)
         );
   S = cvlab::IntersectLines(
         point2d(D[0],D[1]),
         p_centroid,
         point2d(C[0],C[1]),
         point2d(p_centroid.x+1.,p_centroid.y+1.)
         );
   double radius4 = fabs(P.x-S.x);
   if (radius4<radius) radius=radius4;

   return radius;
}

/**
 * Computes the intersection between two segments.
 * @param p1 Starting point of Segment 1
 * @param p2 Ending point of Segment 1
 * @param p3 Starting point of Segment 2
 * @param p4 Ending point of Segment 2
 * @return Point where the segments intersect, or (-1,-1) if they don't.
 */
point2d IntersectSegments(const point2d& p1, const point2d& p2, const point2d& p3, const point2d& p4) {

   double x1 = p1.x, y1 = p1.y;
   double x2 = p2.x, y2 = p2.y;
   double x3 = p3.x, y3 = p3.y;
   double x4 = p4.x, y4 = p4.y;

   double d = (x1-x2)*(y3-y4) - (y1-y2)*(x3-x4);
   if (d == 0.) // parallel segments
      return point2d(-1.,-1.);

   double xi = ((x3-x4)*(x1*y2-y1*x2)-(x1-x2)*(x3*y4-y3*x4))/d;
   double yi = ((y3-y4)*(x1*y2-y1*x2)-(y1-y2)*(x3*y4-y3*x4))/d;

   point2d p(xi,yi);

   if (xi < std::min(x1,x2) || xi > std::max(x1,x2))
      return point2d(-1.,-1.);
   if (xi < std::min(x3,x4) || xi > std::max(x3,x4))
      return point2d(-1.,-1.);

   return p;
}

int GCD(int a, int b){
  if ( b == 0 )
   return a;
  return GCD(b,a%b);
}

int LCM(int a, int b){
   return a*(b/GCD(a,b));
}

point2d IntersectLines(
      const point2d& p1,
      const point2d& p2,
      const point2d& c1,
      const point2d& c2) {

   double X,Y,Z;

   IntersectLines(
         point3d(p1.x,p1.y,0.),
         point3d(p2.x,p2.y,0.),
         point3d(c1.x,c1.y,0.),
         point3d(c2.x,c2.y,0.),
         X,Y,Z
         );

   return point2d(X,Y);
}

/**
 * This function computes the intersection between two lines in 3D space.
 * The two lines are computed given two points each; the points are P1,C1
 * for the left camera and P2,C2 for the right camera.
 * If the lines do not intersect then the minimum euclidean distance between
 * them is computed, and the midpoint of the minimum distance segment is taken
 * as the intersection.
 * The computed intersection is then saved in X,Y,Z.
 *
 * @param P1 First point for the left camera.
 * @param P2 First point for the right camera.
 * @param C1 Second point for the left camera.
 * @param C2 Second point for the right camera.
 * @param X X coord. for the computed 3D intersection point.
 * @param Y Y coord. for the computed 3D intersection point.
 * @param Z Z coord. for the computed 3D intersection point.
 * @return The minimum distance between the two skew lines is returned. If the two lines perfectly intersect, of course, 0 is returned.
 */
double IntersectLines(
      const point3d& P1, const point3d& P2,
      const point3d& C1, const point3d& C2,
      double& X, double& Y, double& Z) {

   point3d C2minC1(C2-C1);

   // Now we find the shortest distance between the two lines and
   // place the 3d point in the middle of the distance segment.

   // p.104 "Mathematics for 3D Game Programming and Computer Graphics"

   // Direction vector for the line passing through C1 and P1
   point3d V1(P1-C1);
   normalize(V1);

   // Direction vector for the line passing through C2 and P2
   point3d V2(P2-C2);
   normalize(V2);

   double V1dotV2 = dot_product(V1,V2);
   double den = (V1dotV2*V1dotV2) - 1.;

   point2d C(dot_product(C2minC1,V1),dot_product(C2minC1,V2));

   point2d Row1(-1.,V1dotV2);
   point2d Row2(-V1dotV2,1.);

   double t1 = dot_product(Row1,C) / den;
   double t2 = dot_product(Row2,C) / den;

    point3d T = cross_product(V1,V2);
    normalize(T);

    double tp1 = dot_product(T,C1), tp2 = dot_product(T,C2);
    double min_distance = fabs(tp1-tp2);

   X = ((C1.x+t1*V1.x) + (C2.x+t2*V2.x))/2.;
   Y = ((C1.y+t1*V1.y) + (C2.y+t2*V2.y))/2.;
   Z = ((C1.z+t1*V1.z) + (C2.z+t2*V2.z))/2.;

   return min_distance;
}


};
