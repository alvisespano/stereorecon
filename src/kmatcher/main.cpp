#include <fstream>
#include <iostream>
#include <QtCore/QCoreApplication>
#include "iidyn.h"
#include <cmath>
#include <time.h>

#include <cstdlib>
#include <vector>
#include <QTextStream>
#include <QFile>
#include <QString>
#include <stdexcept>
#include <algorithm>
#include <QDateTime>

#include <sys/time.h>
#include <time.h>

#define DEV_STD (5)                 // disturbo desiderato nella distribuzione
#define TERMINATION_THRESHOLD (1e-15)

//
// data definitions

using namespace std;

double radius=0;
size_t arg__neighbours, arg__max_iterations, arg__min_good_strategies;
double arg__average_payoff_threshold, arg__payoff_alpha, arg__quality_threshold;

struct feature
{
    vector<double> descr;
    float scale;
    float ori;
    float x, y;
    int index;

    explicit feature(size_t dim) { descr.resize(dim); }
};

struct association
{
    int source;
    int target;
    double sin_alpha, cos_alpha, delta_x, delta_y, delta_scale;
};

ofstream matchinit, matchinit_grp, matchinit_times, matchinit_times_strategies;
char* output_filename;

QTextStream& ws_or_endl(QTextStream& s)
{
    while (!s.atEnd())
    {
        s.skipWhiteSpace();
        qint64 pos = s.pos();
        char c;
        s >> c;
        switch (c)
        {
        case '\n':
        case '\r': continue;
        default:   s.seek(pos); return s;
        }
    }
    return s;
}

/**
 * Open and parse a SIFT keyfile (e.g. 7.key), having the format:
 *
 * <number of features> <length of descriptor>
 * ---now the list of features begins, each in the format:
 * <y> <x> <scale> <orientation in [-pi,pi]>
 * <descriptor line 1, 20 elem.>
 * <descriptor line 2>
 * <descriptor line 3>
 * <descriptor line 4>
 * <descriptor line 5>
 * <descriptor line 6>
 * <descriptor line 7, last 8 elem.>
 */
void parse_sift_keyfile(const QString& filepath, vector<feature>& out_feats)
{
    QFile f(filepath);
    f.open(QFile::ReadOnly | QIODevice::Text);
    QTextStream s(&f);

    size_t nfeats, descr_dim;
    s >> nfeats >> ws >> descr_dim >> ws_or_endl;
    printf("parsing SIFT output file '%s': %d keypoints with %d-dimensional descriptor...\n",
           f.fileName().toStdString().c_str(), nfeats, descr_dim);

    for (int i = 0; i < (int)nfeats; ++i)
    {
        //printf("parse_sift_keyfile: %d\n", i);

        s >> ws_or_endl;
        switch (s.status())
        {
            case QTextStream::Ok:
                break;
            case QTextStream::ReadPastEnd:
                throw runtime_error("unexpected end of stream reached");
            case QTextStream::ReadCorruptData:
                throw runtime_error("stream corrupted");
        }

        feature feat(descr_dim);
        feat.index = i;
        s >> feat.y >> ws >> feat.x >> ws >> feat.scale >> ws >> feat.ori >> ws_or_endl;

        for (size_t di = 0; di < descr_dim; ++di)
        {
            s >> feat.descr[di];
        }

        out_feats.push_back(feat);
    } // next feature
}

/**
 * Read a list_keys file, containing e.g.
 *  ./0.key
 *  ./1.key
 *  ...
 *  ...
 *  ./7.key
 */
vector<QString> imgsFromFile(const char* file){
    ifstream lista;
    lista.open(file);

    vector<QString> listaImgs;
    do
    {
        string name;
        lista >> ws >> name >> ws;
        listaImgs.push_back(QString(name.c_str()));
    } while (!lista.eof());

    lista.close();
    return listaImgs;
}

typedef std::pair<int,double> neighbor_t;

struct less_distant
{
    bool operator() (const neighbor_t& a, const neighbor_t& b) const
    {
        return (a.second < b.second);
    }
};

/**
 *
 */
void neighbours(const feature& f1, const vector<feature>& f2, vector<int>& neighs)
{
    vector<neighbor_t> ns(f2.size());
    const size_t descr_size = f1.descr.size();

    for(size_t i=0;i<f2.size();++i)
    {
        const feature& feat = f2[i];
        double total=0.;

        for(size_t j=0; j<descr_size; ++j)
        {
            const double t0 = (f1.descr[j] - feat.descr[j]);
            total += t0*t0;
        }
        ns[i].first = i;
        ns[i].second = std::sqrt(total);
    }

    std::sort(ns.begin(), ns.end(), less_distant());
    const int nn = std::min(ns.size(), arg__neighbours);

    if (nn <= 0)
        throw std::runtime_error("Asking for a zero or negative amount of neighboring features.");

    neighs.resize(nn);
    for (int k=0; k<nn; ++k)
        neighs[k] = ns[k].first;
}

