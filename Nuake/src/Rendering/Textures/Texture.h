#pragma once

#include "stb_image/stb_image.h"
#include <string>
#include "src/Core/Maths.h"
#include "msdfgen/core/BitmapRef.hpp"
#include "src/Resource/Serializable.h"

namespace Nuake
{
	typedef unsigned int GLenum;

	class Texture : ISerializable
	{
	private:
		unsigned int m_RendererId;
		std::string m_FilePath;
		unsigned char* m_LocalBuffer;
		GLenum m_Format;
		GLenum m_Format2;
		GLenum m_Format3;

		int m_Width;
		int m_Height;
		int m_BPP; // byte per pixel.

	public:
		Texture(const std::string& path);
		Texture(glm::vec2 size, msdfgen::BitmapConstRef<unsigned char, 4>& bitmap, bool t);
		Texture(glm::vec2 size, GLenum format, GLenum format2 = 0, GLenum format3 = 0, void* data = NULL);
		Texture(Vector2 size, unsigned char* data, int len);
		~Texture();

		void Resize(glm::vec2 size);
		void AttachToFramebuffer(GLenum attachment);

		void Bind(unsigned int slot = 0) const;
		void Unbind() const;

		void SetParameter(const GLenum& param, const GLenum& value);

		unsigned int GetID() const { return m_RendererId; }
		inline std::string GetPath() { return m_FilePath; }
		inline int GetWidth() const { return m_Width; }
		inline int GetHeight() const { return m_Height; }
		inline Vector2 GetSize() const { return Vector2(m_Width, m_Height); }

		json Serialize() override
		{
			BEGIN_SERIALIZE();
			j["Path"] = this->m_FilePath;
			END_SERIALIZE();
		}

		bool Deserialize(const std::string& str) override
		{
			BEGIN_DESERIALIZE();
			if (j.contains("Path"))
				return false;


		}
	};
}
