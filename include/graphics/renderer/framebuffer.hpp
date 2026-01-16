#pragma once
#include <glad/glad.h>
#include "game/ecs/components/render_target.hpp"
#include <assert.h>
#include <vector>
#include <span>

struct framebuffer_attachment {
    GLuint id = 0;
    GLenum internal_format = 0;
    framebuffer_attachment_type type;
};


class framebuffer {
public:
    framebuffer() = default;

    framebuffer(const framebuffer&)            = delete;
    framebuffer& operator=(const framebuffer&) = delete;

    framebuffer(framebuffer&& other) noexcept {
        move_from(other);
    }

    framebuffer& operator=(framebuffer&& other) noexcept {
        if (this != &other) {
            release();
            move_from(other);
        }
        return *this;
    }

    ~framebuffer() {
        release();
    }

	bool valid() const {
		return m_id != 0;
	}

	bool needs_resize(const RenderTarget& rt) const {
		return m_width != rt.width || m_height != rt.height;
	}

	bool needs_rebuild(const RenderTarget& rt) const {
		if (m_width != rt.width || m_height != rt.height)
			return true;
		if (m_desc_attachments.size() != rt.attachments.size())
			return true;
		for (size_t i = 0; i < rt.attachments.size(); ++i) {
			if (m_desc_attachments[i].type != rt.attachments[i].type ||
					m_desc_attachments[i].internal_format != rt.attachments[i].internal_format)
				return true;
		}
		return false;
	}



    void create(int width, int height, std::span<const framebuffer_attachment_desc> attachments)
    {
        assert(width > 0 && height > 0);
		release();
        m_width  = width;
        m_height = height;
		m_desc_attachments.assign(attachments.begin(), attachments.end());

        glCreateFramebuffers(1, &m_id);

        for (const auto& desc : attachments) {
            framebuffer_attachment att;
            att.type            = desc.type;
            att.internal_format = desc.internal_format;

            glCreateTextures(GL_TEXTURE_2D, 1, &att.id);

            if (desc.type == framebuffer_attachment_type::color) {
                glTextureStorage2D(att.id, 1, att.internal_format, width, height);
                glNamedFramebufferTexture(
                    m_id,
                    GL_COLOR_ATTACHMENT0 + m_color_attachments.size(),
                    att.id,
                    0
                );
                m_color_attachments.push_back(att);
            }
            else if (desc.type == framebuffer_attachment_type::depth) {
                glTextureStorage2D(att.id, 1, att.internal_format, width, height);
                glNamedFramebufferTexture(
                    m_id,
                    GL_DEPTH_ATTACHMENT,
                    att.id,
                    0
                );
                m_depth_attachment = att;
            }
            else if (desc.type == framebuffer_attachment_type::depth_stencil) {
                glTextureStorage2D(att.id, 1, att.internal_format, width, height);
                glNamedFramebufferTexture(
                    m_id,
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    att.id,
                    0
                );
                m_depth_attachment = att;
            }
        }

        update_draw_buffers();

#if defined(DEBUG)
        assert(glCheckNamedFramebufferStatus(m_id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
#endif
    }

	void resize(int width, int height) {
		if (width == m_width && height == m_height)
			return;

		m_width  = width;
		m_height = height;

		// Delete old textures
		for (auto& att : m_color_attachments) {
			glDeleteTextures(1, &att.id);
		}
		if (m_depth_attachment.id != 0) {
			glDeleteTextures(1, &m_depth_attachment.id);
			m_depth_attachment = {};
		}

		m_color_attachments.clear();

		// Recreate textures based on stored descriptors
		for (const auto& desc : m_desc_attachments) {
			framebuffer_attachment att;
			att.type            = desc.type;
			att.internal_format = desc.internal_format;

			glCreateTextures(GL_TEXTURE_2D, 1, &att.id);

			if (desc.type == framebuffer_attachment_type::color) {
				glTextureStorage2D(att.id, 1, att.internal_format, width, height);
				glNamedFramebufferTexture(
						m_id,
						GL_COLOR_ATTACHMENT0 + m_color_attachments.size(),
						att.id,
						0
						);
				m_color_attachments.push_back(att);
			}
			else if (desc.type == framebuffer_attachment_type::depth) {
				glTextureStorage2D(att.id, 1, att.internal_format, width, height);
				glNamedFramebufferTexture(
						m_id,
						GL_DEPTH_ATTACHMENT,
						att.id,
						0
						);
				m_depth_attachment = att;
			}
			else if (desc.type == framebuffer_attachment_type::depth_stencil) {
				glTextureStorage2D(att.id, 1, att.internal_format, width, height);
				glNamedFramebufferTexture(
						m_id,
						GL_DEPTH_STENCIL_ATTACHMENT,
						att.id,
						0
						);
				m_depth_attachment = att;
			}
		}

		update_draw_buffers();

#if defined(DEBUG)
		assert(glCheckNamedFramebufferStatus(m_id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
#endif
	}

    void bind_draw() const {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
    }

    void bind_read() const {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_id);
    }

    static void bind_default_draw() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    static void bind_default_read() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    GLuint color_attachment(size_t index) const {
        return m_color_attachments[index].id;
    }

    GLuint depth_attachment() const {
        return m_depth_attachment.id;
    }

    int width()  const { return m_width; }
    int height() const { return m_height; }

    void set_debug_name(const char* name) {
#if defined(DEBUG)
        glObjectLabel(GL_FRAMEBUFFER, m_id, -1, name);
#endif
    }

private:
    GLuint m_id = 0;
    int    m_width  = 0;
    int    m_height = 0;

    std::vector<framebuffer_attachment> m_color_attachments;
	std::vector<framebuffer_attachment_desc> m_desc_attachments;
    framebuffer_attachment              m_depth_attachment{};

    void update_draw_buffers() {
        if (m_color_attachments.empty()) {
            glNamedFramebufferDrawBuffer(m_id, GL_NONE);
            glNamedFramebufferReadBuffer(m_id, GL_NONE);
            return;
        }

        std::vector<GLenum> buffers(m_color_attachments.size());
        for (size_t i = 0; i < buffers.size(); ++i)
            buffers[i] = GL_COLOR_ATTACHMENT0 + i;

        glNamedFramebufferDrawBuffers(m_id, buffers.size(), buffers.data());
    }

    void release() {
        for (auto& att : m_color_attachments)
            glDeleteTextures(1, &att.id);

        if (m_depth_attachment.id)
            glDeleteTextures(1, &m_depth_attachment.id);

        if (m_id)
            glDeleteFramebuffers(1, &m_id);

        m_color_attachments.clear();
        m_depth_attachment = {};
        m_id               = 0;
    }

    void move_from(framebuffer& other) {
        m_id               = other.m_id;
        m_width            = other.m_width;
        m_height           = other.m_height;
        m_color_attachments = std::move(other.m_color_attachments);
        m_depth_attachment  = other.m_depth_attachment;

        other.m_id = 0;
        other.m_depth_attachment = {};
    }
};
