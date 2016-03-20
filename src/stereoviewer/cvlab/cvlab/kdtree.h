#ifndef CVLAB_KDTREE_H
#define CVLAB_KDTREE_H

#include <vector>
#include <limits>
#include <ctime>
#include <cstdlib> // for srandom()
#include <algorithm> // for sort()
#include "exceptions.h"

namespace cvlab {

   template<class T>
   class node3d {

      public:

         T val;
         int axis;
         node3d<T> *left,*right,*parent;

         node3d(const T& _v,int _axis):val(_v),axis(_axis),left(NULL),right(NULL),parent(NULL){}

         double getMedian() const {
            switch(axis){
            case 0:
               return val.x;
            case 1:
               return val.y;
            case 2:
               return val.z;
            }
            return 0.;
         }
   };

   template<class T>
   class kdtree {

   public:

      kdtree(const std::vector<T>& points) : root(NULL), nearest_neighbour(NULL), length(0), k(3) {

         uint32_t psize = points.size();
         nodes_pool.reserve(psize);

         std::vector<int> indices(psize);
         srand(time(NULL));

         for(uint32_t i=0;i<psize;++i) indices[i] = i;

         for(uint32_t i=0;i<psize;++i) {
            int j = rand() % psize;
            int tmp_index = indices[j];
            indices[j] = indices[i];
            indices[i] = tmp_index;
         }

         for(uint32_t i=0;i<psize;++i) {
            if(add(points.at(indices[i]))) length++;
         }

      }

      int size() const {
         return length;
      }


      /**
       *
       * @param mp
       * @param k==0  ritorna tutti, altrimenti ritorna i k più vicini
       * @param radius
       * @return
       */
      void neighbors(const point3d& mp, uint32_t k, double radius, std::vector<T>& t) const {

         t.clear();

         point3d minp(mp.x-radius,
                      mp.y-radius,
                      mp.z-radius);
         point3d maxp(mp.x+radius,
                      mp.y+radius,
                      mp.z+radius);

         orthogonalSearch(minp,maxp,t);

         if (k>0) {

            for( uint32_t i = 0; i<t.size(); ++i) {
               double dx = t[i].x - mp.x;
               double dy = t[i].y - mp.y;
               double dz = t[i].z - mp.z;
               t[i].distance = dx*dx + dy*dy + dz*dz;
            }

            std::sort( t.begin(), t.end(), compare_by_distance );

            if(k>0) {
               int size = (t.size()>k)?k:t.size();
               t.resize(size);
            }

         }
      }

      void orthogonalSearch(const point3d& min, const point3d& max, std::vector<T>& result) const {
         std::vector<node3d<T>*> n;
         orthoSearch(root,min,max,n);
         for(uint32_t i=0;i<n.size();++i){
            result.push_back(n[i]->val);
         }
      }

private:

      node3d<T> *root;
      node3d<T> *nearest_neighbour;
      int length;
      int k;
      double d_min;
      std::vector<node3d<T> > nodes_pool;

      inline static bool compare_by_distance (const T& i, const T& j) {
         return (i.distance<j.distance);
      }

      int compare(int axis ,const T& o1, const T& o2) const {
         double a=0,b=0;
         switch(axis){
         case 0:
            a=o1.x;
            b=o2.x;
            break;
         case 1:
            a=o1.y;
            b=o2.y;
            break;
         case 2:
            a=o1.z;
            b=o2.z;
            break;
         }
         if(a<b) return -1;
         else if(a>b) return 1;
         else return 0;
      }

      node3d<T>* get_node(const T& p) const {
         node3d<T> *t = root, *parent = NULL;

         while (t != NULL) {
            const T& test = t->val;
            if(test.x==p.x && test.y==p.y && test.z==p.z)
               return t;
            int comp = compare(t->axis, p, test);
            parent = t;
            if (comp >= 0)
               t = t->right;
            else
               t = t->left;
         }

         return NULL;
      }

      bool contains(const T& p) const {
         if(root==NULL)
            return false;

         return (get_node(p)!=NULL);
      }

