#pragma once

#include <string>

enum class EFileType : uint8_t;

namespace xatrix {

bool isXatrixFile(EFileType type);

void c93_to_wav(const std::string& in_path, const std::string& in_outFilePath);
void z94_to_wav(const std::string& in_path, const std::string& in_outFilePath);
void m95_to_wav(const std::string& in_path, const std::string& in_outFilePath);

} // namespace xatrix
