#include "ColourHelper.hpp"
#include <sstream>
#include <map>
#include "PixelFormat.hpp"

glm::fvec4 htmlColour(const std::string& colourname)
{
	glm::fvec4 kernel(0.0f, 0.0f, 0.0f, 1.0f);
	htmlColour(colourname,kernel);
	return kernel;
}

static const std::map<std::string, PixelRGB_U8> namedColours = {
	{ "aliceblue", PixelRGB_U8{ .r = 240, .g = 248, .b = 255 } },
	{ "antiquewhite", PixelRGB_U8{ .r = 250, .g = 235, .b = 215 } },
	{ "aqua", PixelRGB_U8{ .r = 0, .g = 255, .b = 255 } },
	{ "aquamarine", PixelRGB_U8{ .r = 127, .g = 255, .b = 212 } },
	{ "azure", PixelRGB_U8{ .r = 240, .g = 255, .b = 255 } },
	{ "beige", PixelRGB_U8{ .r = 245, .g = 245, .b = 220 } },
	{ "bisque", PixelRGB_U8{ .r = 255, .g = 228, .b = 196 } },
	{ "black", PixelRGB_U8{ .r = 0, .g = 0, .b = 0 } },
	{ "blanchedalmond", PixelRGB_U8{ .r = 255, .g = 235, .b = 205 } },
	{ "blue", PixelRGB_U8{ .r = 0, .g = 0, .b = 255 } },
	{ "blueviolet", PixelRGB_U8{ .r = 138, .g = 43, .b = 226 } },
	{ "brown", PixelRGB_U8{ .r = 165, .g = 42, .b = 42 } },
	{ "burlywood", PixelRGB_U8{ .r = 222, .g = 184, .b = 135 } },
	{ "cadetblue", PixelRGB_U8{ .r = 95, .g = 158, .b = 160 } },
	{ "chartreuse", PixelRGB_U8{ .r = 127, .g = 255, .b = 0 } },
	{ "chocolate", PixelRGB_U8{ .r = 210, .g = 105, .b = 30 } },
	{ "coral", PixelRGB_U8{ .r = 255, .g = 127, .b = 80 } },
	{ "cornflowerblue", PixelRGB_U8{ .r = 100, .g = 149, .b = 237 } },
	{ "cornsilk", PixelRGB_U8{ .r = 255, .g = 248, .b = 220 } },
	{ "crimson", PixelRGB_U8{ .r = 220, .g = 20, .b = 60 } },
	{ "cyan", PixelRGB_U8{ .r = 0, .g = 255, .b = 255 } },
	{ "darkblue", PixelRGB_U8{ .r = 0, .g = 0, .b = 139 } },
	{ "darkcyan", PixelRGB_U8{ .r = 0, .g = 139, .b = 139 } },
	{ "darkgoldenrod", PixelRGB_U8{ .r = 184, .g = 134, .b = 11 } },
	{ "darkgray", PixelRGB_U8{ .r = 169, .g = 169, .b = 169 } },
	{ "darkgreen", PixelRGB_U8{ .r = 0, .g = 100, .b = 0 } },
	{ "darkkhaki", PixelRGB_U8{ .r = 189, .g = 183, .b = 107 } },
	{ "darkmagenta", PixelRGB_U8{ .r = 139, .g = 0, .b = 139 } },
	{ "darkolivegreen", PixelRGB_U8{ .r = 85, .g = 107, .b = 47 } },
	{ "darkorange", PixelRGB_U8{ .r = 255, .g = 140, .b = 0 } },
	{ "darkorchid", PixelRGB_U8{ .r = 153, .g = 50, .b = 204 } },
	{ "darkred", PixelRGB_U8{ .r = 139, .g = 0, .b = 0 } },
	{ "darksalmon", PixelRGB_U8{ .r = 233, .g = 150, .b = 122 } },
	{ "darkseagreen", PixelRGB_U8{ .r = 143, .g = 188, .b = 139 } },
	{ "darkslateblue", PixelRGB_U8{ .r = 72, .g = 61, .b = 139 } },
	{ "darkslategray", PixelRGB_U8{ .r = 47, .g = 79, .b = 79 } },
	{ "darkturquoise", PixelRGB_U8{ .r = 0, .g = 206, .b = 209 } },
	{ "darkviolet", PixelRGB_U8{ .r = 148, .g = 0, .b = 211 } },
	{ "deeppink", PixelRGB_U8{ .r = 255, .g = 20, .b = 147 } },
	{ "deepskyblue", PixelRGB_U8{ .r = 0, .g = 191, .b = 255 } },
	{ "dimgray", PixelRGB_U8{ .r = 105, .g = 105, .b = 105 } },
	{ "dodgerblue", PixelRGB_U8{ .r = 30, .g = 144, .b = 255 } },
	{ "firebrick", PixelRGB_U8{ .r = 178, .g = 34, .b = 34 } },
	{ "floralwhite", PixelRGB_U8{ .r = 255, .g = 250, .b = 240 } },
	{ "forestgreen", PixelRGB_U8{ .r = 34, .g = 139, .b = 34 } },
	{ "fuchsia", PixelRGB_U8{ .r = 255, .g = 0, .b = 255 } },
	{ "gainsboro", PixelRGB_U8{ .r = 220, .g = 220, .b = 220 } },
	{ "ghostwhite", PixelRGB_U8{ .r = 248, .g = 248, .b = 255 } },
	{ "gold", PixelRGB_U8{ .r = 255, .g = 215, .b = 0 } },
	{ "goldenrod", PixelRGB_U8{ .r = 218, .g = 165, .b = 32 } },
	{ "gray", PixelRGB_U8{ .r = 128, .g = 128, .b = 128 } },
	{ "green", PixelRGB_U8{ .r = 0, .g = 128, .b = 0 } },
	{ "greenyellow", PixelRGB_U8{ .r = 173, .g = 255, .b = 47 } },
	{ "honeydew", PixelRGB_U8{ .r = 240, .g = 255, .b = 240 } },
	{ "hotpink", PixelRGB_U8{ .r = 255, .g = 105, .b = 180 } },
	{ "indianred", PixelRGB_U8{ .r = 205, .g = 92, .b = 92 } },
	{ "indigo", PixelRGB_U8{ .r = 75, .g = 0, .b = 130 } },
	{ "ivory", PixelRGB_U8{ .r = 255, .g = 255, .b = 240 } },
	{ "khaki", PixelRGB_U8{ .r = 240, .g = 230, .b = 140 } },
	{ "lavender", PixelRGB_U8{ .r = 230, .g = 230, .b = 250 } },
	{ "lavenderblush", PixelRGB_U8{ .r = 255, .g = 240, .b = 245 } },
	{ "lawngreen", PixelRGB_U8{ .r = 124, .g = 252, .b = 0 } },
	{ "lemonchiffon", PixelRGB_U8{ .r = 255, .g = 250, .b = 205 } },
	{ "lightblue", PixelRGB_U8{ .r = 173, .g = 216, .b = 230 } },
	{ "lightcoral", PixelRGB_U8{ .r = 240, .g = 128, .b = 128 } },
	{ "lightcyan", PixelRGB_U8{ .r = 224, .g = 255, .b = 255 } },
	{ "lightgoldenrodyellow", PixelRGB_U8{ .r = 250, .g = 250, .b = 210 } },
	{ "lightgray", PixelRGB_U8{ .r = 211, .g = 211, .b = 211 } },
	{ "lightgreen", PixelRGB_U8{ .r = 144, .g = 238, .b = 144 } },
	{ "lightpink", PixelRGB_U8{ .r = 255, .g = 182, .b = 193 } },
	{ "lightsalmon", PixelRGB_U8{ .r = 255, .g = 160, .b = 122 } },
	{ "lightsalmon", PixelRGB_U8{ .r = 255, .g = 160, .b = 122 } },
	{ "lightseagreen", PixelRGB_U8{ .r = 32, .g = 178, .b = 170 } },
	{ "lightskyblue", PixelRGB_U8{ .r = 135, .g = 206, .b = 250 } },
	{ "lightslategray", PixelRGB_U8{ .r = 119, .g = 136, .b = 153 } },
	{ "lightsteelblue", PixelRGB_U8{ .r = 176, .g = 196, .b = 222 } },
	{ "lightyellow", PixelRGB_U8{ .r = 255, .g = 255, .b = 224 } },
	{ "lime", PixelRGB_U8{ .r = 0, .g = 255, .b = 0 } },
	{ "limegreen", PixelRGB_U8{ .r = 50, .g = 205, .b = 50 } },
	{ "linen", PixelRGB_U8{ .r = 250, .g = 240, .b = 230 } },
	{ "magenta", PixelRGB_U8{ .r = 255, .g = 0, .b = 255 } },
	{ "maroon", PixelRGB_U8{ .r = 128, .g = 0, .b = 0 } },
	{ "mediumaquamarine", PixelRGB_U8{ .r = 102, .g = 205, .b = 170 } },
	{ "mediumblue", PixelRGB_U8{ .r = 0, .g = 0, .b = 205 } },
	{ "mediumorchid", PixelRGB_U8{ .r = 186, .g = 85, .b = 211 } },
	{ "mediumpurple", PixelRGB_U8{ .r = 147, .g = 112, .b = 219 } },
	{ "mediumseagreen", PixelRGB_U8{ .r = 60, .g = 179, .b = 113 } },
	{ "mediumslateblue", PixelRGB_U8{ .r = 123, .g = 104, .b = 238 } },
	{ "mediumslateblue", PixelRGB_U8{ .r = 123, .g = 104, .b = 238 } },
	{ "mediumspringgreen", PixelRGB_U8{ .r = 0, .g = 250, .b = 154 } },
	{ "mediumturquoise", PixelRGB_U8{ .r = 72, .g = 209, .b = 204 } },
	{ "mediumvioletred", PixelRGB_U8{ .r = 199, .g = 21, .b = 133 } },
	{ "midnightblue", PixelRGB_U8{ .r = 25, .g = 25, .b = 112 } },
	{ "mintcream", PixelRGB_U8{ .r = 245, .g = 255, .b = 250 } },
	{ "mistyrose", PixelRGB_U8{ .r = 255, .g = 228, .b = 225 } },
	{ "moccasin", PixelRGB_U8{ .r = 255, .g = 228, .b = 181 } },
	{ "navajowhite", PixelRGB_U8{ .r = 255, .g = 222, .b = 173 } },
	{ "navy", PixelRGB_U8{ .r = 0, .g = 0, .b = 128 } },
	{ "oldlace", PixelRGB_U8{ .r = 253, .g = 245, .b = 230 } },
	{ "olive", PixelRGB_U8{ .r = 128, .g = 128, .b = 0 } },
	{ "olivedrab", PixelRGB_U8{ .r = 107, .g = 142, .b = 35 } },
	{ "orange", PixelRGB_U8{ .r = 255, .g = 165, .b = 0 } },
	{ "orangered", PixelRGB_U8{ .r = 255, .g = 69, .b = 0 } },
	{ "orchid", PixelRGB_U8{ .r = 218, .g = 112, .b = 214 } },
	{ "palegoldenrod", PixelRGB_U8{ .r = 238, .g = 232, .b = 170 } },
	{ "palegreen", PixelRGB_U8{ .r = 152, .g = 251, .b = 152 } },
	{ "paleturquoise", PixelRGB_U8{ .r = 175, .g = 238, .b = 238 } },
	{ "palevioletred", PixelRGB_U8{ .r = 219, .g = 112, .b = 147 } },
	{ "papayawhip", PixelRGB_U8{ .r = 255, .g = 239, .b = 213 } },
	{ "peachpuff", PixelRGB_U8{ .r = 255, .g = 218, .b = 185 } },
	{ "peru", PixelRGB_U8{ .r = 205, .g = 133, .b = 63 } },
	{ "pink", PixelRGB_U8{ .r = 255, .g = 192, .b = 203 } },
	{ "plum", PixelRGB_U8{ .r = 221, .g = 160, .b = 221 } },
	{ "powderblue", PixelRGB_U8{ .r = 176, .g = 224, .b = 230 } },
	{ "purple", PixelRGB_U8{ .r = 128, .g = 0, .b = 128 } },
	{ "rebeccapurple", PixelRGB_U8{ .r = 102, .g = 51, .b = 153 } },
	{ "red", PixelRGB_U8{ .r = 255, .g = 0, .b = 0 } },
	{ "rosybrown", PixelRGB_U8{ .r = 188, .g = 143, .b = 143 } },
	{ "royalblue", PixelRGB_U8{ .r = 65, .g = 105, .b = 225 } },
	{ "saddlebrown", PixelRGB_U8{ .r = 139, .g = 69, .b = 19 } },
	{ "salmon", PixelRGB_U8{ .r = 250, .g = 128, .b = 114 } },
	{ "sandybrown", PixelRGB_U8{ .r = 244, .g = 164, .b = 96 } },
	{ "seagreen", PixelRGB_U8{ .r = 46, .g = 139, .b = 87 } },
	{ "seashell", PixelRGB_U8{ .r = 255, .g = 245, .b = 238 } },
	{ "sienna", PixelRGB_U8{ .r = 160, .g = 82, .b = 45 } },
	{ "silver", PixelRGB_U8{ .r = 192, .g = 192, .b = 192 } },
	{ "skyblue", PixelRGB_U8{ .r = 135, .g = 206, .b = 235 } },
	{ "slateblue", PixelRGB_U8{ .r = 106, .g = 90, .b = 205 } },
	{ "slategray", PixelRGB_U8{ .r = 112, .g = 128, .b = 144 } },
	{ "snow", PixelRGB_U8{ .r = 255, .g = 250, .b = 250 } },
	{ "springgreen", PixelRGB_U8{ .r = 0, .g = 255, .b = 127 } },
	{ "steelblue", PixelRGB_U8{ .r = 70, .g = 130, .b = 180 } },
	{ "tan", PixelRGB_U8{ .r = 210, .g = 180, .b = 140 } },
	{ "teal", PixelRGB_U8{ .r = 0, .g = 128, .b = 128 } },
	{ "thistle", PixelRGB_U8{ .r = 216, .g = 191, .b = 216 } },
	{ "tomato", PixelRGB_U8{ .r = 255, .g = 99, .b = 71 } },
	{ "turquoise", PixelRGB_U8{ .r = 64, .g = 224, .b = 208 } },
	{ "violet", PixelRGB_U8{ .r = 238, .g = 130, .b = 238 } },
	{ "wheat", PixelRGB_U8{ .r = 245, .g = 222, .b = 179 } },
	{ "white", PixelRGB_U8{ .r = 255, .g = 255, .b = 255 } },
	{ "whitesmoke", PixelRGB_U8{ .r = 245, .g = 245, .b = 245 } },
	{ "yellow", PixelRGB_U8{ .r = 255, .g = 255, .b = 0 } },
	{ "yellowgreen", PixelRGB_U8{ .r = 154, .g = 205, .b = 50 } }
};

