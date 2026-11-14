#ifndef EUPHCRYPTOPROXYDEVICE_HPP
#define EUPHCRYPTOPROXYDEVICE_HPP
/**
 * @file EuphCryptoProxyDevice.hpp
 * @brief Declares the EuphCryptoProxyDevice API in the Euphemy/Io module.
 *
 * This header is part of the public declaration surface for Euphemy/Io.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDeviceProxy.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
namespace Euph {
namespace Io {

// Enum listing all the supported ciphers
/**
 * @enum CipherType
 * @brief Enumeration of supported cipher types for encryption and decryption.
 *
 * This enum class lists various cipher types, including block ciphers (e.g., AES, DES, IDEA),
 * stream ciphers (e.g., RC4, ChaCha20), and modes of operation (e.g., ECB, CBC, CFB, OFB, CTR).
 *
 * @warning Not all ciphers or modes may be supported by the underlying cryptographic library.
 */
enum class CipherType : uint8_t {
	/**
	 * @brief Null cipher (no encryption).
	 */
	NULL_CIPHER,

	// DES (Data Encryption Standard) ciphers
	/**
	 * @brief DES with Electronic Codebook (ECB) mode.
	 */
	DES_ECB,
	/**
	 * @brief DES with Encrypt-Decrypt-Encrypt (EDE) mode.
	 */
	DES_EDE,
	/**
	 * @brief Triple DES with Encrypt-Decrypt-Encrypt (EDE) mode.
	 */
	DES_EDE3,
	/**
	 * @brief Triple DES with EDE in ECB mode.
	 */
	DES_EDE_ECB,
	/**
	 * @brief Triple DES with EDE in ECB mode (alternative).
	 */
	DES_EDE3_ECB,
	/**
	 * @brief DES with 64-bit Cipher Feedback (CFB) mode.
	 */
	DES_CFB64,
	/**
	 * @brief DES with 1-bit Cipher Feedback (CFB) mode.
	 */
	DES_CFB1,
	/**
	 * @brief DES with 8-bit Cipher Feedback (CFB) mode.
	 */
	DES_CFB8,
	/**
	 * @brief Triple DES with 64-bit CFB mode.
	 */
	DES_EDE_CFB64,
	/**
	 * @brief Triple DES with 64-bit CFB mode (alternative).
	 */
	DES_EDE3_CFB64,
	/**
	 * @brief Triple DES with 1-bit CFB mode.
	 */
	DES_EDE3_CFB1,
	/**
	 * @brief Triple DES with 8-bit CFB mode.
	 */
	DES_EDE3_CFB8,
	/**
	 * @brief DES with Output Feedback (OFB) mode.
	 */
	DES_OFB,
	/**
	 * @brief DES with EDE in OFB mode.
	 */
	DES_EDE_OFB,
	/**
	 * @brief Triple DES with EDE in OFB mode.
	 */
	DES_EDE3_OFB,
	/**
	 * @brief DES with Cipher Block Chaining (CBC) mode.
	 */
	DES_CBC,
	/**
	 * @brief DES with EDE in CBC mode.
	 */
	DES_EDE_CBC,
	/**
	 * @brief Triple DES with EDE in CBC mode.
	 */
	DES_EDE3_CBC,
	/**
	 * @brief DES-X with CBC mode.
	 */
	DESX_CBC,
	/**
	 * @brief Triple DES with EDE wrap mode.
	 */
	DES_EDE3_WRAP,

	// RC (Rivest Cipher) ciphers
	/**
	 * @brief RC4 stream cipher.
	 */
	RC4,
	/**
	 * @brief RC4 with 40-bit key.
	 */
	RC4_40,
	/**
	 * @brief RC4 with HMAC-MD5.
	 */
	RC4_HMAC_MD5,

	// IDEA (International Data Encryption Algorithm) ciphers
	/**
	 * @brief IDEA with ECB mode.
	 */
	IDEA_ECB,
	/**
	 * @brief IDEA with 64-bit CFB mode.
	 */
	IDEA_CFB64,
	/**
	 * @brief IDEA with Output Feedback (OFB) mode.
	 */
	IDEA_OFB,
	/**
	 * @brief IDEA with Cipher Block Chaining (CBC) mode.
	 */
	IDEA_CBC,

