#pragma once

#include <string>

namespace si::i18n {

void set_language(const std::string& code);

const std::string& tr(const std::string& key);

}