void htmlColour(const std::string& colourname, glm::fvec4& colourKernel)
{
	if(colourname[0] == '#' ) {
		// Hexadecimal colour tag, e.g. "#00FF00"
		PixelRGBA_U8 pix{ .r = 0, .g = 0, .b = 0, .a = 255 };
		if(colourname.length() == 7) {
			unsigned r,g,b;
			std::sscanf(colourname.c_str(), "#%02x%02x%02x", &r, &g, &b);
			pix.r = r;
			pix.g = g;
			pix.b = b;
		}
		else if(colourname.length() == 9) {
			unsigned r,g,b,a;
			std::sscanf(colourname.c_str(), "#%02x%02x%02x%02x", &r, &g, &b, &a);
			pix.r = r;
			pix.g = g;
			pix.b = b;
			pix.a = a;
		}
		pix.toKernel(colourKernel);
	} else if(colourname.find('.') != std::string::npos && colourname.find(',') != std::string::npos ) {
		// Normalized colour tag, e.g. "0.332, 0.722, 1.000"
		std::stringstream numStream(colourname);
		std::string immediate;
		uint_fast8_t parsedClr = 0;
		while(std::getline(numStream, immediate, ',') && parsedClr < 4)
		{
			colourKernel[parsedClr] = std::stof(immediate);
			++parsedClr;
		}
	} else {
		// Named colour
		auto namedColour = namedColours.find(colourname);
		if(namedColour != std::end(namedColours)) namedColour->second.toKernel(colourKernel);
	}
}