	// RC2 (Rivest Cipher 2) ciphers
	/**
	 * @brief RC2 with ECB mode.
	 */
	RC2_ECB,
	/**
	 * @brief RC2 with CBC mode.
	 */
	RC2_CBC,
	/**
	 * @brief RC2 with 40-bit key in CBC mode.
	 */
	RC2_40_CBC,
	/**
	 * @brief RC2 with 64-bit key in CBC mode.
	 */
	RC2_64_CBC,
	/**
	 * @brief RC2 with 64-bit CFB mode.
	 */
	RC2_CFB64,
	/**
	 * @brief RC2 with Output Feedback (OFB) mode.
	 */
	RC2_OFB,

	// Blowfish (BF) ciphers
	/**
	 * @brief Blowfish with ECB mode.
	 */
	BF_ECB,
	/**
	 * @brief Blowfish with CBC mode.
	 */
	BF_CBC,
	/**
	 * @brief Blowfish with 64-bit CFB mode.
	 */
	BF_CFB64,
	/**
	 * @brief Blowfish with Output Feedback (OFB) mode.
	 */
	BF_OFB,

	// CAST5 ciphers
	/**
	 * @brief CAST5 with ECB mode.
	 */
	CAST5_ECB,
	/**
	 * @brief CAST5 with CBC mode.
	 */
	CAST5_CBC,
	/**
	 * @brief CAST5 with 64-bit CFB mode.
	 */
	CAST5_CFB64,
	/**
	 * @brief CAST5 with Output Feedback (OFB) mode.
	 */
	CAST5_OFB,

	// RC5 (Rivest Cipher 5) ciphers
	/**
	 * @brief RC5 with 32-bit key, 12 rounds, and 16 bytes in CBC mode.
	 */
	RC5_32_12_16_CBC,
	/**
	 * @brief RC5 with 32-bit key, 12 rounds, and 16 bytes in ECB mode.
	 */
	RC5_32_12_16_ECB,
	/**
	 * @brief RC5 with 32-bit key, 12 rounds, and 16 bytes in 64-bit CFB mode.
	 */
	RC5_32_12_16_CFB64,
	/**
	 * @brief RC5 with 32-bit key, 12 rounds, and 16 bytes in OFB mode.
	 */
	RC5_32_12_16_OFB,

	// AES (Advanced Encryption Standard) ciphers
	/**
	 * @brief AES with 128-bit key in ECB mode.
	 */
	AES_128_ECB,
	/**
	 * @brief AES with 128-bit key in CBC mode.
	 */
	AES_128_CBC,
	/**
	 * @brief AES with 128-bit key in 1-bit CFB mode.
	 */
	AES_128_CFB1,
	/**
	 * @brief AES with 128-bit key in 8-bit CFB mode.
	 */
	AES_128_CFB8,
	/**
	 * @brief AES with 128-bit key in 128-bit CFB mode.
	 */
	AES_128_CFB128,
	/**
	 * @brief AES with 128-bit key in Output Feedback (OFB) mode.
	 */
	AES_128_OFB,
	/**
	 * @brief AES with 128-bit key in Counter (CTR) mode.
	 */
	AES_128_CTR,
	/**
	 * @brief AES with 128-bit key in Counter with CBC-MAC (CCM) mode.
	 */
	AES_128_CCM,
	/**
	 * @brief AES with 128-bit key in Galois/Counter Mode (GCM).
	 */
	AES_128_GCM,
	/**
	 * @brief AES with 128-bit key in XTS mode.
	 */
	AES_128_XTS,
	/**
	 * @brief AES with 128-bit key in wrap mode.
	 */
	AES_128_WRAP,
	/**
	 * @brief AES with 128-bit key in padded wrap mode.
	 */
	AES_128_WRAP_PAD,
	/**
	 * @brief AES with 128-bit key in OCB mode.
	 */
	AES_128_OCB,

