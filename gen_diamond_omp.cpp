#include <iostream>
#include <vector>
#include <array>
#include <omp.h>

struct Vec3 { double x,y,z; };

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <N-cells-per-edge>\n";
        return 1;
    }
    int N = std::stoi(argv[1]);
    double a = 1.0;

    // FCC sites and two-atom basis
    std::vector<Vec3> fcc   = {{0,0,0},{0,0.5,0.5},{0.5,0,0.5},{0.5,0.5,0}};
    std::vector<Vec3> basis = {{0,0,0},{0.25,0.25,0.25}};

    // Compute total points: N^3 * 4 * 2 = 8*N^3
    size_t totalPts = static_cast<size_t>(N) * N * N * fcc.size() * basis.size();
    std::vector<Vec3> pts(totalPts);

    // Parallel generate points and time the region
    double t_start = omp_get_wtime();
    #pragma omp parallel for collapse(3)
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            for (int k = 0; k < N; ++k) {
                size_t cellIdx = (static_cast<size_t>(i) * N + j) * N + k;
                size_t baseOffset = cellIdx * (fcc.size() * basis.size());
                for (size_t fi = 0; fi < fcc.size(); ++fi) {
                    double fx0 = (i + fcc[fi].x) * a;
                    double fy0 = (j + fcc[fi].y) * a;
                    double fz0 = (k + fcc[fi].z) * a;
                    for (size_t bi = 0; bi < basis.size(); ++bi) {
                        Vec3 p { fx0 + basis[bi].x * a,
                                 fy0 + basis[bi].y * a,
                                 fz0 + basis[bi].z * a };
                        size_t idx = baseOffset + fi * basis.size() + bi;
                        pts[idx] = p;
                    }
                }
            }
        }
    }
    double t_end = omp_get_wtime();
    double elapsed = t_end - t_start;

    // Output timing
    std::cout << "Creation time: " << elapsed << " seconds\n";

    // Output .node format (can be piped to file)
   /* std::cout << totalPts << " 3 0 0\n";
    for (size_t i = 0; i < totalPts; ++i) {
        auto &p = pts[i];
        std::cout << (i+1) << " " << p.x << " " << p.y << " " << p.z << "\n";
    } */

    return 0;
}