/**
 *
 */
inline double getCompatibility(const association& a1, const association& a2, const vector<feature>& f1, const vector<feature>& f2)
{
    if(a1.source == a2.source || a1.target == a2.target)
        return 0.;

    const feature& s1 = f1[a1.source];
    const feature& s2 = f1[a2.source];
    const feature& t1 = f2[a1.target];
    const feature& t2 = f2[a2.target];

    const float min_feat_dist = 5.0f;
    if (fabs(s1.x - s2.x) <= min_feat_dist && fabs(s1.y - s2.y) <= min_feat_dist &&
        fabs(t1.x - t2.x) <= min_feat_dist && fabs(t1.y - t2.y) <= min_feat_dist)
    {}//return 0.;

    // apply Tx1 to S2
    const double errorX = t2.x - (a1.delta_scale * (s2.x * a1.cos_alpha - s2.y * a1.sin_alpha) + a1.delta_x);
    const double errorY = t2.y - (a1.delta_scale * (s2.x * a1.sin_alpha + s2.y * a1.cos_alpha) + a1.delta_y);

    // apply Tx2 to S1
    const double errorX2 = t1.x - (a2.delta_scale * (a2.cos_alpha * s1.x - a2.sin_alpha * s1.y) + a2.delta_x);
    const double errorY2 = t1.y - (a2.delta_scale * (a2.sin_alpha * s1.x + a2.cos_alpha * s1.y) + a2.delta_y);

    const double error = std::max(errorX * errorX + errorY * errorY, errorX2 * errorX2 + errorY2 * errorY2);

    //return 1.0 / (1.0 + arg__payoff_alpha * error);
    return std::exp(-arg__payoff_alpha * error);
}

/**
 *
 */
void deleteFeatureFromMatch(
        double *&compMat,
        const vector<feature>&f1,
        const vector<feature> &f2,
        vector<association>& associations,
        const vector< std::pair<size_t,int> >& good_strategies,
        int n_total_strategies,
        int start,
        vector<bool>& active_strategies
){
    for(size_t z=start; z<good_strategies.size(); ++z)
    {
        const association& cur_association = associations[ good_strategies[z].second ];

        double x1 = f1[cur_association.source].x+radius;
        double y1 = f1[cur_association.source].y+radius;
        double x2 = f1[cur_association.source].x-radius;
        double y2 = f1[cur_association.source].y-radius;

        for(size_t i=0;i<(size_t)n_total_strategies;++i){
            if(
                    active_strategies[i] &&
                    f1[associations[i].source].x < x1 &&
                    f1[associations[i].source].x > x2 &&
                    f1[associations[i].source].y < y1 &&
                    f1[associations[i].source].y > y2
              )
            {
                for(size_t j=0;j<(size_t)n_total_strategies;j++)
                {
                    compMat[i*n_total_strategies+j] = 0.;
                    compMat[j*n_total_strategies+i] = 0.;
                }
                active_strategies[i]=false;
            }
        }

        x1 = f2[cur_association.target].x+radius;
        y1 = f2[cur_association.target].y+radius;
        x2 = f2[cur_association.target].x-radius;
        y2 = f2[cur_association.target].y-radius;

        for(size_t i=0;i<(size_t)n_total_strategies;++i){
            if(
                    active_strategies[i] &&
                    f2[associations[i].target].x < x1 &&
                    f2[associations[i].target].x > x2 &&
                    f2[associations[i].target].y < y1 &&
                    f2[associations[i].target].y > y2
              )
            {
                for(size_t j=0;j<(size_t)n_total_strategies;j++)
                {
                    compMat[i*n_total_strategies+j] = 0.;
                    compMat[j*n_total_strategies+i] = 0.;
                }
                active_strategies[i]=false;
            }
        }

    } // next good strategy
}

