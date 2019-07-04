#include "Model.h"

#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iterator>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <array>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <filesystem>
#include <globjects/globjects.h>
#include <globjects/logging.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace minity;
using namespace gl;
using namespace glm;
using namespace globjects;

#ifdef SEPP

// Print progress to console while loading (large models)
#define PRINT_PROGRESS

// A test to see if P1 is on the same side as P2 of a line segment ab
bool sameSide(vec3 p1, vec3 p2, vec3 a, vec3 b)
{
	vec3 cp1 = cross(b - a, p1 - a);
	vec3 cp2 = cross(b - a, p2 - a);

	if (dot(cp1, cp2) >= 0)
		return true;
	else
		return false;
}

// Generate a cross produect normal for a triangle
vec3 triangleNormal(vec3 t1, vec3 t2, vec3 t3)
{
	vec3 u = t2 - t1;
	vec3 v = t3 - t1;

	vec3 normal = cross(u, v);

	return normal;
}

// Check to see if a vec3 Point is within a 3 vec3 Triangle
bool inTriangle(vec3 point, vec3 tri1, vec3 tri2, vec3 tri3)
{
	// Test to see if it is within an infinite prism that the triangle outlines.
	bool within_tri_prisim = sameSide(point, tri1, tri2, tri3) && sameSide(point, tri2, tri1, tri3)
		&& sameSide(point, tri3, tri1, tri2);

	// If it isn't it will never be on the triangle
	if (!within_tri_prisim)
		return false;

	// Calulate Triangle's Normal
	vec3 n = triangleNormal(tri1, tri2, tri3);

	vec3 nn = normalize(n);
	vec3 proj = nn * dot(point, nn);

	// If the distance from the triangle to the point is 0
	//	it lies on the triangle
	if (length(proj) == 0)
		return true;
	else
		return false;
}

// Split a String into a string array at a given token
void split(const std::string &in, std::vector<std::string> &out,	std::string token)
{
	out.clear();

	std::string temp;

	for (int i = 0; i < int(in.size()); i++)
	{
		std::string test = in.substr(i, token.size());

		if (test == token)
		{
			if (!temp.empty())
			{
				out.push_back(temp);
				temp.clear();
				i += (int)token.size() - 1;
			}
			else
			{
				out.push_back("");
			}
		}
		else if (i + token.size() >= in.size())
		{
			temp += in.substr(i, token.size());
			out.push_back(temp);
			break;
		}
		else
		{
			temp += in[i];
		}
	}
}

// Get tail of string after first token and possibly following spaces
std::string tail(const std::string &in)
{
	size_t token_start = in.find_first_not_of(" \t");
	size_t space_start = in.find_first_of(" \t", token_start);
	size_t tail_start = in.find_first_not_of(" \t", space_start);
	size_t tail_end = in.find_last_not_of(" \t");
	if (tail_start != std::string::npos && tail_end != std::string::npos)
	{
		return in.substr(tail_start, tail_end - tail_start + 1);
	}
	else if (tail_start != std::string::npos)
	{
		return in.substr(tail_start);
	}
	return "";
}

// Get first token of string
std::string firstToken(const std::string &in)
{
	if (!in.empty())
	{
		size_t token_start = in.find_first_not_of(" \t");
		size_t token_end = in.find_first_of(" \t", token_start);
		if (token_start != std::string::npos && token_end != std::string::npos)
		{
			return in.substr(token_start, token_end - token_start);
		}
		else if (token_start != std::string::npos)
		{
			return in.substr(token_start);
		}
	}
	return "";
}

// Get element at given index position
template <class T>
inline const T & getElement(const std::vector<T> &elements, std::string &index)
{
	int idx = std::stoi(index);
	if (idx < 0)
		idx = int(elements.size()) + idx;
	else
		idx--;
	return elements[idx];
}

class ObjLoader
{
public:
	// Default Constructor
	ObjLoader()
	{

	}

	ObjLoader(const std::string & path)
	{
		load(path);
	}

