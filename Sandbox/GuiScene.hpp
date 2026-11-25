#ifndef SANDBOX_GUISCENE_HPP
#define SANDBOX_GUISCENE_HPP
/**
 * @file GuiScene.hpp
 * @brief Engine-side GUI objects that a sandbox script creates and drives.
 *
 * The script never touches a `Lotte::Gui` object directly. It asks this scene to
 * make one (`loadTexture()`, `addButton()`), keeps the returned `shared_ptr` in
 * its wrapper object, and reads its state back through accessors. That split is
 * what keeps the JavaScript half disposable: when the temporary SpiderMonkey
 * layer is replaced, the widgets, the input routing and the render path stay.
 *
 * Ownership rules:
 *
 * - **Textures** are cached by path and shared. A widget copies the
 *   @ref Lotte::Gui::GuiTextureBinding (a handle pair), so a button never owns a
 *   GPU resource and a texture can outlive the button that used it.
 * - **Widgets** are owned by this scene for as long as it lives, in addition to
 *   whatever the caller holds. Removing a button takes it off the screen but does
 *   not destroy it, which means a script handle can never dangle.
 * - **Lifetime** is tied to the `Lotte::Gfx::ResourceFactory`: textures and their
 *   widgets must be released before the factory's dispatcher and device, so the
 *   scene has to be destroyed first.
 *
 * The scene does not poll SDL. `GameSystem` forwards the mouse position and the
 * button transitions; hit testing is this class's job because it owns the widget
 * rectangles.
 */
