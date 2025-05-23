#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <sstream>

// simple 3D point
struct Vec3 { double x,y,z; };

int main() {
    // --- 1. Load .node file ---
    std::ifstream nodeFile("diamond1.1.node");
    if(!nodeFile){ std::cerr<<"Cannot open diamond1.1.node\n"; return 1; }

    size_t numPoints;
    int dim, numAttr, numMarkers;
    nodeFile >> numPoints >> dim >> numAttr >> numMarkers;

    std::vector<Vec3> points;
    points.reserve(numPoints);
    for(size_t i=0; i<numPoints; ++i) {
        int id; Vec3 p;
        nodeFile >> id >> p.x >> p.y >> p.z;
        points.push_back(p);
    }
    nodeFile.close();
    std::cout<<"Loaded "<<points.size()<<" points\n";

    // --- 2. Load .ele file ---
    std::ifstream eleFile("diamond1.1.ele");
    if(!eleFile){ std::cerr<<"Cannot open diamond1.1.ele\n"; return 1; }

    size_t numTets;
    int nodesPerTet, eleMarkers;
    eleFile >> numTets >> nodesPerTet >> eleMarkers;

    std::vector<std::array<int,4>> tets;
    tets.reserve(numTets);
    for(size_t i=0; i<numTets; ++i) {
        int id, n0,n1,n2,n3;
        eleFile >> id >> n0 >> n1 >> n2 >> n3;
        // TetGen indices are 1-based—convert to 0-based
        tets.push_back({{n0-1, n1-1, n2-1, n3-1}});
    }
    eleFile.close();
    std::cout<<"Loaded "<<tets.size()<<" tetrahedra\n";

    return 0;
}
