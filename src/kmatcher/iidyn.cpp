#include "iidyn.h"
#include <limits>
#include <iostream>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

inline void mult(const double *A,const double *x, int size, double *y){
  const double *ptr;
  for(int j=0;j<size;++j,++y){
    ptr=x;
    *y=0;
    for(int i=0;i<size;++i,++ptr,++A)
      *y+=*A**ptr;
  }
}

inline void mult_m(const double *A,const double *x, int size, double *y, const bool *mask){
  const double *ptr;
  const bool *m1=mask, *m2;
  for(int j=0;j<size;++j,++y,++m1)
    if(!*m1){
      ptr=x;
      *y=0;
      m2=mask;
      for(int i=0;i<size;++i,++ptr,++A,++m2)
        if(!*m2)
          *y+=*A**ptr;
    }else A+=size;
}


inline void simplexify(double *x, int size){
  double sum=0;
  double *ptr=x;
  for(int i=0;i<size;++i,++ptr)
    if(*ptr>=0)
      sum+=*ptr;
    else
      *ptr=0;
  for(int i=0;i<size;++i,++x)
    *x/=sum;
}

inline void simplexify_m(double *x, int size, const bool *mask){
  double sum=0;
  double *ptr=x;
  const bool *m=mask;
  for(int i=0;i<size;++i,++ptr,++m)
    if(!*m){
      if(*ptr>=0)
        sum+=*ptr;
      else
        *ptr=0;
      }
  m=mask;
  for(int i=0;i<size;++i,++x,++m)
    if(!*m)
      *x/=sum;
}

inline double dot(const double *x, const double *y, int size){
  double sum=0;
  for(int i=0;i<size;++i,++x,++y)
    sum+=*x**y;
  return sum;
}

inline double dot_m(const double *x, const double *y, int size, const bool *mask){
  double sum=0;
  for(int i=0;i<size;++i,++x,++y,++mask)
    if(!*mask)
      sum+=*x**y;
  return sum;
}

inline void scale(double *x, int size, double c){
  for(int i=0;i<size;++i,++x)
    *x*=c;
}

inline void mul_scaled(const double *x, double *y,int size, double c){
  for(int i=0;i<size;++i,++x,++y)
    *y*=*x*c;
}

inline void scale_m(double *x, int size, double c, const bool *mask){
  for(int i=0;i<size;++i,++x,++mask)
    if(!*mask)
      *x*=c;
}

inline void linear_comb(const double *x, double *y, int size,double alfa){
  for(int i=0;i<size;++i,++x,++y)
    *y=alfa*(*x-*y)+*y;
}

inline void linear_comb_m(const double *x, double *y, int size,double alfa,const bool *mask){
  for(int i=0;i<size;++i,++x,++y,++mask)
    if(!*mask)
      *y=alfa*(*x-*y)+*y;
}

inline double nash_error(const double *x, const double *Ax, double xAx, int size){
  double sum=0;
  const double *p_x=x;
  const double *p_Ax=Ax;
  double tmp;
  for(int i=0;i<size;++i,++p_x,++p_Ax){
    tmp=xAx-*p_Ax;
    if(tmp>*p_x)
      tmp=*p_x;
    sum+=tmp*tmp;
  }
  return sum;
}

inline double nash_error_m(const double *x, const double *Ax, double xAx, int size, const bool *mask){
  double sum=0;
  const double *p_x=x;
  const double *p_Ax=Ax;
  double tmp;
  for(int i=0;i<size;++i,++p_x,++p_Ax,++mask)
    if(!*mask){
    tmp=xAx-*p_Ax;
    if(tmp>*p_x)
      tmp=*p_x;
    sum+=tmp*tmp;
  }
  return sum;
}

inline double selectStrategy(const double *Ax, const double *x, int size, double &delta, int &idx){
  const double *ptr1,*ptr2;
  double max=-std::numeric_limits<double>::infinity();
  double min=std::numeric_limits<double>::infinity();

  int max_idx=-1,min_idx=-1;
  ptr1=Ax;
  ptr2=x;

  for(int i=0;i<size;++i,++ptr1,++ptr2){
    if(*ptr1>max){
      max=*ptr1;
      max_idx=i;
    }
    if(*ptr2>0&&*ptr1<min){
      min=*ptr1;
      min_idx=i;
    }
  }
  double xAx=dot(Ax,x,size);

  max-=xAx;
  min=xAx-min;

  idx=max_idx;
  delta=max;
  if(max<min){
    idx=min_idx;
    delta=-min;
  }

  //return max+min;
  double error=nash_error(x,Ax,xAx,size);
  //std::cout << error << std::endl;
  return error;
}

