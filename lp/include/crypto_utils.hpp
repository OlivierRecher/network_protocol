#pragma once
#include <string>

std::string compute_hmac(const std::string &data, const std::string &key);
bool verify_hmac(const std::string &data, const std::string &key, const std::string &expected_hmac);