	// Load a file into the ObjLoader
	//
	// If file is loaded return true
	//
	// If the file is unable to be found
	// or unable to be loaded return false
	bool load(const std::string & path)
	{
		// If the file is not an .obj file return false
		if (path.substr(path.size() - 4, 4) != ".obj")
			return false;

		std::ifstream file(path);

		if (!file.is_open())
			return false;

		m_meshes.clear();
		m_vertices.clear();
		m_indices.clear();

		std::vector<vec3> Positions;
		std::vector<vec2> TCoords;
		std::vector<vec3> Normals;

		std::vector<Vertex> Vertices;
		std::vector<uint> Indices;

		std::vector<std::string> MeshMatNames;

		bool listening = false;
		std::string name;

		Mesh tempMesh;

#ifdef PRINT_PROGRESS
		const unsigned int outputEveryNth = 1000;
		unsigned int outputIndicator = outputEveryNth;
#endif

		std::string curline;
		while (std::getline(file, curline))
		{
#ifdef PRINT_PROGRESS
			if ((outputIndicator = ((outputIndicator + 1) % outputEveryNth)) == 1)
			{
				if (!name.empty())
				{
					std::cout
						<< "\r- " << name
						<< "\t| vertices > " << Positions.size()
						<< "\t| texcoords > " << TCoords.size()
						<< "\t| normals > " << Normals.size()
						<< "\t| triangles > " << (Vertices.size() / 3)
						<< (!MeshMatNames.empty() ? "\t| ObjMaterial: " + MeshMatNames.back() : "");
				}
			}
#endif

			// Generate a Mesh Object or Prepare for an object to be created
			if (firstToken(curline) == "o" || firstToken(curline) == "g" || curline[0] == 'g')
			{
				if (!listening)
				{
					listening = true;

					if (firstToken(curline) == "o" || firstToken(curline) == "g")
					{
						name = tail(curline);
					}
					else
					{
						name = "unnamed";
					}
				}
				else
				{
					// Generate the mesh to put into the array

					if (!Indices.empty() && !Vertices.empty())
					{
						// Create Mesh
						tempMesh = Mesh(Vertices, Indices);
						tempMesh.name = name;

						// Insert Mesh
						m_meshes.push_back(tempMesh);

						// Cleanup
						Vertices.clear();
						Indices.clear();
						name.clear();

						name = tail(curline);
					}
					else
					{
						if (firstToken(curline) == "o" || firstToken(curline) == "g")
						{
							name = tail(curline);
						}
						else
						{
							name = "unnamed";
						}
					}
				}
#ifdef PRINT_PROGRESS
				std::cout << std::endl;
				outputIndicator = 0;
#endif
			}
			// Generate a Vertex Position
			if (firstToken(curline) == "v")
			{
				std::vector<std::string> spos;
				vec3 vpos;
				split(tail(curline), spos, " ");

				vpos.x = std::stof(spos[0]);
				vpos.y = std::stof(spos[1]);
				vpos.z = std::stof(spos[2]);

				Positions.push_back(vpos);
			}
			// Generate a Vertex Texture Coordinate
			if (firstToken(curline) == "vt")
			{
				std::vector<std::string> stex;
				vec2 vtex;
				split(tail(curline), stex, " ");

				vtex.x = std::stof(stex[0]);
				vtex.y = std::stof(stex[1]);

				TCoords.push_back(vtex);
			}
			// Generate a Vertex Normal;
			if (firstToken(curline) == "vn")
			{
				std::vector<std::string> snor;
				vec3 vnor;
				split(tail(curline), snor, " ");

				vnor.x = std::stof(snor[0]);
				vnor.y = std::stof(snor[1]);
				vnor.z = std::stof(snor[2]);

				Normals.push_back(vnor);
			}
			// Generate a Face (vertices & indices)
			if (firstToken(curline) == "f")
			{
				// Generate the vertices
				std::vector<Vertex> vVerts;
				generateVertices(vVerts, Positions, TCoords, Normals, curline);

				// Add Vertices
				for (int i = 0; i < int(vVerts.size()); i++)
				{
					Vertices.push_back(vVerts[i]);

					m_vertices.push_back(vVerts[i]);
				}

				std::vector<unsigned int> iIndices;

				triangulateVertices(iIndices, vVerts);

				// Add Indices
				for (int i = 0; i < int(iIndices.size()); i++)
				{
					unsigned int indnum = (unsigned int)((Vertices.size()) - vVerts.size()) + iIndices[i];
					Indices.push_back(indnum);

					indnum = (unsigned int)((m_vertices.size()) - vVerts.size()) + iIndices[i];
					m_indices.push_back(indnum);

				}
			}
			// Get Mesh ObjMaterial Name
			if (firstToken(curline) == "usemtl")
			{
				MeshMatNames.push_back(tail(curline));

				// Create new Mesh, if ObjMaterial changes within a ObjGroup
				if (!Indices.empty() && !Vertices.empty())
				{
					// Create Mesh
					tempMesh = Mesh(Vertices, Indices);
					tempMesh.name = name;
					int i = 2;
					while (1)
					{
						tempMesh.name = name + "_" + std::to_string(i);

						for (auto &m : m_meshes)
							if (m.name == tempMesh.name)
								continue;
						break;
					}

					// Insert Mesh
					m_meshes.push_back(tempMesh);

					// Cleanup
					Vertices.clear();
					Indices.clear();
				}

#ifdef PRINT_PROGRESS
				outputIndicator = 0;
#endif
			}
			// Load ObjMaterials
			if (firstToken(curline) == "mtllib")
			{
				// Generate LoadedObjMaterial

				// Generate a path to the ObjMaterial file
				std::vector<std::string> temp;
				split(path, temp, "/");

				std::string pathtomat = "";

				if (temp.size() != 1)
				{
					for (int i = 0; i < temp.size() - 1; i++)
					{
						pathtomat += temp[i] + "/";
					}
				}


				pathtomat += tail(curline);

#ifdef PRINT_PROGRESS
				std::cout << std::endl << "- find ObjMaterials in: " << pathtomat << std::endl;
#endif

				// Load ObjMaterials
				loadObjMaterials(pathtomat);
			}
		}

#ifdef PRINT_PROGRESS
		std::cout << std::endl;
#endif

		// Deal with last mesh

		if (!Indices.empty() && !Vertices.empty())
		{
			// Create Mesh
			tempMesh = Mesh(Vertices, Indices);
			tempMesh.name = name;

			// Insert Mesh
			m_meshes.push_back(tempMesh);
		}

		file.close();

		// Set ObjMaterials for each Mesh
		for (int i = 0; i < MeshMatNames.size(); i++)
		{
			std::string matname = MeshMatNames[i];

			// Find corresponding ObjMaterial name in loaded ObjMaterials
			// when found copy ObjMaterial variables into mesh ObjMaterial
			for (int j = 0; j < m_ObjMaterials.size(); j++)
			{
				if (m_ObjMaterials[j].name == matname)
				{
					m_meshes[i].ObjMaterial = m_ObjMaterials[j];
					break;
				}
			}
		}

		if (Normals.empty())
		{
			// SB: compute normals
			std::vector<vec3> faceNormals;

			for (int i = 0; i < m_indices.size()-3; i += 3)
			{
				vec3 A = m_vertices[m_indices[i+0]].position - m_vertices[m_indices[i+1]].position;
				vec3 B = m_vertices[m_indices[i+2]].position - m_vertices[m_indices[i+1]].position;

				vec3 normal = normalize(cross(A, B));

				m_vertices[m_indices[i + 0]].normal += normal;
				m_vertices[m_indices[i + 1]].normal += normal;
				m_vertices[m_indices[i + 2]].normal += normal;
			}

			for (auto & v : m_vertices)
			{
				v.normal = normalize(v.normal);
			}
		}

		if (m_meshes.empty() && m_vertices.empty() && m_indices.empty())
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	const std::vector<Mesh> & meshes() const
	{
		return m_meshes;
	}

	const std::vector<Vertex> & vertices() const
	{
		return m_vertices;
	}

	const std::vector<uint> & indices() const
	{
		return m_indices;
	}

	const std::vector<ObjMaterial> & ObjMaterials() const
	{
		return m_ObjMaterials;
	}


private:
	
	// Generate vertices from a list of positions, tcoords, normals and a face line
	void generateVertices(std::vector<Vertex>& oVerts, const std::vector<vec3>& iPositions, const std::vector<vec2>& iTCoords, const std::vector<vec3>& iNormals, std::string icurline)
	{
		std::vector<std::string> sface, svert;
		Vertex vVert;
		split(tail(icurline), sface, " ");

		bool noNormal = false;

		// For every given vertex do this
		for (int i = 0; i < int(sface.size()); i++)
		{
			// See What type the vertex is.
			int vtype;

			split(sface[i], svert, "/");

			// Check for just position - v1
			if (svert.size() == 1)
			{
				// Only position
				vtype = 1;
			}

			// Check for position & texture - v1/vt1
			if (svert.size() == 2)
			{
				// Position & Texture
				vtype = 2;
			}

			// Check for Position, Texture and Normal - v1/vt1/vn1
			// or if Position and Normal - v1//vn1
			if (svert.size() == 3)
			{
				if (svert[1] != "")
				{
					// Position, Texture, and Normal
					vtype = 4;
				}
				else
				{
					// Position & Normal
					vtype = 3;
				}
			}

			// Calculate and store the vertex
			switch (vtype)
			{
			case 1: // P
			{
				vVert.position = getElement(iPositions, svert[0]);
				vVert.texCoord = vec2(0, 0);
				
				// SB
				vVert.normal = vec3(0);

				noNormal = true;
				oVerts.push_back(vVert);
				break;
			}
			case 2: // P/T
			{
				vVert.position = getElement(iPositions, svert[0]);
				vVert.texCoord = getElement(iTCoords, svert[1]);

				// SB
				vVert.normal = vec3(0);

				noNormal = true;
				oVerts.push_back(vVert);
				break;
			}
			case 3: // P//N
			{
				vVert.position = getElement(iPositions, svert[0]);
				vVert.texCoord = vec2(0, 0);
				vVert.normal = getElement(iNormals, svert[2]);
				oVerts.push_back(vVert);
				break;
			}
			case 4: // P/T/N
			{
				vVert.position = getElement(iPositions, svert[0]);
				vVert.texCoord = getElement(iTCoords, svert[1]);
				vVert.normal = getElement(iNormals, svert[2]);
				oVerts.push_back(vVert);
				break;
			}
			default:
			{
				break;
			}
			}
		}

		// take care of missing normals
		// these may not be truly acurate but it is the 
		// best they get for not compiling a mesh with normals	
		/*
		if (noNormal)
		{
			vec3 A = oVerts[0].position - oVerts[1].position;
			vec3 B = oVerts[2].position - oVerts[1].position;

			vec3 normal = normalize(cross(A, B));

			for (int i = 0; i < int(oVerts.size()); i++)
			{
				oVerts[i].normal = normal;
			}
		}*/
	}

	// Triangulate a list of vertices into a face by printing indices corresponding with triangles within it
	void triangulateVertices(std::vector<unsigned int>& oIndices, const std::vector<Vertex>& iVerts)
	{
		// If there are 2 or less verts,
		// no triangle can be created,
		// so exit
		if (iVerts.size() < 3)
		{
			return;
		}
		// If it is a triangle no need to calculate it
		if (iVerts.size() == 3)
		{
			oIndices.push_back(0);
			oIndices.push_back(1);
			oIndices.push_back(2);
			return;
		}

		// Create a list of vertices
		std::vector<Vertex> tVerts = iVerts;

		while (true)
		{
			// For every vertex
			for (int i = 0; i < int(tVerts.size()); i++)
			{
				// pPrev = the previous vertex in the list
				Vertex pPrev;
				if (i == 0)
				{
					pPrev = tVerts[tVerts.size() - 1];
				}
				else
				{
					pPrev = tVerts[i - 1];
				}

				// pCur = the current vertex;
				Vertex pCur = tVerts[i];

				// pNext = the next vertex in the list
				Vertex pNext;
				if (i == tVerts.size() - 1)
				{
					pNext = tVerts[0];
				}
				else
				{
					pNext = tVerts[i + 1];
				}

				// Check to see if there are only 3 verts left
				// if so this is the last triangle
				if (tVerts.size() == 3)
				{
					// Create a triangle from pCur, pPrev, pNext
					for (int j = 0; j < int(tVerts.size()); j++)
					{
						if (iVerts[j].position == pCur.position)
							oIndices.push_back(j);
						if (iVerts[j].position == pPrev.position)
							oIndices.push_back(j);
						if (iVerts[j].position == pNext.position)
							oIndices.push_back(j);
					}

					tVerts.clear();
					break;
				}
				if (tVerts.size() == 4)
				{
					// Create a triangle from pCur, pPrev, pNext
					for (int j = 0; j < int(iVerts.size()); j++)
					{
						if (iVerts[j].position == pCur.position)
							oIndices.push_back(j);
						if (iVerts[j].position == pPrev.position)
							oIndices.push_back(j);
						if (iVerts[j].position == pNext.position)
							oIndices.push_back(j);
					}

					vec3 tempVec;
					for (int j = 0; j < int(tVerts.size()); j++)
					{
						if (tVerts[j].position != pCur.position
							&& tVerts[j].position != pPrev.position
							&& tVerts[j].position != pNext.position)
						{
							tempVec = tVerts[j].position;
							break;
						}
					}

					// Create a triangle from pCur, pPrev, pNext
					for (int j = 0; j < int(iVerts.size()); j++)
					{
						if (iVerts[j].position == pPrev.position)
							oIndices.push_back(j);
						if (iVerts[j].position == pNext.position)
							oIndices.push_back(j);
						if (iVerts[j].position == tempVec)
							oIndices.push_back(j);
					}

					tVerts.clear();
					break;
				}

				// If Vertex is not an interior vertex
				float angle = dot(pPrev.position - pCur.position, pNext.position - pCur.position);
				angle /= length(pPrev.position - pCur.position) * length(pNext.position - pCur.position);
				angle = degrees(acosf(angle));

				if (angle <= 0 && angle >= 180)
					continue;

				// If any vertices are within this triangle
				bool inTri = false;
				for (int j = 0; j < int(iVerts.size()); j++)
				{
					if (inTriangle(iVerts[j].position, pPrev.position, pCur.position, pNext.position)
						&& iVerts[j].position != pPrev.position
						&& iVerts[j].position != pCur.position
						&& iVerts[j].position != pNext.position)
					{
						inTri = true;
						break;
					}
				}
				if (inTri)
					continue;

				// Create a triangle from pCur, pPrev, pNext
				for (int j = 0; j < int(iVerts.size()); j++)
				{
					if (iVerts[j].position == pCur.position)
						oIndices.push_back(j);
					if (iVerts[j].position == pPrev.position)
						oIndices.push_back(j);
					if (iVerts[j].position == pNext.position)
						oIndices.push_back(j);
				}

				// Delete pCur from the list
				for (int j = 0; j < int(tVerts.size()); j++)
				{
					if (tVerts[j].position == pCur.position)
					{
						tVerts.erase(tVerts.begin() + j);
						break;
					}
				}

				// reset i to the start
				// -1 since loop will add 1 to it
				i = -1;
			}

			// if no triangles were created
			if (oIndices.size() == 0)
				break;

			// if no more vertices
			if (tVerts.size() == 0)
				break;
		}
	}

	// Load ObjMaterials from .mtl file
	bool loadObjMaterials(const std::string & path)
	{
		// If the file is not a ObjMaterial file return false
		if (path.substr(path.size() - 4, path.size()) != ".mtl")
			return false;

		std::ifstream file(path);

		// If the file is not found return false
		if (!file.is_open())
			return false;

		ObjMaterial tempObjMaterial;

		bool listening = false;

		// Go through each line looking for ObjMaterial variables
		std::string curline;
		while (std::getline(file, curline))
		{
			// new ObjMaterial and ObjMaterial name
			if (firstToken(curline) == "newmtl")
			{
				if (!listening)
				{
					listening = true;

					if (curline.size() > 7)
					{
						tempObjMaterial.name = tail(curline);
					}
					else
					{
						tempObjMaterial.name = "none";
					}
				}
				else
				{
					// Generate the ObjMaterial

					// Push Back loaded ObjMaterial
					m_ObjMaterials.push_back(tempObjMaterial);

					// Clear Loaded ObjMaterial
					tempObjMaterial = ObjMaterial();

					if (curline.size() > 7)
					{
						tempObjMaterial.name = tail(curline);
					}
					else
					{
						tempObjMaterial.name = "none";
					}
				}
			}
			// Ambient Color
			if (firstToken(curline) == "Ka")
			{
				std::vector<std::string> temp;
				split(tail(curline), temp, " ");

				if (temp.size() != 3)
					continue;

				tempObjMaterial.Ka.x = std::stof(temp[0]);
				tempObjMaterial.Ka.y = std::stof(temp[1]);
				tempObjMaterial.Ka.z = std::stof(temp[2]);
			}
			// Diffuse Color
			if (firstToken(curline) == "Kd")
			{
				std::vector<std::string> temp;
				split(tail(curline), temp, " ");

				if (temp.size() != 3)
					continue;

				tempObjMaterial.Kd.x = std::stof(temp[0]);
				tempObjMaterial.Kd.y = std::stof(temp[1]);
				tempObjMaterial.Kd.z = std::stof(temp[2]);
			}
			// Specular Color
			if (firstToken(curline) == "Ks")
			{
				std::vector<std::string> temp;
				split(tail(curline), temp, " ");

				if (temp.size() != 3)
					continue;

				tempObjMaterial.Ks.x = std::stof(temp[0]);
				tempObjMaterial.Ks.y = std::stof(temp[1]);
				tempObjMaterial.Ks.z = std::stof(temp[2]);
			}
			// Specular Exponent
			if (firstToken(curline) == "Ns")
			{
				tempObjMaterial.Ns = std::stof(tail(curline));
			}
			// Optical Density
			if (firstToken(curline) == "Ni")
			{
				tempObjMaterial.Ni = std::stof(tail(curline));
			}
			// Dissolve
			if (firstToken(curline) == "d")
			{
				tempObjMaterial.d = std::stof(tail(curline));
			}
			// Illumination
			if (firstToken(curline) == "illum")
			{
				tempObjMaterial.illum = std::stoi(tail(curline));
			}
			// Ambient Texture Map
			if (firstToken(curline) == "map_Ka")
			{
				tempObjMaterial.map_Ka = tail(curline);
			}
			// Diffuse Texture Map
			if (firstToken(curline) == "map_Kd")
			{
				tempObjMaterial.map_Kd = tail(curline);
			}
			// Specular Texture Map
			if (firstToken(curline) == "map_Ks")
			{
				tempObjMaterial.map_Ks = tail(curline);
			}
			// Specular Hightlight Map
			if (firstToken(curline) == "map_Ns")
			{
				tempObjMaterial.map_Ns = tail(curline);
			}
			// Alpha Texture Map
			if (firstToken(curline) == "map_d")
			{
				tempObjMaterial.map_d = tail(curline);
			}
			// Bump Map
			if (firstToken(curline) == "map_Bump" || firstToken(curline) == "map_bump" || firstToken(curline) == "bump")
			{
				tempObjMaterial.map_bump = tail(curline);
			}
		}

		// Deal with last ObjMaterial

		// Push Back loaded ObjMaterial
		m_ObjMaterials.push_back(tempObjMaterial);

		// Test to see if anything was loaded
		// If not return false
		if (m_ObjMaterials.empty())
			return false;
		// If so return true
		else
			return true;
	}

	// Loaded Mesh Objects
	std::vector<Mesh> m_meshes;
	// Loaded Vertex Objects
	std::vector<Vertex> m_vertices;
	// Loaded Index Positions
	std::vector<unsigned int> m_indices;
	// Loaded ObjMaterial Objects
	std::vector<ObjMaterial> m_ObjMaterials;

};

