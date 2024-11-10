#ifndef EUPHCRYPTOPROXYDEVICE_HPP
#define EUPHCRYPTOPROXYDEVICE_HPP
#include <Elvavena/Io/ElvIoDeviceProxy.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
namespace Euph {
namespace Io {

// Enum listing all the supported ciphers
enum class CipherType {
	NULL_CIPHER,
	DES_ECB,
	DES_EDE,
	DES_EDE3,
	DES_EDE_ECB,
	DES_EDE3_ECB,
	DES_CFB64,
	DES_CFB1,
	DES_CFB8,
	DES_EDE_CFB64,
	DES_EDE3_CFB64,
	DES_EDE3_CFB1,
	DES_EDE3_CFB8,
	DES_OFB,
	DES_EDE_OFB,
	DES_EDE3_OFB,
	DES_CBC,
	DES_EDE_CBC,
	DES_EDE3_CBC,
	DESX_CBC,
	DES_EDE3_WRAP,
	RC4,
	RC4_40,
	RC4_HMAC_MD5,
	IDEA_ECB,
	IDEA_CFB64,
	IDEA_OFB,
	IDEA_CBC,
	RC2_ECB,
	RC2_CBC,
	RC2_40_CBC,
	RC2_64_CBC,
	RC2_CFB64,
	RC2_OFB,
	BF_ECB,
	BF_CBC,
	BF_CFB64,
	BF_OFB,
	CAST5_ECB,
	CAST5_CBC,
	CAST5_CFB64,
	CAST5_OFB,
	RC5_32_12_16_CBC,
	RC5_32_12_16_ECB,
	RC5_32_12_16_CFB64,
	RC5_32_12_16_OFB,
	AES_128_ECB,
	AES_128_CBC,
	AES_128_CFB1,
	AES_128_CFB8,
	AES_128_CFB128,
	AES_128_OFB,
	AES_128_CTR,
	AES_128_CCM,
	AES_128_GCM,
	AES_128_XTS,
	AES_128_WRAP,
	AES_128_WRAP_PAD,
	AES_128_OCB,
	AES_192_ECB,
	AES_192_CBC,
	AES_192_CFB1,
	AES_192_CFB8,
	AES_192_CFB128,
	AES_192_OFB,
	AES_192_CTR,
	AES_192_CCM,
	AES_192_GCM,
	AES_192_WRAP,
	AES_192_WRAP_PAD,
	AES_192_OCB,
	AES_256_ECB,
	AES_256_CBC,
	AES_256_CFB1,
	AES_256_CFB8,
	AES_256_CFB128,
	AES_256_OFB,
	AES_256_CTR,
	AES_256_CCM,
	AES_256_GCM,
	AES_256_XTS,
	AES_256_WRAP,
	AES_256_WRAP_PAD,
	AES_256_OCB,
	AES_128_CBC_HMAC_SHA1,
	AES_256_CBC_HMAC_SHA1,
	AES_128_CBC_HMAC_SHA256,
	AES_256_CBC_HMAC_SHA256,
	ARIA_128_ECB,
	ARIA_128_CBC,
	ARIA_128_CFB1,
	ARIA_128_CFB8,
	ARIA_128_CFB128,
	ARIA_128_CTR,
	ARIA_128_OFB,
	ARIA_128_GCM,
	ARIA_128_CCM,
	ARIA_192_ECB,
	ARIA_192_CBC,
	ARIA_192_CFB1,
	ARIA_192_CFB8,
	ARIA_192_CFB128,
	ARIA_192_CTR,
	ARIA_192_OFB,
	ARIA_192_GCM,
	ARIA_192_CCM,
	ARIA_256_ECB,
	ARIA_256_CBC,
	ARIA_256_CFB1,
	ARIA_256_CFB8,
	ARIA_256_CFB128,
	ARIA_256_CTR,
	ARIA_256_OFB,
	ARIA_256_GCM,
	ARIA_256_CCM,
	CAMELLIA_128_ECB,
	CAMELLIA_128_CBC,
	CAMELLIA_128_CFB1,
	CAMELLIA_128_CFB8,
	CAMELLIA_128_CFB128,
	CAMELLIA_128_OFB,
	CAMELLIA_128_CTR,
	CAMELLIA_192_ECB,
	CAMELLIA_192_CBC,
	CAMELLIA_192_CFB1,
	CAMELLIA_192_CFB8,
	CAMELLIA_192_CFB128,
	CAMELLIA_192_OFB,
	CAMELLIA_192_CTR,
	CAMELLIA_256_ECB,
	CAMELLIA_256_CBC,
	CAMELLIA_256_CFB1,
	CAMELLIA_256_CFB8,
	CAMELLIA_256_CFB128,
	CAMELLIA_256_OFB,
	CAMELLIA_256_CTR,
	CHACHA20,
	CHACHA20_POLY1305,
	SEED_ECB,
	SEED_CBC,
	SEED_CFB128,
	SEED_OFB,
	SM4_ECB,
	SM4_CBC,
	SM4_CFB128,
	SM4_OFB,
	SM4_CTR
};

class CryptoWriteDeviceProxy : public Elv::Io::BufferedWriteDeviceProxy
{
private:
	EVP_CIPHER_CTX* encryptCtx;
	std::array<unsigned char, EVP_MAX_IV_LENGTH> iv;
	std::vector<unsigned char> key;  // store encryption key securely
	bool finalized;
	bool firstWrite;
	CipherType cipherType;
protected:
	size_t processInBufferToOutBuffer() override;
	size_t writePrefix(Device& device) override;
	size_t writePostfix(Device& device) override;
public:
	CryptoWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, const std::vector<unsigned char>& key);
	CryptoWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, std::vector<unsigned char>&& key);
	~CryptoWriteDeviceProxy();
	void onFinish() override;
	CipherType getCipherType() const;
};

class CryptoReadDeviceProxy : public Elv::Io::BufferedReadDeviceProxy
{
private:
	EVP_CIPHER_CTX* decryptCtx;
	std::vector<unsigned char> key;
	std::array<unsigned char, EVP_MAX_IV_LENGTH> iv;
	CipherType cipherType;
protected:
	size_t processInBufferToOutBuffer(size_t bytesRead) override;
public:
	CryptoReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, const std::vector<unsigned char>& key);
	CryptoReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, std::vector<unsigned char>&& key);
	~CryptoReadDeviceProxy();
	CipherType getCipherType() const;
};

}
}

#endif // EUPHCRYPTOPROXYDEVICE_HPP
