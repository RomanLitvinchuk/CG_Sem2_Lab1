#include "DX12App.h"
#include <filesystem>

void DX12App::ParseFile(const std::string& filename, const Matrix& transform, UINT instanceCount) {
	const aiScene* scene = aiImportFile(
		filename.c_str(),
		aiProcessPreset_TargetRealtime_MaxQuality |
		aiProcess_Triangulate | 
		aiProcess_CalcTangentSpace);

	if (!scene) {
		std::cout << "Error loading " << filename << std::endl;
		return;
	}

	std::cout << "Parsing: " << filename << " | Num Meshes: " << std::to_string(scene->mNumMeshes) << std::endl;

	int materialOffset = static_cast<int>(materialData.size());

	ParseNode(filename, scene->mRootNode, scene, transform, materialOffset, vertices, indices, instanceCount);
	std::cout << filename << " is parsed!" << std::endl;
}

void DX12App::ParseNode(const std::string& filename, aiNode* node, const aiScene* scene, const Matrix& transform, int materialOffset, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices, UINT instanceCount) {
	for (int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ParseMesh(filename, scene, mesh, transform, materialOffset, vertices, indices, instanceCount);
	}

	for (int i = 0; i < node->mNumChildren; i++) {
		ParseNode(filename, node->mChildren[i], scene, transform, materialOffset, vertices, indices, instanceCount);
	}
}

void DX12App::ParseMesh(const std::string& filename, const aiScene* scene, aiMesh* mesh, const Matrix& transform, int materialOffset, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices, UINT instanceCount) {
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

		if (mesh->mTangents) {
			vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
		}

		if (mesh->mBitangents) {
			vertex.biNormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
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
	submesh.name_ = filename;
	submesh.indexCount = mesh->mNumFaces * 3;
	submesh.startIndiceIndex = startIndex;
	submesh.startVerticeIndex = 0;
	submesh.materialIndex = mesh->mMaterialIndex + materialOffset;

	DirectX::BoundingBox::CreateFromPoints(
		submesh.box,
		mesh->mNumVertices,
		&vertices[baseVertex].pos,
		sizeof(Vertex)
	);


	if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < scene->mNumMaterials) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		ExtractMaterialData(filename, mesh->mMaterialIndex + materialOffset, material);
	}

	submesh.InstanceCount = instanceCount;
	int gridSize = static_cast<int>(std::ceil(std::sqrt(submesh.InstanceCount)));

	float spacing = 4.0f;

	for (int i = 0; i < submesh.InstanceCount; i++) {
		InstanceData instance;

		if (submesh.InstanceCount > 1) {
			int gridX = i % gridSize;
			int gridZ = i / gridSize;

			float offsetX = (gridX - gridSize / 2.0f) * spacing;
			float offsetZ = (gridZ - gridSize / 2.0f) * spacing;

			Matrix gridTranslation = Matrix::CreateTranslation(offsetX, 0.0f, offsetZ);

			instance.World_ = submesh.mWorld * gridTranslation;
		}
		else {
			instance.World_ = submesh.mWorld;
		}

		instance.TexTransform_ = submesh.mTexTransform;

		Matrix invWorld = instance.World_.Invert();
		instance.InvTWorld_ = invWorld.Transpose();

		submesh.instances.push_back(instance);
	}
	mSubmeshes.push_back(submesh);
}

void DX12App::ExtractMaterialData(const std::string& filename, int GlobalMaterialIndex, aiMaterial* material) {
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
		}
		if (wName.find(L"christmas tree color mm_0") != std::wstring::npos) {
			MatConst.isTree = 1;
		}

		if (mTextures.count(wName)) {
			MatConst.diffuseTextureIndex = mTextures[wName]->srvHeapIndex;
		}
		else {
			std::cout << "DO NOT FIND DIFFUSE TEXTURE" << std::endl;
		}
	}
	foundPath = false;
	aiString dispPath;

	if (material->GetTexture(aiTextureType_DISPLACEMENT, 0, &dispPath) == AI_SUCCESS) foundPath = true;
	else if (material->GetTexture(aiTextureType_HEIGHT, 0, &dispPath) == AI_SUCCESS) foundPath = true;

	if (foundPath) {
		std::filesystem::path p(dispPath.C_Str());
		std::wstring wName = p.stem().wstring();
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		std::string sName(wName.begin(), wName.end());
		std::cout << "   Looking for: " << sName << std::endl;

		if (mTextures.count(wName)) {
			if (filename.find("sponza") != std::string::npos) {
				MatConst.normalTextureIndex = mTextures[wName]->srvHeapIndex;
				MatConst.hasNormalTexture = 1;
			}
			else {
				MatConst.displacementTextureIndex = mTextures[wName]->srvHeapIndex;
				MatConst.hasDisplacementTexture = 1;
				
			}
		}
		else {
			std::cout << "DO NOT FIND DISP TEXTURE" << std::endl;
		}
	}

	foundPath = false;
	aiString normPath;
	if (material->GetTexture(aiTextureType_NORMALS, 0, &normPath) == AI_SUCCESS) foundPath = true;

	if (foundPath) {
		std::filesystem::path p(normPath.C_Str());
		std::wstring wName = p.stem().wstring();
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		std::string sName(wName.begin(), wName.end());
		std::cout << "   Looking for: " << sName << std::endl;

		if (mTextures.count(wName)) {
			MatConst.normalTextureIndex = mTextures[wName]->srvHeapIndex;
			MatConst.hasNormalTexture = 1;
		}
		else {
			std::cout << "DO NOT FIND NORMAL TEXTURE" << std::endl;
		}
	}

	if (filename.find("Sketchfab") != std::string::npos) {
		std::wstring wName = L"Stone_Pathway_Normal";
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		if (mTextures.count(wName)) {
			MatConst.normalTextureIndex = mTextures[wName]->srvHeapIndex;
			MatConst.hasNormalTexture = 1;
		}
		wName = L"Stone_Pathway_Height";
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		if (mTextures.count(wName)) {
			MatConst.displacementTextureIndex = mTextures[wName]->srvHeapIndex;
			MatConst.hasDisplacementTexture = 1;
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
	}
	if (material->Get(AI_MATKEY_OPACITY, parameter) == AI_SUCCESS) {
		MatConst.Opacity = parameter;
	}
	materialData[GlobalMaterialIndex] = MatConst;
}