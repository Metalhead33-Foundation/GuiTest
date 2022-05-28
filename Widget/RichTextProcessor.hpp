#ifndef RICHTEXTPROCESSOR_HPP
#define RICHTEXTPROCESSOR_HPP
#include <librtfcpp.h>
#include "FontRepository.hpp"
#include <sstream>
#include <locale>
#include <codecvt>
#include <cuchar>

class RichTextProcessor :
		public format::processor,
		public format::utils::attributes_stack,
		public format::utils::fonts_stack
{
private:
	std::vector<TextBlockUtf32> blocks;
	TextBlockUtf32 currentBlock;
	std::vector<glm::fvec4> colours;
	// Enable stuff
	void enableItalic();
	void enableBold();
	void enableUnderline();
	// Disable stuff
	void disableItalic();
	void disableBold();
	void disableUnderline();
	// Set stuff
	void setFontSize(int siz);
	void setFontFace(int face);
	void setFontColour(int clr);
	int defaultSize;
	int currentSize;
	int currentColour;
	int currentFont;
#ifdef NATIVE_STR32
	std::basic_stringstream<char32_t> sstrm;
#else
	std::stringstream sstrm;
#endif
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
	sFontRepository fontRepo;
	bool isBold;
	bool isItalic;
public:
	sFont getFont(int fontnum);
	glm::fvec4 getColour(int colnum);
	void flush();
	explicit RichTextProcessor(const sFontRepository& repo);
	explicit RichTextProcessor(sFontRepository&& repo);
	virtual void register_color( uchar r, uchar g, uchar b ) override;
	virtual void enable_attr( int attr, int param, bool enable ) override;
	virtual void begin( ) override;
	virtual void end( ) override;
	virtual void title( const std::string & title  ) override;
	virtual void font_smaller_begin( ) override;
	virtual void font_smaller_end( ) override;
	virtual void keywords( const std::string & text ) override;
	virtual void author( const std::string & text ) override;
	virtual void print( const std::string & text ) override;
	virtual void print_symbol( const std::string & sym ) override;
	virtual void print_forced_space( ) override;
	virtual int  print_unicode( int x ) override;
	virtual void hyperlink_base( const std::string & url_base ) override;
	virtual void hyperlink( const std::string & text ) override;
	virtual void print_char( special_characters spc ) override;
	virtual void table_begin () override;
	virtual void table_end () override;
	virtual void table_cell_begin () override;
	virtual void table_cell_end () override;
	virtual void table_row_begin () override;
	virtual void table_row_end () override;
	virtual void summary( summary_categories cat, int param) override;
	const std::vector<TextBlockUtf32>& getBlocks() const;
	std::vector<TextBlockUtf32>& getBlocks();
	format::image_mgr* image_begin(const format::image_descr& image) override;
	void image_end(format::image_mgr&) override;
};
typedef std::shared_ptr<RichTextProcessor> sRichTextProcessor;

#endif // RICHTEXTPROCESSOR_HPP
