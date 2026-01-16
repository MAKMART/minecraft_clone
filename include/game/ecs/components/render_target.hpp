#pragma once
#include <glad/glad.h>

enum class extent_mode {
	fixed,
	follow_viewport
};

enum class framebuffer_attachment_type {
    color,
    depth,
    depth_stencil
};

struct framebuffer_attachment_desc {
    framebuffer_attachment_type type;
    GLenum internal_format;
};

struct RenderTarget {
	public:
		RenderTarget(int width, int height, std::initializer_list<framebuffer_attachment_desc> desc) : width(width), height(height), attachments(desc) {}
		RenderTarget(int width, int height, extent_mode mode, std::initializer_list<framebuffer_attachment_desc> desc) : width(width), height(height), mode(mode), attachments(desc) {}
		int    width  = 0;
		int    height = 0;
		extent_mode mode = extent_mode::follow_viewport;
		std::vector<framebuffer_attachment_desc> attachments;
};