	/**
	 * @brief AES with 192-bit key in ECB mode.
	 */
	AES_192_ECB,
	/**
	 * @brief AES with 192-bit key in CBC mode.
	 */
	AES_192_CBC,
	/**
	 * @brief AES with 192-bit key in 1-bit CFB mode.
	 */
	AES_192_CFB1,
	/**
	 * @brief AES with 192-bit key in 8-bit CFB mode.
	 */
	AES_192_CFB8,
	/**
	 * @brief AES with 192-bit key in 128-bit CFB mode.
	 */
	AES_192_CFB128,
	/**
	 * @brief AES with 192-bit key in Output Feedback (OFB) mode.
	 */
	AES_192_OFB,
	/**
	 * @brief AES with 192-bit key in Counter (CTR) mode.
	 */
	AES_192_CTR,
	/**
	 * @brief AES with 192-bit key in Counter with CBC-MAC (CCM) mode.
	 */
	AES_192_CCM,
	/**
	 * @brief AES with 192-bit key in Galois/Counter Mode (GCM).
	 */
	AES_192_GCM,
	/**
	 * @brief AES with 192-bit key in wrap mode.
	 */
	AES_192_WRAP,
	/**
	 * @brief AES with 192-bit key in padded wrap mode.
	 */
	AES_192_WRAP_PAD,
	/**
	 * @brief AES with 192-bit key in OCB mode.
	 */
	AES_192_OCB,

	/**
	 * @brief AES with 256-bit key in ECB mode.
	 */
	AES_256_ECB,
	/**
	 * @brief AES with 256-bit key in CBC mode.
	 */
	AES_256_CBC,
	/**
	 * @brief AES with 256-bit key in 1-bit CFB mode.
	 */
	AES_256_CFB1,
	/**
	 * @brief AES with 256-bit key in 8-bit CFB mode.
	 */
	AES_256_CFB8,
	/**
	 * @brief AES with 256-bit key in 128-bit CFB mode.
	 */
	AES_256_CFB128,
	/**
	 * @brief AES with 256-bit key in Output Feedback (OFB) mode.
	 */
	AES_256_OFB,
	/**
	 * @brief AES with 256-bit key in Counter (CTR) mode.
	 */
	AES_256_CTR,
	/**
	 * @brief AES with 256-bit key in Counter with CBC-MAC (CCM) mode.
	 */
	AES_256_CCM,
	/**
	 * @brief AES with 256-bit key in Galois/Counter Mode (GCM).
	 */
	AES_256_GCM,
	/**
	 * @brief AES with 256-bit key in XTS mode.
	 */
	AES_256_XTS,
	/**
	 * @brief AES with 256-bit key in wrap mode.
	 */
	AES_256_WRAP,
	/**
	 * @brief AES with 256-bit key in padded wrap mode.
	 */
	AES_256_WRAP_PAD,
	/**
	 * @brief AES with 256-bit key in OCB mode.
	 */
	AES_256_OCB,

	/**
	 * @brief AES with 128-bit key in CBC mode with HMAC-SHA1.
	 */
	AES_128_CBC_HMAC_SHA1,
	/**
	 * @brief AES with 256-bit key in CBC mode with HMAC-SHA1.
	 */
	AES_256_CBC_HMAC_SHA1,
	/**
	 * @brief AES with 128-bit key in CBC mode with HMAC-SHA256.
	 */
	AES_128_CBC_HMAC_SHA256,
	/**
	 * @brief AES with 256-bit key in CBC mode with HMAC-SHA256.
	 */
	AES_256_CBC_HMAC_SHA256,

