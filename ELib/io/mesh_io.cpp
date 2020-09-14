#include "mesh_io.h"
#include "../misc/string_helpers.h"
#include "console.h"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace
{
	struct FaceComponent
	{
		int position_index;	// Used to store index of position in obj vector
		int normal_index;	// Used to store index of normal in obj vector
		int tex_coord_index;// Used to store index of texture coordinate in obj vector
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
		std::vector<ema::vec2> tex_coords;
		std::vector<Face> faces;
	};

	std::vector<std::string> split(const std::string& s, char delimiter)
	{
		std::vector<std::string> out;
		std::istringstream ss(s);
		
		return out;
	}

	FaceComponent parseFaceComponent(const std::string& s)
	{
		FaceComponent out;

		// Get the indices
		std::istringstream ss(s);
		int indices[3] = { 0,0,0 };
		std::string line;
		int i = 0;
		while (std::getline(ss, line, '/'))
		{
			indices[i] = emisc::StringToInt(line) - 1;
			i++;
		}

		out.position_index = indices[0];
		out.tex_coord_index = indices[1];
		out.normal_index = indices[2];
		return out;
	}

	void loadMaterials(egx::MaterialManager& mat_manager, const std::string& file_name)
	{
		std::ifstream file(file_name);
		if (file.fail())
			throw std::runtime_error("Failed to load file " + file_name);

		std::string line;
		egx::Material current_material("");

		while (std::getline(file, line))
		{
			std::stringstream ss(line);
			std::string identifier;
			ss >> identifier;
			if (identifier == "newmtl") // Specular exponent
			{
				if (current_material.Name() != "")
				{
					mat_manager.AddMaterial(current_material);
				}
				std::string material_name;
				ss >> material_name;
				current_material = egx::Material(material_name);
			}
			else if (identifier == "Ns") // Specular exponent
			{
				float new_spec_exp;
				ss >> new_spec_exp;
				current_material.SetSpecularExponent(new_spec_exp);
			}
			else if (identifier == "Kd") // Diffuse color
			{
				ema::vec3 c;
				ss >> c.x >> c.y >> c.z;
				current_material.SetDiffuseColor(c);
			}
			else if (identifier == "map_Kd")
			{
				std::string diffuse_map_file_path;
				ss >> diffuse_map_file_path;
				current_material.SetDiffuseMapName(diffuse_map_file_path);
			}
			else
			{
				// Everything else is not supported
			}
		}
		mat_manager.AddMaterial(current_material);
	}

	void loadMeshFromOBJ(
		const std::string& obj_name, egx::MaterialManager& mat_manager,
		std::vector<std::vector<egx::MeshVertex>>& vertex_arrays,
		std::vector<std::vector<unsigned long>>& index_arrays
	)
	{
		// Load mesh
		eio::Console::Log(obj_name + ": Parsing mesh ");
		std::ifstream file(obj_name + ".obj");
		if (file.fail())
			throw std::runtime_error("Failed to load file " + obj_name + ".obj");

		// Loading data from file
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
			else if (identifier == "vt")
			{
				ema::vec2 tex_coord;
				ss >> tex_coord.x >> tex_coord.y;
				obj.tex_coords.push_back(tex_coord);
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
				current_material_index = mat_manager.GetMaterialIndex(material_name);
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

		// Splitting faces into triangles
		eio::Console::Log(obj_name + ": Triangulating faces");
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
					FaceComponent& comp2 = face.components[(long long)i + 1];
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

		// Creating vertices
		eio::Console::Log(obj_name + ": Creating vertices");
		std::vector<FaceComponent*> face_components;
		for (auto& face : obj.faces)
		{
			for (auto& comp : face.components)
			{
				face_components.push_back(&comp);
			}
		}
		std::sort(face_components.begin(), face_components.end(), compareFaceComponents);

		// Create meshes

		FaceComponent* last_component = nullptr;
		for (auto comp : face_components)
		{
			if (last_component == nullptr || !equalFaceComponents(*comp, *last_component))
			{
				// Create new vertex
				egx::MeshVertex vertex;
				vertex.position = obj.positions[comp->position_index];
				vertex.normal = obj.normals[comp->normal_index];
				vertex.tex_coord = obj.tex_coords[comp->tex_coord_index];
				vertex_arrays[comp->material_index].push_back(vertex);
			}
			// Update vertex index in component
			comp->vertex_index = (int)vertex_arrays[comp->material_index].size() - 1;
			last_component = comp;
		}

		eio::Console::Log(obj_name + ": Creating indices");
		for (Face& face : obj.faces)
		{
			for (FaceComponent& comp : face.components)
			{
				index_arrays[comp.material_index].push_back(comp.vertex_index);
			}
		}
	}
}



std::vector<egx::Mesh> eio::LoadMeshFromOBJ(egx::Device& dev, egx::CommandContext& context, const std::string& obj_name, egx::MaterialManager& mat_manager)
{
	eio::Console::SetColor(5);
	eio::Console::Log(obj_name + ": Loading file");

	// Load materials
	eio::Console::Log(obj_name + ": Parsing materials ");
	int material_index_start = mat_manager.MaterialCount();
	loadMaterials(mat_manager, obj_name + ".mtl");
	int num_materials = mat_manager.MaterialCount() - material_index_start;

	std::vector<std::vector<egx::MeshVertex>> vertex_arrays(num_materials);
	std::vector<std::vector<unsigned long>> index_arrays(num_materials);

	loadMeshFromOBJ(obj_name, mat_manager, vertex_arrays, index_arrays);

	// Create meshes
	Console::Log(obj_name + ": Creating meshes");
	std::vector<egx::Mesh> meshes;
	for (int i = 0; i < num_materials; i++)
	{
		if(index_arrays[i].size() > 0)
			meshes.push_back(egx::Mesh(dev, context, obj_name + emisc::ToString(i), vertex_arrays[i], index_arrays[i], i + material_index_start));
	}

	int vertex_count = 0;
	int index_count = 0;
	for (int i = 0; i < num_materials; i++)
	{
		vertex_count += (int)vertex_arrays[i].size();
		index_count += (int)index_arrays[i].size();
	}

	Console::Log(obj_name + ": Total vertices: " + emisc::ToString(vertex_count) + " total indices: " + emisc::ToString(index_count));

	Console::Log(obj_name + ": Load finished");
	Console::SetColor(15);

	return meshes;
}

/* 
	.objb format 
	(Number of meshes)
	(mesh1 vertex number)(mesh1 index number)(mesh1 material index)
	(mesh1 vertex data)
	(mesh1 index data)
	(mesh2 vertex number)...

*/
void eio::ConvertOBJToOBJB(const std::string& obj_name)
{
	// Load obj file
	// Load materials
	eio::Console::Log(obj_name + ": Parsing materials ");
	egx::MaterialManager mat_manager;
	loadMaterials(mat_manager, obj_name + ".mtl");
	int num_materials = mat_manager.MaterialCount();

	std::vector<std::vector<egx::MeshVertex>> vertex_arrays(num_materials);
	std::vector<std::vector<unsigned long>> index_arrays(num_materials);

	loadMeshFromOBJ(obj_name, mat_manager, vertex_arrays, index_arrays);

	std::ofstream file(obj_name + ".objb", std::ios::out | std::ios::binary);
	if (file.fail())
		throw std::runtime_error("Failed to open file " + obj_name + ".objb");
	
	int mesh_count = num_materials;
	file.write(reinterpret_cast<const char*>(&mesh_count), sizeof(int));

	for (int i = 0; i < mesh_count; i++)
	{
		int vertex_count = (int)vertex_arrays[i].size();
		int index_count = (int)index_arrays[i].size();
		int material_index = i;

		file.write(reinterpret_cast<const char*>(&vertex_count), sizeof(int));
		file.write(reinterpret_cast<const char*>(&index_count), sizeof(int));
		file.write(reinterpret_cast<const char*>(&material_index), sizeof(int));

		file.write(reinterpret_cast<const char*>(vertex_arrays[i].data()), sizeof(egx::MeshVertex) * vertex_count);
		file.write(reinterpret_cast<const char*>(index_arrays[i].data()), sizeof(unsigned long) * index_count);
	}
}

std::vector<egx::Mesh> eio::LoadMeshFromOBJB(egx::Device& dev, egx::CommandContext& context, const std::string& obj_name, egx::MaterialManager& mat_manager)
{
	Console::Log(obj_name + ": Loading");

	eio::Console::Log(obj_name + ": Parsing materials ");
	int material_start_index = mat_manager.MaterialCount();
	loadMaterials(mat_manager, obj_name + ".mtl");
	int num_materials = mat_manager.MaterialCount() - material_start_index;

	std::ifstream file(obj_name + ".objb", std::ios::in | std::ios::binary);
	if (file.fail())
		throw std::runtime_error("Failed to open file " + obj_name + ".objb");

	int mesh_count = 0;
	file.read(reinterpret_cast<char*>(&mesh_count), sizeof(int));

	std::vector<std::vector<egx::MeshVertex>> vertex_arrays(mesh_count);
	std::vector<std::vector<unsigned long>> index_arrays(mesh_count);

	Console::Log(obj_name + ": Reading data");
	for (int i = 0; i < mesh_count; i++)
	{
		int vertex_count = 0;
		int index_count = 0;
		int material_index = 0;

		file.read(reinterpret_cast<char*>(&vertex_count), sizeof(int));
		file.read(reinterpret_cast<char*>(&index_count), sizeof(int));
		file.read(reinterpret_cast<char*>(&material_index), sizeof(int));

		vertex_arrays[i].resize(vertex_count);
		index_arrays[i].resize(index_count);

		file.read(reinterpret_cast<char*>(vertex_arrays[i].data()), sizeof(egx::MeshVertex) * vertex_count);
		file.read(reinterpret_cast<char*>(index_arrays[i].data()), sizeof(unsigned long) * index_count);
	}

	// Create meshes
	Console::Log(obj_name + ": Creating meshes");
	std::vector<egx::Mesh> meshes;
	for (int i = 0; i < mesh_count; i++)
	{
		if (index_arrays[i].size() > 0)
			meshes.push_back(egx::Mesh(dev, context, obj_name + emisc::ToString(i), vertex_arrays[i], index_arrays[i], i + material_start_index));
	}

	int vertex_count = 0;
	int index_count = 0;
	for (int i = 0; i < mesh_count; i++)
	{
		vertex_count += (int)vertex_arrays[i].size();
		index_count += (int)index_arrays[i].size();
	}

	Console::Log(obj_name + ": Total vertices: " + emisc::ToString(vertex_count) + " total indices: " + emisc::ToString(index_count));

	Console::Log(obj_name + ": Load finished");
	Console::SetColor(15);

	return meshes;
}