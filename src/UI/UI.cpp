#include "UI.h"
#include "Shader.h"
#include "glm/gtc/type_ptr.hpp"
#include <iostream>
#include <cstring>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include "defines.h"

using namespace Rml::Input;
UI::UI(int width, int height, Shader *ui_shader, std::filesystem::path fontPath, std::filesystem::path docPath) : viewport_width(width), viewport_height(height) {

    // System and file interfaces owned by UI (RAII)
    systemInterface = std::make_unique<SystemInterface>();
    fileInterface = std::make_unique<FileInterface>();

    Rml::SetRenderInterface(this);
    Rml::SetSystemInterface(systemInterface.get());
    Rml::SetFileInterface(fileInterface.get());
    shader = ui_shader;
    Rml::Initialise();

    context = Rml::CreateContext("main", {width, height});
#ifdef DEBUG
    Rml::Debugger::Initialise(context);
#endif
    if (!Rml::LoadFontFace(fontPath.string())) {
        throw std::runtime_error("Failed to load font: " + fontPath.string());
    }
    doc = context->LoadDocument(docPath.string());
    if (!doc) {
        throw std::runtime_error("Failed to load RML document: " + docPath.string());
    } else {
        doc->Show();
    }
}

UI::~UI() {
    for (auto &[handle, geo] : geometry_map) {
        glDeleteVertexArrays(1, &geo.vao);
        glDeleteBuffers(1, &geo.vbo);
        glDeleteBuffers(1, &geo.ebo);
    }

    // No texture_map clearing needed since RmlUI handles it internally
    if (doc) {
        doc->Close();
    }
    Rml::RemoveContext("main");
    context = nullptr;
    Rml::SetRenderInterface(nullptr);
    Rml::SetSystemInterface(nullptr);
    Rml::SetFileInterface(nullptr);
    Rml::Shutdown();
}

Rml::CompiledGeometryHandle UI::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    GLuint vao, vbo, ebo;
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ebo);

    glNamedBufferData(vbo, vertices.size() * sizeof(Rml::Vertex), vertices.data(), GL_STATIC_DRAW);
    glNamedBufferData(ebo, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Rml::Vertex));
    glEnableVertexArrayAttrib(vao, 0); // pos
    glEnableVertexArrayAttrib(vao, 1); // color
    glEnableVertexArrayAttrib(vao, 2); // texcoord

    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, offsetof(Rml::Vertex, position));
    glVertexArrayAttribFormat(vao, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(Rml::Vertex, colour));
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Rml::Vertex, tex_coord));

    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 0);
    glVertexArrayAttribBinding(vao, 2, 0);
    glVertexArrayElementBuffer(vao, ebo);

    Geometry geo{vao, vbo, ebo, static_cast<GLsizei>(indices.size())};
    Rml::CompiledGeometryHandle handle = next_geometry_handle++;
    geometry_map[handle] = geo;
    return handle;
}

