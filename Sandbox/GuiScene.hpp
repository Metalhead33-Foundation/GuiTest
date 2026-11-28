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
#include <Lotte/Gui/LteGuiViewport.hpp>
#include <Lotte/Gui/LteSlider.hpp>
#include <Lotte/Gui/LteTexturedWidget.hpp>
#include <Lotte/Gui/LteTheme.hpp>
#include <Lotte/Gui/LteThemeDefinition.hpp>
#include <Lotte/Gui/LteWidget.hpp>
#include <Lotte/Procedural/LteGuiAtlas.hpp>
#include <Lotte/Procedural/LteGuiAtlasParts.hpp>
#include <Lotte/Text/LteTextAttributes.hpp>
#include <Lotte/Text/LteTextLayout.hpp>
#include <Lotte/Text/LteTextRenderer.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <Elvavena/Util/ElvSignal.hpp>
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <map>
#include <optional>
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
 * @brief A widget's or label's rectangle in virtual canvas units.
 *
 * The scene keeps every geometry in *canvas* units and derives the real-pixel
 * rectangle through its @ref Lotte::Gui::GuiViewport, so a layout is written once
 * and re-applied whenever the window (or the GUI scale) changes. A script that
 * authors for 800x600 gets the same menu on any window size.
 */
struct CanvasRect {
	/** @brief Top-left corner in canvas units. */
	glm::fvec2 origin = glm::fvec2(0.0f);
	/** @brief Extent in canvas units. */
	glm::fvec2 size = glm::fvec2(0.0f);
};

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
	/**
	 * @brief Draws the current state's quad, if the widget is visible.
	 * @param renderer Renderer to submit the quad to.
	 *
	 * When a theme element is set (@ref setThemeElement) and the theme resolves a skin
	 * for the current state, that skin's rectangle, tint and texture win. Otherwise the
	 * widget falls back to its own style, which keeps an unthemed sandbox working.
	 */
	void render(Lotte::Gui::GuiRenderer& renderer) override;
	///@}

	/**
	 * @brief Draws this button through a theme element instead of its own style.
	 * @param theme Theme to query; null restores the self-contained look.
	 * @param element Element whose skins describe the button (normally `Button`).
	 */
	void setThemeElement(const Lotte::Gui::Theme* theme, Lotte::Gui::ThemeElement element) noexcept;
	/** @brief The theme this button draws through, or null. */
	const Lotte::Gui::Theme* themedBy() const noexcept { return themedTheme; }

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
	/** @brief Theme this button draws through; null keeps @ref style. */
	const Lotte::Gui::Theme* themedTheme = nullptr;
	/** @brief Element whose skins the theme is asked for. */
	Lotte::Gui::ThemeElement themedElement = Lotte::Gui::ThemeElement::Button;
};

/**
 * @brief Horizontal placement of a label's text inside its box.
 */
enum class LabelAlign : std::uint8_t {
	Left,   /**< Text starts at the box's left edge. */
	Centre, /**< Text is centred horizontally. */
	Right   /**< Text ends at the box's right edge. */
};

/**
 * @brief A text block the script owns and the text pass draws.
 *
 * Labels are deliberately *not* `Lotte::Gui::Widget`s: a widget draws through
 * @ref Lotte::Gui::GuiRenderer, while text goes through the SDF pipeline of
 * @ref Lotte::Text::TextRenderer. Keeping them apart means the widget layer needs no
 * font knowledge and the text layer needs no widget knowledge; the scene owns both
 * lists and draws them in order.
 *
 * A label stores its source text, its attributes and a box; the *layout* is produced
 * by the application (which owns the fonts) through
 * @ref GuiScene::setLabelLayoutService(), and this class only signals when it has to
 * be redone via @ref dirty().
 */
class ScriptLabel
{
public:
	/**
	 * @brief Creates a label.
	 * @param text UTF-8 source text.
	 * @param attributes Styling the text is laid out with.
	 */
	ScriptLabel(std::string text, Lotte::Text::TextAttributes attributes);

	/** @brief Replaces the source text and requests a new layout. */
	void setText(std::string newText);
	/** @brief The source text. */
	const std::string& text() const noexcept { return source; }
	/** @brief Replaces the styling and requests a new layout. */
	void setAttributes(const Lotte::Text::TextAttributes& newAttributes);
	/** @brief The styling the text is laid out with. */
	const Lotte::Text::TextAttributes& attributes() const noexcept { return style; }

