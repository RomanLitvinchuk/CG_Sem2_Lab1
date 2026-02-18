#ifndef VERTEX_H_
#define VERTEX_H_
#include <SimpleMath.h>
#include <d3d12.h>
#include <fstream>
#include <sstream>



using namespace DirectX;
using namespace SimpleMath;

struct Vertex
{
    Vector3 pos;
    Vector3 normal;
    Vector2 uv;

    bool operator==(const Vertex& other) const
    {
        return pos.x == other.pos.x && pos.y == other.pos.y && pos.z == other.pos.z &&
            normal.x == other.normal.x && normal.y == other.normal.y && normal.z == other.normal.z &&
            uv.x == other.uv.x && uv.y == other.uv.y;
    }
};





namespace std {
    template<> struct hash<Vertex>
    {
        size_t operator()(const Vertex& v) const
        {
            return ((std::hash<float>()(v.pos.x) ^
                (std::hash<float>()(v.pos.y) << 1)) >> 1) ^
                (std::hash<float>()(v.pos.z) << 1);
        }
    };
}


class ObjLoader
{
public:

    const std::vector<Vertex>& GetVertices() const { return vertices; }
    const std::vector<UINT>& GetIndices() const { return indices; }

    bool Load(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "v")
            {
                float x, y, z;
                ss >> x >> y >> z;
                positions.push_back({ x, y, z });
            }
            else if (type == "vn")
            {
                float x, y, z;
                ss >> x >> y >> z;
                normals.push_back({ x, y, z });
            }
            else if (type == "vt")
            {
                float u, v;
                ss >> u >> v;
                uvs.push_back({ u, 1.0f - v }); 
            }
            else if (type == "f")
            {
                std::string vertStr;
                while (ss >> vertStr)
                {
                    std::istringstream vs(vertStr);
                    std::string idx[3];
                    int i = 0;
                    while (std::getline(vs, idx[i], '/')) i++;

                    int vi = std::stoi(idx[0]) - 1;
                    int uvi = idx[1].empty() ? 0 : std::stoi(idx[1]) - 1;
                    int ni = idx[2].empty() ? 0 : std::stoi(idx[2]) - 1;

                    Vertex vert;
                    vert.pos = positions[vi];
                    vert.uv = uvs.empty() ? Vector2(0, 0) : uvs[uvi];
                    vert.normal = normals.empty() ? Vector3(0, 1, 0) : normals[ni];

                    auto it = std::find(vertices.begin(), vertices.end(), vert);
                    if (it == vertices.end())
                    {
                        vertices.push_back(vert);
                        indices.push_back((UINT)vertices.size() - 1);
                    }
                    else
                    {
                        indices.push_back((UINT)(it - vertices.begin()));
                    }
                }
            }
        }
    }
private:
    UINT m_indexCount;
    std::vector<Vector3> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> uvs;
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;
public:
    UINT GetIndexCount() const { return m_indexCount; }
    void SetIndexCount(UINT ic) { m_indexCount = ic; }
};

#endif //VERTEX_H_
