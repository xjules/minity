#pragma once
#include "Renderer.h"
#include <memory>

#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>

#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/Framebuffer.h>
#include <globjects/Renderbuffer.h>
#include <globjects/Texture.h>
#include <globjects/base/File.h>
#include <globjects/TextureHandle.h>
#include <globjects/NamedString.h>
#include <globjects/base/StaticStringSource.h>

namespace minity
{
	class Viewer;

	class ModelRenderer : public Renderer
	{
	public:
		ModelRenderer(Viewer *viewer);
		virtual void display();
		virtual std::list<globjects::File*> shaderFiles() const;

	private:

		std::unique_ptr<globjects::Buffer> m_vertices = std::make_unique<globjects::Buffer>();
		std::unique_ptr< globjects::Buffer > m_indices = std::make_unique<globjects::Buffer>();

		std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();

		std::unique_ptr<globjects::StaticStringSource> m_shaderSourceDefines = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderDefines = nullptr;

		std::unique_ptr<globjects::File> m_shaderSourceGlobals = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderGlobals = nullptr;

		std::unique_ptr<globjects::File> m_vertexShaderSourceBase = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_vertexShaderTemplateBase = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderBase = nullptr;

		std::unique_ptr<globjects::File> m_geometryShaderSourceBase = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_geometryShaderTemplateBase = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderBase = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceBase = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateBase = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderBase = nullptr;

		std::unique_ptr<globjects::Program> m_programBase = std::make_unique<globjects::Program>();
		
	};

}