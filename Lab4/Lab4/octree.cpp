#include "octree.h"
#include <algorithm>

void Octree::Build(const std::vector<Submesh>& submeshes, int maxDepth, int maxObjectsPerNode)
{
    if (submeshes.empty()) return;

    BoundingBox sceneBounds = submeshes[0].box;
    for (size_t i = 1; i < submeshes.size(); ++i) {
        BoundingBox::CreateMerged(sceneBounds, sceneBounds, submeshes[i].box);
    }

    root = std::make_unique<OctreeNode>();
    root->bounds = sceneBounds;

    std::vector<UINT> allIndices(submeshes.size());
    for (UINT i = 0; i < (UINT)submeshes.size(); ++i) {
        allIndices[i] = i;
    }

    BuildRecursive(root.get(), submeshes, allIndices, 0, maxDepth, maxObjectsPerNode);
}

void Octree::GetVisibleObjects(
    const XMVECTOR planes[6],
    const std::vector<Submesh>& submeshes,
    std::vector<uint32_t>& visibilityFences,
    uint32_t frameID,
    std::vector<UINT>& outVisibleIndices) const
{
    outVisibleIndices.clear();

    if (root == nullptr) return;

    GetVisibleObjectsRecursive(root.get(), planes, submeshes, visibilityFences, frameID, outVisibleIndices);

}

void Octree::BuildRecursive(OctreeNode* node, const std::vector<Submesh>& submeshes, const std::vector<UINT>& currentIndices, int depth, int maxDepth, int maxObjects)
{
    if (depth >= maxDepth || currentIndices.size() <= maxObjects)
    {
        node->isLeaf = true;
        node->submeshIndices = currentIndices;
        return;
    }

    node->isLeaf = false;

    XMFLOAT3 parentCenter = node->bounds.Center;
    XMFLOAT3 parentExtents = node->bounds.Extents;

    XMFLOAT3 childExtents(parentExtents.x * 0.5f, parentExtents.y * 0.5f, parentExtents.z * 0.5f);

    const float offsets[8][3] = {
        {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}
    };

    for (int i = 0; i < 8; ++i)
    {
        node->children[i] = std::make_unique<OctreeNode>();

        XMFLOAT3 childCenter(
            parentCenter.x + offsets[i][0] * childExtents.x,
            parentCenter.y + offsets[i][1] * childExtents.y,
            parentCenter.z + offsets[i][2] * childExtents.z
        );

        node->children[i]->bounds = BoundingBox(childCenter, childExtents);
    }

    for (UINT idx : currentIndices)
    {
        const BoundingBox& objBox = submeshes[idx].box;

        for (int i = 0; i < 8; ++i)
        {
            if (node->children[i]->bounds.Intersects(objBox))
            {
                node->children[i]->submeshIndices.push_back(idx);
            }
        }
    }

    for (int i = 0; i < 8; ++i)
    {
        BuildRecursive(node->children[i].get(), submeshes, node->children[i]->submeshIndices, depth + 1, maxDepth, maxObjects);
    }
}

void Octree::GetVisibleObjectsRecursive(
    const OctreeNode* node,
    const XMVECTOR planes[6],
    const std::vector<Submesh>& submeshes,
    std::vector<uint32_t>& visibilityFences,
    uint32_t frameID,
    std::vector<UINT>& outVisibleIndices) const
{
    bool fullyInside = true;
    bool isOutside = false;

    for (int i = 0; i < 6; ++i)
    {
        PlaneIntersectionType type = node->bounds.Intersects(planes[i]);

        if (type == PlaneIntersectionType::BACK)
        {
            isOutside = true;
            break;
        }
        else if (type == PlaneIntersectionType::INTERSECTING)
        {
            fullyInside = false;
        }
    }

    if (isOutside) return;

    if (fullyInside)
    {
        auto CollectAll = [&](auto& self, const OctreeNode* n) -> void {
            if (n->isLeaf) {
                for (UINT idx : n->submeshIndices) {
                    if (visibilityFences[idx] != frameID) {
                        outVisibleIndices.push_back(idx);
                        visibilityFences[idx] = frameID;
                    }
                }
            }
            else {
                for (int i = 0; i < 8; ++i) {
                    self(self, n->children[i].get());
                }
            }
            };
        CollectAll(CollectAll, node);
        return;
    }

    if (node->isLeaf)
    {
        for (UINT idx : node->submeshIndices)
        {
            if (visibilityFences[idx] == frameID) continue;

            const BoundingBox& objBox = submeshes[idx].box;
            bool objVisible = true;

            for (int i = 0; i < 6; ++i)
            {
                if (objBox.Intersects(planes[i]) == PlaneIntersectionType::BACK) {
                    objVisible = false;
                    break;
                }
            }

            if (objVisible)
            {
                outVisibleIndices.push_back(idx);
                visibilityFences[idx] = frameID;
            }
        }
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            GetVisibleObjectsRecursive(node->children[i].get(), planes, submeshes, visibilityFences, frameID, outVisibleIndices);
        }
    }
}


