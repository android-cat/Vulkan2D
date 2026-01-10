#pragma once

// Vulkan2D - A 2D Game Library using Vulkan 1.3
// Main include header

#include "Core/Engine.h"
#include "Core/Window.h"
#include "Renderer/VulkanContext.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/Texture.h"
#include "Renderer/SpriteBatch.h"
#include "Graphics/Color.h"
#include "Graphics/Sprite.h"
#include "Graphics/Camera2D.h"
#include "Graphics/Font.h"
#include "Graphics/TextRenderer.h"
#include "Math/Transform2D.h"
#include "Input/InputManager.h"
#include "Audio/Sound.h"
#include "Audio/AudioManager.h"

namespace V2D {

// Version information
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

inline const char* GetVersionString() {
    return "1.0.0";
}

} // namespace V2D
