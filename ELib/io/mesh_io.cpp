#include "mesh_io.h"
#include "../misc/string_helpers.h"
#include "console.h"
#include <fstream>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace
{
	struct FaceComponent
	{
		int position_index;	// Used to store index of position in obj vector
		int normal_index;	// Used to store index of normal in obj vector
		int material_index;	// Used to store index of material in material vector
		int vertex_index;	// Used to store index of vertex when vertices has been created
	};
	bool equalFaceComponents(const FaceComponent& c1, const FaceComponent& c2)
	{
		return c1.position_index == c2.position_index &&
			c1.normal_index == c2.normal_index &&
			c1.material_index == c2.material_index;
	}
	bool compareFaceComponents(const FaceComponent* c1, const FaceComponent* c2)
	{
		if (c1->position_index < c2->position_index)
			return true;
		else if (c1->position_index == c2->position_index)
		{
			if (c1->normal_index < c2->normal_index)
				return true;
			else if (c1->normal_index == c2->normal_index)
			{
				if (c1->material_index < c2->material_index)
					return true;
			}
		}
		return false;
	}

	struct Face
	{
		std::vector<FaceComponent> components;
	};

	struct OBJIntermediate
	{
		std::vector<ema::vec3> positions;
		std::vector<ema::vec3> normals;
		std::vector<Face> faces;
	};

	enum class LineActions
	{
		ReadName, ReadPosition, ReadNormal, ReadFace, ReadMaterial
	};

	std::vector<std::string> split(const std::string& s, char delimiter)
	{
		std::vector<std::string> out;
		std::stringstream ss(s);
		const int max_line_length = 256;
		char line[max_line_length];
		while (ss.getline(line, max_line_length, delimiter))
		{
			out.push_back(std::string(line));
		}
		return out;
	}

	std::unordered_map<std::string, egx::Material> parse_materials(const std::string& mtl_name)
	{
		std::unordered_map<std::string, egx::Material> materials;
		std::string current_m_name;

		std::ifstream file(mtl_name);
		if (file.fail())
			throw std::runtime_error("Failed to load file " + mtl_name);

		std::string current_material_name;
		const int max_line_length = 256;
		char line[max_line_length];
		while (file.getline(line, max_line_length))
		{
			std::stringstream ss(line);
			std::string identifier;
			ss >> identifier;
			if (identifier == "newmtl") // Specular exponent
			{
				ss >> current_m_name;
				materials[current_m_name] = egx::Material();
			}
			else if (identifier == "Ns") // Specular exponent
			{
				ss >> materials[current_m_name].specular_exponent;
			}
			else if (identifier == "Kd") // Diffuse color
			{
				ss >> materials[current_m_name].diffuse_color.x;
				ss >> materials[current_m_name].diffuse_color.y;
				ss >> materials[current_m_name].diffuse_color.z;
			}
			else
			{
				// Everything else is not supported
			}
		}

		return materials;
	}

	FaceComponent parseFaceComponent(const std::string& s)
	{
		FaceComponent out;
		auto vals = split(s, '/');
		out.position_index = emisc::StringToInt(vals[0]) - 1;
		if (out.position_index == -1)
			int i = 0;
		out.normal_index = emisc::StringToInt(vals[2]) - 1;
		return out;
	}
}

egx::Mesh eio::LoadMeshFromOBJ(egx::Device& dev, egx::CommandContext& context, const std::string& obj_name, const std::string& mtl_name)
{
	Console::SetColor(5);
	Console::Log(obj_name + ": Loading file");
	Console::Log(obj_name + ": Parsing materials ");

	// Initalize materials for indexing
	auto materials = parse_materials(mtl_name);
	std::unordered_map<std::string, int> material_index_map;
	std::vector<egx::Material> material_vector;
	for (auto m : materials)
	{
		material_vector.push_back(m.second);
		material_index_map[m.first] = (int)material_vector.size() - 1;
	}

	Console::Log(obj_name + ": Parsing mesh ");
	std::ifstream file(obj_name);
	if (file.fail())
		throw std::runtime_error("Failed to load file " + mtl_name);

	OBJIntermediate obj;
	int current_material_index = 0;

	std::string line;
	while (std::getline(file, line))
	{
		std::stringstream ss(line);
		std::string identifier;
		ss >> identifier;

		// Identifiers: v, vn, f, usemtl
		if (identifier == "v") // Vertex poisition
		{
			ema::vec3 pos;
			ss >> pos.x >> pos.y >> pos.z;
			obj.positions.push_back(pos);
		}
		else if (identifier == "vn")
		{
			ema::vec3 normal;
			ss >> normal.x >> normal.y >> normal.z;
			obj.normals.push_back(normal);
		}
		else if (identifier == "usemtl")
		{
			std::string material_name;
			ss >> material_name;
			assert(material_index_map.find(material_name) != material_index_map.end());
			current_material_index = material_index_map[material_name];
		}
		else if (identifier == "f")
		{
			Face face;
			while (ss >> identifier)
			{
				auto face_comp = parseFaceComponent(identifier);
				face_comp.material_index = current_material_index;
				face.components.push_back(face_comp);
			}
			obj.faces.push_back(face);
		}
		else
		{
			// Not supported
		}
	}

	Console::Log(obj_name + ": Triangulating faces");
	std::vector<Face> triangle_faces;
	for (auto& face : obj.faces)
	{
		if (face.components.size() > 3)
		{
			FaceComponent& main_comp = face.components[0];
			
			for (int i = 1; i < (int)face.components.size() - 1; i++)
			{
				Face trig_face;
				FaceComponent& comp1 = face.components[i];
				FaceComponent& comp2 = face.components[(long long)i+1];
				trig_face.components.push_back(main_comp);
				trig_face.components.push_back(comp1);
				trig_face.components.push_back(comp2);
				triangle_faces.push_back(trig_face);
			}
		}
		else
		{
			triangle_faces.push_back(face);
		}
	}

	obj.faces = triangle_faces;

	Console::Log(obj_name + ": Creating vertices");
	std::vector<FaceComponent*> face_components;
	for (auto& face : obj.faces)
	{
		for (auto& comp : face.components)
		{
			face_components.push_back(&comp);
		}
	}
	std::sort(face_components.begin(), face_components.end(), compareFaceComponents);

	std::vector<egx::MeshVertex> vertices;
	FaceComponent* last_component = nullptr;
	for (auto comp : face_components)
	{
		if (last_component == nullptr || !equalFaceComponents(*comp, *last_component))
		{
			// Create new vertex
			egx::MeshVertex vertex;
			vertex.position = obj.positions[comp->position_index];
			vertex.normal = obj.normals[comp->normal_index];
			vertex.color = material_vector[comp->material_index].diffuse_color;
			vertex.specular_exp = material_vector[comp->material_index].specular_exponent;
			vertices.push_back(vertex);
		}
		// Update vertex index in component
		comp->vertex_index = (int)vertices.size() - 1;
		last_component = comp;
	}

	Console::Log(obj_name + ": Creating indices");
	std::vector<unsigned long> indices;
	for (Face& face : obj.faces)
	{
		for (FaceComponent& comp : face.components)
		{
			indices.push_back(comp.vertex_index);
		}
	}

	Console::Log(obj_name + ": Total vertices: " + emisc::ToString(vertices.size()) + " total indices: " + emisc::ToString(indices.size()));

	Console::Log(obj_name + ": Load finished");
	Console::SetColor(15);

	return egx::Mesh(dev, context, obj_name, vertices, indices);
}