	/**
	 * @brief Replaces the box the text is centred in.
	 * @param newTopLeft Top-left corner in window pixels.
	 * @param newBottomRight Bottom-right corner in window pixels.
	 */
	void setBox(const glm::fvec2& newTopLeft, const glm::fvec2& newBottomRight);
	/** @brief Top-left corner of the box. */
	const glm::fvec2& topLeft() const noexcept { return boxTopLeft; }
	/** @brief Bottom-right corner of the box. */
	const glm::fvec2& bottomRight() const noexcept { return boxBottomRight; }
	/** @brief Size of the box. */
	glm::fvec2 size() const noexcept { return boxBottomRight - boxTopLeft; }
	/** @brief Replaces the horizontal alignment and requests a new layout. */
	void setAlign(LabelAlign newAlign) noexcept;
	/** @brief Horizontal alignment inside the box. */
	LabelAlign align() const noexcept { return alignment; }
	/** @brief Enables or disables drawing. */
	void setHidden(bool newHidden) noexcept { hiddenFlag = newHidden; }
	/** @brief True when the label is not drawn. */
	bool hidden() const noexcept { return hiddenFlag; }
	/** @brief Global scale applied on top of the attribute size; requests a new layout. */
	void setScale(float newScale) noexcept;
	/** @brief Global scale applied on top of the attribute size. */
	float scale() const noexcept { return layoutScale; }
	/**
	 * @brief Sets the real-pixels-per-canvas-unit factor the scene's viewport asks for.
	 *
	 * Kept apart from @ref setScale() so a script's own multiplier and the resolution
	 * scale do not overwrite each other: the layout service multiplies the two.
	 */
	void setPixelScale(float newScale) noexcept;
	/** @brief Real pixels per canvas unit, as applied to the font size. */
	float pixelScale() const noexcept { return resolutionScale; }

	/** @brief True when the layout has to be recomputed before drawing. */
	bool dirty() const noexcept { return needsLayout; }
	/** @brief Requests a new layout. */
	void invalidate() noexcept { needsLayout = true; }
	/** @brief Adopts a freshly computed layout and clears @ref dirty(). */
	void setLayout(Lotte::Text::TextLayout&& newLayout);
	/** @brief The laid-out glyphs, in window pixels. */
	const Lotte::Text::TextLayout& layout() const noexcept { return placed; }

private:
	/** @brief UTF-8 source text. */
	std::string source;
	/** @brief Styling the text is laid out with. */
	Lotte::Text::TextAttributes style;
	/** @brief Layout result, positioned in window pixels. */
	Lotte::Text::TextLayout placed;
	/** @brief Top-left corner of the box the text is centred in. */
	glm::fvec2 boxTopLeft = glm::fvec2(0.0f);
	/** @brief Bottom-right corner of the box the text is centred in. */
	glm::fvec2 boxBottomRight = glm::fvec2(0.0f);
	/** @brief Horizontal alignment inside the box. */
	LabelAlign alignment = LabelAlign::Centre;
	/** @brief Global scale on top of the attribute size. */
	float layoutScale = 1.0f;
	/** @brief Real pixels per canvas unit, from the scene's viewport. */
	float resolutionScale = 1.0f;
	/** @brief True when @ref placed is stale. */
	bool needsLayout = true;
	/** @brief True when the label is not drawn. */
	bool hiddenFlag = false;
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
	/** @brief Shared ownership of a slider. */
	using SliderPtr = std::shared_ptr<Lotte::Gui::Slider>;
	/** @brief Shared ownership of a text label. */
	using LabelPtr = std::shared_ptr<ScriptLabel>;
	/**
	 * @brief Produces a label's layout.
	 *
	 * The application supplies this because it owns the font repository; the scene
	 * only knows *when* a layout is needed and where the result goes (see
	 * @ref relayoutLabels()). The callable lays the label's text out at the origin,
	 * measured from its own baseline; placement inside the box is the scene's job.
	 */
	using LabelLayoutFunction = std::function<Lotte::Text::TextLayout(const ScriptLabel&)>;

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
	 * @brief Uploads an already decoded image and publishes it under a theme alias.
	 * @param alias Theme alias the resolver answers to.
	 * @param image Tightly packed RGBA8 pixels.
	 * @param manifest Optional atlas manifest, copied; lets part-based skins resolve.
	 * @return True when the image had pixels and a texture was created.
	 */
	bool addAtlas(const std::string& alias, const Lotte::Gfx::Rgba8Image& image,
				  const Lotte::Procedural::GuiAtlas* manifest = nullptr);