#include <Lotte/GFX/LteResourceFactory.hpp>
#include <Lotte/GFX/LteWriteableTexture2D.hpp>
#include <Lotte/Gui/LteGuiRenderable.hpp>
#include <Lotte/Gui/LteGuiRenderer.hpp>
#include <Lotte/Gui/LteGuiRootNode.hpp>
#include <Lotte/Gui/LteTexturedWidget.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <Elvavena/Util/ElvSignal.hpp>
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Sandbox {

/** @brief The four looks a @ref ScriptButton distinguishes. */
enum class ButtonVisual : std::uint8_t {
	Idle,     /**< Enabled, not hovered, not pressed. */
	Hover,    /**< Enabled and hovered. */
	Pressed,  /**< Enabled and held down. */
	Disabled  /**< The `Enabled` state bit is clear. */
};

/** @brief Number of @ref ButtonVisual entries. */
inline constexpr std::size_t ButtonVisualCount = 4;

/**
 * @brief Colours and atlas rectangles a @ref ScriptButton draws with.
 *
 * `colours` is indexed by @ref ButtonVisual and is used as the fill of an
 * untextured button, or as the tint of a textured one. `coordinates` is the
 * per-state table @ref Lotte::Gui::TexturedWidget already consumes, so the
 * widget keeps working if a future texture atlas supplies one rectangle per
 * state combination.
 */
struct ButtonStyle {
	/** @brief Fill/tint per visual state. */
	std::array<glm::fvec4, ButtonVisualCount> colours = {
		glm::fvec4(0.20f, 0.27f, 0.33f, 1.0f),
		glm::fvec4(0.29f, 0.38f, 0.47f, 1.0f),
		glm::fvec4(0.13f, 0.18f, 0.24f, 1.0f),
		glm::fvec4(0.15f, 0.15f, 0.15f, 0.6f)
	};
	/** @brief Atlas rectangle per @ref Lotte::Gui::Widget::stateIndex(). */
	std::array<Lotte::Gui::TextureCoordDuo, Lotte::Gui::WidgetStateCount> coordinates {};
};

/**
 * @brief A decoded PNG mirrored into a Kaldi texture, shared by widgets.
 *
 * The object owns the CPU pixels as well as the GPU handle, so it has to outlive
 * every recorded upload command and every draw that samples it.
 */
class ScriptTexture
{
public:
	/**
	 * @brief Adopts an uploaded surface.
	 * @param texture Owning surface; must be valid.
	 * @param path Virtual path the image was read from, used for caching and logs.
	 */
	ScriptTexture(std::unique_ptr<Lotte::Gfx::WriteableTexture2D> texture, std::string path);

	/** @brief The handle pair a widget binds. */
	const Lotte::Gui::GuiTextureBinding& binding() const noexcept { return bindingInfo; }
	/** @brief Image width in pixels. */
	std::uint32_t width() const noexcept;
	/** @brief Image height in pixels. */
	std::uint32_t height() const noexcept;
	/** @brief Virtual path the image was decoded from. */
	const std::string& path() const noexcept { return sourcePath; }

private:
	/** @brief The owning RGBA8 surface and its GPU handle. */
	std::unique_ptr<Lotte::Gfx::WriteableTexture2D> texture;
	/** @brief The handle pair copied into every widget that uses this image. */
	Lotte::Gui::GuiTextureBinding bindingInfo {};
	/** @brief Where the image came from. */
	std::string sourcePath;
};

/**
 * @brief A push button that reports completed clicks to its listeners.
 *
 * The visual state is the widget's own bitmask: `Enabled` selects between the
 * normal and the disabled look, `InMouseFocus` is hover, and `Clicked` is the
 * held-down state. A click is *completed* when a press and its release both
 * happen inside the rectangle; @ref activated is emitted exactly then, which is
 * the event a script's `onClick` handler corresponds to. Dragging off the button
 * before releasing cancels instead of firing.
 *
 * The button is both textured and coloured on purpose: with a texture it draws a
 * tinted @ref Lotte::Gui::ColouredTexturedQuad, without one a plain
 * @ref Lotte::Gui::ColouredQuad. That lets a script skin a button from a PNG
 * when one is available and still get a visible control when it is not.
 */
class ScriptButton final : public Lotte::Gui::TexturedWidget
{
public:
	/**
	 * @brief Creates a button with a style and an optional texture.
	 * @param binding Texture and sampler to sample; a zero texture means "untextured".
	 * @param style Colours and per-state atlas rectangles.
	 */
	ScriptButton(Lotte::Gui::GuiTextureBinding binding, const ButtonStyle& style);

	/// @name Widget
	///@{
	/**
	 * @brief Applies a press or a release.
	 * @param offset Click position relative to the top-left corner; a press and its
	 *        release only count when this lies inside the rectangle.
	 * @param button Backend mouse-button index; unused.
	 * @param state Zero for a release, non-zero for a press.
	 * @param clicks Click count of the event; unused.
	 * @return Always true: a button consumes the event.
	 */
	bool onClick(const glm::fvec2& offset, std::uint8_t button,
				 std::uint8_t state, std::uint8_t clicks) override;
	/** @brief Draws the current state's quad, if the widget is visible. */
	void render(Lotte::Gui::GuiRenderer& renderer) override;
	///@}

	/**
	 * @brief Sets or clears the hover bit.
	 * @param hovered True when the pointer is over the button.
	 */
	void setHovered(bool hovered);
	/** @brief True while the primary button is held on this widget. */
	bool pressed() const noexcept;
	/** @brief The colours and atlas rectangles this button draws with. */
	const ButtonStyle& buttonStyle() const noexcept { return style; }
	/**
	 * @brief Replaces the style of a live button.
	 * @param newStyle Colours and per-state atlas rectangles to adopt.
	 */
	void setButtonStyle(const ButtonStyle& newStyle);

	/** @brief Emitted when a press and its release both landed inside the button. */
	Elv::Util::Signal<> activated;

private:
	/** @brief The colour or tint that matches the current state bitmask. */
	glm::fvec4 currentTint() const noexcept;

	/** @brief Colours and atlas rectangles. */
	ButtonStyle style;
	/** @brief True between a press inside the rectangle and its release. */
	bool pressInside = false;
};

/**
 * @brief Ownership root, hit tester and renderer of the script-created widgets.
 *
 * See the file comment for the ownership rules. The class is deliberately not a
 * `Lotte::Gui::Widget`: it is a scene, not a drawable node.
 */
class GuiScene
{
public:
	/** @brief Shared ownership of a decoded texture. */
	using TexturePtr = std::shared_ptr<ScriptTexture>;
	/** @brief Shared ownership of a scripted button. */
	using ButtonPtr = std::shared_ptr<ScriptButton>;

	/**
	 * @brief Creates an empty scene.
	 * @param factory Factory that records the texture create/destroy commands.
	 */
	explicit GuiScene(Lotte::Gfx::ResourceFactory& factory);

	/**
	 * @brief Decodes a PNG and caches the resulting texture by path.
	 * @param io Filesystem the image is read from.
	 * @param path Virtual path of the image.
	 * @return The cached or freshly decoded texture; never null.
	 * @throws std::runtime_error When the file cannot be opened or decoded.
	 */
	TexturePtr loadTexture(Elv::Io::System& io, const std::string& path);

	/**
	 * @brief Registers a button so it is drawn and receives input.
	 * @param button Widget to add; null is ignored.
	 */
	void addButton(ButtonPtr button);
	/**
	 * @brief Takes a button off the screen without destroying it.
	 * @param button Widget to detach.
	 * @return True when the button was on screen.
	 */
	bool removeButton(const ScriptButton* button);
	/** @brief Takes every button off the screen. */
	void clear();

	/**
	 * @brief Updates the hover state of every on-screen button.
	 * @param position Pointer position in window pixels.
	 */
	void setMousePosition(glm::fvec2 position);
	/**
	 * @brief Delivers a mouse-button transition to the button under the pointer.
	 * @param position Pointer position in window pixels.
	 * @param button Backend mouse-button index.
	 * @param down True for a press, false for a release.
	 * @param clicks Click count of the event.
	 */
	void setMouseButton(glm::fvec2 position, std::uint8_t button, bool down,
						std::uint8_t clicks);

	/**
	 * @brief Draws every on-screen widget in accumulated z-index order.
	 * @param renderer Renderer to submit the quads to.
	 */
	void render(Lotte::Gui::GuiRenderer& renderer);
	/** @brief Last known pointer position in window pixels. */
	const glm::fvec2& mousePosition() const noexcept { return mouse; }
	/** @brief Number of buttons currently on screen. */
	std::size_t visibleButtonCount() const noexcept { return root.childCount(); }
	/** @brief Number of buttons ever created by this scene. */
	std::size_t buttonCount() const noexcept { return buttons.size(); }

private:
	/** @brief Factory used for every texture this scene creates. */
	Lotte::Gfx::ResourceFactory& factory;
	/** @brief On-screen widgets; the render list and hit testing walk these. */
	Lotte::Gui::RootNode root;
	/** @brief Every widget this scene created, including detached ones. */
	std::vector<ButtonPtr> buttons;
	/** @brief Texture cache keyed by virtual path. */
	std::unordered_map<std::string, TexturePtr> textures;
	/** @brief Last known pointer position in window pixels. */
	glm::fvec2 mouse = glm::fvec2(0.0f);
};

} // namespace Sandbox

#endif // SANDBOX_GUISCENE_HPP
