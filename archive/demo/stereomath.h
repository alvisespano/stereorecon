#ifndef STEREOMATH_H
#define STEREOMATH_H

#include <QVector>

namespace multi_view_stereo{

template<class T>
class matrix : private QVector<T>
{
		  private:
					 int width_;
					 int height_;

		  public:

					 const int& width;
					 const int& height;

					 typedef typename QVector<T>::iterator iterator;
					 typedef typename QVector<T>::const_iterator const_iterator;
					 typedef T type;

					 matrix(int w, int h) : width_(w), height_(h), width(width_), height(height_), QVector<T>(w*h) {}

					 T& operator() (int i, int j) { return QVector<T>::operator[](i*width_+j); }
					 const T& operator() (int i, int j) const { return QVector<T>::operator[](i*width_+j); }

					 iterator begin() { return QVector<T>::begin() ; }
					 const_iterator begin() const { return QVector<T>::begin() ; }
					 iterator end() { return QVector<T>::end() ; }
					 const_iterator end() const { return QVector<T>::end() ; }
};

}

#endif // STEREOMATH_H

