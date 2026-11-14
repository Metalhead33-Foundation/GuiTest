#include "EuphCryptoProxyDevice.hpp"
#include <cstring>

namespace Euph {
namespace Io {

// Function to get EVP_CIPHER based on the CipherType enum
static const EVP_CIPHER* getCipher(CipherType type) {
	switch (type) {
		case CipherType::NULL_CIPHER: return EVP_enc_null();
		case CipherType::DES_ECB: return EVP_des_ecb();
		case CipherType::DES_EDE: return EVP_des_ede();
		case CipherType::DES_EDE3: return EVP_des_ede3();
		case CipherType::DES_EDE_ECB: return EVP_des_ede_ecb();
		case CipherType::DES_EDE3_ECB: return EVP_des_ede3_ecb();
		case CipherType::DES_CFB64: return EVP_des_cfb64();
		case CipherType::DES_CFB1: return EVP_des_cfb1();
		case CipherType::DES_CFB8: return EVP_des_cfb8();
		case CipherType::DES_EDE_CFB64: return EVP_des_ede_cfb64();
		case CipherType::DES_EDE3_CFB64: return EVP_des_ede3_cfb64();
		case CipherType::DES_EDE3_CFB1: return EVP_des_ede3_cfb1();
		case CipherType::DES_EDE3_CFB8: return EVP_des_ede3_cfb8();
		case CipherType::DES_OFB: return EVP_des_ofb();
		case CipherType::DES_EDE_OFB: return EVP_des_ede_ofb();
		case CipherType::DES_EDE3_OFB: return EVP_des_ede3_ofb();
		case CipherType::DES_CBC: return EVP_des_cbc();
		case CipherType::DES_EDE_CBC: return EVP_des_ede_cbc();
		case CipherType::DES_EDE3_CBC: return EVP_des_ede3_cbc();
		case CipherType::DESX_CBC: return EVP_desx_cbc();
		case CipherType::DES_EDE3_WRAP: return EVP_des_ede3_wrap();
		case CipherType::RC4: return EVP_rc4();
		case CipherType::RC4_40: return EVP_rc4_40();
		case CipherType::RC4_HMAC_MD5: return EVP_rc4_hmac_md5();
		case CipherType::IDEA_ECB: return EVP_idea_ecb();
		case CipherType::IDEA_CFB64: return EVP_idea_cfb64();
		case CipherType::IDEA_OFB: return EVP_idea_ofb();
		case CipherType::IDEA_CBC: return EVP_idea_cbc();
		case CipherType::RC2_ECB: return EVP_rc2_ecb();
		case CipherType::RC2_CBC: return EVP_rc2_cbc();
		case CipherType::RC2_40_CBC: return EVP_rc2_40_cbc();
		case CipherType::RC2_64_CBC: return EVP_rc2_64_cbc();
		case CipherType::RC2_CFB64: return EVP_rc2_cfb64();
		case CipherType::RC2_OFB: return EVP_rc2_ofb();
		case CipherType::BF_ECB: return EVP_bf_ecb();
		case CipherType::BF_CBC: return EVP_bf_cbc();
		case CipherType::BF_CFB64: return EVP_bf_cfb64();
		case CipherType::BF_OFB: return EVP_bf_ofb();
		case CipherType::CAST5_ECB: return EVP_cast5_ecb();
		case CipherType::CAST5_CBC: return EVP_cast5_cbc();
		case CipherType::CAST5_CFB64: return EVP_cast5_cfb64();
		case CipherType::CAST5_OFB: return EVP_cast5_ofb();
		case CipherType::RC5_32_12_16_CBC: return EVP_rc5_32_12_16_cbc();
		case CipherType::RC5_32_12_16_ECB: return EVP_rc5_32_12_16_ecb();
		case CipherType::RC5_32_12_16_CFB64: return EVP_rc5_32_12_16_cfb64();
		case CipherType::RC5_32_12_16_OFB: return EVP_rc5_32_12_16_ofb();
		case CipherType::AES_128_ECB: return EVP_aes_128_ecb();
		case CipherType::AES_128_CBC: return EVP_aes_128_cbc();
		case CipherType::AES_128_CFB1: return EVP_aes_128_cfb1();
		case CipherType::AES_128_CFB8: return EVP_aes_128_cfb8();
		case CipherType::AES_128_CFB128: return EVP_aes_128_cfb128();
		case CipherType::AES_128_OFB: return EVP_aes_128_ofb();
		case CipherType::AES_128_CTR: return EVP_aes_128_ctr();
		case CipherType::AES_128_CCM: return EVP_aes_128_ccm();
		case CipherType::AES_128_GCM: return EVP_aes_128_gcm();
		case CipherType::AES_128_XTS: return EVP_aes_128_xts();
		case CipherType::AES_128_WRAP: return EVP_aes_128_wrap();
		case CipherType::AES_128_WRAP_PAD: return EVP_aes_128_wrap_pad();
		case CipherType::AES_128_OCB: return EVP_aes_128_ocb();
		case CipherType::AES_192_ECB: return EVP_aes_192_ecb();
		case CipherType::AES_192_CBC: return EVP_aes_192_cbc();
		case CipherType::AES_192_CFB1: return EVP_aes_192_cfb1();
		case CipherType::AES_192_CFB8: return EVP_aes_192_cfb8();
		case CipherType::AES_192_CFB128: return EVP_aes_192_cfb128();
		case CipherType::AES_192_OFB: return EVP_aes_192_ofb();
		case CipherType::AES_192_CTR: return EVP_aes_192_ctr();
		case CipherType::AES_192_CCM: return EVP_aes_192_ccm();
		case CipherType::AES_192_GCM: return EVP_aes_192_gcm();
		case CipherType::AES_192_WRAP: return EVP_aes_192_wrap();
		case CipherType::AES_192_WRAP_PAD: return EVP_aes_192_wrap_pad();
		case CipherType::AES_192_OCB: return EVP_aes_192_ocb();
		case CipherType::AES_256_ECB: return EVP_aes_256_ecb();
		case CipherType::AES_256_CBC: return EVP_aes_256_cbc();
		case CipherType::AES_256_CFB1: return EVP_aes_256_cfb1();
		case CipherType::AES_256_CFB8: return EVP_aes_256_cfb8();
		case CipherType::AES_256_CFB128: return EVP_aes_256_cfb128();
		case CipherType::AES_256_OFB: return EVP_aes_256_ofb();
		case CipherType::AES_256_CTR: return EVP_aes_256_ctr();
		case CipherType::AES_256_CCM: return EVP_aes_256_ccm();
		case CipherType::AES_256_GCM: return EVP_aes_256_gcm();
		case CipherType::AES_256_XTS: return EVP_aes_256_xts();
		case CipherType::AES_256_WRAP: return EVP_aes_256_wrap();
		case CipherType::AES_256_WRAP_PAD: return EVP_aes_256_wrap_pad();
		case CipherType::AES_256_OCB: return EVP_aes_256_ocb();
		case CipherType::AES_128_CBC_HMAC_SHA1: return EVP_aes_128_cbc_hmac_sha1();
		case CipherType::AES_256_CBC_HMAC_SHA1: return EVP_aes_256_cbc_hmac_sha1();
		case CipherType::AES_128_CBC_HMAC_SHA256: return EVP_aes_128_cbc_hmac_sha256();
		case CipherType::AES_256_CBC_HMAC_SHA256: return EVP_aes_256_cbc_hmac_sha256();
		case CipherType::ARIA_128_ECB: return EVP_aria_128_ecb();
		case CipherType::ARIA_128_CBC: return EVP_aria_128_cbc();
		case CipherType::ARIA_128_CFB1: return EVP_aria_128_cfb1();
		case CipherType::ARIA_128_CFB8: return EVP_aria_128_cfb8();
		case CipherType::ARIA_128_CFB128: return EVP_aria_128_cfb128();
		case CipherType::ARIA_128_CTR: return EVP_aria_128_ctr();
		case CipherType::ARIA_128_OFB: return EVP_aria_128_ofb();
		case CipherType::ARIA_128_GCM: return EVP_aria_128_gcm();
		case CipherType::ARIA_128_CCM: return EVP_aria_128_ccm();
		case CipherType::ARIA_192_ECB: return EVP_aria_192_ecb();
		case CipherType::ARIA_192_CBC: return EVP_aria_192_cbc();
		case CipherType::ARIA_192_CFB1: return EVP_aria_192_cfb1();
		case CipherType::ARIA_192_CFB8: return EVP_aria_192_cfb8();
		case CipherType::ARIA_192_CFB128: return EVP_aria_192_cfb128();
		case CipherType::ARIA_192_CTR: return EVP_aria_192_ctr();
		case CipherType::ARIA_192_OFB: return EVP_aria_192_ofb();
		case CipherType::ARIA_192_GCM: return EVP_aria_192_gcm();
		case CipherType::ARIA_192_CCM: return EVP_aria_192_ccm();
		case CipherType::ARIA_256_ECB: return EVP_aria_256_ecb();
		case CipherType::ARIA_256_CBC: return EVP_aria_256_cbc();
		case CipherType::ARIA_256_CFB1: return EVP_aria_256_cfb1();
		case CipherType::ARIA_256_CFB8: return EVP_aria_256_cfb8();
		case CipherType::ARIA_256_CFB128: return EVP_aria_256_cfb128();
		case CipherType::ARIA_256_CTR: return EVP_aria_256_ctr();
		case CipherType::ARIA_256_OFB: return EVP_aria_256_ofb();
		case CipherType::ARIA_256_GCM: return EVP_aria_256_gcm();
		case CipherType::ARIA_256_CCM: return EVP_aria_256_ccm();
		case CipherType::CAMELLIA_128_ECB: return EVP_camellia_128_ecb();
		case CipherType::CAMELLIA_128_CBC: return EVP_camellia_128_cbc();
		case CipherType::CAMELLIA_128_CFB1: return EVP_camellia_128_cfb1();
		case CipherType::CAMELLIA_128_CFB8: return EVP_camellia_128_cfb8();
		case CipherType::CAMELLIA_128_CFB128: return EVP_camellia_128_cfb128();
		case CipherType::CAMELLIA_128_OFB: return EVP_camellia_128_ofb();
		case CipherType::CAMELLIA_128_CTR: return EVP_camellia_128_ctr();
		case CipherType::CAMELLIA_192_ECB: return EVP_camellia_192_ecb();
		case CipherType::CAMELLIA_192_CBC: return EVP_camellia_192_cbc();
		case CipherType::CAMELLIA_192_CFB1: return EVP_camellia_192_cfb1();
		case CipherType::CAMELLIA_192_CFB8: return EVP_camellia_192_cfb8();
		case CipherType::CAMELLIA_192_CFB128: return EVP_camellia_192_cfb128();
		case CipherType::CAMELLIA_192_OFB: return EVP_camellia_192_ofb();
		case CipherType::CAMELLIA_192_CTR: return EVP_camellia_192_ctr();
		case CipherType::CAMELLIA_256_ECB: return EVP_camellia_256_ecb();
		case CipherType::CAMELLIA_256_CBC: return EVP_camellia_256_cbc();
		case CipherType::CAMELLIA_256_CFB1: return EVP_camellia_256_cfb1();
		case CipherType::CAMELLIA_256_CFB8: return EVP_camellia_256_cfb8();
		case CipherType::CAMELLIA_256_CFB128: return EVP_camellia_256_cfb128();
		case CipherType::CAMELLIA_256_OFB: return EVP_camellia_256_ofb();
		case CipherType::CAMELLIA_256_CTR: return EVP_camellia_256_ctr();
		case CipherType::CHACHA20: return EVP_chacha20();
		case CipherType::CHACHA20_POLY1305: return EVP_chacha20_poly1305();
		case CipherType::SEED_ECB: return EVP_seed_ecb();
		case CipherType::SEED_CBC: return EVP_seed_cbc();
		case CipherType::SEED_CFB128: return EVP_seed_cfb128();
		case CipherType::SEED_OFB: return EVP_seed_ofb();
		case CipherType::SM4_ECB: return EVP_sm4_ecb();
		case CipherType::SM4_CBC: return EVP_sm4_cbc();
		case CipherType::SM4_CFB128: return EVP_sm4_cfb128();
		case CipherType::SM4_OFB: return EVP_sm4_ofb();
		case CipherType::SM4_CTR: return EVP_sm4_ctr();
		default: return nullptr;
	}
}


CipherType CryptoWriteDeviceProxy::getCipherType() const
{
	return cipherType;
}

size_t CryptoWriteDeviceProxy::processInBufferToOutBuffer()
{
	int outLen;
	EVP_EncryptUpdate(encryptCtx, reinterpret_cast<unsigned char*>(outBuffer.data()), &outLen,
					  reinterpret_cast<const unsigned char*>(inBuffer.data()), bufferCursor);
	return outLen;
}

size_t CryptoWriteDeviceProxy::writePrefix(Device& device)
{
	if (firstWrite) {
		firstWrite = false;
		return device.write(iv.data(), 1, iv.size());
	} else {
		return 0;
	}
}

size_t CryptoWriteDeviceProxy::writePostfix(Device& device)
{
	return 0;
}

CryptoWriteDeviceProxy::CryptoWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, const std::vector<unsigned char>& key)
	: BufferedWriteDeviceProxy(underlyingDevice, inBufferSize, outBufferSize), encryptCtx(EVP_CIPHER_CTX_new()), key(key), finalized(false), firstWrite(true), cipherType(cipherType)
{
	// Generate a random IV
	RAND_bytes(iv.data(), iv.size());

	// Initialize the encryption context
	EVP_EncryptInit_ex(encryptCtx, getCipher(cipherType), nullptr, key.data(), iv.data());
}

CryptoWriteDeviceProxy::CryptoWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, std::vector<unsigned char>&& key)
	: BufferedWriteDeviceProxy(underlyingDevice, inBufferSize, outBufferSize), encryptCtx(EVP_CIPHER_CTX_new()), key(std::move(key)), finalized(false), firstWrite(true), cipherType(cipherType)
{
	// Generate a random IV
	RAND_bytes(iv.data(), iv.size());

	// Initialize the encryption context
	EVP_EncryptInit_ex(encryptCtx, getCipher(cipherType), nullptr, key.data(), iv.data());
}