	virtual void loadObjMaterials(const std::string & strFilename)
	{
		static char vcDrive[1024], vcDirectory[1024], vcFilename[1024], vcExtension[1024], vcPath[1024];
		_splitpath_s(strFilename.c_str(), vcDrive, vcDirectory, vcFilename, vcExtension);

		static char vcBuffer[4096];

		FILE *pFile = fopen(strFilename.c_str(), "r");

		if (!pFile)
			return;

		while (fscanf_s(pFile, "%s", vcBuffer, sizeof(vcBuffer)) != EOF)
		{
			switch (vcBuffer[0])
			{
				// comment
			case '#':
			{
				// eat up rest of line
				fgets(vcBuffer, sizeof(vcBuffer), pFile);
			}
			break;

			// newmtl
			case 'n':
			{
				fgets(vcBuffer, sizeof(vcBuffer), pFile);
				// sscanf_s(vcBuffer, "%s %s", vcBuffer, sizeof(vcBuffer), vcBuffer, sizeof(vcBuffer));

				const std::string strObjMaterialName = trim(vcBuffer);
				materialMap.insert(std::make_pair(strObjMaterialName, int(materials.size())));
				materials.push_back(ObjMaterial(strObjMaterialName));
			}
			break;

			case 'N':
			{
				switch (vcBuffer[1])
				{
				case 's':
				{
					float fShininess = 0.0f;
					fscanf_s(pFile, "%f", &fShininess);

					// wavefront shininess is from [0, 1000], so scale for OpenGL
					//fShininess *= 128.0f;
					//fShininess /= 1000.0f;

					materials.back().m_fShininess = fShininess;
				}
				break;

				default:
				{
					// eat up rest of line
					fgets(vcBuffer, sizeof(vcBuffer), pFile);
				}
				break;
				}
			}
			break;

			case 'K':

				switch (vcBuffer[1])
				{
				case 'd':
				{
					float fR = 1.0f, fG = 1.0f, fB = 1.0f, fA = 1.0f;
					fscanf_s(pFile, "%f %f %f", &fR, &fG, &fB);
					materials.back().m_colDiffuse = Color(fR, fG, fB, fA);
				}
				break;

				case 's':
				{
					float fR = 1.0f, fG = 1.0f, fB = 1.0f, fA = 1.0f;
					fscanf_s(pFile, "%f %f %f", &fR, &fG, &fB);
					materials.back().m_colSpecular = Color(fR, fG, fB, fA);
				}
				break;

				case 'a':
				{
					float fR = 1.0f, fG = 1.0f, fB = 1.0f, fA = 1.0f;
					fscanf_s(pFile, "%f %f %f", &fR, &fG, &fB);
					materials.back().m_colAmbient = Color(fR, fG, fB, fA);
				}
				break;

				default:
				{
					// eat up rest of line
					fgets(vcBuffer, sizeof(vcBuffer), pFile);
				}
				break;
				}
				break;

			case 'm':
			{
				switch (vcBuffer[5])
				{
				case 'd':
				{
					fscanf_s(pFile, "%s", &vcBuffer, sizeof(vcBuffer));
					ImageResource *pTexture = VolumeShop::Get().GetObject<ImageResource>(vcBuffer);

					if (!pTexture)
					{
						pTexture = VolumeShop::Get().createObject< Image<unsigned char, 4> >(vcBuffer, Object::ATTRIBUTE_DESTROYABLE);

						if (pTexture)
						{
							ImporterPlugin *pImporter = VolumeShop::Get().createObject< ImporterPlugin >("Texture Importer", Object::ATTRIBUTE_ACCESSIBLE);

							if (pImporter)
							{
								pImporter->SetFilename("plugin_importer_image.dll");
								pTexture->add(pImporter);

								if (pImporter->IsInitialized())
								{
									const std::string strTextureFilename = std::string(vcDrive) + std::string(vcDirectory) + std::string(vcBuffer);
									pImporter->GetProperty("Filename") = std::string(strTextureFilename);
									pTexture->startLoad(pImporter);
								}
							}
						}
					}
					materials.back().m_hanDiffuse = pTexture;
				}
				break;
				/*
										case 's':
											{
												float fR = 1.0f, fG = 1.0f, fB = 1.0f, fA = 1.0f;
												fscanf_s(pFile, "%f %f %f",&fR, &fG, &fB);
												materials.back().m_colSpecular = Color(fR,fG,fB,fA);
											}
											break;

										case 'a':
											{
												float fR = 1.0f, fG = 1.0f, fB = 1.0f, fA = 1.0f;
												fscanf_s(pFile, "%f %f %f",&fR, &fG, &fB);
												materials.back().m_colAmbient = Color(fR,fG,fB,fA);
											}
											break;
				*/
				default:
				{
					// eat up rest of line
					fgets(vcBuffer, sizeof(vcBuffer), pFile);
				}
				break;
				}
				break;
			}
			break;

			default:
				// eat up rest of line
				fgets(vcBuffer, sizeof(vcBuffer), pFile);
				break;
			}
		}

		fclose(pFile);
	};

private:

	TriangleMesh & m_mesMesh;
	ModifiedObserver m_modMeshObserver;

	unsigned int m_uFilePosition;
	unsigned int m_uFileSize;

	TriangleMesh m_mesImport;
	std::string m_strMessage;
	FILE *file;
	bool m_bCancelled;

	std::unordered_map< std::string, typename std::list<ObjGroup>::iterator > groupMap;
	std::list<ObjGroup> groupList;
	std::list<ObjGroup>::iterator groupIterator;

	std::vector< Voxel<float, 3> > positions;
	std::vector< Voxel<float, 3> > normals;
	std::vector< Voxel<float, 2> > texCoords;

	std::unordered_map< std::string, int > materialMap;
	std::vector<ObjMaterial> materials;
	std::string currentMaterial;
};

#endif



std::string trimLeft(const std::string &str, const std::string &whitespace = "\n\r\t ")
{
	size_t uIndex = str.find_first_not_of(whitespace);
	if (uIndex != std::string::npos)
		return str.substr(uIndex);

	return "";
}

std::string trimRight(const std::string &str, const std::string &whitespace = "\n\r\t ")
{
	size_t  uIndex = str.find_last_not_of(whitespace);
	if (uIndex != std::string::npos)
		return str.substr(0, uIndex + 1);

	return str;
}