	// ARIA (Academic and Research Institute of Korea Algorithm) ciphers
	/**
	 * @brief ARIA with 128-bit key in ECB mode.
	 */
	ARIA_128_ECB,
	/**
	 * @brief ARIA with 128-bit key in CBC mode.
	 */
	ARIA_128_CBC,
	/**
	 * @brief ARIA with 128-bit key in 1-bit CFB mode.
	 */
	ARIA_128_CFB1,
	/**
	 * @brief ARIA with 128-bit key in 8-bit CFB mode.
	 */
	ARIA_128_CFB8,
	/**
	 * @brief ARIA with 128-bit key in 128-bit CFB mode.
	 */
	ARIA_128_CFB128,
	/**
	 * @brief ARIA with 128-bit key in Counter (CTR) mode.
	 */
	ARIA_128_CTR,
	/**
	 * @brief ARIA with 128-bit key in Output Feedback (OFB) mode.
	 */
	ARIA_128_OFB,
	/**
	 * @brief ARIA with 128-bit key in Galois/Counter Mode (GCM).
	 */
	ARIA_128_GCM,
	/**
	 * @brief ARIA with 128-bit key in Counter with CBC-MAC (CCM) mode.
	 */
	ARIA_128_CCM,

	/**
	 * @brief ARIA with 192-bit key in ECB mode.
	 */
	ARIA_192_ECB,
	/**
	 * @brief ARIA with 192-bit key in CBC mode.
	 */
	ARIA_192_CBC,
	/**
	 * @brief ARIA with 192-bit key in 1-bit CFB mode.
	 */
	ARIA_192_CFB1,
	/**
	 * @brief ARIA with 192-bit key in 8-bit CFB mode.
	 */
	ARIA_192_CFB8,
	/**
	 * @brief ARIA with 192-bit key in 128-bit CFB mode.
	 */
	ARIA_192_CFB128,
	/**
	 * @brief ARIA with 192-bit key in Counter (CTR) mode.
	 */
	ARIA_192_CTR,
	/**
	 * @brief ARIA with 192-bit key in Output Feedback (OFB) mode.
	 */
	ARIA_192_OFB,
	/**
	 * @brief ARIA with 192-bit key in Galois/Counter Mode (GCM).
	 */
	ARIA_192_GCM,
	/**
	 * @brief ARIA with 192-bit key in Counter with CBC-MAC (CCM) mode.
	 */
	ARIA_192_CCM,

	/**
	 * @brief ARIA with 256-bit key in ECB mode.
	 */
	ARIA_256_ECB,
	/**
	 * @brief ARIA with 256-bit key in CBC mode.
	 */
	ARIA_256_CBC,
	/**
	 * @brief ARIA with 256-bit key in 1-bit CFB mode.
	 */
	ARIA_256_CFB1,
	/**
	 * @brief ARIA with 256-bit key in 8-bit CFB mode.
	 */
	ARIA_256_CFB8,
	/**
	 * @brief ARIA with 256-bit key in 128-bit CFB mode.
	 */
	ARIA_256_CFB128,
	/**
	 * @brief ARIA with 256-bit key in Counter (CTR) mode.
	 */
	ARIA_256_CTR,
	/**
	 * @brief ARIA with 256-bit key in Output Feedback (OFB) mode.
	 */
	ARIA_256_OFB,
	/**
	 * @brief ARIA with 256-bit key in Galois/Counter Mode (GCM).
	 */
	ARIA_256_GCM,
	/**
	 * @brief ARIA with 256-bit key in Counter with CBC-MAC (CCM) mode.
	 */
	ARIA_256_CCM,

	// Camellia ciphers
	/**
	 * @brief Camellia with 128-bit key in ECB mode.
	 */
	CAMELLIA_128_ECB,
	/**
	 * @brief Camellia with 128-bit key in CBC mode.
	 */
	CAMELLIA_128_CBC,
	/**
	 * @brief Camellia with 128-bit key in 1-bit CFB mode.
	 */
	CAMELLIA_128_CFB1,
	/**
	 * @brief Camellia with 128-bit key in 8-bit CFB mode.
	 */
	CAMELLIA_128_CFB8,
	/**
	 * @brief Camellia with 128-bit key in 128-bit CFB mode.
	 */
	CAMELLIA_128_CFB128,
	/**
	 * @brief Camellia with 128-bit key in Output Feedback (OFB) mode.
	 */
	CAMELLIA_128_OFB,
	/**
	 * @brief Camellia with 128-bit key in Counter (CTR) mode.
	 */
	CAMELLIA_128_CTR,

