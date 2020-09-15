#pragma once
#include "../graphics/mesh.h"
#include "../graphics/materials.h"

namespace eio
{
	std::vector<std::shared_ptr<egx::Mesh>> LoadMeshFromOBJ(egx::Device& dev, egx::CommandContext& context, const std::string& obj_name, egx::MaterialManager& mat_manager);


	void ConvertOBJToOBJB(const std::string& obj_name);
	std::vector<std::shared_ptr<egx::Mesh>> LoadMeshFromOBJB(egx::Device& dev, egx::CommandContext& context, const std::string& obj_name, egx::MaterialManager& mat_manager);
}

/* Code for generating tangent and bitangent vectors
	 if (mesh.normals.size() > 0)
    {
        for (int i = 0; i < mesh.indices.size(); i += 3)
        {
            const glm::vec3& v0 = mesh.vertices[mesh.indices[i + 0]];
            const glm::vec3& v1 = mesh.vertices[mesh.indices[i + 1]];
            const glm::vec3& v2 = mesh.vertices[mesh.indices[i + 2]];

            const glm::vec2& uv0 = mesh.textureCoordinates[mesh.indices[i + 0]];
            const glm::vec2& uv1 = mesh.textureCoordinates[mesh.indices[i + 1]];
            const glm::vec2& uv2 = mesh.textureCoordinates[mesh.indices[i + 2]];

            glm::vec3 deltaPos1 = v1 - v0;
            glm::vec3 deltaPos2 = v2 - v0;

            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

            tangents[mesh.indices[i + 0]] = tangent;
            tangents[mesh.indices[i + 1]] = tangent;
            tangents[mesh.indices[i + 2]] = tangent;

            bitangents[mesh.indices[i + 0]] = bitangent;
            bitangents[mesh.indices[i + 1]] = bitangent;
            bitangents[mesh.indices[i + 2]] = bitangent;
        }
    }
*/