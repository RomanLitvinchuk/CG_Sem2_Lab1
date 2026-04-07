#ifndef OCTREE_H_
#define OCTREE_H_
#include <DirectXCollision.h>
#include <vector>
#include <memory>
#include "submesh.h"

using namespace DirectX;

struct OctreeNode {
    BoundingBox bounds;
    std::vector<UINT> submeshIndices;
    std::unique_ptr<OctreeNode> children[8];
    bool isLeaf = true;
};

class Octree {
public:
    void Build(const std::vector<Submesh>& submeshes, int maxDepth, int maxObjectsPerNode);
    void GetVisibleObjects(const XMVECTOR planes[6],
        const std::vector<Submesh>& submeshes,
        std::vector<uint32_t>& visibilityFences,
        uint32_t frameID,
        std::vector<UINT>& outVisibleIndices) const;

private:
    std::unique_ptr<OctreeNode> root;

    void BuildRecursive(OctreeNode* node, const std::vector<Submesh>& submeshes, const std::vector<UINT>& currentIndices, int depth, int maxDepth, int maxObjects);
    void GetVisibleObjectsRecursive(const OctreeNode* node,
        const XMVECTOR planes[6],
        const std::vector<Submesh>& submeshes,
        std::vector<uint32_t>& visibilityFences,
        uint32_t frameID,
        std::vector<UINT>& outVisibleIndices) const;
};

#endif //OCTREE_H_