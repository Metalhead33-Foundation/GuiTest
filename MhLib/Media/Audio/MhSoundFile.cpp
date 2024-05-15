#include <MhLib/Media/Audio/MhSoundFile.hpp>
#include <sndfile.h>
#include <cstring>
#include <cassert>

sf_count_t sfGetFilelen( void *user_data );
sf_count_t sfSeek( sf_count_t offset, int whence, void *user_data );
sf_count_t sfRead( void *ptr, sf_count_t count, void *user_data );
sf_count_t sfWrite( const void *ptr, sf_count_t count, void *user_data );
sf_count_t sfTell( void *user_data );
static SF_VIRTUAL_IO sndFileIO = { sfGetFilelen, sfSeek, sfRead, sfWrite,
								  sfTell };

#define SFHNDL_A reinterpret_cast<SNDFILE*>(handleA)
#define SFHNDL_B reinterpret_cast<SF_INFO*>(handleB)
namespace MH33 {
namespace Audio {

/*
	void *handleA,*handleB;
	sIoDevice iodev;
*/

SoundFile::SoundFile(SoundFile &&mov)
	: handleA(mov.handleA), handleB(mov.handleB), iodev(std::move(mov.iodev))
{
	mov.handleA = nullptr;
	mov.handleB = nullptr;
}

SoundFile &SoundFile::operator=(SoundFile &&mov)
{
	if(handleA) {
		sf_close(SFHNDL_A);
	}
	this->handleA = mov.handleA;
	mov.handleA = nullptr;
	if(handleB) {
		free(handleB);
	}
	this->handleB = mov.handleB;
	mov.handleB = nullptr;
	this->iodev = std::move(mov.iodev);
	return *this;
}


SoundFile::~SoundFile()
{
	if(handleA) {
		sf_close(SFHNDL_A);
	}
	if(handleB) {
		free(handleB);
	}
}

SoundFile::SoundFile(const Io::sDevice &iodev)
	: handleA(nullptr), handleB(nullptr), iodev(iodev)
{
	assert(this->iodev);
	if(!this->iodev) return;
	handleB = malloc(sizeof(SF_INFO));
	memset( handleB, 0, sizeof( SF_INFO ) );
	switch (this->iodev->getMode()) {
	case MH33::Io::Mode::READ: handleA = sf_open_virtual(&sndFileIO,SFM_READ,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_WRITE,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::READ_WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_RDWR,SFHNDL_B,this->iodev.get()); break;
	default: break;
	}
}

SoundFile::SoundFile(Io::sDevice &&iodev)
	: handleA(nullptr), handleB(nullptr), iodev(std::move(iodev))
{
	assert(this->iodev);
	if(!this->iodev) return;
	handleB = malloc(sizeof(SF_INFO));
	memset( handleB, 0, sizeof( SF_INFO ) );
	switch (this->iodev->getMode()) {
	case MH33::Io::Mode::READ: handleA = sf_open_virtual(&sndFileIO,SFM_READ,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_WRITE,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::READ_WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_RDWR,SFHNDL_B,this->iodev.get()); break;
	default: break;
	}
}

SoundFile::SoundFile(Io::DeviceCreator iodev_creator, Io::Mode mode)
	: handleA(nullptr), handleB(nullptr), iodev(iodev_creator(mode))
{
	assert(this->iodev);
	if(!this->iodev) return;
	handleB = malloc(sizeof(SF_INFO));
	memset( handleB, 0, sizeof( SF_INFO ) );
	switch (this->iodev->getMode()) {
	case MH33::Io::Mode::READ: handleA = sf_open_virtual(&sndFileIO,SFM_READ,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_WRITE,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::READ_WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_RDWR,SFHNDL_B,this->iodev.get()); break;
	default: break;
	}
}

SoundFile::SoundFile(Io::System &iosys, const char *path, Io::Mode mode)
	: handleA(nullptr), handleB(nullptr), iodev(iosys.open(path,mode))
{
	assert(this->iodev);
	if(!this->iodev) return;
	handleB = malloc(sizeof(SF_INFO));
	memset( handleB, 0, sizeof( SF_INFO ) );
	switch (iodev->getMode()) {
	case MH33::Io::Mode::READ: handleA = sf_open_virtual(&sndFileIO,SFM_READ,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_WRITE,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::READ_WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_RDWR,SFHNDL_B,this->iodev.get()); break;
	default: break;
	}
}

SoundFile::SoundFile(Io::System &iosys, const std::string &path, Io::Mode mode)
	: handleA(nullptr), handleB(nullptr), iodev(iosys.open(path,mode))
{
	assert(this->iodev);
	if(!this->iodev) return;
	handleB = malloc(sizeof(SF_INFO));
	memset( handleB, 0, sizeof( SF_INFO ) );
	switch (iodev->getMode()) {
	case MH33::Io::Mode::READ: handleA = sf_open_virtual(&sndFileIO,SFM_READ,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_WRITE,SFHNDL_B,this->iodev.get()); break;
	case MH33::Io::Mode::READ_WRITE: handleA = sf_open_virtual(&sndFileIO,SFM_RDWR,SFHNDL_B,this->iodev.get()); break;
	default: break;
	}
}
// IO
FrameIndex SoundFile::seekSet( FrameCount frames ) const {
	assert(handleA);
	if(handleA) return FrameIndex(sf_seek(SFHNDL_A,frames.var,SEEK_SET));
	else return FrameIndex(0);
}
FrameIndex SoundFile::seekCur( FrameCount frames ) const {
	assert(handleA);
	if(handleA) return FrameIndex(sf_seek(SFHNDL_A,frames.var,SEEK_CUR));
	else return FrameIndex(0);
}
FrameIndex SoundFile::seekEnd( FrameCount frames ) const {
	assert(handleA);
	if(handleA) return FrameIndex(sf_seek(SFHNDL_A,frames.var,SEEK_END));
	else return FrameIndex(0);
}
SampleCount SoundFile::read( short *ptr, SampleCount samples ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return SampleCount(sf_read_short(SFHNDL_A,ptr,samples.var));
	else return SampleCount(0);
}
SampleCount SoundFile::read( int *ptr, SampleCount samples ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return SampleCount(sf_read_int(SFHNDL_A,ptr,samples.var));
	else return SampleCount(0);
}
SampleCount SoundFile::read( float *ptr, SampleCount samples ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return SampleCount(sf_read_float(SFHNDL_A,ptr,samples.var));
	else return SampleCount(0);
}
SampleCount SoundFile::read( double *ptr, SampleCount samples ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return SampleCount(sf_read_double(SFHNDL_A,ptr,samples.var));
	else return SampleCount(0);
}
FrameCount SoundFile::readf( short *ptr, FrameCount frames ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return FrameCount(sf_readf_short(SFHNDL_A,ptr,frames.var));
	else return FrameCount(0);
}
FrameCount SoundFile::readf( int *ptr, FrameCount frames ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return FrameCount(sf_readf_int(SFHNDL_A,ptr,frames.var));
	else return FrameCount(0);
}
FrameCount SoundFile::readf( float *ptr, FrameCount frames ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return FrameCount(sf_readf_float(SFHNDL_A,ptr,frames.var));
	else return FrameCount(0);
}
FrameCount SoundFile::readf( double *ptr, FrameCount frames ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return FrameCount(sf_readf_double(SFHNDL_A,ptr,frames.var));
	else return FrameCount(0);
}
SampleCount SoundFile::write( short *ptr, SampleCount samples ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return SampleCount(sf_write_short(SFHNDL_A,ptr,samples.var));
	else return SampleCount(0);
}
SampleCount SoundFile::write( int *ptr, SampleCount samples ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return SampleCount(sf_write_int(SFHNDL_A,ptr,samples.var));
	else return SampleCount(0);
}
SampleCount SoundFile::write( float *ptr, SampleCount samples ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return SampleCount(sf_write_float(SFHNDL_A,ptr,samples.var));
	else return SampleCount(0);
}
SampleCount SoundFile::write( double *ptr, SampleCount samples ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return SampleCount(sf_write_double(SFHNDL_A,ptr,samples.var));
	else return SampleCount(0);
}
FrameCount SoundFile::writef( short *ptr, FrameCount frames ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return FrameCount(sf_writef_short(SFHNDL_A,ptr,frames.var));
	else return FrameCount(0);
}
FrameCount SoundFile::writef( int *ptr, FrameCount frames ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return FrameCount(sf_writef_int(SFHNDL_A,ptr,frames.var));
	else return FrameCount(0);
}
FrameCount SoundFile::writef( float *ptr, FrameCount frames ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return FrameCount(sf_writef_float(SFHNDL_A,ptr,frames.var));
	else return FrameCount(0);
}
FrameCount SoundFile::writef( double *ptr, FrameCount frames ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return FrameCount(sf_writef_double(SFHNDL_A,ptr,frames.var));
	else return FrameCount(0);
}
long SoundFile::read_raw( void *ptr, long bytes ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return sf_read_raw(SFHNDL_A,ptr,bytes);
	else return 0;
}
long SoundFile::write_raw( void *ptr, long bytes ) const {
	assert(handleA);
	assert(ptr);
	if(handleA) return sf_write_raw(SFHNDL_A,ptr,bytes);
	else return 0;
}
void SoundFile::write_sync( ) const {
	assert(handleA);
	if(handleA) sf_write_sync(SFHNDL_A);
}

// Getters of non-text data
FrameCount SoundFile::getFrameNum( ) const {
	assert(handleB);
	if(handleB) return FrameCount(SFHNDL_B->frames);
	else return FrameCount(0);
}
FrameRate SoundFile::getFrameRate( ) const {
	assert(handleB);
	if(handleB) return FrameRate(SFHNDL_B->samplerate);
	else return FrameRate(0);
}
ChannelCount SoundFile::getChannels( ) const {
	assert(handleB);
	if(handleB) return ChannelCount(SFHNDL_B->channels);
	else return ChannelCount(0);
}
int SoundFile::getFormat( ) const {
	assert(handleB);
	if(handleB) return SFHNDL_B->format;
	else return 0;
}
int SoundFile::getSections( ) const {
	assert(handleB);
	if(handleB) return SFHNDL_B->sections;
	else return 0;
}
int SoundFile::getSeekable( ) const {
	assert(handleB);
	if(handleB) return SFHNDL_B->seekable;
	else return 0;
}

// Text getters
const char *SoundFile::getTitle( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_TITLE);
	else return nullptr;
}
const char *SoundFile::getCopyright( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_COPYRIGHT);
	else return nullptr;
}
const char *SoundFile::getSoftware( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_SOFTWARE);
	else return nullptr;
}
const char *SoundFile::getArtist( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_ARTIST);
	else return nullptr;
}
const char *SoundFile::getComment( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_COMMENT);
	else return nullptr;
}
const char *SoundFile::getDate( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_DATE);
	else return nullptr;
}
const char *SoundFile::getAlbum( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_ALBUM);
	else return nullptr;
}
const char *SoundFile::getLicense( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_LICENSE);
	else return nullptr;
}
const char *SoundFile::getTrackNumber( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_TRACKNUMBER);
	else return nullptr;
}
const char *SoundFile::getGenre( ) const {
	assert(handleA);
	if(handleA) return sf_get_string(SFHNDL_A, SF_STR_GENRE);
	else return nullptr;
}