      void searchParent(const node3d<T>* node, const T& p) {

         //controllo intersezione con l'iperrettangolo più vicino
         node3d<T>* const parent = node->parent;

         if (parent==NULL)
            return;

         //verifico la situazione del parent
         double new_d_min = getSQDistance(parent->v, p);
         if(new_d_min<d_min){
            d_min = new_d_min;
            nearest_neighbour = parent ;
         }

         //ricerca fratello
         node3d<T> *brother = parent->left;
         if(node==parent->left)
            brother = parent->right;

         if(brother!=NULL) {
            T nearest = p;
            switch(parent->axis){
            case 0:
               nearest.x=parent->v.x;
               break;
            case 1:
               nearest.y=parent->v.y;
               break;
            case 2:
               nearest.z=parent->v.z;
               break;
            }
            //candidato?
            double d_hyperrect = getSQDistance(nearest, p);
            if(d_hyperrect<d_min){
               //possibile soluzione migliorativa
               brother->parent = NULL;
               findNearest(brother,p);
               brother->parent = parent;
            }
         }

         searchParent(parent,p);
      }

      void findNearest(node3d<T>* root,const T& p){
         node3d<T>* const node = get_node(root,p);
         if(node==NULL)
            return;

         double new_d_min = getSQDistance(node->v, p);
         if(new_d_min<d_min){
            d_min = new_d_min;
            nearest_neighbour = node ;
         }

         searchParent(node, p);
      }

      node3d<T>* getNearest(const T& p) {
         d_min = std::numeric_limits<double>::max();
         nearest_neighbour = root;
         if(root==NULL)
            return NULL ;
         findNearest(root, p);
         //if(p == nearest_neighbour->v)
         //  return nearest_neighbour ;
         return nearest_neighbour;
      }

      node3d<T>* get_node(node3d<T>* root, const T& p) const {
         node3d<T> *t = root, *parent = NULL;
         while (t != NULL) {
            int comp = compare(t->axis,p,t->val);
            parent = t;
            if (comp >= 0)
               t = t->right;
            else
               t = t->left;
         }
         return parent;
      }

      /**
       * @throw std::logic_error
       */
      void orthoSearch(node3d<T> *n, const point3d& min, const point3d& max, std::vector<node3d<T>*>& result) const {
         if(n==NULL)
            return;

         double median = n->getMedian();
         double intervalMin = 0;
         double intervalMax = 0;
         switch(n->axis){
         case 0:
            intervalMin = min.x;
            intervalMax = max.x;
            break;
         case 1:
            intervalMin = min.y;
            intervalMax = max.y;
            break;
         case 2:
            intervalMin = min.z;
            intervalMax = max.z;
            break;
         default:
            throw std::logic_error("Unknown axis");
         }
         const T& v = n->val;
         if(v.x>=min.x && v.x<=max.x &&
            v.y>=min.y && v.y<=max.y &&
            v.z>=min.z && v.z<=max.z ){
            result.push_back(n);
         }
         if (median >intervalMax){
            orthoSearch(n->left,min,max,result);
         }else if (median <=intervalMin){
            orthoSearch(n->right,min,max,result);
         }else{
            orthoSearch(n->left,min,max,result);
            orthoSearch(n->right,min,max,result);
         }
      }

      /**
       * @throw null_pointer
       */
      bool add(const T& p){
         int axis = 0;
         if(contains(p))
            return false;

         node3d<T>* const parent = get_node(root, p);

         //non vengono tenute copie dei punti
         if(parent!=NULL)
            axis = (parent->axis+1) % k;

         nodes_pool.push_back(node3d<T>(p,axis));

         node3d<T> *newnode3d = &(nodes_pool.back());

         if(root!=NULL){
            newnode3d->parent = parent;
            if(compare(parent->axis,p,parent->val)>=0){
               parent->right = newnode3d;
            }else{
               parent->left = newnode3d;
            }
         }
         else
            root=newnode3d;

         return true;
      }

   };


} // namespace cvlab

#endif // CVLAB_KDTREE_H
