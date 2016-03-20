#include <QtCore/QCoreApplication>
#include <iostream>
#include <cv.h>
#include <highgui.h>
#include <fstream>
#include "surf.h"
#include "surflib.h"
#include <vector>
#include <string>

using namespace std;

#define IS_USURF (false)              // run in rotation invariant mode?
#define OCTAVE_NUMBER (3)             // number of octaves to calculate
#define INTERVALS_PER_OCTAVE (4)      // number of intervals per octave
#define INITIAL_SAMPLING_STEP (2)     // initial sampling step

double SURF_NUMBER_PARAM;               // blob response threshold

int scriviKey(const char * nomeImg, const char * nomeOutput){
    ofstream key;
    IpVec ipts;
    IplImage *img=cvLoadImage(nomeImg);
    if(img==NULL){
        cout<<"No good image: "<<nomeImg<<endl;
        return 1;
    }
    key.open(nomeOutput);
    surfDetDes(img, ipts, IS_USURF, OCTAVE_NUMBER, INTERVALS_PER_OCTAVE, INITIAL_SAMPLING_STEP, SURF_NUMBER_PARAM);
    Ipoint t=ipts.at(0);
    key<<ipts.size()<<" "<<(double)sizeof(t.descriptor)/(double)sizeof(t.descriptor[0])<<"\n";

    for(size_t i=0;i<ipts.size();i++)
    {
        t=ipts.at(i);

        key<<t.y<<" ";
        key<<t.x<<" ";
        key<<t.scale<<" ";
        key<<t.orientation<<"\n";
        for(int j=0;j<(double)sizeof(t.descriptor)/(double)sizeof(t.descriptor[j]);j++)
        {
          key<<" "<<t.descriptor[j];

        }
        key<<"\n";
    }


    free(img);
    key.close();
    cout<<endl<<"terminato"<<endl;
    return 0;
}

int main(int argc, char *argv[])
{
    if(argv[1]==NULL && argv[2]==NULL && argv[3]==NULL)
    {
        cout<<"usage: add files [image path][output file][blob threshold as argument"<<endl;
        return 1;
    }
    const char *img=argv[1];
    const char *output=argv[2];
    SURF_NUMBER_PARAM=atof(argv[3]);
    cout<<"matching image: "<<img<<" into: "<<output<<" with response "<<SURF_NUMBER_PARAM<<endl;
    return scriviKey(img, output);
}


