#include <iostream>
#include <vector>
#include <array>
#include <omp.h>          // OpenMP

struct Vec3 { double x,y,z; };

int main(int argc, char* argv[]) {
    if (argc!=2) { std::cerr<<"Usage: "<<argv[0]<<" <N>\n"; return 1; }
    int N = std::stoi(argv[1]);
    double a = 1.0;

    // FCC sites and two‐atom basis
    std::vector<Vec3> fcc   = {{0,0,0},{0,0.5,0.5},{0.5,0,0.5},{0.5,0.5,0}};
    std::vector<Vec3> basis = {{0,0,0},{0.25,0.25,0.25}};

    // precompute total points:  N^3 * fcc.size() * basis.size()
    size_t totalPts = size_t(N)*N*N * fcc.size() * basis.size();
    std::vector<Vec3> pts(totalPts);

    // parallel fill with a collapsed loop
    #pragma omp parallel for collapse(3)
    for(int i=0; i<N; ++i){
      for(int j=0; j<N; ++j){
        for(int k=0; k<N; ++k){
          size_t cellIdx = (size_t(i)*N + j)*N + k;
          size_t baseOffset = cellIdx * (fcc.size()*basis.size());
          for(size_t fi=0; fi<fcc.size(); ++fi){
            double fx0 = (i + fcc[fi].x)*a;
            double fy0 = (j + fcc[fi].y)*a;
            double fz0 = (k + fcc[fi].z)*a;
            for(size_t bi=0; bi<basis.size(); ++bi){
              auto &b = basis[bi];
              Vec3 p { fx0 + b.x*a,
                       fy0 + b.y*a,
                       fz0 + b.z*a };
              size_t idx = baseOffset + fi*basis.size() + bi;
              pts[idx] = p;
            }
          }
        }
      }
    }

    // pass to TetGen
    std::cout<<pts.size()<<" 3 0 0\n";
    for(size_t i=0;i<pts.size();++i){
      auto &p = pts[i];
      std::cout<<(i+1)<<" "<<p.x<<" "<<p.y<<" "<<p.z<<"\n";
    }
}
