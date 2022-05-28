#include "RichTextProcessor.hpp"
#include "../Util/NormDenorm.hpp"

void RichTextProcessor::flush()
{
	auto tmpStr = sstrm.str();
	if(!tmpStr.empty()) {
		sstrm.str(std::string());
#ifdef NATIVE_STR32
		currentBlock.text = std::move(tmpStr);
#else
		currentBlock.text = convert.from_bytes(tmpStr);
#endif
		currentBlock.colour = getColour(currentColour);
		currentBlock.font = getFont(currentFont).get();
		blocks.push_back(currentBlock);
	}
}

const std::vector<TextBlockUtf32>& RichTextProcessor::getBlocks() const
{
	return blocks;
}

std::vector<TextBlockUtf32>& RichTextProcessor::getBlocks()
{
	return blocks;
}

void RichTextProcessor::enableItalic()
{
	if(!isItalic) {
		isItalic = true;
		flush();
	}
}

void RichTextProcessor::enableBold()
{
	if(!isBold) {
		isBold = true;
		flush();
	}
}

void RichTextProcessor::enableUnderline()
{

}

void RichTextProcessor::disableItalic()
{
	if(isItalic) {
		isItalic = false;
		flush();
	}
}

void RichTextProcessor::disableBold()
{
	if(isBold) {
		isBold = false;
		flush();
	}
}

void RichTextProcessor::disableUnderline()
{

}

void RichTextProcessor::setFontSize(int siz)
{

}

void RichTextProcessor::setFontFace(int face)
{
	if(currentFont != face) {
		currentFont = face;
		flush();
	}
}

void RichTextProcessor::setFontColour(int clr)
{
	if(currentColour != clr) {
		currentColour = clr;
		flush();
	}
}

sFont RichTextProcessor::getFont(int fontnum)
{
	return fontRepo->getFont(fonts_stack::fontname(fontnum), isBold, isItalic );
}

glm::fvec4 RichTextProcessor::getColour(int colnum)
{
	return colours[colnum % colours.size()];
}

/*
	int defaultSize;
	int currentSize;
	int currentColour;
	int currentFont;
	bool isBold;
	bool isItalic;
*/

RichTextProcessor::RichTextProcessor(const sFontRepository& repo)
	: fontRepo(repo), defaultSize(10), currentSize(10), currentColour(0), currentFont(0), isBold(false), isItalic(false)
{
	colours.emplace_back(1.0f,1.0f,1.0f,1.0f);
	currentBlock.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	currentBlock.font = nullptr;
	int num = 0;
	for(auto it = std::begin(repo->getFonts()); it != std::end(repo->getFonts());++it) {
		register_font(num,it->first.c_str());
		++num;
	}
}

RichTextProcessor::RichTextProcessor(sFontRepository&& repo)
	: fontRepo(std::move(repo)), defaultSize(10), currentSize(10), currentColour(0), currentFont(0), isBold(false), isItalic(false)
{
	//register_color(255,255,255);
	colours.emplace_back(1.0f,1.0f,1.0f,1.0f);
	currentBlock.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	currentBlock.font = nullptr;
	int num = 0;
	for(auto it = std::begin(fontRepo->getFonts()); it != std::end(fontRepo->getFonts());++it) {
		register_font(num,it->first.c_str());
		++num;
	}
}

void RichTextProcessor::register_color(uchar r, uchar g, uchar b)
{
	colours.push_back(glm::fvec4(normalize(r),normalize(g),normalize(b),1.0f));
}