std::string trim(const std::string &str, const std::string & whitespace = "\n\r\t ")
{
	return trimRight(trimLeft(str, whitespace), whitespace);
}

template<class e, class t, int N>
std::basic_istream<e, t>& operator>>(std::basic_istream<e, t>& in, const e(&sliteral)[N])
{
	std::array<e, N - 1> buffer; //get buffer
	in >> buffer[0]; //skips whitespace
	if (N > 2)
		in.read(&buffer[1], N - 2); //read the rest
	if (strncmp(&buffer[0], sliteral, N - 1)) //if it failed
		in.setstate(in.rdstate() | std::ios::failbit); //set the state
	return in;
}

template<class e, class t>
std::basic_istream<e, t>& operator>>(std::basic_istream<e, t>& in, const e& cliteral)
{
	e buffer;  //get buffer
	in >> buffer; //read data
	if (buffer != cliteral) //if it failed
		in.setstate(in.rdstate() | std::ios::failbit); //set the state
	return in;
}

//redirect mutable char arrays to their normal function
template<class e, class t, int N>
std::basic_istream<e, t>& operator>>(std::basic_istream<e, t>& in, e(&carray)[N])
{
	return std::operator>>(in, carray);
}

class ObjLoader
{
public:

	struct ObjGroup
	{
		std::string name;
		std::string material;
		std::vector<unsigned int> positionIndices;
		std::vector<unsigned int> normalIndices;
		std::vector<unsigned int> texCoordIndices;
	};

	struct ObjMaterial
	{
		// Material Name
		std::string name;
		// Ambient Color
		vec3 Ka = vec3(0.0f);
		// Diffuse Color
		vec3 Kd = vec3(0.0f);
		// Specular Color
		vec3 Ks = vec3(0.0f);
		// Specular Exponent
		float Ns = 0.0f;
		// Optical Density
		float Ni = 0.0f;
		// Dissolve
		float d = 0.0f;
		// Illumination
		int illum = 0;
		// Ambient Texture Map
		std::string map_Ka;
		// Diffuse Texture Map
		std::string map_Kd;
		// Specular Texture Map
		std::string map_Ks;
		// Specular Hightlight Map
		std::string map_Ns;
		// Alpha Texture Map
		std::string map_d;
		// Bump Map
		std::string map_bump;
	};

