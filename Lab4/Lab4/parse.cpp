#include "DX12App.h"
#include <filesystem>

void DX12App::ParseFile() {
	sponza = aiImportFile(
		"sponza.obj",
		aiProcessPreset_TargetRealtime_MaxQuality |
		aiProcess_Triangulate);
	std::cout << "Num Meshes : " << std::to_string(sponza->mNumMeshes) << std::endl;
	ParseNode(sponza->mRootNode, sponza, vertices, indices);
	std::cout << "sponza.obj is parsed" << std::endl;
	//std::cout << "Num Vertices is:" << std::to_string(vertices.size()) << std::endl;
	//std::cout << "Num Faces is: " << std::to_string(sponza->mMeshes[0]->mNumFaces) << std::endl;
	//std::cout << "Num indices is:" << std::to_string(indices.size()) << std::endl;

}

void DX12App::ParseNode(aiNode* node, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices) {
	for (int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ParseMesh(scene, mesh, vertices, indices);
	}

	for (int i = 0; i < node->mNumChildren; i++) {
		ParseNode(node->mChildren[i], scene, vertices, indices);
	}
}

void DX12App::ParseMesh(const aiScene* scene, aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices) {
	size_t vertexOffset = vertices.size();

	UINT baseVertex = static_cast<UINT>(vertices.size());
	UINT startIndex = static_cast<UINT>(indices.size());

	for (int i = 0; i < mesh->mNumVertices; ++i) {
		Vertex vertex;

		vertex.pos.x = mesh->mVertices[i].x;
		vertex.pos.y = mesh->mVertices[i].y;
		vertex.pos.z = mesh->mVertices[i].z;

		if (mesh->HasNormals()) {
			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;
			//std::cout << vertex.normal.x << vertex.normal.y << vertex.normal.z << std::endl;
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
	submesh.materialIndex = mesh->mMaterialIndex;


	if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < scene->mNumMaterials) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		if (materialData.size() <= mesh->mMaterialIndex) {
			materialData.resize(mesh->mMaterialIndex + 1);
			materialNames.resize(mesh->mMaterialIndex + 1);
		}

		ExtractMaterialData(mesh->mMaterialIndex, material);
	}

	mSubmeshes.push_back(submesh);
}

void DX12App::ExtractMaterialData(int MaterialIndex, aiMaterial* material) {
	MaterialConstants& MatConst = materialData[MaterialIndex];
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
	else if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS) foundPath = true;

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
		if (mTextures.count(wName)) {
			MatConst.diffuseTextureIndex = mTextures[wName]->srvHeapIndex;
			MatConst.HasDiffuseTexture = 1;
		}
		else {
			std::cout << "   NOT FOUND in mTextures map!" << std::endl;
		}
	}

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
		//std::cout << "shininess is:" << std::to_string(parameter) << std::endl;
	}
	if (material->Get(AI_MATKEY_OPACITY, parameter) == AI_SUCCESS) {
		MatConst.Opacity = parameter;
		//std::cout << "opacity is:" << std::to_string(parameter) << std::endl;
	}

	if (materialData.size() <= MaterialIndex) {
		materialData.resize(MaterialIndex + 1);
	}

	materialData[MaterialIndex] = MatConst;
}