	/**
	 * @brief Resolves a theme atlas alias to its handles, size and manifest.
	 * @param alias Alias to look up.
	 * @return The binding, or `std::nullopt` when this scene has no such atlas.
	 */
	std::optional<Lotte::Gui::ThemeAtlasBinding> resolveAtlas(std::string_view alias) const;

	/**
	 * @brief Selects the theme new and existing widgets are drawn through.
	 * @param theme Theme to adopt; null clears it. The caller keeps ownership.
	 */
	void setTheme(const Lotte::Gui::Theme* theme) noexcept;
	/** @brief The active theme, or null. */
	const Lotte::Gui::Theme* theme() const noexcept { return activeTheme; }

	/**
	 * @brief Replaces the canvas-to-window mapping every rectangle is derived from.
	 * @param viewport Viewport to adopt; null restores a 1:1 mapping. Shared ownership:
	 *        the script and the application may keep the same object and mutate it.
	 */
	void setViewport(std::shared_ptr<Lotte::Gui::GuiViewport> viewport);
	/** @brief The viewport in force; never null. */
	const std::shared_ptr<Lotte::Gui::GuiViewport>& viewport() const noexcept { return canvasViewport; }
	/**
	 * @brief Adopts a new framebuffer size and re-applies every rectangle.
	 * @param pixels Real window size, as the renderer reports it.
	 */
	void setResolution(glm::fvec2 pixels);

	/**
	 * @brief Stores a widget's rectangle in canvas units and applies it to the widget.
	 * @param widget Widget to place; it stays owned by the scene.
	 * @param rect Rectangle in canvas units.
	 */
	void setCanvasRect(Lotte::Gui::Widget& widget, const CanvasRect& rect);
	/** @brief The canvas rectangle of a widget, or a zero rectangle when unknown. */
	CanvasRect canvasRect(const Lotte::Gui::Widget& widget) const;
	/**
	 * @brief Stores a label's box in canvas units and applies it to the label.
	 * @param label Label to place.
	 * @param rect Box in canvas units; the text is centred in it.
	 */
	void setCanvasRect(ScriptLabel& label, const CanvasRect& rect);
	/** @brief The canvas box of a label, or a zero rectangle when unknown. */
	CanvasRect canvasRect(const ScriptLabel& label) const;
	/**
	 * @brief Re-derives every pixel rectangle and font scale from the canvas rectangles.
	 *
	 * Called by @ref setViewport(), @ref setResolution() and @ref setCanvasRect(); a
	 * caller that mutated the shared viewport directly can call it to catch up.
	 */
	void applyViewport();

	/**
	 * @brief Registers a button so it is drawn and receives input.
	 * @param button Widget to add; null is ignored.
	 */
	void addButton(ButtonPtr button);
	/**
	 * @brief Registers a slider so it is drawn and receives input.
	 * @param slider Widget to add; null is ignored.
	 */
	void addSlider(SliderPtr slider);
	/**
	 * @brief Registers a label so it is laid out and drawn.
	 * @param label Label to add; null is ignored.
	 */
	void addLabel(LabelPtr label);
	/**
	 * @brief Takes a widget off the screen without destroying it.
	 * @param widget Widget to detach.
	 * @return True when the widget was on screen.
	 */
	bool removeWidget(const Lotte::Gui::Widget* widget);
	/**
	 * @brief Takes a label off the screen without destroying it.
	 * @param label Label to detach.
	 * @return True when the label was on screen.
	 */
	bool removeLabel(const ScriptLabel* label);
	/** @brief Takes every widget and label off the screen. */
	void clear();

