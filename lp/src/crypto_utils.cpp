#include "crypto_utils.hpp"
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>

std::string to_hex(const unsigned char *data, size_t len)
{
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return oss.str();
}

std::string compute_hmac(const std::string &data, const std::string &key)
{
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int len;
    HMAC(EVP_sha256(), key.data(), key.size(), (unsigned char *)data.data(), data.size(), result, &len);
    return to_hex(result, len);
}

bool verify_hmac(const std::string &data, const std::string &key, const std::string &expected_hmac)
{
    return compute_hmac(data, key) == expected_hmac;
}