CryptoWriteDeviceProxy::~CryptoWriteDeviceProxy()
{
	if(encryptCtx) {
		onFinish();
		EVP_CIPHER_CTX_free(encryptCtx);
	}
}

CryptoWriteDeviceProxy::CryptoWriteDeviceProxy(CryptoWriteDeviceProxy&& mov)
	: Elv::Io::BufferedWriteDeviceProxy(std::move(mov)), key(std::move(mov.key)), finalized(mov.finalized), firstWrite(mov.firstWrite), cipherType(mov.cipherType)
{
	std::memcpy(iv.data(), mov.iv.data(), mov.iv.size());
	mov.finalized = true;
	this->encryptCtx = mov.encryptCtx;
	mov.encryptCtx = nullptr;
}

CryptoWriteDeviceProxy& CryptoWriteDeviceProxy::operator=(CryptoWriteDeviceProxy&& mov)
{
	this->cipherType = mov.cipherType;
	this->firstWrite = mov.firstWrite;
	this->finalized = mov.finalized;
	mov.finalized = true;
	this->key = std::move(mov.key);
	std::memcpy(iv.data(), mov.iv.data(), mov.iv.size());
	this->encryptCtx = mov.encryptCtx;
	mov.encryptCtx = nullptr;
	BufferedWriteDeviceProxy::operator=(std::move(mov));
	return *this;
}