	/**
	 * @brief Camellia with 192-bit key in ECB mode.
	 */
	CAMELLIA_192_ECB,
	/**
	 * @brief Camellia with 192-bit key in CBC mode.
	 */
	CAMELLIA_192_CBC,
	/**
	 * @brief Camellia with 192-bit key in 1-bit CFB mode.
	 */
	CAMELLIA_192_CFB1,
	/**
	 * @brief Camellia with 192-bit key in 8-bit CFB mode.
	 */
	CAMELLIA_192_CFB8,
	/**
	 * @brief Camellia with 192-bit key in 128-bit CFB mode.
	 */
	CAMELLIA_192_CFB128,
	/**
	 * @brief Camellia with 192-bit key in Output Feedback (OFB) mode.
	 */
	CAMELLIA_192_OFB,
	/**
	 * @brief Camellia with 192-bit key in Counter (CTR) mode.
	 */
	CAMELLIA_192_CTR,

	/**
	 * @brief Camellia with 256-bit key in ECB mode.
	 */
	CAMELLIA_256_ECB,
	/**
	 * @brief Camellia with 256-bit key in CBC mode.
	 */
	CAMELLIA_256_CBC,
	/**
	 * @brief Camellia with 256-bit key in 1-bit CFB mode.
	 */
	CAMELLIA_256_CFB1,
	/**
	 * @brief Camellia with 256-bit key in 8-bit CFB mode.
	 */
	CAMELLIA_256_CFB8,
	/**
	 * @brief Camellia with 256-bit key in 128-bit CFB mode.
	 */
	CAMELLIA_256_CFB128,
	/**
	 * @brief Camellia with 256-bit key in Output Feedback (OFB) mode.
	 */
	CAMELLIA_256_OFB,
	/**
	 * @brief Camellia with 256-bit key in Counter (CTR) mode.
	 */
	CAMELLIA_256_CTR,

	// ChaCha20 ciphers
	/**
	 * @brief ChaCha20 stream cipher.
	 */
	CHACHA20,
	/**
	 * @brief ChaCha20 with Poly1305 MAC.
	 */
	CHACHA20_POLY1305,

	// SEED ciphers
	/**
	 * @brief SEED with ECB mode.
	 */
	SEED_ECB,
	/**
	 * @brief SEED with CBC mode.
	 */
	SEED_CBC,
	/**
	 * @brief SEED with 128-bit CFB mode.
	 */
	SEED_CFB128,
	/**
	 * @brief SEED with Output Feedback (OFB) mode.
	 */
	SEED_OFB,

	// SM4 ciphers
	/**
	 * @brief SM4 with ECB mode.
	 */
	SM4_ECB,
	/**
	 * @brief SM4 with CBC mode.
	 */
	SM4_CBC,
	/**
	 * @brief SM4 with 128-bit CFB mode.
	 */
	SM4_CFB128,
	/**
	 * @brief SM4 with Output Feedback (OFB) mode.
	 */
	SM4_OFB,
	/**
	 * @brief SM4 with Counter (CTR) mode.
	 */
	SM4_CTR
};

/**
 * @addtogroup EuphIo
 * @{
 */

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(CryptoWriteDeviceProxy)
/**
 * @class CryptoWriteDeviceProxy
 * @brief A write proxy device that encrypts data before writing it to an underlying Device.
 *
 * This class derives from Elv::Io::BufferedWriteDeviceProxy and utilizes OpenSSL for encryption.
 * It provides a way to securely write data to a device by encrypting the data in transit.
 *
 * @warning This class is non-copyable. Use move semantics for transfers.
 */
class MH_EUPH_API CryptoWriteDeviceProxy : public Elv::Io::BufferedWriteDeviceProxy
{
private:
	EVP_CIPHER_CTX* encryptCtx; ///< Encryption context for OpenSSL operations.
	std::array<unsigned char, EVP_MAX_IV_LENGTH> iv; ///< Initialization vector for encryption.
	std::vector<unsigned char> key; ///< Encryption key (stored securely).
	bool finalized; ///< Flag indicating if the encryption context is finalized.
	bool firstWrite; ///< Flag for tracking the first write to handle IV.
	CipherType cipherType; ///< Type of cipher used for encryption.