/*
void deletePopulation(double *population, const vector<feature>&f1, const vector<feature> &f2, association* const & a1, const  vector<int>& out,const int& dim, const int&start){

    for(size_t z=start; z<(size_t) out.size(); z++)
    {
        int index=out[z];
        double x1 = f1[a1[index].source].x;
        double y1 = f1[a1[index].source].y;
        for(size_t i=0;i<(size_t)dim;i++){
            if(f1[a1[i].source].x<x1+width && f1[a1[i].source].x>x1-width&& f1[a1[i].source].y<y1+width && f1[a1[i].source].y>y1-width)
            {
                population[i]=0;
            }
        }

        double x2 = f2[a1[index].target].x;
        double y2 = f2[a1[index].target].y;
        for(size_t i=start;i<(size_t)dim;i++){
            if(f2[a1[i].target].x<x2+width && f2[a1[i].target].x>x2-width&& f2[a1[i].target].y<y2+width && f2[a1[i].target].y>y2-width)
                population[i]=0;
        }
    }

}
*/
/*
double avg_payoff(const double *x, const double* A, const size_t & dim){
    double payoff=0;
    double *tmp=new double[dim];
    for(size_t i=0; i<dim; ++i){
        tmp[i]=0;
        for(size_t j=0;j<dim;++j)
            tmp[i]+=x[j]*A[i*dim+j];
    }
    for(size_t i=0; i<dim; ++i)
        payoff+=tmp[i]*x[i];
    delete [] tmp;
    return payoff;
}
*/

inline double dot(const vector<double>& x, const double *y)
{
    double sum = 0.;
    for(int i=0; i<(int)x.size(); ++i,++y)
        sum += x[i] * (*y);
    return sum;
}

/**
 *
 *
 * @param feats_from
 * @param feats_to
 * @param img_from
 * @param img_to
 */