void CryptoWriteDeviceProxy::onFinish()
{
	if (!finalized)
	{
		int finalLen = 0;
		//std::memset(outBuffer.data(), 0, outBuffer.size());
		EVP_EncryptFinal_ex(encryptCtx, reinterpret_cast<unsigned char*>(outBuffer.data()), &finalLen);
		if (finalLen > 0) {
			size_t writeLen = ((finalLen + 15) / 16 * 16)+1; // Round up to the next 16-byte boundary
			underlyingDevice->write(outBuffer.data(), 1, std::min(writeLen,outBuffer.size()));
		}
		underlyingDevice->flush();
		finalized = true;
	}
}

CipherType CryptoReadDeviceProxy::getCipherType() const
{
	return cipherType;
}

size_t CryptoReadDeviceProxy::processInBufferToOutBuffer(size_t bytesRead)
{
	int outLen;
	EVP_DecryptUpdate(decryptCtx, reinterpret_cast<unsigned char*>(outBuffer.data()), &outLen,
					  reinterpret_cast<const unsigned char*>(inBuffer.data()), bytesRead);
	return outLen;
}

CryptoReadDeviceProxy::CryptoReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, const std::vector<unsigned char>& key)
	: BufferedReadDeviceProxy(underlyingDevice, inBufferSize, outBufferSize), decryptCtx(EVP_CIPHER_CTX_new()), key(key), cipherType(cipherType)
{
	// Read the IV from the beginning of the encrypted data
	underlyingDevice->read(iv.data(), 1, iv.size());

	// Initialize the decryption context with the same key and IV
	EVP_DecryptInit_ex(decryptCtx, getCipher(cipherType), nullptr, key.data(), iv.data());
}

