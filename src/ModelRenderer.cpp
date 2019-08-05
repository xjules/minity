#include "ModelRenderer.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
#include "Viewer.h"
#include "Scene.h"
#include "Model.h"
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace minity;
using namespace gl;
using namespace glm;
using namespace globjects;

ModelRenderer::ModelRenderer(Viewer* viewer) : Renderer(viewer)
{
	Shader::hintIncludeImplementation(Shader::IncludeImplementation::Fallback);

	//	std::cout << viewer()->scene()->model()->shapes().

	m_shaderSourceDefines = StaticStringSource::create("");
	m_shaderDefines = NamedString::create("/defines.glsl", m_shaderSourceDefines.get());

	m_shaderSourceGlobals = File::create("./res/model/globals.glsl");
	m_shaderGlobals = NamedString::create("/globals.glsl", m_shaderSourceGlobals.get());

	m_vertexShaderSourceBase = Shader::sourceFromFile("./res/model/base-vs.glsl");
	m_geometryShaderSourceBase = Shader::sourceFromFile("./res/model/base-gs.glsl");
	m_fragmentShaderSourceBase = Shader::sourceFromFile("./res/model/base-fs.glsl");

	m_vertexShaderTemplateBase = Shader::applyGlobalReplacements(m_vertexShaderSourceBase.get());
	m_geometryShaderTemplateBase = Shader::applyGlobalReplacements(m_geometryShaderSourceBase.get());
	m_fragmentShaderTemplateBase = Shader::applyGlobalReplacements(m_fragmentShaderSourceBase.get());

	m_vertexShaderBase = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateBase.get());
	m_geometryShaderBase = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateBase.get());
	m_fragmentShaderBase = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateBase.get());

	//m_programBase->attach(m_vertexShaderBase.get(), m_geometryShaderBase.get(), m_fragmentShaderBase.get());
	m_programBase->attach(m_vertexShaderBase.get(), m_fragmentShaderBase.get());




	
}

std::list<globjects::File*> ModelRenderer::shaderFiles() const
{
	return std::list<globjects::File*>({
		m_vertexShaderSourceBase.get(),
		m_geometryShaderSourceBase.get(),
		m_fragmentShaderSourceBase.get()
		});
}

void ModelRenderer::display()
{
	// Save OpenGL state
	auto currentState = State::currentState();

	// retrieve/compute all necessary matrices and related properties
	const mat4 viewMatrix = viewer()->viewTransform();
	const mat4 inverseViewMatrix = inverse(viewMatrix);
	const mat4 modelViewMatrix = viewer()->modelViewTransform();
	const mat4 inverseModelViewMatrix = inverse(modelViewMatrix);
	const mat4 modelViewProjectionMatrix = viewer()->modelViewProjectionTransform();
	const mat4 inverseModelViewProjectionMatrix = inverse(modelViewProjectionMatrix);
	const mat4 projectionMatrix = viewer()->projectionTransform();
	const mat4 inverseProjectionMatrix = inverse(projectionMatrix);
	const mat3 normalMatrix = mat3(transpose(inverseModelViewMatrix));
	const mat3 inverseNormalMatrix = inverse(normalMatrix);

	//viewer()->scene()->model()->shapes().at(0).model.indices

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	m_programBase->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	viewer()->scene()->model()->vertexArray().bind();
	
	m_programBase->use();
	//m_vao->drawArrays(GL_POINTS, 0, viewer()->scene()->model()->indices().size());

	const std::vector<Group> & groups = viewer()->scene()->model()->groups();
	const std::vector<Material> & materials = viewer()->scene()->model()->materials();

	bool open = true;
	ImGui::ShowDemoWindow(&open);

	static std::vector<bool> groupEnabled(groups.size(), true);

	ImGui::Begin("Model Renderer");

	if (ImGui::CollapsingHeader("Configuration"))
	{
		for (uint i=0;i<groups.size();i++)
		{
			bool checked = groupEnabled.at(i);
			ImGui::Checkbox(groups.at(i).name.c_str(), &checked);
			groupEnabled[i] = checked;
		}

	}

	ImGui::End();



	for (uint i = 0; i < groups.size(); i++)
	{

		if (groupEnabled.at(i))
		{
			const Material & material = materials.at(groups.at(i).materialIndex);

			m_programBase->setUniform("diffuseColor", material.diffuse);

			if (material.diffuseTexture)
			{
				m_programBase->setUniform("diffuseTexture", 0);
				material.diffuseTexture->bindActive(0);
			}

			//			std::cout << groups.at(i).name << std::endl;
			viewer()->scene()->model()->vertexArray().drawElements(GL_TRIANGLES, groups.at(i).count(), GL_UNSIGNED_INT, (void*)(sizeof(GLuint)*groups.at(i).startIndex));

			if (material.diffuseTexture)
			{
				material.diffuseTexture->unbind();
			}
		}
	}



	//m_vao->drawElements(GL_TRIANGLES, viewer()->scene()->model()->indices().size(), GL_UNSIGNED_INT);
	m_programBase->release();

	viewer()->scene()->model()->vertexArray().unbind();

	// Restore OpenGL state
	currentState->apply();
}