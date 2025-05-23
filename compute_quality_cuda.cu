// compute_quality_cuda.cu
// CUDA version: loads a TetGen mesh, computes aspect ratios on the GPU, and records kernel time.

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cmath>
#include <limits>
#include <cuda_runtime.h>

// Simple 3D point
struct Vec3 { double x, y, z; };

// Device function to compute aspect ratio
__device__ double aspectRatioDevice(const Vec3& A, const Vec3& B,
                                    const Vec3& C, const Vec3& D) {
    double minLen = 1e308;
    double maxLen = 0.0;
    // list of edges
    double dx, dy, dz, len;
    // AB
    dx = B.x - A.x; dy = B.y - A.y; dz = B.z - A.z;
    len = sqrt(dx*dx + dy*dy + dz*dz);
    minLen = fmin(minLen, len); maxLen = fmax(maxLen, len);
    // AC
    dx = C.x - A.x; dy = C.y - A.y; dz = C.z - A.z;
    len = sqrt(dx*dx + dy*dy + dz*dz);
    minLen = fmin(minLen, len); maxLen = fmax(maxLen, len);
    // AD
    dx = D.x - A.x; dy = D.y - A.y; dz = D.z - A.z;
    len = sqrt(dx*dx + dy*dy + dz*dz);
    minLen = fmin(minLen, len); maxLen = fmax(maxLen, len);
    // BC
    dx = C.x - B.x; dy = C.y - B.y; dz = C.z - B.z;
    len = sqrt(dx*dx + dy*dy + dz*dz);
    minLen = fmin(minLen, len); maxLen = fmax(maxLen, len);
    // BD
    dx = D.x - B.x; dy = D.y - B.y; dz = D.z - B.z;
    len = sqrt(dx*dx + dy*dy + dz*dz);
    minLen = fmin(minLen, len); maxLen = fmax(maxLen, len);
    // CD
    dx = D.x - C.x; dy = D.y - C.y; dz = D.z - C.z;
    len = sqrt(dx*dx + dy*dy + dz*dz);
    minLen = fmin(minLen, len); maxLen = fmax(maxLen, len);
    return maxLen / minLen;
}

// Kernel: one thread per tetrahedron
__global__ void computeAspectKernel(const Vec3* d_points,
                                    const int4* d_tets,
                                    double* d_ratios,
                                    size_t numTets) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < numTets) {
        int4 tet = d_tets[idx];
        Vec3 A = d_points[tet.x];
        Vec3 B = d_points[tet.y];
        Vec3 C = d_points[tet.z];
        Vec3 D = d_points[tet.w];
        d_ratios[idx] = aspectRatioDevice(A, B, C, D);
    }
}

int main() {
    // 1. Load mesh (host)
    std::ifstream nodeFile("diamond1.1.node");
    if (!nodeFile) { std::cerr<<"Cannot open diamond1.1.node\n"; return 1; }
    size_t numPoints; int dim, numAttr, numMarkers;
    nodeFile >> numPoints >> dim >> numAttr >> numMarkers;
    std::vector<Vec3> h_points(numPoints);
    for (size_t i = 0; i < numPoints; ++i) {
        int id; nodeFile >> id >> h_points[i].x >> h_points[i].y >> h_points[i].z;
    }
    nodeFile.close();

    std::ifstream eleFile("diamond1.1.ele");
    if (!eleFile) { std::cerr<<"Cannot open diamond1.1.ele\n"; return 1; }
    size_t numTets; int nodesPerTet, eleMarkers;
    eleFile >> numTets >> nodesPerTet >> eleMarkers;
    std::vector<std::array<int,4>> tets(numTets);
    for (size_t i = 0; i < numTets; ++i) {
        int id, n0, n1, n2, n3;
        eleFile >> id >> n0 >> n1 >> n2 >> n3;
        tets[i] = {{n0-1, n1-1, n2-1, n3-1}};
    }
    eleFile.close();

    // 2. Prepare device data
    Vec3* d_points;
    int4* d_tets;
    double* d_ratios;
    cudaMalloc(&d_points, numPoints * sizeof(Vec3));
    cudaMalloc(&d_tets,   numTets   * sizeof(int4));
    cudaMalloc(&d_ratios, numTets   * sizeof(double));

    cudaMemcpy(d_points, h_points.data(), numPoints * sizeof(Vec3), cudaMemcpyHostToDevice);
    // convert tets to int4
    std::vector<int4> h_tets_int4(numTets);
    for (size_t i = 0; i < numTets; ++i) {
        auto& tet = tets[i];
        h_tets_int4[i] = make_int4(tet[0], tet[1], tet[2], tet[3]);
    }
    cudaMemcpy(d_tets, h_tets_int4.data(), numTets * sizeof(int4), cudaMemcpyHostToDevice);

    // 3. Launch kernel & time with CUDA events
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    int threadsPerBlock = 256;
    int blocks = (numTets + threadsPerBlock - 1) / threadsPerBlock;

    cudaEventRecord(start);
    computeAspectKernel<<<blocks, threadsPerBlock>>>(d_points, d_tets, d_ratios, numTets);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float ms;
    cudaEventElapsedTime(&ms, start, stop);
    double elapsed_s = ms / 1000.0;

    // 4. Copy back and compute host-side stats
    std::vector<double> ratios(numTets);
    cudaMemcpy(ratios.data(), d_ratios, numTets * sizeof(double), cudaMemcpyDeviceToHost);

    double sum = 0.0;
    double minRatio = std::numeric_limits<double>::infinity();
    double maxRatio = 0.0;
    for (double r : ratios) {
        sum += r;
        minRatio = std::min(minRatio, r);
        maxRatio = std::max(maxRatio, r);
    }
    double avgRatio = sum / numTets;

    // 5. Print results
    std::cout << "GPU: Elapsed_s: " << elapsed_s << "\n";
    std::cout << "Min ratio: " << minRatio
              << ", Avg ratio: " << avgRatio
              << ", Max ratio: " << maxRatio << "\n";

    // 6. Write raw ratios for MATLAB
    std::ofstream fout("aspect_values.txt");
    for (double r : ratios) fout << r << "\n";
    fout.close();

    // 7. Cleanup
    cudaFree(d_points);
    cudaFree(d_tets);
    cudaFree(d_ratios);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    return 0;
}