void RichTextProcessor::enable_attr(int attr, int param, bool enable)
{

	switch( attr )
	{
	case none:
		break;
	case bold:
		if( enable ) enableBold();
		else disableBold();
		break;
	case italic:
		if( enable ) enableItalic();
		else disableItalic();
		break;
	case underline:
		if( enable ) enableUnderline();
		else disableUnderline();
		break;
	case double_ul:
	case word_ul :
	case thick_ul:
	case wave_ul:
	case dot_ul:
	case dash_ul:
	case dot_dash_ul:
	case dbl_dot_dash_ul:
		break;
	case fontsize:
		if( enable ) setFontSize(param);
		else setFontSize(defaultSize);
		break;
	case std_fontsize:
		if( enable ) setFontSize(param);
		else setFontSize(defaultSize);
		break;
	case fontface:
		if(enable) setFontFace(param);
		else setFontColour(0);
		break;
	case foreground:
		if(enable) setFontColour(param);
		else setFontColour(0);
		break;
	case background:
		break;
	case caps:
		break;
	case smallcaps:
		break;

	case shadow:
	case outline:
	case emboss:
	case engrave:

	case super:
	case sub:
	case strike:
	case dbl_strike:
	case expand:
		break;
	}
}
void RichTextProcessor::begin( ) {
	attr_push_style( format::attributes::style::character, 0);
}
void RichTextProcessor::end( ) {
	//flush();
}
void RichTextProcessor::title( const std::string & title  ) {
	(void)title;
}
void RichTextProcessor::font_smaller_begin( ) {
	setFontSize(currentSize / 2);
}
void RichTextProcessor::font_smaller_end( ) {
	setFontSize(currentSize * 2);
}
void RichTextProcessor::keywords( const std::string & text ) {
	(void)text;
}
void RichTextProcessor::author( const std::string & text ) {
	(void)text;
}
void RichTextProcessor::print( const std::string & text ) {
#ifdef NATIVE_STR32
	sstrm << convert.from_bytes(text);
#else
	sstrm << text;
#endif
}
void RichTextProcessor::print_symbol(const std::string & sym) {
#ifdef NATIVE_STR32
	sstrm << convert.from_bytes(sym);
#else
	sstrm << sym;
#endif
}
void RichTextProcessor::print_forced_space( ) {
	sstrm << ' ';
}
int  RichTextProcessor::print_unicode( int x ) {
#ifdef NATIVE_STR32
	sstrm << char32_t(x);
#else
	sstrm << convert.to_bytes(char32_t(x));
#endif
	return sizeof(char32_t);
}
void RichTextProcessor::hyperlink_base( const std::string & url_base ) {
	(void)url_base;
}
void RichTextProcessor::hyperlink( const std::string & text ) {
#ifdef NATIVE_STR32
	sstrm << sstrm << convert.from_bytes(text);
#else
	sstrm << text;
#endif
}
void RichTextProcessor::print_char( special_characters spc ) {
#ifdef NATIVE_STR32
	switch(spc)
	{
	case bullet:
	case left_quote:
		sstrm << '\''; break;
	case right_quote:
		sstrm << '\''; break;
	case left_dbl_quote:
		sstrm << '\"'; break;
	case right_dbl_quote:
		sstrm << '\"'; break;
	case nonbreaking_space:
		sstrm << ' '; break;
	case emdash:
		sstrm << char32_t(0x2014); break;
	case endash:
		sstrm << char32_t(0x2013); break;
	case lessthan:
		sstrm << '<'; break;
	case greaterthan:
		sstrm << '>'; break;
	case amp:
		sstrm << '&'; break;
	case copyright:
		sstrm << char32_t(0x00A9); break;
	case trademark:
		sstrm << char32_t(0x2122); break;
	case nonbreaking_hyphen:
		sstrm << '-'; break;
	case optional_hyphen:
		sstrm << '-'; break;
	case dbl_quote:
		sstrm << '\"'; break;
	case line_break:
		sstrm << '\n'; break;
	case page_break:
		sstrm << '\f'; break;
	case paragraph_begin:
		sstrm << '\t'; break;
	case center_begin:
		sstrm << ' '; break;
	case right_begin:
		sstrm << ' '; break;
	case justify_begin:
		sstrm << ' '; break;
	case center_end:
		sstrm << ' '; break;
	case right_end:
		sstrm << ' '; break;
	case justify_end:
		sstrm << ' '; break;
	case tab:
		sstrm << '\t'; break;
	default:
		break;
	}
#else
	switch(spc)
	{
	case bullet:
	case left_quote:
		sstrm << '\''; break;
	case right_quote:
		sstrm << '\''; break;
	case left_dbl_quote:
		sstrm << '\"'; break;
	case right_dbl_quote:
		sstrm << '\"'; break;
	case nonbreaking_space:
		sstrm << ' '; break;
	case emdash:
		sstrm << "\u2014"; break;
	case endash:
		sstrm << "\u2013"; break;
	case lessthan:
		sstrm << '<'; break;
	case greaterthan:
		sstrm << '>'; break;
	case amp:
		sstrm << '&'; break;
	case copyright:
		sstrm << "\u00A9"; break;
	case trademark:
		sstrm << "\u2122"; break;
	case nonbreaking_hyphen:
		sstrm << '-'; break;
	case optional_hyphen:
		sstrm << '-'; break;
	case dbl_quote:
		sstrm << '\"'; break;
	case line_break:
		sstrm << '\n'; break;
	case page_break:
		sstrm << '\f'; break;
	case paragraph_begin:
		sstrm << '\t'; break;
	case center_begin:
		//sstrm << ' '; break;
	case right_begin:
		//sstrm << ' '; break;
	case justify_begin:
		//sstrm << ' '; break;
	case center_end:
		//sstrm << ' '; break;
	case right_end:
		//sstrm << ' '; break;
	case justify_end:
		//sstrm << ' '; break;
		break;
	case tab:
		sstrm << '\t'; break;
	default:
		break;
	}
#endif
}
void RichTextProcessor::table_begin () {
}
void RichTextProcessor::table_end () {
}
void RichTextProcessor::table_cell_begin () {
}
void RichTextProcessor::table_cell_end () {
}
void RichTextProcessor::table_row_begin () {
}
void RichTextProcessor::table_row_end () {
}
void RichTextProcessor::summary( summary_categories cat, int param) {
	(void)cat;
	(void)param;
}


format::image_mgr* RichTextProcessor::image_begin(const format::image_descr& image)
{
	return nullptr;
}

void RichTextProcessor::image_end(format::image_mgr&)
{

}
