#include "LteFontRepository.hpp"
#include <Euphemy/Io/EuphFile.hpp>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace Lotte {
namespace Text {

FontRepository::FontRepository(Gfx::ResourceFactory& factory, FontAtlasSettings atlasSettings)
	: factory_(&factory), atlasSettings_(atlasSettings)
{
}

FontRepository::~FontRepository() = default;

std::string FontRepository::normalize(std::string_view name)
{
	std::string result;
	result.reserve(name.size());
	for (const char character : name) {
		const unsigned char value = static_cast<unsigned char>(character);
		// MML removes all whitespace inside a tag, so family names match whether
		// they are written as "Times New Roman" or "timesnewroman".
		if (std::isspace(value)) {
			continue;
		}
		result.push_back(static_cast<char>(std::tolower(value)));
	}
	return result;
}

const FontRepository::Entry* FontRepository::findEntry(std::string_view id) const
{
	const auto found = byId_.find(normalize(id));
	return found == byId_.end() ? nullptr : &entries_[found->second];
}

std::string FontRepository::addFace(std::shared_ptr<const PreprocessedFontFace> face, FontStyle style)
{
	if (!face) {
		throw std::invalid_argument("FontRepository: the font face is null");
	}
	std::string id = face->familyName;
	if (id.empty()) {
		id = "unnamed";
	}
	const std::string registered = id;
	addFace(std::move(id), std::move(face), style);
	return registered;
}

void FontRepository::addFace(std::string id, std::shared_ptr<const PreprocessedFontFace> face,
							 FontStyle style)
{
	if (!face) {
		throw std::invalid_argument("FontRepository: the font face is null");
	}
	// Read the family name before moving the shared pointer into the next overload:
	// argument evaluation order is not guaranteed.
	std::string familyName = face->familyName;
	addFace(std::move(id), std::move(familyName), std::move(face), style);
}

void FontRepository::addFace(std::string id, std::string familyName,
							 std::shared_ptr<const PreprocessedFontFace> face, FontStyle style)
{
	if (!face) {
		throw std::invalid_argument("FontRepository: the font face is null");
	}
	if (familyName.empty()) {
		familyName = face->familyName;
	}
	if (id.empty()) {
		id = familyName.empty() ? "unnamed" : familyName;
	}
	const std::string key = normalize(id);
	const std::string familyKey = normalize(familyName);

	const auto existing = byId_.find(key);
	std::size_t entryIndex = 0;
	if (existing != byId_.end()) {
		entryIndex = existing->second;
		Entry& entry = entries_[entryIndex];
		// Drop an atlas that belongs to the face being replaced; the old pointer is
		// about to become unreachable and the atlas would leak its pages.
		if (entry.face != face) {
			atlases_.erase(entry.face.get());
		}
		if (entry.familyKey != familyKey) {
			auto& oldFamily = byFamily_[entry.familyKey];
			oldFamily.erase(std::remove(oldFamily.begin(), oldFamily.end(), entryIndex),
							oldFamily.end());
		}
		entry.id = std::move(id);
		entry.style = style;
		entry.face = std::move(face);
		entry.familyKey = familyKey;
	} else {
		entryIndex = entries_.size();
		Entry entry;
		entry.id = std::move(id);
		entry.familyKey = familyKey;
		entry.style = style;
		entry.face = std::move(face);
		entries_.push_back(std::move(entry));
		byId_.emplace(key, entryIndex);
	}
	auto& familyEntries = byFamily_[familyKey];
	if (std::find(familyEntries.begin(), familyEntries.end(), entryIndex) == familyEntries.end()) {
		familyEntries.push_back(entryIndex);
	}
	if (defaultFaceId_.empty()) {
		defaultFaceId_ = entries_[entryIndex].id;
	}
}

const PreprocessedFontFace* FontRepository::face(std::string_view id) const
{
	const Entry* entry = findEntry(id);
	return entry == nullptr ? nullptr : entry->face.get();
}

std::shared_ptr<const PreprocessedFontFace> FontRepository::sharedFace(std::string_view id) const
{
	const Entry* entry = findEntry(id);
	return entry == nullptr ? nullptr : entry->face;
}

const PreprocessedFontFace* FontRepository::faceFor(std::string_view family, bool bold) const
{
	const auto found = byFamily_.find(normalize(family));
	if (found != byFamily_.end() && !found->second.empty()) {
		const Entry* regular = nullptr;
		for (const std::size_t index : found->second) {
			const Entry& entry = entries_[index];
			if (entry.style.bold == bold) {
				return entry.face.get();
			}
			if (!entry.style.bold && regular == nullptr) {
				regular = &entry;
			}
		}
		if (regular != nullptr) {
			return regular->face.get();
		}
		return entries_[found->second.front()].face.get();
	}
	if (!defaultFaceId_.empty()) {
		if (const PreprocessedFontFace* fallback = face(defaultFaceId_)) {
			return fallback;
		}
	}
	return entries_.empty() ? nullptr : entries_.front().face.get();
}

bool FontRepository::isBoldVariant(const PreprocessedFontFace* face) const
{
	if (face == nullptr) {
		return false;
	}
	for (const Entry& entry : entries_) {
		if (entry.face.get() == face && entry.style.bold) {
			return true;
		}
	}
	return false;
}

FontAtlas* FontRepository::atlasFor(const PreprocessedFontFace* face)
{
	if (face == nullptr) {
		return nullptr;
	}
	const auto existing = atlases_.find(face);
	if (existing != atlases_.end()) {
		return existing->second.get();
	}
	const Entry* owner = nullptr;
	for (const Entry& entry : entries_) {
		if (entry.face.get() == face) {
			owner = &entry;
			break;
		}
	}
	if (owner == nullptr) {
		throw std::invalid_argument("FontRepository: the face is not registered");
	}
	auto atlas = std::make_unique<FontAtlas>(*factory_, owner->face, atlasSettings_);
	FontAtlas* raw = atlas.get();
	atlases_.emplace(face, std::move(atlas));
	return raw;
}

FontAtlas* FontRepository::atlasFor(std::string_view id)
{
	return atlasFor(face(id));
}

FontAtlas* FontRepository::atlasForFamily(std::string_view family, bool bold)
{
	return atlasFor(faceFor(family, bold));
}

void FontRepository::setDefaultFaceId(std::string id)
{
	const Entry* entry = findEntry(id);
	if (entry == nullptr) {
		throw std::invalid_argument("FontRepository: unknown fallback face");
	}
	defaultFaceId_ = entry->id;
}

std::shared_ptr<const PreprocessedFontFace> FontRepository::addFaceFromFile(std::string id,
																			 const std::string& path,
																			 FontStyle style)
{
	std::shared_ptr<PreprocessedFontFace> loaded = loadFontFace(path);
	if (id.empty()) {
		id = loaded->familyName;
	}
	addFace(id, loaded, style);
	return loaded;
}

std::shared_ptr<PreprocessedFontFace> loadFontFace(const std::string& path)
{
	Euph::Io::File file(path.c_str(), Elv::Io::Mode::READ);
	if (!file.isValid()) {
		throw std::runtime_error("FontRepository: cannot open '" + path + "'");
	}
	auto face = std::make_shared<PreprocessedFontFace>();
	FontDataStream<> stream(file);
	stream >> *face;
	return face;
}

} // namespace Text
} // namespace Lotte