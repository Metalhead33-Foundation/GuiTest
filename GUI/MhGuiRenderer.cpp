#include "MhGuiRenderer.hpp"
namespace MH33 {
namespace GUI {

void Renderer::render(const std::span<const ColouredQuad>& target)
{
	for(const auto& it : target) {
		render(it);
	}
}

void Renderer::render(const std::span<const GradientQuad>& target)
{
	for(const auto& it : target) {
		render(it);
	}
}

void Renderer::render(const std::span<const ColouredTriangle>& target)
{
	for(const auto& it : target) {
		render(it);
	}
}

void Renderer::render(const std::span<const GradientTriangle>& target)
{
	for(const auto& it : target) {
		render(it);
	}
}

void Renderer::render(const std::span<const ColouredLine>& target)
{
	for(const auto& it : target) {
		render(it);
	}
}

void Renderer::render(const std::span<const GradientLine>& target)
{
	for(const auto& it : target) {
		render(it);
	}
}

void Renderer::render(const std::span<const TexturedQuad>& target)
{
	for(const auto& it : target) {
		render(it);
	}
}

void Renderer::render(const std::span<const ColouredTexturedQuad>& target)
{
	for(const auto& it : target) {
		render(it);
	}
}

void Renderer::render(const std::span<const GradientTexturedQuad>& target)
{
	for(const auto& it : target) {
		render(it);
	}
}

}
}
