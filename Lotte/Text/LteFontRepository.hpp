#ifndef LTEFONTREPOSITORY_HPP
#define LTEFONTREPOSITORY_HPP
/**
 * @file LteFontRepository.hpp
 * @brief Faces, style variants and their lazily created glyph atlases.
 *
 * MML selects a font by family name (`[font=Name]`, CSS `font-family`), and the
 * same text block may mix families. A layout therefore cannot own a single face;
 * it records the face it used per glyph (`TextLayout::faces` /
 * `PlacedGlyph::faceIndex`). This repository is the other half of that contract:
 * it resolves a family (and a bold request) to a face and hands out one
 * `FontAtlas` per face, creating the atlas on first use.
 *
 * @section lte_font_repository_styles Styles
 *
 * `FontStyle` separates *face selection* from *synthesis*. A family may ship a
 * real bold face; when it does, `faceFor(family, true)` returns it. When it does
 * not, the regular face is returned and `TextLayout` marks the run bold so the
 * SDF shader dilates the contour. Italic is always synthetic (a shear in
 * `LteTextLayout`), so it does not affect face selection.
 *
 * @section lte_font_repository_atlas Atlas association
 *
 * One atlas per face is the default. The atlas itself is multi-page, so CJK
 * coverage spills into additional 2D pages instead of requiring an array
 * texture (`maxImageArrayLayers` is far below what a full CJK set needs).
 * Atlases are keyed by the face pointer; the repository keeps the face alive.
 *
 * @warning A repository owns Kaldi textures through its atlases and therefore has
 *          to be destroyed while the resource factory, its command buffer and the
 *          device are still alive. Register or replace faces at startup, before a
 *          layout or a frame references them: replacing an entry releases the old
 *          shared face and its atlas.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteFontAtlas.hpp>
#include <Lotte/Text/LteFontFace.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Lotte {
namespace Text {

/** @brief Selector for a family variant. */
struct FontStyle {
	/** @brief Prefer a bold face when the family provides one. */
	bool bold = false;
	/** @brief Prefer an italic face when the family provides one.
	 *
	 * Reserved: italic is currently synthesized as a layout shear, so face
	 * selection does not consult this flag yet.
	 */
	bool italic = false;

	/** @brief Compares every field. */
	bool operator==(const FontStyle&) const = default;
};

/**
 * @brief Owns preprocessed faces and one lazily created atlas per face.
 *
 * Identifiers and family names are matched case-insensitively and without
 * whitespace, which is how MML parses tag arguments. The registered `id` defaults
 * to the face's own family name, so `faceFor("Vinque")` works without an explicit
 * registration id.
 */
class MH_LOTTE_API FontRepository
{
public:
	/**
	 * @brief Creates an empty repository.
	 * @param factory Factory used to create atlas pages.
	 * @param atlasSettings Page layout shared by every atlas.
	 */
	explicit FontRepository(Gfx::ResourceFactory& factory, FontAtlasSettings atlasSettings = {});
	~FontRepository();

	FontRepository(const FontRepository&) = delete;
	FontRepository& operator=(const FontRepository&) = delete;
	FontRepository(FontRepository&&) = delete;
	FontRepository& operator=(FontRepository&&) = delete;

	/**
	 * @brief Registers a face under its own family name.
	 *
	 * Convenience for the common single-face case. A second variant of the same
	 * family (for example a real bold file) must use an explicit id and the
	 * id+family overload, otherwise it would replace this entry.
	 * @param face Face to adopt.
	 * @param style Variant marker.
	 * @return Identifier the face was registered under.
	 */
	std::string addFace(std::shared_ptr<const PreprocessedFontFace> face, FontStyle style = {});

	/**
	 * @brief Registers a face under an explicit identifier.
	 * @param id Identifier used by @ref face and @ref atlasFor.
	 * @param face Face to adopt.
	 * @param style Variant marker.
	 */
	void addFace(std::string id, std::shared_ptr<const PreprocessedFontFace> face,
				 FontStyle style = {});

	/**
	 * @brief Registers a face under an explicit identifier and family name.
	 *
	 * This is how a bold/italic file whose internal family name differs (for
	 * example `"DejaVu Serif Bold"`) is attached to the family requested by MML
	 * (`"DejaVu Serif"`).
	 * @param id Identifier used by @ref face and @ref atlasFor.
	 * @param familyName Family name used for style resolution.
	 * @param face Face to adopt.
	 * @param style Variant marker.
	 */
	void addFace(std::string id, std::string familyName,
				 std::shared_ptr<const PreprocessedFontFace> face, FontStyle style = {});

	/**
	 * @brief Loads a `.wodf` file through an @ref Elv::Io::System and registers it.
	 * @param id Identifier to register under; empty uses the family name.
	 * @param iosys I/O system that opens @p path (for example PhysFS, a filesystem,
	 *        or a test memory system).
	 * @param path Virtual path resolved by @p iosys.
	 * @param style Variant marker.
	 * @return The loaded face.
	 * @throws std::runtime_error When the system cannot open the file or the
	 *         container cannot be parsed.
	 */
	std::shared_ptr<const PreprocessedFontFace> addFaceFromFile(
		std::string id, Elv::Io::System& iosys, const std::string& path,
		FontStyle style = {});