	/**
	 * @brief Installs the layout producer for labels.
	 * @param service Callable that lays one label out, or an empty function to disable it.
	 */
	void setLabelLayoutService(LabelLayoutFunction service);
	/**
	 * @brief Lays out every label whose text, styling or box changed.
	 *
	 * Called once per frame by the application, before @ref prepareText().
	 */
	void relayoutLabels();
	/**
	 * @brief Ensures the glyphs of every label exist in the text renderer's atlases.
	 * @param renderer Text renderer to prepare through.
	 * @return True when new glyphs or atlas pages were recorded and have to be submitted
	 *         before the frame opens.
	 */
	bool prepareText(Lotte::Text::TextRenderer& renderer);
	/**
	 * @brief Queues every visible label into a text renderer.
	 * @param renderer Text renderer to submit through; its `flush()` stays the caller's.
	 */
	void renderText(Lotte::Text::TextRenderer& renderer);

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
	/** @brief Number of widgets currently on screen. */
	std::size_t visibleButtonCount() const noexcept { return root.childCount(); }
	/** @brief Number of widgets ever created by this scene. */
	std::size_t buttonCount() const noexcept { return buttons.size(); }
	/** @brief Number of labels this scene owns. */
	std::size_t labelCount() const noexcept { return labels.size(); }

private:
	/** @brief One uploaded theme atlas: its texture, size and optional manifest. */
	struct AtlasEntry {
		/** @brief Owning surface; records its destroy command through the factory. */
		std::unique_ptr<Lotte::Gfx::WriteableTexture2D> texture;
		/** @brief Atlas size in pixels, for the pixel-rect to UV conversion. */
		glm::fvec2 pixelSize = glm::fvec2(0.0f);
		/** @brief Manifest of a generated atlas, or null for a file atlas. */
		std::shared_ptr<const Lotte::Procedural::GuiAtlas> manifest;
	};

	/**
	 * @brief Applies one canvas rectangle to a widget as real pixels.
	 * @param widget Widget to place.
	 * @param rect Rectangle in canvas units.
	 */
	void applyWidgetRect(Lotte::Gui::Widget& widget, const CanvasRect& rect) const;
	/**
	 * @brief Applies one canvas box and the font scale to a label.
	 * @param label Label to place.
	 * @param rect Box in canvas units.
	 */
	void applyLabelRect(ScriptLabel& label, const CanvasRect& rect) const;
	/**
	 * @brief Forgets the placement of a label that is leaving the scene.
	 * @param label Label whose canvas box is no longer tracked.
	 */
	void forgetLabelPlacement(const ScriptLabel* label);

	/** @brief Factory used for every texture this scene creates. */
	Lotte::Gfx::ResourceFactory& factory;
	/** @brief On-screen widgets; the render list and hit testing walk these. */
	Lotte::Gui::RootNode root;
	/** @brief Every widget this scene created, including detached ones. */
	std::vector<std::shared_ptr<Lotte::Gui::Widget>> buttons;
	/** @brief Every label this scene owns. */
	std::vector<LabelPtr> labels;
	/** @brief Lays a label out; supplied by the application, may be empty. */
	LabelLayoutFunction labelLayoutService;
	/** @brief Texture cache keyed by virtual path. */
	std::unordered_map<std::string, TexturePtr> textures;
	/** @brief Uploaded theme atlases keyed by their theme alias. */
	std::map<std::string, AtlasEntry> atlases;
	/** @brief Theme widgets are drawn through, or null. */
	const Lotte::Gui::Theme* activeTheme = nullptr;
	/** @brief Canvas-to-window mapping; never null. */
	std::shared_ptr<Lotte::Gui::GuiViewport> canvasViewport;
	/** @brief A widget and the canvas rectangle it was placed with. */
	struct WidgetPlacement {
		/** @brief The placed widget; the scene's keep-alive list owns it. */
		Lotte::Gui::Widget* widget = nullptr;
		/** @brief Rectangle in canvas units. */
		CanvasRect rect {};
	};
	/** @brief A label and the canvas box it was placed with. */
	struct LabelPlacement {
		/** @brief The placed label; @ref labels owns it. */
		ScriptLabel* label = nullptr;
		/** @brief Box in canvas units. */
		CanvasRect rect {};
	};

	/** @brief Every widget the scene placed, with its canvas rectangle. */
	std::vector<WidgetPlacement> widgetPlacements;
	/** @brief Every label the scene placed, with its canvas box. */
	std::vector<LabelPlacement> labelPlacements;
	/** @brief Last known pointer position in window pixels. */
	glm::fvec2 mouse = glm::fvec2(0.0f);
};

} // namespace Sandbox

#endif // SANDBOX_GUISCENE_HPP