CryptoReadDeviceProxy::CryptoReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, std::vector<unsigned char>&& key)
	: BufferedReadDeviceProxy(underlyingDevice, inBufferSize, outBufferSize), decryptCtx(EVP_CIPHER_CTX_new()), key(std::move(key)), cipherType(cipherType)
{
	// Read the IV from the beginning of the encrypted data
	underlyingDevice->read(iv.data(), 1, iv.size());

	// Initialize the decryption context with the same key and IV
	EVP_DecryptInit_ex(decryptCtx, getCipher(cipherType), nullptr, key.data(), iv.data());
}

CryptoReadDeviceProxy::~CryptoReadDeviceProxy()
{
	if(decryptCtx) EVP_CIPHER_CTX_free(decryptCtx);
}

CryptoReadDeviceProxy::CryptoReadDeviceProxy(CryptoReadDeviceProxy&& mov)
	: Elv::Io::BufferedReadDeviceProxy(std::move(mov)), key(std::move(mov.key)), cipherType(mov.cipherType)
{
	std::memcpy(iv.data(), mov.iv.data(), mov.iv.size());
	this->decryptCtx = mov.decryptCtx;
	mov.decryptCtx = nullptr;
}

CryptoReadDeviceProxy& CryptoReadDeviceProxy::operator=(CryptoReadDeviceProxy&& mov)
{
	this->cipherType = mov.cipherType;
	this->key = std::move(mov.key);
	std::memcpy(iv.data(), mov.iv.data(), mov.iv.size());
	this->decryptCtx = mov.decryptCtx;
	mov.decryptCtx = nullptr;
	BufferedReadDeviceProxy::operator=(std::move(mov));
	return *this;
}


}
}