inline double selectStrategy_m(const double *Ax, const double *x, int size, double &delta, int &idx, const bool *mask){
  const double *ptr1,*ptr2;
  double max=-std::numeric_limits<double>::infinity();
  double min=std::numeric_limits<double>::infinity();

  int max_idx=-1,min_idx=-1;
  ptr1=Ax;
  ptr2=x;
  const bool *ptr_b=mask;
  for(int i=0;i<size;++i,++ptr1,++ptr2,++ptr_b)
    if(!*ptr_b){
      if(*ptr1>max){
        max=*ptr1;
        max_idx=i;
      }
      if(*ptr2>0&&*ptr1<min){
        min=*ptr1;
        min_idx=i;
      }
    }

  //if(max_idx<0||min_idx<0)return 0;
  double xAx=dot_m(Ax,x,size,mask);
  max-=xAx;
  min=xAx-min;

  idx=max_idx;
  delta=max;
  if(max<min){
    idx=min_idx;
    delta=-min;
  }

  double error=nash_error_m(x,Ax,xAx,size,mask);
  //std::cout << error << std::endl;
  return error;
  //return max+min;
}



int iidyn(const double *A, double *x, int size, double toll, int max_iters){
  int niter=0;
  /* Calculate Ax */
  double *Ax=new double[size];
  simplexify(x,size);
  mult(A,x,size,Ax);

  /*** OCCHIO **/
  toll*=toll;

  double delta=0;
  while(niter < max_iters)
  {
    int idx=-1;
    if(selectStrategy(Ax,x,size,delta,idx)<toll)
      break;

    double den=A[idx*(size+1)]-Ax[idx]-delta;
    bool do_remove=false;
    double mu,tmp;
    if(delta>=0){
      mu=1;
      if(den<0){
        tmp=-delta/den;
        if(mu>tmp)mu=tmp;
        if(mu<0)mu=0;
      }
    }else{
      //mu=1.0-1.0/(1-x[idx]);
      mu=x[idx]/(x[idx]-1);
      do_remove=true;
      if(den<0){
        tmp=-delta/den;
        if(mu<tmp){
          mu=tmp;
          do_remove=false;
        }
        if(mu>0)mu=0;
      }
    }
    scale(x,size,1-mu);
    x[idx]=do_remove?0:x[idx]+mu;
    simplexify(x,size);

    linear_comb(A+idx*size,Ax,size,mu);

    ++niter;
  }

  delete[] Ax;

  return niter;
}

void print_masked(const double *x, int size, const bool *mask){
  for(int i=0;i<size;++i,++x,++mask)
    if(*x>0&&!*mask)
      std::cout << i << ":" << *x << ", ";
  std::cout << std::endl;
}


int iidyn_m(const double *A, double *x, int size, double toll, const bool *mask){
  int niter=0;
  /* Calculate Ax */
  double *Ax=new double[size];
  simplexify_m(x,size,mask);
  mult_m(A,x,size,Ax,mask);

  /* OCCHIO */
  toll*=toll;

  double delta=0;
  while(true){
    int idx=-1;
    if(selectStrategy_m(Ax,x,size,delta,idx,mask)<toll)
      break;

    double den=A[idx*(size+1)]-Ax[idx]-delta;
    bool do_remove=false;
    double mu,tmp;
    if(delta>=0){
      mu=1;
      if(den<0){
        tmp=-delta/den;
        if(mu>tmp)mu=tmp;
        //if(mu<0)mu=0;
      }
    }else{
      //mu=1.0-1.0/(1-x[idx]);
      mu=x[idx]/(x[idx]-1);
      do_remove=true;
      if(den<0){
        tmp=-delta/den;
        if(mu<tmp){
          mu=tmp;
          do_remove=false;
        }
        //if(mu>0)mu=0;
      }
    }
    scale_m(x,size,1-mu,mask);
    x[idx]=do_remove?0:x[idx]+mu;

    simplexify_m(x,size,mask);

    linear_comb_m(A+idx*size,Ax,size,mu,mask);


    ++niter;
  }

  /* TO REMOVE !!! */
  double xAx=dot_m(Ax,x,size,mask);
  for(int i=0;i<size;++i)
    x[i]=Ax[i]/xAx>0.8?1:0;

  delete[] Ax;

  return niter;
}


void print(const double *x, int size){
  for(int i=0;i<size;++i,++x)
    if(*x>0)
      std::cout << i << ":" << *x << ", ";
  std::cout << std::endl;
}


void print_m(const double *A, int size){
  for(int j=0;j<size;j++){
    for(int i=0;i<size;++i,++A)
      std::cout << *A << " ";
    std::cout << std::endl;
  }
}

bool seeded=false;

