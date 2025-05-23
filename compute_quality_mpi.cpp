// compute_quality_mpi.cpp
// MPI parallel version: loads a TetGen mesh, computes aspect ratios, and uses MPI_Reduce
// to aggregate statistics and timing (with MPI_Wtime).

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cmath>
#include <limits>

// Simple 3D point
struct Vec3 { double x, y, z; };

// Compute aspect ratio: longest edge / shortest edge
double aspectRatio(const Vec3& A, const Vec3& B,
                   const Vec3& C, const Vec3& D) {
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

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 1. Load mesh on rank 0, then broadcast
    std::vector<Vec3> points;
    std::vector<std::array<int,4>> tets;
    size_t numPoints = 0, numTets = 0;
    if (rank == 0) {
        // Load .node
        std::ifstream nodeFile("diamond1.1.node");
        if (!nodeFile) { std::cerr << "Cannot open diamond1.1.node\n"; MPI_Abort(MPI_COMM_WORLD,1); }
        int dim, numAttr, numMarkers;
        nodeFile >> numPoints >> dim >> numAttr >> numMarkers;
        points.resize(numPoints);
        for (size_t i = 0; i < numPoints; ++i) {
            int id; nodeFile >> id >> points[i].x >> points[i].y >> points[i].z;
        }
        nodeFile.close();
        // Load .ele
        std::ifstream eleFile("diamond1.1.ele");
        if (!eleFile) { std::cerr << "Cannot open diamond1.1.ele\n"; MPI_Abort(MPI_COMM_WORLD,1); }
        int nodesPerTet, eleMarkers;
        eleFile >> numTets >> nodesPerTet >> eleMarkers;
        tets.resize(numTets);
        for (size_t i = 0; i < numTets; ++i) {
            int id, n0,n1,n2,n3;
            eleFile >> id >> n0 >> n1 >> n2 >> n3;
            tets[i] = {{n0-1, n1-1, n2-1, n3-1}};
        }
        eleFile.close();
    }
    // Broadcast sizes
    MPI_Bcast(&numPoints, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&numTets,   1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    // Broadcast data
    if (rank != 0) points.resize(numPoints);
    MPI_Bcast(points.data(), numPoints * sizeof(Vec3), MPI_BYTE, 0, MPI_COMM_WORLD);
    if (rank != 0) tets.resize(numTets);
    MPI_Bcast(tets.data(),   numTets * sizeof(std::array<int,4>), MPI_BYTE, 0, MPI_COMM_WORLD);

    // 2. Partition work among ranks
    size_t perRank = numTets / size;
    size_t start   = rank * perRank;
    size_t end     = (rank == size-1) ? numTets : start + perRank;

    // 3. Compute aspect ratios with MPI_Wtime
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();
    double localMin = std::numeric_limits<double>::infinity();
    double localMax = 0.0;
    double localSum = 0.0;
    for (size_t i = start; i < end; ++i) {
        const auto& tet = tets[i];
        double r = aspectRatio(points[tet[0]], points[tet[1]],
                               points[tet[2]], points[tet[3]]);
        localMin = std::min(localMin, r);
        localMax = std::max(localMax, r);
        localSum += r;
    }
    double t_end = MPI_Wtime();
    double localElapsed = t_end - t_start;

    // 4. Reduce statistics to rank 0
    double globalMin, globalMax, globalSum, globalElapsed;
    MPI_Reduce(&localMin, &globalMin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&localMax, &globalMax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&localSum, &globalSum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&localElapsed, &globalElapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // 5. Rank 0 prints results
    if (rank == 0) {
        double avgRatio = globalSum / static_cast<double>(numTets);
        std::cout << "MPI ranks: " << size << "\n";
        std::cout << "Elapsed_s: " << globalElapsed << "\n";
        std::cout << "Min ratio: " << globalMin
                  << ", Avg ratio: " << avgRatio
                  << ", Max ratio: " << globalMax << "\n";
    }

    MPI_Finalize();
    return 0;
}