	/**
	 * @brief Deleted copy constructor to prevent copying.
	 */
	CryptoWriteDeviceProxy(const CryptoWriteDeviceProxy& cpy) = delete;

	/**
	 * @brief Deleted copy assignment operator to prevent copying.
	 */
	CryptoWriteDeviceProxy& operator=(const CryptoWriteDeviceProxy& cpy) = delete;

protected:
	/**
	 * @brief Processes the input buffer by encrypting its contents and writing the result to the output buffer.
	 *
	 * Utilizes the OpenSSL EVP_EncryptUpdate function for encryption.
	 *
	 * @return Number of bytes written to the output buffer.
	 */
	size_t processInBufferToOutBuffer() override;

	/**
	 * @brief Writes a prefix (initialization vector) before the encrypted data to the underlying Device.
	 *
	 * This is automatically handled during the first write operation.
	 *
	 * @param device Reference to the underlying Device.
	 * @return Number of bytes written as the prefix (IV).
	 */
	size_t writePrefix(Device& device) override;

	/**
	 * @brief Writes a postfix after the encrypted data to the underlying Device.
	 *
	 * Currently, no postfix is written.
	 *
	 * @param device Reference to the underlying Device.
	 * @return Number of bytes written as the postfix (always 0).
	 */
	size_t writePostfix(Device& device) override;

public:
	/**
	 * @brief Constructor for CryptoWriteDeviceProxy with a const key vector.
	 *
	 * Initializes the encryption context with the provided cipher type and key.
	 * Generates a random initialization vector (IV) for encryption.
	 *
	 * @param underlyingDevice Pointer to the underlying Device.
	 * @param inBufferSize Size of the input buffer in bytes.
	 * @param outBufferSize Size of the output buffer in bytes.
	 * @param cipherType Type of cipher to use for encryption.
	 * @param key Const reference to the encryption key.
	 */
	CryptoWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, const std::vector<unsigned char>& key);

	/**
	 * @brief Constructor for CryptoWriteDeviceProxy with an rvalue key vector.
	 *
	 * Initializes the encryption context with the provided cipher type and key.
	 * Generates a random initialization vector (IV) for encryption.
	 *
	 * @param underlyingDevice Pointer to the underlying Device.
	 * @param inBufferSize Size of the input buffer in bytes.
	 * @param outBufferSize Size of the output buffer in bytes.
	 * @param cipherType Type of cipher to use for encryption.
	 * @param key Rvalue reference to the encryption key (moved into the object).
	 */
	CryptoWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, std::vector<unsigned char>&& key);

	/**
	 * @brief Destructor for CryptoWriteDeviceProxy.
	 *
	 * Finalizes the encryption context (if not already done) and frees OpenSSL resources.
	 */
	~CryptoWriteDeviceProxy();

	/**
	 * @brief Move constructor for CryptoWriteDeviceProxy.
	 *
	 * Transfers ownership of the encryption context and resources.
	 *
	 * @param mov The CryptoWriteDeviceProxy to move from.
	 */
	CryptoWriteDeviceProxy(CryptoWriteDeviceProxy&& mov);

	/**
	 * @brief Move assignment operator for CryptoWriteDeviceProxy.
	 *
	 * Transfers ownership of the encryption context and resources.
	 *
	 * @param mov The CryptoWriteDeviceProxy to move from.
	 * @return Reference to this CryptoWriteDeviceProxy after the move.
	 */
	CryptoWriteDeviceProxy& operator=(CryptoWriteDeviceProxy&& mov);

	/**
	 * @brief Callback function called when the device is finished.
	 *
	 * Finalizes the encryption context if not already done and writes any remaining encrypted data.
	 */
	void onFinish() override;

	/**
	 * @brief Retrieves the type of cipher used for encryption.
	 *
	 * @return The CipherType enum value representing the cipher in use.
	 */
	CipherType getCipherType() const;
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(CryptoReadDeviceProxy)
/**
 * @class CryptoReadDeviceProxy
 * @brief A read proxy device that decrypts data read from an underlying Device.
 *
 * This class derives from Elv::Io::BufferedReadDeviceProxy and utilizes OpenSSL for decryption.
 * It provides a way to securely read data from a device by decrypting the data in transit.
 *
 * @warning This class is non-copyable. Use move semantics for transfers.
 */