void kmatcher2(
        const vector<feature>& feats_from,
        const vector<feature>& feats_to,
        int img_from,
        int img_to,
        int& n_total_matches,
        int& n_groups)
{
    const size_t n_strategies = arg__neighbours*feats_from.size();
    vector<association> associations(n_strategies);

    n_groups = 0;

    // Get the first N neighbors

    for(size_t i=0; i < feats_from.size(); ++i)
    {
        vector<int> neighs;
        neighbours(feats_from[i], feats_to, neighs);
        const int ii=i*arg__neighbours;
        for(size_t j=0; j<arg__neighbours; ++j){
            associations[ii+j].source=i;
            associations[ii+j].target=neighs[j];
        }
    }

    // Fill the compatibility matrix

    double* compat_matrix = new double[n_strategies*n_strategies];
    std::fill(compat_matrix, compat_matrix + (n_strategies*n_strategies), 0.);

    for (size_t i = 0; i < n_strategies; ++i)
    {
        const feature& src = feats_from.at(associations[i].source);
        const feature& targ = feats_to.at(associations[i].target);

        //const double a = (targ.ori > 0. ? targ.ori : 2. * M_PI + targ.ori);
        //const double b = (src.ori > 0. ? src.ori : 2. * M_PI + src.ori);
        //const double a = (-targ.ori > 0. ? -targ.ori : 2. * M_PI - targ.ori);
        //const double b = (-src.ori > 0. ? -src.ori : 2. * M_PI - src.ori);
        const double a = targ.ori;
        const double b = src.ori;

        const double deltaRot = a - b;
        const double cosAlfa = std::cos(deltaRot);
        const double sinAlfa = std::sin(deltaRot);
        const double deltaScale = (double)targ.scale/(double)src.scale;

        const double scaledX = src.x*deltaScale;
        const double scaledY = src.y*deltaScale;
        const double x = scaledX * cosAlfa - scaledY * sinAlfa;
        const double y = scaledX * sinAlfa + scaledY * cosAlfa;

        associations[i].cos_alpha = cosAlfa;
        associations[i].sin_alpha = sinAlfa;
        associations[i].delta_x = targ.x - x;
        associations[i].delta_y = targ.y - y;
        associations[i].delta_scale = deltaScale;
    }

    for(size_t i = 0; i < n_strategies - 1; ++i) {
        for(size_t j = i + 1; j < n_strategies; ++j)
        {
            const double x = getCompatibility(associations[i], associations[j], feats_from, feats_to);
            compat_matrix[i*n_strategies + j] = x;
            compat_matrix[j*n_strategies + i] = compat_matrix[i*n_strategies + j];
        }
    }

    // Create the initial population and slightly perturb it

    double *population=new double[n_strategies];
    vector<double> perturbed(n_strategies);

    for(size_t i=0; i<n_strategies; ++i)
        perturbed[i] = 1.0/(double)n_strategies;

    for(size_t i=0;i<n_strategies;++i)
    {
        double tmp=rand()%DEV_STD;
        int indTmp = rand()%n_strategies;
        perturbed[indTmp] += (tmp =( perturbed[indTmp] * tmp/100. ));
        indTmp = rand()%n_strategies;
        perturbed[indTmp] -= tmp;
    }

    //

    //cout<<" finding cluster  "<<QDateTime::currentDateTime().toString("ddd dd/MM/yyyy hh:mm:ss").toStdString() << endl;

    vector<bool> active_strategies(n_strategies, true);
    vector< std::pair<size_t, int> > good_strategies; // <group,index>

    vector<double> AP(n_strategies); // used for comput. average payoff
    vector< std::pair<size_t,double> > convergence_times;

    for (size_t loop_iter = 0; loop_iter < arg__max_iterations; ++loop_iter)
    {
        // re-initialize with the perturbed population
        std::copy(perturbed.begin(), perturbed.end(), population);

        QTime timer; timer.start();

        int iidyn_iters = iidyn(compat_matrix, population, n_strategies, TERMINATION_THRESHOLD, 10000);

        const double elapsed = (double)timer.elapsed() * 1e-3;
        cout << "iidyn: " << iidyn_iters << " iterations "<<endl;
        cout << "convergence time: " << elapsed << " sec."<<endl;

        // average payoff is P'AP
        // where P is 'population'

        for(size_t z=0; z<n_strategies; ++z){
            AP[z] = 0.;
            for(size_t zz=0; zz<n_strategies; ++zz)
                AP[z] += compat_matrix[z*n_strategies+zz] * population[zz];
        }
        const double avg_payoff = dot(AP, population);

        cout << "average payoff = " << avg_payoff << endl;
        if (avg_payoff < arg__average_payoff_threshold){
            if(loop_iter==0){
                cout << "[" << img_from << "," << img_to << "] exit condition: no good strategies at the first iteration (avg payoff: " << avg_payoff << " < " << arg__average_payoff_threshold << ")" << endl;
                break;
            }
            else
                cout << "[" << img_from << "," << img_to << "] exit condition: average payoff below threshold (" << avg_payoff << " < " << arg__average_payoff_threshold << ")" << endl;
            break;
        }

        // Keep the best strategies (below some fixed thresh wrt the max population)

        const double population_thresh = *std::max_element(population, population + n_strategies) * arg__quality_threshold;
        vector<int> good_strategies_tmp;
        for (size_t i = 0; i < n_strategies; ++i) {
            if (population[i] > population_thresh)
                good_strategies_tmp.push_back(i);
        }

        const size_t n_good_strategies = good_strategies_tmp.size();

        foreach (const int& gs, good_strategies_tmp)
        {
            good_strategies.push_back(std::make_pair(loop_iter, gs));
        }
        cout << "matches found: " << n_good_strategies << endl;

        //DEBUG print an ascii histogram
        /*{
            std::ofstream hist("hist.txt", std::ios::out);
            const int maxchars = 80;
            const double maxe = *std::max_element(population, population + n_strategies);
            for (size_t x=0; x<n_strategies; ++x) {
                const double v = population[x];
                if (v==0.) continue;
                const int nchars = static_cast<int>( round(maxchars * (v / maxe)) );
                for (int ch=0; ch<nchars; ++ch) hist << "#";
                hist << endl;
            }
        }*/

        //

        const size_t cur_iter_strategies = n_strategies - good_strategies.size();

        if (loop_iter != arg__max_iterations - 1)
            deleteFeatureFromMatch(
                    compat_matrix,
                    feats_from, feats_to,
                    associations,
                    good_strategies,
                    n_strategies,
                    good_strategies.size() - n_good_strategies,
                    active_strategies);

        if (n_good_strategies < arg__min_good_strategies)
        {
            cout << "[" << img_from << "," << img_to << "] few matches (< " << arg__min_good_strategies << "), skipping..." << endl;
            good_strategies.erase(good_strategies.end() - n_good_strategies, good_strategies.end());
        }
        else {
            convergence_times.push_back( std::make_pair(cur_iter_strategies,elapsed) );
            ++n_groups;
        }

    } // next iteration (candidate group)

    cout << "[" << img_from << "," << img_to << "] Total matches: " << good_strategies.size() << endl;
    n_total_matches = (int)good_strategies.size();

    if(good_strategies.size() >= 17){
        matchinit << img_from << " " << img_to << endl;
        matchinit << good_strategies.size() << endl;

        matchinit_grp << img_from << " " << img_to << endl;
        matchinit_grp << good_strategies.size() << endl;

        typedef std::pair<size_t, int> strategy_pair;
        foreach (const strategy_pair& p, good_strategies)
        {
            size_t group = p.first;
            const int i = p.second;
            const int a = feats_from[associations[i].source].index;
            const int b = feats_to[associations[i].target].index;
            matchinit << a << " " << b << endl;
            matchinit_grp << a << " " << b << " " << group << endl;
        }
    }
    else {
        cout << "[" << img_from << "," << img_to << "] WARNING: too few matches for bundle adjustment. " <<
            "Bundler needs at least 16 matches to estimate the fundamental matrix." << endl;
        //matchinit << img_from << " " << img_to << endl;
        //matchinit << "0" << endl;
    }

    matchinit_times << convergence_times.size();
    matchinit_times_strategies << convergence_times.size();
    for (size_t i=0; i<convergence_times.size(); ++i)
    {
        matchinit_times << " " << convergence_times[i].second;
        matchinit_times_strategies << " " << convergence_times[i].first;
        cout << " " << convergence_times[i].first;
    }
    cout << endl;

    //cout<<" cluster iteration ending "<<QDateTime::currentDateTime().toString("ddd dd/MM/yyyy hh:mm:ss").toStdString() << endl;

    delete [] compat_matrix;
    delete [] population;
}

