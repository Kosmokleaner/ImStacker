#pragma once

extern const char* fragment_shader_text0;
extern const char* fragment_shader_text1;

// 0:for testing, 1: default
#define SHADER_SUPPORT 1

// @param inCode must not be 0
void recompileShaders(const char* inCode, std::string &warningsAndErrors);

// todo:refactor
void deinit();