	bool loadObjFile(const std::string & filename)
	{
		std::filesystem::path path(filename);
		std::ifstream is(filename);

		if (!is.is_open())
			return false;

		std::vector< vec3 > positions;
		std::vector< vec3 > normals;
		std::vector< vec2 > texCoords;

		positions.push_back(vec3(0.0f));
		normals.push_back(vec3(0.0f));
		texCoords.push_back(vec2(1.0f));

		std::unordered_map< std::string, int > materialMap;
		std::vector<ObjMaterial> materials;

		ObjMaterial defaultMaterial;
		defaultMaterial.name = "default";

		materialMap.insert(std::make_pair(defaultMaterial.name, int(materials.size())));
		materials.push_back(defaultMaterial);

		std::string currentMaterial;
		currentMaterial = defaultMaterial.name;

		std::unordered_map< std::string, typename std::list<ObjGroup>::iterator > groupMap;
		std::list<ObjGroup> groupList;
		std::list<ObjGroup>::iterator groupIterator;

		ObjGroup defaultGroup;
		defaultGroup.name = "default";
		groupList.push_back(defaultGroup);
		groupIterator = groupList.end();
		groupIterator--;

		std::string buffer;

		while (is.good())
		{
			if (getline(is, buffer))
			{
				std::istringstream iss(buffer);
				std::string token;

				if (iss >> token)
				{
					switch (token.at(0))
					{
						// v, vn, vt
					case 'v':
					{
						if (token == "v")
						{
							vec3 p(0.0f);

							if (iss >> p.x >> p.y >> p.z)
								positions.push_back(p);
						}
						else if (token == "vn")
						{
							vec3 n(0.0f);

							if (iss >> n.x >> n.y >> n.z)
								normals.push_back(n);
						}
						else if (token == "vt")
						{
							vec2 t(0.0f);

							if (iss >> t.x >> t.y)
								texCoords.push_back(t);
						}
					}
					break;

					// mtllib
					case 'm':
					{
						std::string libraryName;

						while (iss >> libraryName)
						{
							libraryName = trim(libraryName);
							std::filesystem::path libraryPath = path.parent_path();
							libraryPath.append(libraryName);

							loadMtlFile(libraryPath.string(), materials, materialMap);
						}
					}
					break;

					// use material
					case 'u':
					{
						std::string materialName;

						if (iss >> materialName)
						{
							materialName = trim(materialName);
							currentMaterial = materialName;
							groupIterator->material = currentMaterial;
						}
					}
					break;

					// group
					case 'g':
					case 'o':
					{
						std::string groupName;
						
						if (getline(iss,groupName))
						{
							groupName = trim(groupName);
							std::unordered_map< std::string, typename std::list<ObjGroup>::iterator >::iterator j = groupMap.find(groupName);

							if (j == groupMap.end())
							{
								ObjGroup newGroup;
								newGroup.name = groupName;

								groupList.push_back(newGroup);
								groupIterator = groupList.end();
								groupIterator--;
							}
							else
								groupIterator = j->second;

							groupIterator->material = currentMaterial;
						}
					}
					break;

					// face
					case 'f':
					{
						int v = 0, n = 0, t = 0;

						if (buffer.find("//") != std::string::npos)
						{
							// v//n
							if (iss >> v >> ("//") >> n)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(n < 0 ? unsigned int(n + normals.size()) : unsigned int(n));
							}

							if (iss >> v >> ("//") >> n)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(n < 0 ? unsigned int(n + normals.size()) : unsigned int(n));
							}

							if (iss >> v >> ("//") >> n)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(n < 0 ? unsigned int(n + normals.size()) : unsigned int(n));
							}

							while (iss >> v >> ("//") >> n)
							{
								groupIterator->positionIndices.push_back(groupIterator->positionIndices[groupIterator->positionIndices.size() - 3]);
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(groupIterator->normalIndices[groupIterator->normalIndices.size() - 3]);

								groupIterator->positionIndices.push_back(groupIterator->positionIndices[groupIterator->positionIndices.size() - 2]);
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(groupIterator->normalIndices[groupIterator->normalIndices.size() - 2]);

								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(n < 0 ? unsigned int(n + normals.size()) : unsigned int(n));
							}

							break;
						}

						iss = std::istringstream(buffer);
						iss >> token;

						if (iss >> v >> ("/") >> t >> ("/") >> n)
						{
							// v/t/n
							groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
							groupIterator->texCoordIndices.push_back(t < 0 ? unsigned int(t + texCoords.size()) : unsigned int(t));
							groupIterator->normalIndices.push_back(n < 0 ? unsigned int(n + normals.size()) : unsigned int(n));

							if (iss >> v >> ("/") >> t >> ("/") >> n)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(t < 0 ? unsigned int(t + texCoords.size()) : unsigned int(t));
								groupIterator->normalIndices.push_back(n < 0 ? unsigned int(n + normals.size()) : unsigned int(n));
							}

							if (iss >> v >> ("/") >> t >> ("/") >> n)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(t < 0 ? unsigned int(t + texCoords.size()) : unsigned int(t));
								groupIterator->normalIndices.push_back(n < 0 ? unsigned int(n + normals.size()) : unsigned int(n));
							}

							while (iss >> v >> ("/") >> t >> ("/") >> n)
							{
								groupIterator->positionIndices.push_back(groupIterator->positionIndices[groupIterator->positionIndices.size() - 3]);
								groupIterator->texCoordIndices.push_back(groupIterator->texCoordIndices[groupIterator->texCoordIndices.size() - 3]);
								groupIterator->normalIndices.push_back(groupIterator->normalIndices[groupIterator->normalIndices.size() - 3]);

								groupIterator->positionIndices.push_back(groupIterator->positionIndices[groupIterator->positionIndices.size() - 2]);
								groupIterator->texCoordIndices.push_back(groupIterator->texCoordIndices[groupIterator->texCoordIndices.size() - 2]);
								groupIterator->normalIndices.push_back(groupIterator->normalIndices[groupIterator->normalIndices.size() - 2]);

								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(t < 0 ? unsigned int(t + texCoords.size()) : unsigned int(t));
								groupIterator->normalIndices.push_back(n < 0 ? unsigned int(n + normals.size()) : unsigned int(n));
							}

							break;
						}

						iss = std::istringstream(buffer);
						iss >> token;

						if (iss >> v >> ("/") >> t)
						{
							// v/t
							groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
							groupIterator->texCoordIndices.push_back(t < 0 ? unsigned int(t + texCoords.size()) : unsigned int(t));
							groupIterator->normalIndices.push_back(0);

							if (iss >> v >> ("/") >> t)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(t < 0 ? unsigned int(t + texCoords.size()) : unsigned int(t));
								groupIterator->normalIndices.push_back(0);
							}

							if (iss >> v >> ("/") >> t)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(t < 0 ? unsigned int(t + texCoords.size()) : unsigned int(t));
								groupIterator->normalIndices.push_back(0);
							}