void mrand(double *A, int size, double offset, double density){
  if(!seeded){srand(time(NULL)); seeded=true;}
  for(int j=0;j<size;++j){
    A[j*size+j]=0;
    for(int i=j+1;i<size;++i)
      A[j*size+i]=A[i*size+j]=(double)rand()/RAND_MAX<=density?offset+(double)rand()/RAND_MAX:0;
  }
}

void vrand(double *x, int size){
  if(!seeded){srand(time(NULL)); seeded=true;}
  for(int i=0;i<size;++i,++x)
    *x=(double)rand()/RAND_MAX;
}

inline void set_m(double *x,int size, double c,bool *mask){
    for(int j=0;j<size;++j,++mask,++x)
      if(!*mask)
        *x=c;
}

int clustering(const double *A, int *clusters, int size, int k){
  bool *mask=new bool[size];
  bool *ptr_b=mask;
  int n_clusters=0;
  //int *ptr_i=clusters;
  for(int i=0;i<size;++i,++ptr_b){
    *ptr_b=false;
    //*ptr_i=-1;
  }
  double *x=new double[size], *ptr_d;
  int n=size;
  for(int i=0;i<k&&n>0;++i){
    set_m(x,size,1.0/n,mask);
    int niter=iidyn_m(A,x,size,1E-7,mask);



    int cluster_size=n;
    ptr_d=x;
    ptr_b=mask;
    double max=0;
    int max_idx=-1;
    for(int j=0;j<size;++j,++ptr_d,++ptr_b)
      if(!*ptr_b&&*ptr_d>0){
        *ptr_b=true;
        if(*ptr_d>max){
          max=*ptr_d;
          max_idx=j;
        }
        //*ptr_i=i;
        --n;
      }
    *clusters++=max_idx;
    ++n_clusters;

    //for(int j=0;j<size;j++)std::cout << mask[j] << " " ;
   //std::cout << std::endl;

    std::cout << "cluster " << i << " iter="<<niter << " size="<<(cluster_size-n)<<std::endl;
  }

  delete[] mask;
  delete[] x;
  return n_clusters;
}

int clustering_noreass(const double *A, int *clusters, int size, int k){
  bool *mask=new bool[size];
  bool *ptr_b=mask;
  int n_clusters=0;
  //int *ptr_i=clusters;
  for(int i=0;i<size;++i,++ptr_b){
    *ptr_b=false;
    clusters[i]=0;
    //*ptr_i=-1;
  }
  double *x=new double[size], *ptr_d;
  int n=size;
  for(int i=0;i<k&&n>0;++i){
    set_m(x,size,1.0/n,mask);
    int niter=iidyn_m(A,x,size,1E-5,mask);



    int cluster_size=n;
    ptr_d=x;
    ptr_b=mask;

    int *p_clusters=clusters;

    for(int j=0;j<size;++j,++ptr_d,++ptr_b,++p_clusters)
      if(!*ptr_b&&*ptr_d>0){
        *ptr_b=true;
        *p_clusters=i+1;
        //*ptr_i=i;
        --n;
      }
    ++n_clusters;

    //for(int j=0;j<size;j++)std::cout << mask[j] << " " ;
   //std::cout << std::endl;

    //std::cout << "cluster " << i << " iter="<<niter << " size="<<(cluster_size-n)<<std::endl;
  }

  delete[] mask;
  delete[] x;
  return n_clusters+1;
}


int repdyn(const double *A, double *x, int size, double toll){
  toll*=toll;
  double *Ax=new double[size];
  double xAx;
  int niter=0;
  simplexify(x,size);
  while(true){
    mult(A,x,size,Ax);
    xAx=dot(Ax,x,size);
    if(nash_error(x,Ax,xAx,size)<toll)break;
    mul_scaled(Ax,x,size,1.0/xAx);
    ++niter;
  }
  delete[] Ax;
  return niter;
}

int repdyn_v(const double *A, double *x, int size, double toll, double &error){
  toll*=toll;
  double *Ax=new double[size];
  double *xold=new double[size];
  double xAx=0;
  int niter=0;
  simplexify(x,size);
  double vel=2*toll;
  double tmp;
  while(vel>toll){
    mult(A,x,size,Ax);
    xAx=dot(Ax,x,size);
    memcpy(xold,x,sizeof(double)*size);
    mul_scaled(Ax,x,size,1.0/xAx);
    ++niter;
    vel=0;
    for(int i=0;i<size;++i){
      tmp=x[i]-xold[i];
      vel+=tmp*tmp;
    }
  }
  error=sqrt(nash_error(x,Ax,xAx,size));
  delete[] Ax;
  delete[] xold;
  return niter;
}
