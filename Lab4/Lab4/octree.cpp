#include "octree.h"
#include <algorithm>

void BVH::Build(const std::vector<Submesh>& submeshes)
{
    if (submeshes.empty()) return;

    BoundingBox sceneBounds = submeshes[0].box;
    for (size_t i = 1; i < submeshes.size(); ++i) {
        BoundingBox::CreateMerged(sceneBounds, sceneBounds, submeshes[i].box);
    }

    root = std::make_unique<BVHNode>();
    root->bounds = sceneBounds;

    std::vector<UINT> allIndices(submeshes.size());
    for (UINT i = 0; i < (UINT)submeshes.size(); ++i) {
        allIndices[i] = i;
    }

    BuildRecursive(root.get(), submeshes, allIndices);
}

void BVH::GetVisibleObjects(
    const XMVECTOR planes[6],
    const std::vector<Submesh>& submeshes,
    std::vector<UINT>& outVisibleIndices) const
{
    outVisibleIndices.clear();
    if (root == nullptr) return;
    GetVisibleObjectsRecursive(root.get(), planes, submeshes, outVisibleIndices);
}

void BVH::GetAllNodes(std::vector<BVHNode*>& outNodes)
{
    outNodes.clear();
    if (root != nullptr) {
        CollectAllNodesRecursive(root.get(), outNodes);
    }
}

void BVH::CollectAllNodesRecursive(BVHNode* node, std::vector<BVHNode*>& outNodes) {
    if (node == nullptr) return;

    outNodes.push_back(node);
    if (!node->isLeaf) {
        CollectAllNodesRecursive(node->left.get(), outNodes);
        CollectAllNodesRecursive(node->right.get(), outNodes); 
    }
}

void BVH::BuildRecursive(BVHNode* node, const std::vector<Submesh>& submeshes, std::vector<UINT>& indices)
{
    BoundingBox actualBounds;
    if (!indices.empty()) {
        actualBounds = submeshes[indices[0]].box;
        for (size_t i = 1; i < indices.size(); ++i) {
            BoundingBox::CreateMerged(actualBounds, actualBounds, submeshes[indices[i]].box);
        }
    }
    node->bounds = actualBounds;

    if (indices.size() <= MAX_OBJECTS_PER_NODE) {
        node->isLeaf = true;
        node->submeshIndices = std::move(indices);
        return;
    }

    node->isLeaf = false;

    XMFLOAT3 size = actualBounds.Extents;
    int axis = 0;
    if (size.y > size.x) axis = 1;
    if (size.z > (axis == 0 ? size.x : size.y)) axis = 2;

    std::sort(indices.begin(), indices.end(), [&](UINT a, UINT b) {
        float centerA, centerB;
        if (axis == 0) { centerA = submeshes[a].box.Center.x; centerB = submeshes[b].box.Center.x; }
        else if (axis == 1) { centerA = submeshes[a].box.Center.y; centerB = submeshes[b].box.Center.y; }
        else { centerA = submeshes[a].box.Center.z; centerB = submeshes[b].box.Center.z; }
        return centerA < centerB;
        });

    size_t mid = indices.size() / 2;
    std::vector<UINT> leftIndices(indices.begin(), indices.begin() + mid);
    std::vector<UINT> rightIndices(indices.begin() + mid, indices.end());

    node->left = std::make_unique<BVHNode>();
    node->right = std::make_unique<BVHNode>();

    BuildRecursive(node->left.get(), submeshes, leftIndices);
    BuildRecursive(node->right.get(), submeshes, rightIndices);
}

void BVH::GetVisibleObjectsRecursive(
    const BVHNode* node,
    const XMVECTOR planes[6],
    const std::vector<Submesh>& submeshes,
    std::vector<UINT>& outVisibleIndices) const
{
    bool fullyInside = true;

    for (int i = 0; i < 6; ++i) {
        PlaneIntersectionType type = node->bounds.Intersects(planes[i]);
        if (type == PlaneIntersectionType::BACK) {
            return;
        }
        if (type == PlaneIntersectionType::INTERSECTING) {
            fullyInside = false;
        }
    }

    if (fullyInside) {
        auto CollectAll = [&](auto& self, const BVHNode* n) -> void {
            if (n->isLeaf) {
                for (UINT idx : n->submeshIndices) {
                    outVisibleIndices.push_back(idx);
                }
            }
            else {
                if (n->left) self(self, n->left.get());
                if (n->right) self(self, n->right.get());
            }
            };
        CollectAll(CollectAll, node);
        return;
    }

    if (node->isLeaf) {
        for (UINT idx : node->submeshIndices) {
            const BoundingBox& objBox = submeshes[idx].box;
            bool objVisible = true;

            for (int i = 0; i < 6; ++i) {
                if (objBox.Intersects(planes[i]) == PlaneIntersectionType::BACK) {
                    objVisible = false;
                    break;
                }
            }

            if (objVisible) {
                outVisibleIndices.push_back(idx);
            }
        }
    }

    else {
        if (node->left) GetVisibleObjectsRecursive(node->left.get(), planes, submeshes, outVisibleIndices);
        if (node->right) GetVisibleObjectsRecursive(node->right.get(), planes, submeshes, outVisibleIndices);
    }
}