							while (iss >> v >> ("/") >> t)
							{
								groupIterator->positionIndices.push_back(groupIterator->positionIndices[groupIterator->positionIndices.size() - 3]);
								groupIterator->texCoordIndices.push_back(groupIterator->texCoordIndices[groupIterator->texCoordIndices.size() - 3]);
								groupIterator->normalIndices.push_back(0);

								groupIterator->positionIndices.push_back(groupIterator->positionIndices[groupIterator->positionIndices.size() - 2]);
								groupIterator->texCoordIndices.push_back(groupIterator->texCoordIndices[groupIterator->texCoordIndices.size() - 2]);
								groupIterator->normalIndices.push_back(0);

								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(t < 0 ? unsigned int(t + texCoords.size()) : unsigned int(t));
								groupIterator->normalIndices.push_back(0);
							}

							break;
						}
						else
						{
							iss = std::istringstream(buffer);
							iss >> token;

							// v
							if (iss >> v)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(0);
							}

							if (iss >> v)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(0);
							}

							if (iss >> v)
							{
								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(0);
							}

							while (iss >> v)
							{
								groupIterator->positionIndices.push_back(groupIterator->positionIndices[groupIterator->positionIndices.size() - 3]);
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(0);

								groupIterator->positionIndices.push_back(groupIterator->positionIndices[groupIterator->positionIndices.size() - 2]);
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(0);

								groupIterator->positionIndices.push_back(v < 0 ? unsigned int(v + positions.size()) : unsigned int(v));
								groupIterator->texCoordIndices.push_back(0);
								groupIterator->normalIndices.push_back(0);
							}
						}
					}
					break;
					}
				}
			}
		}

		if (materials.size() <= 1)
		{
			std::filesystem::path libraryPath = path;
			libraryPath.replace_extension("mtl");
			loadMtlFile(libraryPath.string(), materials, materialMap);
		}

		// compute normals if not present in the file
		if (normals.size() <= 1)
		{
			// compute face normals
			for (std::list<ObjGroup>::iterator i = groupList.begin(); i != groupList.end(); i++)
			{
				if (i->positionIndices.size() > 0)
				{
					for (unsigned int j = 0; j < i->positionIndices.size() - 2; j += 3)
					{
						const vec3 & p0 = positions[i->positionIndices[j + 0]];
						const vec3 & p1 = positions[i->positionIndices[j + 1]];
						const vec3 & p2 = positions[i->positionIndices[j + 2]];

						if (i->normalIndices[j + 0] == 0 || i->normalIndices[j + 1] == 0 || i->normalIndices[j + 2] == 0)
						{
							if (i->normalIndices[j + 0] == 0)
								i->normalIndices[j + 0] = normals.size();

							if (i->normalIndices[j + 1] == 0)
								i->normalIndices[j + 1] = normals.size();

							if (i->normalIndices[j + 2] == 0)
								i->normalIndices[j + 2] = normals.size();

							const vec3 a(p2 - p1);
							const vec3 b(p0 - p1);
							const vec3 n = normalize(cross(a, b));
							normals.push_back(n);
						}
					}
				}
			}

			// compute vertex normals
			std::vector< vec3 > vertexNormals(positions.size());
			std::vector< vec3 > groupNormals(positions.size());

			for (std::list<ObjGroup>::iterator i = groupList.begin(); i != groupList.end(); i++)
			{
				if (i->positionIndices.size() > 0)
				{
					for (unsigned int j = 0; j < i->positionIndices.size() - 2; j += 3)
					{
						groupNormals[i->positionIndices[j + 0]] = groupNormals[i->positionIndices[j + 0]] + normals[i->normalIndices[j + 0]];
						groupNormals[i->positionIndices[j + 1]] = groupNormals[i->positionIndices[j + 1]] + normals[i->normalIndices[j + 1]];
						groupNormals[i->positionIndices[j + 2]] = groupNormals[i->positionIndices[j + 2]] + normals[i->normalIndices[j + 2]];
					}

					for (unsigned int j = 0; j < i->positionIndices.size() - 2; j += 3)
					{
						vertexNormals[i->positionIndices[j + 0]] = normalize(groupNormals[i->positionIndices[j + 0]]);
						vertexNormals[i->positionIndices[j + 1]] = normalize(groupNormals[i->positionIndices[j + 1]]);
						vertexNormals[i->positionIndices[j + 2]] = normalize(groupNormals[i->positionIndices[j + 2]]);
					}

					for (unsigned int j = 0; j < i->positionIndices.size() - 2; j += 3)
					{
						groupNormals[i->positionIndices[j + 0]] = vec3(0.0f);
						groupNormals[i->positionIndices[j + 1]] = vec3(0.0f);
						groupNormals[i->positionIndices[j + 2]] = vec3(0.0f);
					}

					i->normalIndices = i->positionIndices;
				}
			}

			normals.swap(vertexNormals);
		}

		m_vertices.resize(positions.size());

		for (std::list<ObjGroup>::iterator i = groupList.begin(); i != groupList.end(); i++)
		{
			if (i->positionIndices.size() > 0)
			{
				Group newGroup;
				newGroup.name = i->name;
				newGroup.startIndex = m_indices.size();

				std::unordered_map<std::string, int>::iterator j = materialMap.find(i->material);

				if (j != materialMap.end())
					newGroup.materialIndex = j->second;
				else
					newGroup.materialIndex = 0;

				for (unsigned int j = 0; j < i->positionIndices.size(); j++)
				{
					const uint index = i->positionIndices[j];

					Vertex vertex;
					vertex.position = positions[index];
					vertex.normal = normals[i->normalIndices[j]];
					vertex.texcoord = texCoords[i->texCoordIndices[j]];

					if (m_vertices[index].position == vertex.position)
					{
						if (m_vertices[index].texcoord != vertex.texcoord || m_vertices[index].normal != vertex.normal)
						{
							m_indices.push_back(m_vertices.size());
							m_vertices.push_back(vertex);
						}
						else
						{
							m_vertices[index] = vertex;
							m_indices.push_back(index);
						}
					}
					else
					{
						m_vertices[index] = vertex;
						m_indices.push_back(index);
					}
				}

				newGroup.endIndex = m_indices.size();
				m_groups.push_back(newGroup);
			}
		}

		m_materials.reserve(materials.size());
		

		for (auto & m : materials)
		{
			Material newMaterial;
			newMaterial.ambient = m.Ka;
			newMaterial.diffuse = m.Kd;
			newMaterial.specular = m.Ks;
			newMaterial.shininess = m.Ns;

			if (!m.map_Kd.empty())
			{
				std::filesystem::path texturePath = path.parent_path();
				texturePath.append(m.map_Kd);
				newMaterial.diffuseTexture = std::move(loadTexture(texturePath.string()));
			}
			m_materials.push_back(newMaterial);

			
		}

		return true;
	}

	bool loadMtlFile(const std::string & filename, std::vector<ObjMaterial> & materials, std::unordered_map< std::string, int > & materialMap)
	{
		std::ifstream is(filename);

		if (!is.is_open())
			return false;

		std::string buffer;

		while (is.good())
		{
			if (getline(is, buffer))
			{
				std::istringstream iss(buffer);
				std::string token;

				if (iss >> token)
				{
					if (token == "newmtl")
					{
						std::string materialName;

						if (iss >> materialName)
						{
							materialName = trim(materialName);

							ObjMaterial newMaterial;
							newMaterial.name = materialName;
							materialMap.insert(std::make_pair(newMaterial.name, int(materials.size())));

							materials.push_back(newMaterial);
						}
					}
					// Ambient Color
					else if (token == "Ka")
					{
						vec3 Ka(0.0f);
						if (iss >> Ka.x >> Ka.y >> Ka.z)
							materials.back().Ka = Ka;
					}
					// Diffuse Color
					else if (token == "Kd")
					{
						vec3 Kd(0.0f);
						if (iss >> Kd.x >> Kd.y >> Kd.z)
							materials.back().Kd = Kd;
					}
					// Specular Color
					else if (token == "Ks")
					{
						vec3 Ks(0.0f);
						if (iss >> Ks.x >> Ks.y >> Ks.z)
							materials.back().Ks = Ks;
					}
					// Specular Exponent
					else if (token == "Ns")
					{
						float Ns = 0.0f;
						if (iss >> Ns)
							materials.back().Ns = Ns;
					}
					// Optical Density
					else if (token == "Ni")
					{
						float Ni = 0.0f;
						if (iss >> Ni)
							materials.back().Ni = Ni;
					}
					// Dissolve
					else if (token == "d")
					{
						float d = 0.0f;
						if (iss >> d)
							materials.back().d = d;
					}
					// Illumination
					else if (token == "illum")
					{
						int illum = 0;
						if (iss >> illum)
							materials.back().illum = illum;
					}
					// Ambient Texture Map
					else if (token == "map_Ka")
					{
						std::string map_Ka;
						if (iss >> map_Ka)
							materials.back().map_Ka = map_Ka;
					}
					// Diffuse Texture Map
					else if (token == "map_Kd")
					{
						std::string map_Kd;
						if (iss >> map_Kd)
							materials.back().map_Kd = map_Kd;
					}
					// Specular Texture Map
					else if (token == "map_Ks")
					{
						std::string map_Ks;
						if (iss >> map_Ks)
							materials.back().map_Ks = map_Ks;
					}
					// Specular Hightlight Map
					else if (token == "map_Ns")
					{
						std::string map_Ns;
						if (iss >> map_Ns)
							materials.back().map_Ns = map_Ns;
					}
					// Alpha Texture Map
					else if (token == "map_d")
					{
						std::string map_d;
						if (iss >> map_d)
							materials.back().map_d = map_d;
					}
					// Bump Map
					else if (token == "map_bump" || buffer == "map_Bump" || buffer == "bump")
					{
						std::string map_bump;
						if (iss >> map_bump)
							materials.back().map_bump = map_bump;
					}
				}
			}
		}

		return true;
	}

	std::unique_ptr<Texture> loadTexture(const std::string & filename)
	{
		int width, height, channels;

		stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, 0);

		if (data)
		{
			std::cout << "Loaded " << filename << std::endl;

			auto texture = Texture::create(GL_TEXTURE_2D);
			texture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			texture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			texture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			texture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			GLenum format = GL_RGBA;

			switch (channels)
			{
			case 1:
				format = GL_RED;
				break;

			case 2:
				format = GL_RG;
				break;

			case 3:
				format = GL_RGB;
				break;

			case 4:
				format = GL_RGBA;
				break;
			}
				
			texture->image2D(0, format, ivec2(width,height), 0, format, GL_UNSIGNED_BYTE, data);
			texture->generateMipmap();

			stbi_image_free(data);

			return texture;
		}

		return std::unique_ptr<Texture>();
	}

	const std::vector<Group> & groups() const
	{
		return m_groups;
	}

	const std::vector<Vertex> & vertices() const
	{
		return m_vertices;
	}

	const std::vector<uint> & indices() const
	{
		return m_indices;
	}

	const std::vector<Material> & materials() const
	{
		return m_materials;
	}