// Text getters - C string
int SoundFile::setTitle( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_TITLE, str);
	else return 0;
}
int SoundFile::setCopyright( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_COPYRIGHT, str);
	else return 0;
}
int SoundFile::setSoftware( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_SOFTWARE, str);
	else return 0;
}
int SoundFile::setArtist( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_ARTIST, str);
	else return 0;
}
int SoundFile::setComment( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_COMMENT, str);
	else return 0;
}
int SoundFile::setDate( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_DATE, str);
	else return 0;
}
int SoundFile::getAlbum( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_ALBUM, str);
	else return 0;
}
int SoundFile::setTrackNumber( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_TRACKNUMBER, str);
	else return 0;
}
int SoundFile::setLicense( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_LICENSE, str);
	else return 0;
}
int SoundFile::setGenre( const char *str ) const {
	assert(handleA);
	if(handleA) return sf_set_string(SFHNDL_A, SF_STR_GENRE, str);
	else return 0;
}

// Text getters - C++ string
int SoundFile::setTitle( const std::string &str ) const {
	return setTitle(str.c_str());
}
int SoundFile::setCopyright( const std::string &str ) const {
	return setCopyright(str.c_str());
}
int SoundFile::setSoftware( const std::string &str ) const {
	return setSoftware(str.c_str());
}
int SoundFile::setArtist( const std::string &str ) const {
	return setArtist(str.c_str());
}
int SoundFile::setComment( const std::string &str ) const {
	return setComment(str.c_str());
}
int SoundFile::setDate( const std::string &str ) const {
	return setDate(str.c_str());
}
int SoundFile::getAlbum( const std::string &str ) const {
	return getAlbum(str.c_str());
}
int SoundFile::setTrackNumber( const std::string &str ) const {
	return setTrackNumber(str.c_str());
}
int SoundFile::setLicense( const std::string &str ) const {
	return setLicense(str.c_str());
}
int SoundFile::setGenre( const std::string &str ) const {
	return setGenre(str.c_str());
}

}
}
sf_count_t sfGetFilelen( void *user_data ) {
	assert(user_data);
	return sf_count_t(
		reinterpret_cast< MH33::Io::Device* >( user_data )->size( ) );
}
sf_count_t sfSeek( sf_count_t offset, int whence, void *user_data ) {
	assert(user_data);
	auto chandle = reinterpret_cast< MH33::Io::Device* >( user_data );
	bool cunt = false;
	switch ( whence ) {
	case SEEK_SET:
		cunt =
			sf_count_t( chandle->seek( MH33::Io::SeekOrigin::SET, offset ) );
		break;
	case SEEK_CUR:
		cunt =
			sf_count_t( chandle->seek( MH33::Io::SeekOrigin::CUR, offset ) );
		break;
	case SEEK_END:
		cunt =
			sf_count_t( chandle->seek(  MH33::Io::SeekOrigin::END, offset ) );
		break;
	default:
		cunt = false;
	}
	if ( cunt )
		return chandle->tell( );
	else
		return -1;
}
sf_count_t sfRead( void *ptr, sf_count_t count, void *user_data ) {
	assert(user_data);
	assert(ptr);
	return sf_count_t(
		reinterpret_cast< MH33::Io::Device* >( user_data )->read( ptr, count ) );
}
sf_count_t sfWrite( const void *ptr, sf_count_t count, void *user_data ) {
	assert(user_data);
	assert(ptr);
	return sf_count_t(
		reinterpret_cast< MH33::Io::Device* >( user_data )->write( ptr, count ) );
}
sf_count_t sfTell( void *user_data ) {
	assert(user_data);
	return sf_count_t(
		reinterpret_cast< MH33::Io::Device* >( user_data )->tell( ) );
}
