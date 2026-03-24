#include "DX12App.h"
#include <filesystem>

void DX12App::ParseFile(const std::string& filename, const Matrix& transform) {
	const aiScene* scene = aiImportFile(
		filename.c_str(),
		aiProcessPreset_TargetRealtime_MaxQuality |
		aiProcess_Triangulate);

	if (!scene) {
		std::cout << "Error loading " << filename << std::endl;
		return;
	}

	std::cout << "Parsing: " << filename << " | Num Meshes: " << std::to_string(scene->mNumMeshes) << std::endl;

	int materialOffset = static_cast<int>(materialData.size());

	ParseNode(scene->mRootNode, scene, transform, materialOffset, vertices, indices);
	std::cout << filename << " is parsed!" << std::endl;
}

void DX12App::ParseNode(aiNode* node, const aiScene* scene, const Matrix& transform, int materialOffset, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices) {
	for (int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ParseMesh(scene, mesh, transform, materialOffset, vertices, indices);
	}

	for (int i = 0; i < node->mNumChildren; i++) {
		ParseNode(node->mChildren[i], scene, transform, materialOffset, vertices, indices);
	}
}

void DX12App::ParseMesh(const aiScene* scene, aiMesh* mesh, const Matrix& transform, int materialOffset, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices) {
	size_t vertexOffset = vertices.size();
	UINT baseVertex = static_cast<UINT>(vertices.size());
	UINT startIndex = static_cast<UINT>(indices.size());

	for (int i = 0; i < mesh->mNumVertices; ++i) {
		Vertex vertex;

		Vector3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		vertex.pos = Vector3::Transform(pos, transform);

		if (mesh->HasNormals()) {
			Vector3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			vertex.normal = Vector3::TransformNormal(normal, transform);
			vertex.normal.Normalize();
		}
		else {
			vertex.normal = Vector3(0.0f, 1.0f, 0.0f);
		}

		if (mesh->HasTextureCoords(0)) {
			vertex.uv.x = mesh->mTextureCoords[0][i].x;
			vertex.uv.y = 1.0f - mesh->mTextureCoords[0][i].y;
		}
		else {
			vertex.uv = Vector2(0.0f, 0.0f);
		}

		vertices.push_back(vertex);
	}

	for (int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j] + vertexOffset);
		}
	}

	MeshIndexCounts.push_back(mesh->mNumFaces * 3);
	Submesh submesh;
	submesh.indexCount = mesh->mNumFaces * 3;
	submesh.startIndiceIndex = startIndex;
	submesh.startVerticeIndex = 0;
	submesh.materialIndex = mesh->mMaterialIndex + materialOffset;

	if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < scene->mNumMaterials) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		ExtractMaterialData(mesh->mMaterialIndex + materialOffset, material);
	}

	mSubmeshes.push_back(submesh);
}

void DX12App::ExtractMaterialData(int GlobalMaterialIndex, aiMaterial* material) {
	if (materialData.size() <= GlobalMaterialIndex) {
		materialData.resize(GlobalMaterialIndex + 1);
		materialNames.resize(GlobalMaterialIndex + 1); 
	}

	MaterialConstants& MatConst = materialData[GlobalMaterialIndex];
	MatConst = {};
	MatConst.MatTransform = Matrix::Identity;

	aiColor3D color = { 1.0f, 1.0f, 1.0f };
	float parameter = 0.0f;

	if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
		MatConst.DiffuseColor = { color.r, color.g, color.b, 1.0f };
	}

	aiString texPath;
	bool foundPath = false;
	if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) foundPath = true;

	if (foundPath) {
		std::filesystem::path p(texPath.C_Str());
		std::wstring wName = p.stem().wstring();
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		std::string sName(wName.begin(), wName.end());
		std::cout << "   Looking for: " << sName << std::endl;

		if (wName.find(L"lion") != std::wstring::npos) {
			MatConst.isLion = 1;
			std::cout << "LION IS DETECTED" << std::endl;
		}
		if (wName.find(L"christmas tree color mm_0") != std::wstring::npos) {
			MatConst.isTree = 1;
			std::cout << "TREE DETECTED" << std::endl;
		}

		if (mTextures.count(wName)) {
			MatConst.diffuseTextureIndex = mTextures[wName]->srvHeapIndex;
		}
		else {
			std::cout << "   NOT FOUND in mTextures map!" << std::endl;
		}
	}
	foundPath = false;
	aiString normPath;
	if (material->GetTexture(aiTextureType_DISPLACEMENT, 0, &normPath) == AI_SUCCESS) foundPath = true;

	if (foundPath) {
		std::filesystem::path p(normPath.C_Str());
		std::wstring wName = p.stem().wstring();
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		std::string sName(wName.begin(), wName.end());
		std::cout << "   Looking for: " << sName << std::endl;

		if (mTextures.count(wName)) {
			MatConst.normalTextureIndex = mTextures[wName]->srvHeapIndex;
			MatConst.hasNormalTexture = 1;
			std::wcout << L"FOUND NORMAL MAP " << wName << std::endl;
		}
		else {
			std::cout << "   NOT FOUND in mTextures map!" << std::endl;
		}
	}
	else std::cout << "DON'T FIND NORMAL MAP" << std::endl;
	if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
		MatConst.AmbientColor = { color.r, color.g, color.b, 1.0f };
	}
	if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
		MatConst.SpecularColor = { color.r, color.g, color.b, 1.0f };
	}
	if (material->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
		MatConst.EmissiveColor = { color.r, color.g, color.b, 1.0f };
	}
	if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, color) == AI_SUCCESS) {
		MatConst.TransparentColor = { color.r, color.g, color.b, 1.0f };
	}
	if (material->Get(AI_MATKEY_SHININESS, parameter) == AI_SUCCESS) {
		MatConst.Shininess = parameter;
	}
	if (material->Get(AI_MATKEY_OPACITY, parameter) == AI_SUCCESS) {
		MatConst.Opacity = parameter;
	}
	materialData[GlobalMaterialIndex] = MatConst;
}