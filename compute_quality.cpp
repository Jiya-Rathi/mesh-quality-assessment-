#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>

// Simple 3D point structure
struct Vec3 { double x, y, z; };

// Compute aspect ratio: longest edge / shortest edge
double aspectRatio(const Vec3& A, const Vec3& B, const Vec3& C, const Vec3& D) {
    std::array<Vec3,6> edges = {
        Vec3{B.x-A.x, B.y-A.y, B.z-A.z},
        Vec3{C.x-A.x, C.y-A.y, C.z-A.z},
        Vec3{D.x-A.x, D.y-A.y, D.z-A.z},
        Vec3{C.x-B.x, C.y-B.y, C.z-B.z},
        Vec3{D.x-B.x, D.y-B.y, D.z-B.z},
        Vec3{D.x-C.x, D.y-C.y, D.z-C.z}
    };
    double minLen = std::numeric_limits<double>::infinity();
    double maxLen = 0.0;
    for (const auto& e : edges) {
        double len = std::sqrt(e.x*e.x + e.y*e.y + e.z*e.z);
        minLen = std::min(minLen, len);
        maxLen = std::max(maxLen, len);
    }
    return maxLen / minLen;
}

int main() {
    // 1. Load .node file
    std::ifstream nodeFile("diamond1.1.node");
    if (!nodeFile) {
        std::cerr << "Cannot open diamond1.1.node\n";
        return 1;
    }
    size_t numPoints;
    int dim, numAttr, numMarkers;
    nodeFile >> numPoints >> dim >> numAttr >> numMarkers;
    std::vector<Vec3> points(numPoints);
    for (size_t i = 0; i < numPoints; ++i) {
        int id;
        nodeFile >> id >> points[i].x >> points[i].y >> points[i].z;
    }
    nodeFile.close();

    // 2. Load .ele file
    std::ifstream eleFile("diamond1.1.ele");
    if (!eleFile) {
        std::cerr << "Cannot open diamond1.1.ele\n";
        return 1;
    }
    size_t numTets;
    int nodesPerTet, eleMarkers;
    eleFile >> numTets >> nodesPerTet >> eleMarkers;
    std::vector<std::array<int,4>> tets(numTets);
    for (size_t i = 0; i < numTets; ++i) {
        int id, n0, n1, n2, n3;
        eleFile >> id >> n0 >> n1 >> n2 >> n3;
        tets[i] = {{n0-1, n1-1, n2-1, n3-1}};
    }
    eleFile.close();

    std::cout << "Loaded " << numPoints << " points and " << numTets << " tetrahedra.\n";

    // 3. Compute aspect ratios and time the loop
    std::vector<double> ratios;
    ratios.reserve(numTets);
    auto t0 = std::chrono::high_resolution_clock::now();
    for (const auto& tet : tets) {
        const Vec3& A = points[tet[0]];
        const Vec3& B = points[tet[1]];
        const Vec3& C = points[tet[2]];
        const Vec3& D = points[tet[3]];
        ratios.push_back(aspectRatio(A, B, C, D));
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    // 4. Compute statistics
    double sum = 0.0;
    double minRatio = std::numeric_limits<double>::infinity();
    double maxRatio = 0.0;
    for (double r : ratios) {
        sum += r;
        minRatio = std::min(minRatio, r);
        maxRatio = std::max(maxRatio, r);
    }
    double avgRatio = sum / ratios.size();

    std::cout << "Computed aspect ratios in " << elapsed << " seconds.\n";
    std::cout << "Min ratio: " << minRatio
              << ", Avg ratio: " << avgRatio
              << ", Max ratio: " << maxRatio << "\n";

    // 5. Write raw ratios to file for MATLAB
    std::ofstream fout("aspect_values.txt");
    if (!fout) {
        std::cerr << "Cannot open aspect_values.txt for writing\n";
        return 1;
    }
    for (double r : ratios) {
        fout << r << "\n";
    }
    fout.close();

    return 0;
}
