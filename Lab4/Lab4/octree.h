#ifndef OCTREE_H_
#define OCTREE_H_
#include <DirectXCollision.h>
#include <vector>
#include <memory>
#include "submesh.h"

using namespace DirectX;

struct BVHNode {
    BoundingBox bounds;
    std::vector<UINT> submeshIndices;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
    bool isLeaf = true;
};

class BVH {
public:
    void Build(const std::vector<Submesh>& submeshes);
    void GetVisibleObjects(const XMVECTOR planes[6],
        std::vector<UINT>& outVisibleIndices) const;

private:
    std::unique_ptr<BVHNode> root;
    UINT MAX_OBJECTS_PER_NODE = 10;
    void BuildRecursive(BVHNode* node, const std::vector<Submesh>& submeshes, std::vector<UINT>& indices);
    void GetVisibleObjectsRecursive(const BVHNode* node,
        const XMVECTOR planes[6],
        std::vector<UINT>& outVisibleIndices) const;
};

#endif //OCTREE_H_