void UI::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) {
    auto it = geometry_map.find(handle);
    if (it == geometry_map.end())
        return;

    // Setup shader uniforms, texture bind, etc. -- omitted for brevity
    projection = glm::ortho(0.0f, (float)viewport_width, (float)viewport_height, 0.0f, -1.0f, 1.0f);
    // Combine the model matrix from SetTransform with the translation
    glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, 0.0f));
    glm::mat4 final_model = model * translation_matrix; // Apply SetTransform's model first, then translation

    shader->use();
    shader->setMat4("uModel", final_model);

    shader->setInt("uHasTexture", texture != 0 ? 1 : 0);

    // Enable stencil testing for masking
    if (clip_mask_enabled) {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilMask(0x00);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    if (texture != 0) {
        auto tex = texture_map.find(texture);
        if (tex != texture_map.end()) {
            tex->second.Bind(3);
        } else {
            // Invalid handle, maybe unbind texture or log error
            // tex->second.Unbind(3);
            // Or log warning
            throw std::runtime_error("Invalid texture");
        }
        Geometry &geo = it->second;
        glBindVertexArray(geo.vao);
        DrawElementsWrapper(GL_TRIANGLES, geo.index_count, GL_UNSIGNED_INT, nullptr);
        tex->second.Unbind(3);
    } else {
        Geometry &geo = it->second;
        glBindVertexArray(geo.vao);
        DrawElementsWrapper(GL_TRIANGLES, geo.index_count, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
    glDisable(GL_STENCIL_TEST);
}

void UI::ReleaseGeometry(Rml::CompiledGeometryHandle handle) {
    auto it = geometry_map.find(handle);
    if (it != geometry_map.end()) {
        Geometry &geo = it->second;
        glDeleteVertexArrays(1, &geo.vao);
        glDeleteBuffers(1, &geo.vbo);
        glDeleteBuffers(1, &geo.ebo);
        geometry_map.erase(it);
    }
}

Rml::TextureHandle UI::LoadTexture(Rml::Vector2i &out_dimensions, const Rml::String &source) {

    Texture tex(source, GL_RGBA, GL_REPEAT, GL_LINEAR);
    out_dimensions.x = tex.getWidth();
    out_dimensions.y = tex.getHeight();

    Rml::TextureHandle handle = next_texture_handle++;
    texture_map.emplace(handle, std::move(tex));
    std::cout << "[UI] Loaded texture handle " << handle << " from " << source << std::endl;
    return handle;
}

Rml::TextureHandle UI::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) {
    // Create an empty texture using your Texture class
    Texture tex;
    tex.createEmpty(source_dimensions.x, source_dimensions.y, GL_RGBA8);

    // Upload the pixel data (source.data())
    glTextureSubImage2D(tex.getID(), 0, 0, 0, source_dimensions.x, source_dimensions.y, GL_RGBA, GL_UNSIGNED_BYTE, source.data());

    // Set filtering parameters
    glTextureParameteri(tex.getID(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex.getID(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Rml::TextureHandle handle = next_texture_handle++;
    texture_map.emplace(handle, std::move(tex)); // Store the full Texture object
    std::cout << "[UI] Generated texture handle " << handle << std::endl;
    return handle;
}

void UI::ReleaseTexture(Rml::TextureHandle handle) {
    auto it = texture_map.find(handle);
    if (it != texture_map.end()) {
        std::cout << "[UI] Releasing texture handle " << handle << std::endl;
        texture_map.erase(it);
    } else {
        std::cerr << "[UI] Warning: Attempt to release invalid texture handle " << handle << std::endl;
    }
}

void UI::EnableScissorRegion(bool enable) {
    if (enable)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
}

void UI::SetScissorRegion(Rml::Rectanglei region) {
    if (region.Valid())
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);

    if (region.Valid()) {
        // Some render APIs don't like offscreen positions (WebGL in particular), so clamp them to the viewport.
        const int x = Rml::Math::Clamp(region.Left(), 0, viewport_width);
        const int y = Rml::Math::Clamp(viewport_height - region.Bottom(), 0, viewport_height);

        glScissor(x, y, region.Width(), region.Height());
    }
}

void UI::SetViewportSize(int width, int height) {
    viewport_width = width;
    viewport_height = height;
    projection = glm::ortho(0.0f, (float)viewport_width, (float)viewport_height, 0.0f, -1.0f, 1.0f);
    glViewport(0, 0, width, height);
    shader->setMat4("uProjection", projection);
    context->SetDimensions({width, height});
    context->Update();
}
// Optional functions default implementations

void UI::EnableClipMask(bool enable) {
    clip_mask_enabled = enable;
    if (enable) {
        glEnable(GL_STENCIL_TEST);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
}
void UI::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) {
    auto it = geometry_map.find(geometry);
    if (it == geometry_map.end())
        return;

    Geometry &geo = it->second;

    // Setup transform
    glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, 0.0f));
    glm::mat4 final_model = model * translation_matrix;

    shader->use();
    shader->setMat4("uModel", final_model);
    shader->setMat4("uProjection", projection);
    shader->setInt("uHasTexture", 0); // Mask geometry doesn't use textures

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Don't write colors
    glDepthMask(GL_FALSE);                               // Don't write depth
    glEnable(GL_STENCIL_TEST);
    const bool clear_stencil = (operation == Rml::ClipMaskOperation::Set || operation == Rml::ClipMaskOperation::SetInverse);
    if (clear_stencil) {
        // @performance Increment the reference value instead of clearing each time.
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
    }
    // Configure stencil ops based on operation
    switch (operation) {
    case Rml::ClipMaskOperation::Set:
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF); // Allow writing stencil
        break;

    case Rml::ClipMaskOperation::Intersect:
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_EQUAL, 1, 0xFF); // Keep only where stencil == 1
        glStencilMask(0x00);              // Don’t modify stencil
        break;
    case Rml::ClipMaskOperation::SetInverse:
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 0, 0xFF); // Set to 0
        glStencilMask(0xFF);
        break;
    }

    glBindVertexArray(geo.vao);
    DrawElementsWrapper(GL_TRIANGLES, geo.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Restore state
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0x00); // Prevent further stencil writes unless re-enabled
}

