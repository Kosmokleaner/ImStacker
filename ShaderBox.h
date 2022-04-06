#pragma once

extern const char* fragment_shader_text0;
extern const char* fragment_shader_text1;


// @param inCode must not be 0
void recompileShaders(const char* inCode, std::string &warningsAndErrors);
