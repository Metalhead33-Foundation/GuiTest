#include "EuphSoundFile.hpp"
#include <cstring>
sf_count_t sfGetFilelen( void *user_data ) {
	return sf_count_t(
		reinterpret_cast< Elv::Io::Device* >( user_data )->size( ) );
}
sf_count_t sfSeek( sf_count_t offset, int whence, void *user_data ) {
	auto chandle = reinterpret_cast< Elv::Io::Device* >( user_data );
	bool cunt = false;
	switch ( whence ) {
		case SEEK_SET:
			cunt =
				sf_count_t( chandle->seek( offset, Elv::Io::SeekOrigin::SET ) );
			break;
		case SEEK_CUR:
			cunt =
				sf_count_t( chandle->seek( offset, Elv::Io::SeekOrigin::CUR ) );
			break;
		case SEEK_END:
			cunt =
				sf_count_t( chandle->seek( offset, Elv::Io::SeekOrigin::END ) );
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
	return sf_count_t(
		reinterpret_cast< Elv::Io::Device* >( user_data )->read( ptr, 1, count ) );
}
sf_count_t sfWrite( const void *ptr, sf_count_t count, void *user_data ) {
	return sf_count_t(
		reinterpret_cast< Elv::Io::Device* >( user_data )->write( ptr, 1, count ) );
}
sf_count_t sfTell( void *user_data ) {
	return sf_count_t(
		reinterpret_cast< Elv::Io::Device* >( user_data )->tell( ) );
}

static SF_VIRTUAL_IO sndFileIO = { sfGetFilelen, sfSeek, sfRead, sfWrite,
								  sfTell };
namespace Euph {
namespace Media {
namespace Audio {

SoundFile::SoundFile(SoundFile&& mov)
	: fileDevice(std::move(mov.fileDevice)), sndfile(std::move(mov.sndfile))
{
	this->info = mov.info;
}

SoundFile& SoundFile::operator=(SoundFile&& mov)
{
	this->sndfile = std::move(mov.sndfile);
	this->fileDevice = std::move(mov.fileDevice);
	this->info = mov.info;
	return *this;
}

SoundFile::~SoundFile()
{
	
}

SoundFile::SoundFile(Elv::Io::uDevice&& fileDev, const SF_INFO* infoFill)
	: fileDevice( std::move(fileDev) ), sndfile(nullptr, sf_close)
{
	if(infoFill) {
		std::memcpy(&info, infoFill, sizeof(SF_INFO));
	}
	switch (fileDevice->getMode()) {
		case Elv::Io::Mode::READ:
			sndfile = SndfilePtr(sf_open_virtual(&sndFileIO, SFM_READ, &info, fileDevice.get() ), sf_close);
			break;
		case Elv::Io::Mode::WRITE:
			sndfile = SndfilePtr(sf_open_virtual(&sndFileIO, SFM_WRITE, &info, fileDevice.get() ), sf_close);
			break;
		case Elv::Io::Mode::APPEND:
			sndfile = SndfilePtr(sf_open_virtual(&sndFileIO, SFM_WRITE, &info, fileDevice.get() ), sf_close);
			break;
		case Elv::Io::Mode::READ_WRITE:
			sndfile = SndfilePtr(sf_open_virtual(&sndFileIO, SFM_RDWR, &info, fileDevice.get() ), sf_close);
			break;
		case Elv::Io::Mode::READ_APPEND:
			sndfile = SndfilePtr(sf_open_virtual(&sndFileIO, SFM_RDWR, &info, fileDevice.get() ), sf_close);
			break;
		default:
			break;
	}
}

FrameIndex SoundFile::seekSet(FrameCount frames) const
{
	return FrameIndex(sf_seek(sndfile.get(),frames.var,SEEK_SET));
}

FrameIndex SoundFile::seekCur(FrameCount frames) const
{
	return FrameIndex(sf_seek(sndfile.get(),frames.var,SEEK_CUR));
}

FrameIndex SoundFile::seekEnd(FrameCount frames) const
{
	return FrameIndex(sf_seek(sndfile.get(),frames.var,SEEK_END));
}

SampleCount SoundFile::read(short* ptr, SampleCount samples) const
{
	return SampleCount(sf_read_short(sndfile.get(),ptr,samples.var));
}

SampleCount SoundFile::read(int* ptr, SampleCount samples) const
{
	return SampleCount(sf_read_int(sndfile.get(),ptr,samples.var));
}

SampleCount SoundFile::read(float* ptr, SampleCount samples) const
{
	return SampleCount(sf_read_float(sndfile.get(),ptr,samples.var));
}

SampleCount SoundFile::read(double* ptr, SampleCount samples) const
{
	return SampleCount(sf_read_double(sndfile.get(),ptr,samples.var));
}

FrameCount SoundFile::readf(short* ptr, FrameCount frames) const
{
	return FrameCount(sf_readf_short(sndfile.get(),ptr,frames.var));
}

FrameCount SoundFile::readf(int* ptr, FrameCount frames) const
{
	return FrameCount(sf_readf_int(sndfile.get(),ptr,frames.var));
}

FrameCount SoundFile::readf(float* ptr, FrameCount frames) const
{
	return FrameCount(sf_readf_float(sndfile.get(),ptr,frames.var));
}

FrameCount SoundFile::readf(double* ptr, FrameCount frames) const
{
	return FrameCount(sf_readf_double(sndfile.get(),ptr,frames.var));
}

SampleCount SoundFile::write(short* ptr, SampleCount samples) const
{
	return SampleCount(sf_write_short(sndfile.get(),ptr,samples.var));
}

SampleCount SoundFile::write(int* ptr, SampleCount samples) const
{
	return SampleCount(sf_write_int(sndfile.get(),ptr,samples.var));
}

SampleCount SoundFile::write(float* ptr, SampleCount samples) const
{
	return SampleCount(sf_write_float(sndfile.get(),ptr,samples.var));
}

SampleCount SoundFile::write(double* ptr, SampleCount samples) const
{
	return SampleCount(sf_write_double(sndfile.get(),ptr,samples.var));
}

FrameCount SoundFile::writef(short* ptr, FrameCount frames) const
{
	return FrameCount(sf_writef_short(sndfile.get(),ptr,frames.var));
}

FrameCount SoundFile::writef(int* ptr, FrameCount frames) const
{
	return FrameCount(sf_writef_int(sndfile.get(),ptr,frames.var));
}

FrameCount SoundFile::writef(float* ptr, FrameCount frames) const
{
	return FrameCount(sf_writef_float(sndfile.get(),ptr,frames.var));
}

FrameCount SoundFile::writef(double* ptr, FrameCount frames) const
{
	return FrameCount(sf_writef_double(sndfile.get(),ptr,frames.var));
}

long SoundFile::read_raw(void* ptr, long bytes) const
{
	return sf_read_raw(sndfile.get(), ptr, bytes);
}

long SoundFile::write_raw(void* ptr, long bytes) const
{
	return sf_write_raw(sndfile.get(), ptr, bytes);
}

void SoundFile::write_sync() const
{
	sf_write_sync(sndfile.get());
}

FrameCount SoundFile::getFrameNum() const
{
	return FrameCount( info.frames );
}

SampleRate SoundFile::getFrameRate() const
{
	return SampleRate( info.samplerate );
}

ChannelCount SoundFile::getChannels() const
{
	return ChannelCount( info.channels );
}

int SoundFile::getFormat() const
{
	return info.format;
}

int SoundFile::getSections() const
{
	return info.sections;
}

int SoundFile::getSeekable() const
{
	return info.seekable;
}

const char* SoundFile::getTitle() const
{
	return sf_get_string(sndfile.get() , SF_STR_TITLE );
}

const char* SoundFile::getCopyright() const
{
	return sf_get_string(sndfile.get() , SF_STR_COPYRIGHT );
}

const char* SoundFile::getSoftware() const
{
	return sf_get_string(sndfile.get() , SF_STR_SOFTWARE );
}

const char* SoundFile::getArtist() const
{
	return sf_get_string(sndfile.get() , SF_STR_ARTIST );
}

const char* SoundFile::getComment() const
{
	return sf_get_string(sndfile.get() , SF_STR_COMMENT );
}

const char* SoundFile::getDate() const
{
	return sf_get_string(sndfile.get() , SF_STR_DATE );
}

const char* SoundFile::getAlbum() const
{
	return sf_get_string(sndfile.get() , SF_STR_ALBUM );
}

const char* SoundFile::getLicense() const
{
	return sf_get_string(sndfile.get() , SF_STR_LICENSE );
}

const char* SoundFile::getTrackNumber() const
{
	return sf_get_string(sndfile.get() , SF_STR_TRACKNUMBER );
}

const char* SoundFile::getGenre() const
{
	return sf_get_string(sndfile.get() , SF_STR_GENRE );
}

int SoundFile::setTitle(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_TITLE, str );
}

int SoundFile::setCopyright(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_COPYRIGHT, str );
}

int SoundFile::setSoftware(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_COPYRIGHT, str );
}

int SoundFile::setArtist(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_ARTIST, str );
}

int SoundFile::setComment(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_COMMENT, str );
}

int SoundFile::setDate(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_DATE, str );
}

int SoundFile::setAlbum(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_ALBUM, str );
}

int SoundFile::setTrackNumber(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_TRACKNUMBER, str );
}

int SoundFile::setLicense(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_LICENSE, str );
}

int SoundFile::setGenre(const char* str) const
{
	return sf_set_string(sndfile.get() , SF_STR_GENRE, str );
}

}
}
}
