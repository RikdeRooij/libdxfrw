#pragma once

#include <sstream>
#include "dx_data.h"
#include "dx_iface.h"

// format variable arguments
const char* va(const char* format, ...);

// format variable arguments
std::string string_format(const char *fmt, ...);


bool WriteEntity(dx_iface *input, DRW_Entity * e, std::stringstream &svg, const double &s, bool alt);

bool extractSvg(dx_iface *input, const char* outFileName);
