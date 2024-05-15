#ifndef MHSTCLFMT_HPP
#define MHSTCLFMT_HPP
#include <MhLib/Util/MhNormDenorm.hpp>
#include <limits>

namespace MH33 {
namespace Image {

template <typename ColourResolution> struct alignas(sizeof(ColourResolution)) RG {
	ColourResolution r,g;
};
template <typename ColourResolution> struct alignas(sizeof(ColourResolution)) RGB {
	ColourResolution r,g,b;
};
template <typename ColourResolution> struct alignas(sizeof(ColourResolution)) BGR {
	ColourResolution b,g,r;
};
template <typename ColourResolution> struct alignas(sizeof(ColourResolution)) RGBA {
	ColourResolution r,g,b,a;
};
template <typename ColourResolution> struct alignas(sizeof(ColourResolution)) BGRA {
	ColourResolution b,g,r,a;
};
template <typename ColourResolution> struct alignas(sizeof(ColourResolution)) ARGB {
	ColourResolution a,r,g,b;
};

template <typename PixelType> struct ColourFormatTraits {
	static constexpr int colour_components() { return std::is_integral<PixelType>() || std::is_floating_point<PixelType>(); }
	static constexpr bool greyscale() { return std::is_integral<PixelType>() || std::is_floating_point<PixelType>(); }
	static constexpr bool has_red() { return false; }
	static constexpr bool has_green() { return false; }
	static constexpr bool has_blue() { return false; }
	static constexpr bool has_alpha() { return false; }
	static constexpr PixelType maximum() { return std::numeric_limits<PixelType>::max(); }
};
template <typename ColourResolution> struct ColourFormatTraits<RG<ColourResolution>> {
	static constexpr int colour_components() { return 2; }
	static constexpr int greyscale() { return false; }
	static constexpr bool has_red() { return true; }
	static constexpr bool has_green() { return true; }
	static constexpr bool has_blue() { return false; }
	static constexpr bool has_alpha() { return false; }
	static constexpr ColourResolution maximum() { return std::numeric_limits<ColourResolution>::max(); }
};
template <typename ColourResolution> struct ColourFormatTraits<RGB<ColourResolution>> {
	static constexpr int colour_components() { return 3; }
	static constexpr int greyscale() { return false; }
	static constexpr bool has_red() { return true; }
	static constexpr bool has_green() { return true; }
	static constexpr bool has_blue() { return true; }
	static constexpr bool has_alpha() { return false; }
	static constexpr ColourResolution maximum() { return std::numeric_limits<ColourResolution>::max(); }
};
template <typename ColourResolution> struct ColourFormatTraits<BGR<ColourResolution>> {
	static constexpr int colour_components() { return 3; }
	static constexpr int greyscale() { return false; }
	static constexpr bool has_red() { return true; }
	static constexpr bool has_green() { return true; }
	static constexpr bool has_blue() { return true; }
	static constexpr bool has_alpha() { return false; }
	static constexpr ColourResolution maximum() { return std::numeric_limits<ColourResolution>::max(); }
};
template <typename ColourResolution> struct ColourFormatTraits<RGBA<ColourResolution>> {
	static constexpr int colour_components() { return 4; }
	static constexpr int greyscale() { return false; }
	static constexpr bool has_red() { return true; }
	static constexpr bool has_green() { return true; }
	static constexpr bool has_blue() { return true; }
	static constexpr bool has_alpha() { return true; }
	static constexpr ColourResolution maximum() { return std::numeric_limits<ColourResolution>::max(); }
};
template <typename ColourResolution> struct ColourFormatTraits<BGRA<ColourResolution>> {
	static constexpr int colour_components() { return 4; }
	static constexpr int greyscale() { return false; }
	static constexpr bool has_red() { return true; }
	static constexpr bool has_green() { return true; }
	static constexpr bool has_blue() { return true; }
	static constexpr bool has_alpha() { return true; }
	static constexpr ColourResolution maximum() { return std::numeric_limits<ColourResolution>::max(); }
};
template <typename ColourResolution> struct ColourFormatTraits<ARGB<ColourResolution>> {
	static constexpr int colour_components() { return 4; }
	static constexpr int greyscale() { return false; }
	static constexpr bool has_red() { return true; }
	static constexpr bool has_green() { return true; }
	static constexpr bool has_blue() { return true; }
	static constexpr bool has_alpha() { return true; }
	static constexpr ColourResolution maximum() { return std::numeric_limits<ColourResolution>::max(); }
};
template <typename PixelType> constexpr int colour_components() { return ColourFormatTraits<PixelType>::colour_components(); }
template <typename PixelType> constexpr int greyscale() { return ColourFormatTraits<PixelType>::greyscale(); }
template <typename PixelType> constexpr bool has_red() { return ColourFormatTraits<PixelType>::has_red(); }
template <typename PixelType> constexpr bool has_green() { return ColourFormatTraits<PixelType>::has_green(); }
template <typename PixelType> constexpr bool has_blue() { return ColourFormatTraits<PixelType>::has_blue(); }
template <typename PixelType> constexpr bool has_alpha() { return ColourFormatTraits<PixelType>::has_alpha(); }
template <typename PixelType> constexpr auto maximum() { return ColourFormatTraits<PixelType>::maximum(); }

template <typename PixelTypeA, typename PixelTypeB> void colour_cast(const PixelTypeA& src, PixelTypeB& dst) {
	if constexpr(greyscale<PixelTypeB>()) {
		if constexpr(greyscale<PixelTypeA>()) {
			Util::normalizing_cast(src,dst);
			return;
		} else {
			switch (colour_components<PixelTypeA>()) {
			case 2:
				dst = Util::fdenormalize<PixelTypeB>((Util::fnormalize(src.r)+Util::fnormalize(src.g))*0.5f);
				break;
			case 3:
			case 4:
				dst =  Util::fdenormalize<PixelTypeB>(( (Util::fnormalize(src.r)*0.2126) +
														(Util::fnormalize(src.g)*0.7152) +
														(Util::fnormalize(src.b)*0.0722) ));
				break;
			default:
				return;
			}
			return;
		}
	}
	if constexpr(greyscale<PixelTypeA>()) {
		if constexpr(has_red<PixelTypeB>()) {
				Util::normalizing_cast(src,dst.r);
		}
		if constexpr(has_red<PixelTypeB>()) {
			Util::normalizing_cast(src,dst.r);
		}
		if constexpr(has_blue<PixelTypeB>()) {
			Util::normalizing_cast(src,dst.g);
		}
		if constexpr(has_green<PixelTypeB>()) {
			Util::normalizing_cast(src,dst.b);
		}
		return;
	}
	if constexpr(has_alpha<PixelTypeB>()) {
		if constexpr(has_alpha<PixelTypeA>()) {
			Util::normalizing_cast(src.a,dst.a);
		} else dst.a = maximum<PixelTypeB>();
	}
	if constexpr(has_red<PixelTypeB>()) {
		if constexpr(has_red<PixelTypeA>()) {
			Util::normalizing_cast(src.r,dst.r);
		} else dst.r = 0;
	}
	if constexpr(has_blue<PixelTypeB>()) {
		if constexpr(has_blue<PixelTypeA>()) {
			Util::normalizing_cast(src.b,dst.b);
		} else dst.b = 0;
	}
	if constexpr(has_green<PixelTypeB>()) {
		if constexpr(has_green<PixelTypeA>()) {
			Util::normalizing_cast(src.g,dst.g);
		} else dst.g = 0;
	}
}

}
}
#endif // MHSTCLFMT_HPP