class MH_EUPH_API CryptoReadDeviceProxy : public Elv::Io::BufferedReadDeviceProxy
{
private:
	EVP_CIPHER_CTX* decryptCtx; ///< Decryption context for OpenSSL operations.
	std::vector<unsigned char> key; ///< Decryption key (stored securely).
	std::array<unsigned char, EVP_MAX_IV_LENGTH> iv; ///< Initialization vector for decryption.
	CipherType cipherType; ///< Type of cipher used for decryption.

	/**
	 * @brief Deleted copy constructor to prevent copying.
	 */
	CryptoReadDeviceProxy(const CryptoReadDeviceProxy& cpy) = delete;

	/**
	 * @brief Deleted copy assignment operator to prevent copying.
	 */
	CryptoReadDeviceProxy& operator=(const CryptoReadDeviceProxy& cpy) = delete;

protected:
	/**
	 * @brief Processes the input buffer by decrypting its contents and writing the result to the output buffer.
	 *
	 * Utilizes the OpenSSL EVP_DecryptUpdate function for decryption.
	 *
	 * @param bytesRead Number of bytes read from the underlying Device.
	 * @return Number of bytes written to the output buffer.
	 */
	size_t processInBufferToOutBuffer(size_t bytesRead) override;

public:
	/**
	 * @brief Constructor for CryptoReadDeviceProxy with a const key vector.
	 *
	 * Initializes the decryption context with the provided cipher type and key.
	 * Reads the initialization vector (IV) from the beginning of the encrypted data.
	 *
	 * @param underlyingDevice Pointer to the underlying Device.
	 * @param inBufferSize Size of the input buffer in bytes.
	 * @param outBufferSize Size of the output buffer in bytes.
	 * @param cipherType Type of cipher to use for decryption.
	 * @param key Const reference to the decryption key.
	 */
	CryptoReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, const std::vector<unsigned char>& key);

	/**
	 * @brief Constructor for CryptoReadDeviceProxy with an rvalue key vector.
	 *
	 * Initializes the decryption context with the provided cipher type and key.
	 * Reads the initialization vector (IV) from the beginning of the encrypted data.
	 *
	 * @param underlyingDevice Pointer to the underlying Device.
	 * @param inBufferSize Size of the input buffer in bytes.
	 * @param outBufferSize Size of the output buffer in bytes.
	 * @param cipherType Type of cipher to use for decryption.
	 * @param key Rvalue reference to the decryption key (moved into the object).
	 */
	CryptoReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize, CipherType cipherType, std::vector<unsigned char>&& key);

	/**
	 * @brief Destructor for CryptoReadDeviceProxy.
	 *
	 * Frees OpenSSL resources.
	 */
	~CryptoReadDeviceProxy();

	/**
	 * @brief Move constructor for CryptoReadDeviceProxy.
	 *
	 * Transfers ownership of the decryption context and resources.
	 *
	 * @param mov The CryptoReadDeviceProxy to move from.
	 */
	CryptoReadDeviceProxy(CryptoReadDeviceProxy&& mov);

	/**
	 * @brief Move assignment operator for CryptoReadDeviceProxy.
	 *
	 * Transfers ownership of the decryption context and resources.
	 *
	 * @param mov The CryptoReadDeviceProxy to move from.
	 * @return Reference to this CryptoReadDeviceProxy after the move.
	 */
	CryptoReadDeviceProxy& operator=(CryptoReadDeviceProxy&& mov);

	/**
	 * @brief Retrieves the type of cipher used for decryption.
	 *
	 * @return The CipherType enum value representing the cipher in use.
	 */
	CipherType getCipherType() const;
};

/** @} */ // End of Doxygen group EuphIo

}
}

#endif // EUPHCRYPTOPROXYDEVICE_HPP