private:

	std::vector < Group > m_groups;
	std::vector < Vertex > m_vertices;
	std::vector < glm::uint > m_indices;
	std::vector < Material > m_materials;

};

Model::Model()
{

}

Model::Model(const std::string& filename)
{
	load(filename);
}

void Model::load(const std::string& filename)
{
	globjects::debug() << "Loading file " << filename << " ...";

	m_minimumBounds = vec3(std::numeric_limits<float>::max());
	m_maximumBounds = vec3(-std::numeric_limits<float>::max());

	ObjLoader loader;

	if (loader.loadObjFile(filename))
	{
		m_filename = filename;
		m_vertices = loader.vertices();
		m_indices = loader.indices();
		m_materials = loader.materials();
		m_groups = loader.groups();

		for (auto i : m_indices)
		{
			const auto & v = m_vertices[i];
			m_minimumBounds = min(m_minimumBounds, v.position);
			m_maximumBounds = max(m_maximumBounds, v.position);
		}

		globjects::debug() << "Minimum bounds: " << m_minimumBounds;
		globjects::debug() << "Maximum bounds: " << m_maximumBounds;
	}
	else
	{
		globjects::debug() << "Error loading << " << filename << "!";
	}
}

const std::string & Model::filename() const
{
	return m_filename;
}

const std::vector<Group> & Model::groups() const
{
	return m_groups;
}

const std::vector<Vertex> & Model::vertices() const
{
	return m_vertices;
}

const std::vector<uint> & Model::indices() const
{
	return m_indices;
}

const std::vector<Material> & Model::materials() const
{
	return m_materials;
}

vec3 Model::minimumBounds() const
{
	return m_minimumBounds;
}

vec3 Model::maximumBounds() const
{
	return m_maximumBounds;
}