	/**
	 * @brief Loads a `.wodf` face from an already-open device and registers it.
	 *
	 * The device is read from its current position and is **not** closed or
	 * repositioned by this call. Use this overload when the caller already owns a
	 * mounted/streaming device and does not want a second `open()`.
	 * @param id Identifier to register under; empty uses the family name.
	 * @param device Open readable device.
	 * @param style Variant marker.
	 * @return The loaded face.
	 * @throws std::runtime_error When @p device is invalid or the container cannot
	 *         be parsed.
	 */
	std::shared_ptr<const PreprocessedFontFace> addFaceFromDevice(
		std::string id, Elv::Io::Device& device, FontStyle style = {});

	/** @brief Resolves an identifier, or null. */
	const PreprocessedFontFace* face(std::string_view id) const;
	/** @brief Resolves an identifier to the shared face, or null. */
	std::shared_ptr<const PreprocessedFontFace> sharedFace(std::string_view id) const;

	/**
	 * @brief Returns whether @p face is registered as a real bold variant.
	 *
	 * A synthetic bold pass must not be applied on top of a real bold face, so the
	 * renderer asks this before using `TextStyle::bold` as a dilation.
	 * @param face Face to test; null yields `false`.
	 */
	bool isBoldVariant(const PreprocessedFontFace* face) const;

	/**
	 * @brief Resolves a family and bold request to a face.
	 *
	 * Returns a matching bold variant when one exists, otherwise the regular
	 * variant (the caller synthesizes bold). Falls back to the default face when
	 * the family is unknown.
	 * @param family Family name from the text attributes.
	 * @param bold Whether the run asks for bold.
	 */
	const PreprocessedFontFace* faceFor(std::string_view family, bool bold = false) const;

	/**
	 * @brief Returns the atlas of @p face, creating it on first use.
	 * @param face A face registered with this repository.
	 * @return The atlas, or null when @p face is null.
	 * @throws std::invalid_argument When @p face does not belong to this repository.
	 */
	FontAtlas* atlasFor(const PreprocessedFontFace* face);
	/** @brief Atlas of an identifier, or null. */
	FontAtlas* atlasFor(std::string_view id);
	/** @brief Atlas of a family/style pair, or null. */
	FontAtlas* atlasForFamily(std::string_view family, bool bold = false);

	/**
	 * @brief Sets the face returned when a family cannot be resolved.
	 * @param id Registered identifier.
	 * @throws std::invalid_argument When @p id is unknown.
	 */
	void setDefaultFaceId(std::string id);
	/** @brief Currently selected fallback identifier, or empty. */
	const std::string& defaultFaceId() const noexcept { return defaultFaceId_; }

	/** @brief Number of registered faces. */
	std::size_t faceCount() const noexcept { return entries_.size(); }
	/** @brief Number of atlases created so far. */
	std::size_t atlasCount() const noexcept { return atlases_.size(); }
	/** @brief Factory that records atlas commands. */
	Gfx::ResourceFactory& factory() const noexcept { return *factory_; }
	/** @brief Page layout used for new atlases. */
	const FontAtlasSettings& atlasSettings() const noexcept { return atlasSettings_; }

private:
	/** @brief One registered face. */
	struct Entry {
		/** @brief Caller-facing identifier. */
		std::string id;
		/** @brief Lower-case family key. */
		std::string familyKey;
		/** @brief Variant marker. */
		FontStyle style;
		/** @brief Owning reference to the face. */
		std::shared_ptr<const PreprocessedFontFace> face;
	};

	/** @brief Lower-cases and strips whitespace from a name for lookup. */
	static std::string normalize(std::string_view name);
	/** @brief Finds an entry by identifier. */
	const Entry* findEntry(std::string_view id) const;

	/** @brief Non-owning factory; outlives the repository by contract. */
	Gfx::ResourceFactory* factory_ = nullptr;
	/** @brief Page layout for new atlases. */
	FontAtlasSettings atlasSettings_;
	/** @brief Registered faces; the pointers they own are stable. */
	std::vector<Entry> entries_;
	/** @brief Normalized identifier to entry index. */
	std::unordered_map<std::string, std::size_t> byId_;
	/** @brief Normalized family name to entry indices. */
	std::unordered_map<std::string, std::vector<std::size_t>> byFamily_;
	/** @brief Lazy atlases keyed by the owned face pointer. */
	std::unordered_map<const PreprocessedFontFace*, std::unique_ptr<FontAtlas>> atlases_;
	/** @brief Identifier of the fallback face, or empty. */
	std::string defaultFaceId_;
};

/**
 * @brief Loads one `.wodf` font face through an @ref Elv::Io::System.
 * @param iosys I/O system that opens @p path.
 * @param path Virtual path resolved by @p iosys.
 * @return The parsed face.
 * @throws std::runtime_error When the system cannot open the file or the container
 *         cannot be parsed.
 */
std::shared_ptr<PreprocessedFontFace> MH_LOTTE_API loadFontFace(
	Elv::Io::System& iosys, const std::string& path);

/**
 * @brief Loads one `.wodf` font face from an already-open device.
 *
 * The device is read from its current position and remains owned by the caller.
 * @param device Open readable device.
 * @return The parsed face.
 * @throws std::runtime_error When @p device is invalid or the container cannot be
 *         parsed.
 */
std::shared_ptr<PreprocessedFontFace> MH_LOTTE_API loadFontFace(Elv::Io::Device& device);

} // namespace Text
} // namespace Lotte

#endif // LTEFONTREPOSITORY_HPP