/**
 *
 */
int main(int argc, char *argv[])
{
    static const int arg_num = 11;

    if (argc < arg_num - 1 || argc > arg_num + 1)
    {
        std::cout << "usage: " << argv[0]
                  << " <KEYFILE LIST FILENAME> <OUTPUT FILENAME> <IMG WIDTH> <NEIGHBOURS> <MAX ITERATIONS> <AVG PAYOFF THRESHOLD> <PAYOFF ALPHA> <QUALITY THRESHOLD> <MIN GOOD STRATEGIES> [#IMG REFERENCE]" << std::endl;
        return 1;
    }

    // ref. image if one-vs-all is selected
    const char* list_filename = argv[1];
    output_filename = argv[2];

    radius = 15.;
    cout << "RADIUS: " << radius << endl;

    arg__neighbours = atoi(argv[4]);
    arg__max_iterations = atoi(argv[5]);
    arg__average_payoff_threshold = atof(argv[6]);
    arg__payoff_alpha = atof(argv[7]);
    arg__quality_threshold = atof(argv[8]);
    arg__min_good_strategies = atoi(argv[9]);

    cout << "MIN GOOD STRATEGIES: " << arg__min_good_strategies << endl;

    const int img_base = (argc == arg_num ? atoi(argv[arg_num - 1]) : -1);
    const bool one_vs_all = (img_base != -1);

    const vector<QString> keyfiles = imgsFromFile(list_filename);
    const size_t n_imgs = keyfiles.size();

    matchinit.open(output_filename);
    matchinit_grp.open(QString(output_filename).append(".grp").toStdString().c_str());
    matchinit_times.open(QString(output_filename).append(".exp").toStdString().c_str());
    matchinit_times_strategies.open(QString(output_filename).append(".strats").toStdString().c_str());

    if(one_vs_all){
        cout << "one-vs-all" << endl;

        vector<feature> reference_features;
        parse_sift_keyfile(keyfiles[img_base], reference_features);

        cout<<" got "<<reference_features.size()<<" random features"<<endl;

        for(size_t j=0;j<n_imgs;++j){
            if(j==(size_t)img_base) continue;
            cout<<"matching image pair #"<<keyfiles[img_base].toStdString()<<","<<keyfiles[j].toStdString()<<endl;
            vector<feature> feat;
            parse_sift_keyfile(keyfiles[j], feat);
            cout<<" got "<<feat.size()<<" random features"<<endl;
            int nmatches, ngroups;
            kmatcher2(reference_features, feat, img_base, j, nmatches, ngroups);
        } // next image
    }

    // all-vs-all
    else
    {
        cout << "all-vs-all" << endl;

        vector< vector<feature> > feats; // contains the features for each image

        for(size_t i=0; i<n_imgs; ++i)
        {
            vector<feature> image_feats;
            parse_sift_keyfile(keyfiles[i], image_feats);
            feats.push_back(image_feats);
        } // next image

        std::ofstream matches_groups(QString(output_filename).append(".mgrp").toStdString().c_str(), std::ios::out);

        for(size_t i=0; i<n_imgs-1; ++i){
            for(size_t j=i+1; j<n_imgs; ++j)
            {
                cout << "matching image pair [" <<
                        keyfiles[i].toStdString() << " " <<
                        keyfiles[j].toStdString() << "]" << endl;
                int n_matches, n_groups;
                kmatcher2(feats[i], feats[j], i, j, n_matches, n_groups);

                matches_groups << i << " " << j << " " << n_matches << " " << n_groups << endl;

            } // next image
        } // next image

        matches_groups.close();
    }

    matchinit.close();
    matchinit_grp.close();
    matchinit_times.close();
    matchinit_times_strategies.close();
}