void UI::SetTransform(const Rml::Matrix4f *transform) {
    // Store or reset the current transform matrix for use during RenderGeometry.
    if (transform) {
        // Convert Rml::Matrix4f to glm::mat4 (both column-major)
        std::memcpy(&model[0][0], transform->data(), 16 * sizeof(float));
        shader->setMat4("uModel", model);
    } else {
        model = glm::mat4(1.0f); // Identity matrix
        shader->setMat4("uModel", model);
    }
}

Rml::LayerHandle UI::PushLayer() {
    // We don't support layers, so return 0 for base layer always
    return 0;
}

void UI::CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode, Rml::Span<const Rml::CompiledFilterHandle> filters) {
    // No layers or filters support - just ignore
    (void)source;
    (void)destination;
    (void)blend_mode;
    (void)filters;
}

void UI::PopLayer() {
    // No layers supported, do nothing
}

Rml::TextureHandle UI::SaveLayerAsTexture() {
    // No layer texture support, return 0 (invalid texture handle)
    return 0;
}

Rml::CompiledFilterHandle UI::SaveLayerAsMaskImage() {
    // No filter support, return 0 (invalid filter handle)
    return 0;
}

Rml::CompiledFilterHandle UI::CompileFilter(const Rml::String &name, const Rml::Dictionary &parameters) {
    // No filters supported, return 0
    (void)name;
    (void)parameters;
    return 0;
}

void UI::ReleaseFilter(Rml::CompiledFilterHandle filter) {
    // Nothing to release since we don't compile filters
    (void)filter;
}

Rml::CompiledShaderHandle UI::CompileShader(const Rml::String &name, const Rml::Dictionary &parameters) {
    // No shaders supported beyond the default shader
    (void)name;
    (void)parameters;
    return 0;
}

void UI::RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) {
    // No custom shader support, fallback to RenderGeometry or do nothing
    (void)shader;
    (void)geometry;
    (void)translation;
    (void)texture;
}

void UI::ReleaseShader(Rml::CompiledShaderHandle shader) {
    // Nothing to release since we don't compile shaders
    (void)shader;
}

void UI::render(void) {
    context->Update();
    context->Render();
}

int UI::GetKeyModifiers(void) {
    int modifiers = 0;
    if (isShiftDown)
        modifiers |= Rml::Input::KM_SHIFT;
    if (isCtrlDown)
        modifiers |= Rml::Input::KM_CTRL;
    if (isAltDown)
        modifiers |= Rml::Input::KM_ALT;
    if (isMetaDown)
        modifiers |= Rml::Input::KM_META;
    return modifiers;
}

Rml::Input::KeyIdentifier UI::MapKey(int glfw_key) {

    switch (glfw_key) {
    case GLFW_KEY_A:
        return KI_A;
    case GLFW_KEY_B:
        return KI_B;
    case GLFW_KEY_ENTER:
        return KI_RETURN;
    case GLFW_KEY_ESCAPE:
        return KI_ESCAPE;
    case GLFW_KEY_LEFT:
        return KI_LEFT;
    case GLFW_KEY_RIGHT:
        return KI_RIGHT;
    case GLFW_KEY_UP:
        return KI_UP;
    case GLFW_KEY_DOWN:
        return KI_DOWN;

    default:
        return KI_UNKNOWN;
    }
}
