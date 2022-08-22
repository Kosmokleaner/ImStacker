// to prevent error C4996: 'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead.
#define _CRT_SECURE_NO_WARNINGS 1

#include "CppStackerBox.h"
#include <vector>
#include <assert.h>
#include "imgui.h"
#include "ShaderBox.h"


#ifdef _WIN32
// We rely on string literate compare for simpler code.
// warning C4130 : '==' : logical operation on address of string constant
#pragma warning( disable : 4130 )
#else
#define	sprintf_s(buffer, buffer_size, stringbuffer, ...) (sprintf(buffer, stringbuffer, __VA_ARGS__))
#endif

// literal (number, slider): bool, int, float, string, color, bitmap, struct? float slider
// input: camera
// conversion float->int, int->float, int->string, string->int,  ...
// + - * / pow(a, b), sin(), cos(), frac()
// get_var, set_var
// print
// composite e.g. float slider in 0..1 range

// todo: copy,paste, delete with key binding

// 
EDataType combineTypes(EDataType a, EDataType b) {

  if(a > b) {
    std::swap(a, b);
  }
  return b;
}

// @return vIndex
int32 upgradeTypeIfNeeded(GenerateCodeContext& context, const EDataType targetDataType, const CppStackerBox& param) {
  if (param.dataType == targetDataType)
    return param.vIndex;

  return param.castTo(context, targetDataType);
}

// @return vIndex
int32 CppStackerBox::castTo(GenerateCodeContext& context, const EDataType dstDataType) const {
  if (dataType == dstDataType) {
    // no cast needed
    return vIndex;
  }

  if (context.code) {
    const char* dstDataTypeStr = getGLSLTypeName(dstDataType);
    char str[256];

    // cast int:float
    // ivec2:vec2, ivec3:vec3, ivec4:vec4, float->vec2, float->vec3, float->vec4
    const char* fmt = "%s v%d = %s(v%d);\n%s\n";

    // expand by one
    if((dataType == EDT_Float && dstDataType == EDT_Float2)
    || (dataType == EDT_Float2 && dstDataType == EDT_Float3)
    || (dataType == EDT_Float3 && dstDataType == EDT_Float4))
      fmt = "%s v%d = %s(v%d, 0);\n%s\n";

    // expand by two
    if((dataType == EDT_Float && dstDataType == EDT_Float3)
      || (dataType == EDT_Float2 && dstDataType == EDT_Float4))
      fmt = "%s v%d = %s(v%d, 0, 0);\n%s\n";

    sprintf_s(str, sizeof(str), fmt,
      dstDataTypeStr,
      context.nextFreeVIndex,
      dstDataTypeStr,
      vIndex,
      context.indentationStr);

    *context.code += str;
  }

  return context.nextFreeVIndex++;
}


// see https://github.com/ocornut/imgui/issues/2035
static int InputTextCallback(ImGuiInputTextCallbackData* data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    // Resize string callback
    std::string* str = (std::string*)data->UserData;
    IM_ASSERT(data->Buf == str->c_str());
    str->resize(data->BufTextLen);
    data->Buf = (char*)str->c_str();
  }
  return 0;
}
bool imguiInputText(const char* label, std::string& str, ImGuiInputTextFlags flags) {
  flags |= ImGuiInputTextFlags_CallbackResize;
  return ImGui::InputText(label, (char*)str.c_str(), str.capacity() + 1, flags, InputTextCallback, (void*)&str);
}

// @param tooltip must not be 0
void imguiToolTip(const char* tooltip) {
  assert(tooltip);

  if (ImGui::IsItemHovered()) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::SetTooltip("%s", tooltip);
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(1);
  }
}

StackerBox* CppAppConnection::createNode(const char* className) {
  assert(className);

  if (strcmp(className, "CppStackerBox") == 0)
    return new CppStackerBox();

  if (strcmp(className, "CppStackerBoxConstant") == 0)
    return new CppStackerBoxConstant();

  return nullptr;
}

void CppAppConnection::startCompile() {
  generatedCode.clear();
  generatedCode += fragment_shader_text0;

}
void CppAppConnection::endCompile() {
  // trim right
  generatedCode.erase(generatedCode.find_last_not_of("\t ") + 1);
  generatedCode += fragment_shader_text1;
}

void CppAppConnection::reCompile() {
  recompileShaders(generatedCode.c_str(), warningsAndErrors);
}

void CppAppConnection::openContextMenu(StackerUI& stackerUI, const StackerBoxRect& rect) {

// @param className e.g. CppStackerBox
#define ENTRY(className, enumName, displayName, tooltip) \
    if(ImGui::MenuItem(displayName, nullptr)) { \
        auto obj = new className(); \
        obj->nodeType = enumName; \
        obj->rect = rect; \
        obj->name = displayName; \
        stackerUI.addFromUI(*obj); \
    } \
    imguiToolTip(tooltip);

  // @param inConstantType e.g. ECT_Float3
#define ENTRY_CONSTANT(inName, inConstantType, tooltip) \
    if(ImGui::MenuItem(#inName, nullptr)) { \
        auto obj = new CppStackerBoxConstant(); \
        obj->constantType = inConstantType;\
        obj->name = "Constant"; \
        obj->rect = rect; \
        obj->dataType = obj->getDataType(); \
        stackerUI.addFromUI(*obj); \
    } \
    imguiToolTip(tooltip);

  {
    ImGui::TextDisabled("No input");
    ImGui::Indent();
  //    ENTRY(IntVariable, "Integer variable (no fractional part)");
    ENTRY_CONSTANT(Constant, CppStackerBoxConstant::ECT_Float, "Floating point constant (with fractional part)\ne.g. Float, Float2, Float3, Float4, ColorRGB, ColorRGBA");
  //  ENTRY_CONSTANT(Vec2 Constant, CppStackerBoxConstant::ECT_Float2, "Float2 constant");
  //  ENTRY_CONSTANT(Vec3 Constant, CppStackerBoxConstant::ECT_Float3, "Float3 constant");
  //  ENTRY_CONSTANT(Vec4 Constant, CppStackerBoxConstant::ECT_Float4, "Float4 constant");
  //  ENTRY_CONSTANT(RGB Constant, CppStackerBoxConstant::ECT_ColorRGB, "Color RGB constant");
  //  ENTRY_CONSTANT(RGBA Constant, CppStackerBoxConstant::ECT_RGBA, "Color RGBA constant");
    ENTRY(CppStackerBox, NT_FragCoord, "FragCoord", "see OpenGL gl_FragCoord");
    ENTRY(CppStackerBox, NT_ScreenUV, "ScreenUV", "screen xy in 0..1 range");
    ENTRY(CppStackerBox, NT_Rand, "Rand", "float random in 0..1 range");
    ENTRY(CppStackerBox, NT_Time, "Time", "float time in seconds (precision loss when large)");
    ImGui::Unindent();
  }

  ImGui::Separator();

  {
    ImGui::TextDisabled("One input");
    ImGui::Indent();
    ENTRY(CppStackerBox, NT_Sin, "Sin", "Sin() in radiants");
    ENTRY(CppStackerBox, NT_Cos, "Cos", "Cos() in radiants");
    ENTRY(CppStackerBox, NT_Frac, "Frac", "like HLSL frac() = x - floor(x)");
    ENTRY(CppStackerBox, NT_Saturate, "Saturate", "like HLSL saturate(), clamp betwen 0 and 1)");
    ENTRY(CppStackerBox, NT_Sqrt, "Sqrt", "like HLSL sqrt(), square root");
    ENTRY(CppStackerBox, NT_OneMinusX, "1-x", "like HLSL sqrt(), square root");
    ImGui::Unindent();
  }
  ImGui::Separator();

  {
    ImGui::TextDisabled("Two or more inputs");
    ImGui::Indent();  
    ENTRY(CppStackerBox, NT_Add, "Add", "Sum up multiple inputs of same type");
    ENTRY(CppStackerBox, NT_Sub, "Sub", "Subtract two numbers or negate one number");
    ENTRY(CppStackerBox, NT_Mul, "Mul", "Multiple multiple numbers");
    ENTRY(CppStackerBox, NT_Div, "Div", "Divide two numbers, 1/x a single number");
    ENTRY(CppStackerBox, NT_Pow, "Pow", "Raise x to the power of y = x^y");
    ENTRY(CppStackerBox, NT_Lerp, "Lerp", "like HLSL lerp(x0,x1,a) = x0*(1-a) + x1*a, linear interpolation");
    ENTRY(CppStackerBox, NT_Dot, "Dot", "like HLSL dot(a, b), dot(a,a) if there is no b");
    ENTRY(CppStackerBoxSwizzle, NT_Swizzle, "Swizzle", "like UE4, ComponentMask and AppendVector x,y,z,w");
    ENTRY(CppStackerBox, NT_RGBOutput, "RGBOutput", "output vec4 as postprocess RGB color (Alpha is ignored)");
    ImGui::Unindent();
  }

#undef ENTRY

  ImGui::Separator();
  stackerUI.cutCopyPasteMenu(rect.x, rect.y);
}

const char* getGLSLTypeName(const EDataType dataType) {
  switch (dataType) {
  case EDT_Void: return "void";
  case EDT_Int: return "int";
  case EDT_Float: return "float";
  case EDT_Float2: return "vec2";
  case EDT_Float3: return "vec3";
  case EDT_Float4: return "vec4";
  default:
    assert(0);
  }

  return "";
}

bool CppStackerBox::imGui() {
  validate();
  // todo: move to tooltip
  ImGui::TextUnformatted("DataType: ");
  ImGui::SameLine();
  ImGui::TextUnformatted(getGLSLTypeName(dataType));
  imguiInputText("name", name, 0);
  ImGui::Separator();
  validate();
  return false;
}

void CppStackerBox::validate() const {
  assert((uint32)dataType < EDT_MAX);
}

void CppStackerBox::drawBox(const StackerUI& stackerUI, const ImVec2 minR, const ImVec2 maxR) {
  validate();
  (void)stackerUI;

  ImVec2 sizeR(maxR.x - minR.x, maxR.y - minR.y);

  const char* nodeName = enumToCStr(nodeType);
  ImVec2 textSize = ImGui::CalcTextSize(nodeName);
  ImGui::SetCursorScreenPos(ImVec2(minR.x + (sizeR.x - textSize.x) / 2, minR.y + (sizeR.y - textSize.y) / 2));
  ImGui::TextUnformatted(nodeName);
  validate();
}

bool CppStackerBox::load(const rapidjson::Document::ValueType& doc) {
  // call parent
  if (!StackerBox::load(doc))
    return false;

  // todp
//    typeName = doc["typeName"].GetUint() != 0;
  name = doc["name"].GetString();

  // todo: consider string serialize
  nodeType = (ENodeType)doc["nodeType"].GetUint();
  if (nodeType >= (uint32)NT_NodeTypeTerminator) {
    assert(false);
    return false;
  }

  validate();
  return true;
}

void CppStackerBox::save(rapidjson::Document& d, rapidjson::Value& objValue) const {
  // call parent
  StackerBox::save(d, objValue);

  objValue.AddMember("nodeType", (uint32)nodeType, d.GetAllocator());
  {
    rapidjson::Value tmp;
    tmp.SetString(name.c_str(), d.GetAllocator());
    objValue.AddMember("name", tmp, d.GetAllocator());
  }
  objValue.AddMember("dataType", dataType, d.GetAllocator());
  validate();
}

bool CppStackerBox::generateCode(GenerateCodeContext& context) {
  validate();

  char str[256];

  if (nodeType == NT_FragCoord && context.params.size() == 0) {
    if (context.code) {
      dataType = EDT_Float4;
      sprintf_s(str, sizeof(str), "%s v%d = gl_FragCoord; // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  if (nodeType == NT_ScreenUV && context.params.size() == 0) {
    if (context.code) {
      dataType = EDT_Float2;
      sprintf_s(str, sizeof(str), "%s v%d = gl_FragCoord.xy * uniform0[1].zw; // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  if (nodeType == NT_Rand && context.params.size() == 0) {
    if (context.code) {
      dataType = EDT_Float;
      sprintf_s(str, sizeof(str), "%s v%d = uniform0[0][0]; // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  if (nodeType == NT_Time && context.params.size() == 0) {
    if (context.code) {
      dataType = EDT_Float;
      sprintf_s(str, sizeof(str), "%s v%d = uniform0[0][1]; // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  dataType = EDT_Void;

  if (!context.params.empty()) {
    dataType = ((CppStackerBox*)context.params[0])->dataType;
    validate();

    // largest type of all input parameters
    for (size_t i = 1, count = context.params.size(); i < count; ++i) {
      const EDataType paramDataType = ((CppStackerBox*)context.params[i])->dataType;
      dataType = combineTypes(dataType, paramDataType);
      validate();
    }
    validate();
  }

  if (dataType == EDT_Void) {
    validate();
    return false;
  }

  // one or more inputs, all same dataType which is not EDT_Unknown
  CppStackerBox& param0 = (CppStackerBox&)*context.params[0];

  if (nodeType == NT_Add) {
    validate();
    if (context.code) {
      int32 param0VIndex = upgradeTypeIfNeeded(context, dataType, param0);
      // todo: implement max Parameter count
      int32 paramNVIndex[16];
      for (size_t i = 1, count = context.params.size(); i < count; ++i) {
        paramNVIndex[i] = upgradeTypeIfNeeded(context, dataType, (CppStackerBox&)*context.params[i]);
      }
      sprintf_s(str, sizeof(str), "  %s v%d = v%d",
        getGLSLTypeName(dataType),
        vIndex,
        param0VIndex);
      validate();
      *context.code += str;
      
      for (size_t i = 1, count = context.params.size(); i < count; ++i) {
        sprintf_s(str, sizeof(str), " + v%d",
          paramNVIndex[i]);

        *context.code += str;
      }
      validate();
      sprintf_s(str, sizeof(str), "; // %s\n", name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  if (nodeType == NT_Sub) {
    // unary minus to negate
    if(context.params.size() == 1) {
      if (context.code) {
        sprintf_s(str, sizeof(str), "%s v%d = v%d; // %s\n",
          getGLSLTypeName(dataType),
          vIndex,
          param0.vIndex,
          name.c_str());
        *context.code += str;
      }
      validate();
      return true;
    } else if (context.params.size() == 2) {
      // a-b
      if (context.code) {
        CppStackerBox& param1 = (CppStackerBox&)*context.params[1];
        int32 param0VIndex = upgradeTypeIfNeeded(context, dataType, param0);
        int32 param1VIndex = upgradeTypeIfNeeded(context, dataType, param1);
        sprintf_s(str, sizeof(str), "%s v%d = v%d - v%d; // %s\n",
          getGLSLTypeName(dataType),
          vIndex,
          param0VIndex,
          param1VIndex,
          name.c_str());
        *context.code += str;
      }
      validate();
      return true;
    }
  }

  if (nodeType == NT_RGBOutput && context.params.size() == 1) {
    if (context.code) {
      int32 paramVIndex = param0.castTo(context, EDT_Float4);
      sprintf_s(str, sizeof(str), "  ret = v%d", paramVIndex);
      *context.code += str;
      sprintf_s(str, sizeof(str), "; // %s\n", name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  if (nodeType == NT_Mul) {
    if (context.code) {
      int32 param0VIndex = upgradeTypeIfNeeded(context, dataType, param0);
      // todo: implement max Parameter count
      int32 paramNVIndex[16];
      for (size_t i = 1, count = context.params.size(); i < count; ++i) {
        paramNVIndex[i] = upgradeTypeIfNeeded(context, dataType, (CppStackerBox&)*context.params[i]);
      }
      sprintf_s(str, sizeof(str), "  %s v%d = v%d",
        getGLSLTypeName(dataType),
        vIndex,
        param0VIndex);
      *context.code += str;

      for (size_t i = 1, count = context.params.size(); i < count; ++i) {
        sprintf_s(str, sizeof(str), " * v%d",
          paramNVIndex[i]);

        *context.code += str;
      }
      sprintf_s(str, sizeof(str), "; // %s\n", name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  // dot product
  if (nodeType == NT_Dot) {
    if (context.params.size() == 1) {
      // dot product with itself
      dataType = EDT_Float;
      if (context.code) {
        sprintf_s(str, sizeof(str), "%s v%d = dot(v%d, v%d); // %s\n",
          getGLSLTypeName(dataType),
          vIndex,
          param0.vIndex,
          param0.vIndex,
          name.c_str()
        );
        *context.code += str;
      }
      validate();
      return true;
    }
    else if(context.params.size() == 2) {
      // dot(a,b)
      dataType = EDT_Float;
      if (context.code) {
        CppStackerBox& param1 = (CppStackerBox&)*context.params[1];
        int32 param0VIndex = upgradeTypeIfNeeded(context, dataType, param0);
        int32 param1VIndex = upgradeTypeIfNeeded(context, dataType, param1);
        sprintf_s(str, sizeof(str), "%s v%d = dot(v%d, v%d); // %s\n",
          getGLSLTypeName(dataType),
          vIndex,
          param0VIndex,
          param1VIndex,
          name.c_str()
        );
        *context.code += str;
      }
      validate();
      return true;
    }
  }

  if (nodeType == NT_Swizzle && context.params.size() == 1) {
    if (context.code) {
      CppStackerBoxSwizzle& self = (CppStackerBoxSwizzle&)*this;
      dataType = self.computeOutputDataType(param0.dataType);

      if (dataType == EDT_Void)
        return false;

      sprintf_s(str, sizeof(str), "%s v%d = (v%d).%s;\n",
        getGLSLTypeName(dataType),
        vIndex,
        param0.vIndex,
        self.xyzw);
      *context.code += str;
    }
    validate();
    return true;
  }

  // divide
  if (nodeType == NT_Div) {
    if (context.params.size() == 1) {
      // 1/b (works with float, vec2, vec3, vec4)
      if (context.code) {
        sprintf_s(str, sizeof(str), "%s v%d = 1.0f / v%d; // %s\n",
          getGLSLTypeName(dataType),
          vIndex,
          param0.vIndex,
          name.c_str());
        *context.code += str;
      }
      validate();
      return true;
    }
    else if (context.params.size() == 2) {
      // a/b
      if (context.code) {
        CppStackerBox& param1 = (CppStackerBox&)*context.params[1];
        int32 param0VIndex = upgradeTypeIfNeeded(context, dataType, param0);
        int32 param1VIndex = upgradeTypeIfNeeded(context, dataType, param1);
        sprintf_s(str, sizeof(str), "%s v%d = v%d / v%d; // %s\n",
          getGLSLTypeName(dataType),
          vIndex,
          param0VIndex,
          param1VIndex,
          name.c_str());
        *context.code += str;
      }
      validate();
      return true;
    }
  }

  // Power x^y
  if (nodeType == NT_Pow) {
    if (context.params.size() == 2) {
      // x^y
      if (context.code) {
        CppStackerBox& param1 = (CppStackerBox&)*context.params[1];
        int32 param0VIndex = upgradeTypeIfNeeded(context, dataType, param0);
        int32 param1VIndex = param1.castTo(context, EDT_Float);
        sprintf_s(str, sizeof(str), "%s v%d = pow(v%d, v%d); // %s\n",
          getGLSLTypeName(dataType),
          vIndex,
          param0VIndex,
          param1VIndex,
          name.c_str());
        *context.code += str;
      }
      validate();
      return true;
    }
  }

  // Saturate
  if (nodeType == NT_Saturate && context.params.size() == 1) {
    if (context.code) {
      int32 paramVIndex = param0.castTo(context, EDT_Float);
      sprintf_s(str, sizeof(str), "%s v%d = clamp(v%d, 0.0f, 1.0f); // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        paramVIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  // Frac
  if (nodeType == NT_Frac && context.params.size() == 1) {
    if (context.code) {
//      int32 paramVIndex = param0.castTo(context, EDT_Float);
      sprintf_s(str, sizeof(str), "%s v%d = fract(v%d); // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        param0.vIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  // Sqrt
  if (nodeType == NT_Sqrt && context.params.size() == 1) {
    if (context.code) {
      int32 paramVIndex = param0.castTo(context, EDT_Float);
      sprintf_s(str, sizeof(str), "%s v%d = sqrt(v%d); // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        paramVIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  // 1-x (works with float, vec2, vec3, vec4)
  if (nodeType == NT_OneMinusX && context.params.size() == 1) {
    if (context.code) {
      sprintf_s(str, sizeof(str), "%s v%d = 1.0f - v%d; // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        param0.vIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  // Sin
  if (nodeType == NT_Sin && context.params.size() == 1) {
    if (context.code) {
      int32 paramVIndex = param0.castTo(context, EDT_Float);
      sprintf_s(str, sizeof(str), "%s v%d = sin(v%d); // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        paramVIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  // Cos
  if (nodeType == NT_Cos && context.params.size() == 1) {
    if (context.code) {
      int32 paramVIndex = param0.castTo(context, EDT_Float);
      sprintf_s(str, sizeof(str), "%s v%d = cos(v%d); // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        paramVIndex,
        name.c_str());
      *context.code += str;
    }
    validate();
    return true;
  }

  validate();
  return false;
}

bool CppStackerBoxConstant::generateCode(GenerateCodeContext& context) {
  char str[256];

  if (context.code) {
    if (dataType == EDT_Float) {
      sprintf_s(str, sizeof(str), "%s v%d = %f; // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        value.x,
        name.c_str());
      *context.code += str;
    }
    else if (dataType == EDT_Float2) {
      sprintf_s(str, sizeof(str), "%s v%d = vec2(%f, %f); // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        value.x, value.y,
        name.c_str());
      *context.code += str;
    }
    else if (dataType == EDT_Float3) {
      sprintf_s(str, sizeof(str), "%s v%d = vec3(%f, %f, %f); // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        value.x, value.y, value.z,
        name.c_str());
      *context.code += str;
    }
    else if(dataType == EDT_Float4) {
      sprintf_s(str, sizeof(str), "%s v%d = vec4(%f, %f, %f, %f); // %s\n",
        getGLSLTypeName(dataType),
        vIndex,
        value.x, value.y, value.z, value.w,
        name.c_str());
      *context.code += str;
    }
    else {
      assert(0);
    }
  }
  return true;
}

void CppStackerBoxConstant::drawBox(const StackerUI& stackerUI, const ImVec2 minR, const ImVec2 maxR) {
  ImVec2 sizeR(maxR.x - minR.x, maxR.y - minR.y);
  ImGuiStyle& style = ImGui::GetStyle();

  float sliderSizeX = sizeR.x - stackerUI.scale * 1.0f;

  if (sliderSizeX > 0) {
    float sliderSizeY = 1.0f * style.FramePadding.y + ImGui::GetFontSize();
    float border = 2;

    // centerx, top
// if there is enough space        if(maxR.y - minR.y > stackerUI.scale) 
    {
      ImVec2 s = ImGui::CalcTextSize(name.c_str());
      ImGui::SetCursorScreenPos(ImVec2(minR.x + (sizeR.x - s.x) / 2, minR.y + border));
      ImGui::TextUnformatted(name.c_str());
    }

    // centerx, bottom
    ImGui::SetCursorScreenPos(ImVec2(minR.x + (sizeR.x - sliderSizeX) / 2, minR.y + sizeR.y - sliderSizeY - border));
    // centerx, centery
//        ImGui::SetCursorScreenPos(ImVec2(minR.x + (sizeR.x - sliderSizeX) / 2, minR.y + (sizeR.y - sliderSizeY) / 2));

    ImGui::SetNextItemWidth(sliderSizeX);

    if (constantType == ECT_ColorRGB || constantType == ECT_ColorRGBA) {
      // todo: show alpha as well
      ImVec4 col3(value.x, value.y, value.z, 1.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, col3);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col3);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, col3);
      ImGui::Button("", ImVec2(sliderSizeX, (float)stackerUI.connectorSize));
      ImGui::PopStyleColor(3);
    }
  
    char str[256];

    if (dataType == EDT_Float) {
      sprintf_s(str, 80, "%.2f", value.x);
      ImVec2 s = ImGui::CalcTextSize(str);
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderSizeX - s.x) / 2);
      //          ImGui::SliderFloat("", &value.x, minSlider, maxSlider);
      ImGui::TextUnformatted(str);
    }
    else if (dataType == EDT_Float2) {
      sprintf_s(str, 80, "%.2f %.2f", value.x, value.y);
      ImVec2 s = ImGui::CalcTextSize(str);
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderSizeX - s.x) / 2);
      ImGui::TextUnformatted(str);
    }
    else if (dataType == EDT_Float3) {
      sprintf_s(str, 80, "%.2f %.2f %.2f", value.x, value.y, value.z);
      ImVec2 s = ImGui::CalcTextSize(str);
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderSizeX - s.x) / 2);
      ImGui::TextUnformatted(str);
    }
    else if (dataType == EDT_Float4) {
      sprintf_s(str, 80, "%.2f %.2f %.2f %.2f", value.x, value.y, value.z, value.w);
      ImVec2 s = ImGui::CalcTextSize(str);
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderSizeX - s.x) / 2);
      ImGui::TextUnformatted(str);
    }
    else {
      assert(0);
    }
  }
}

EDataType CppStackerBoxConstant::getDataType() const {
  switch(constantType) {
    case ECT_Float: return EDT_Float;
    case ECT_Float2: return EDT_Float2;
    case ECT_Float3: return EDT_Float3;
    case ECT_Float4: return EDT_Float4;
    case ECT_ColorRGB: return EDT_Float3;
    case ECT_ColorRGBA: return EDT_Float4;
    default:
      assert(0);
  }
  return EDT_Void;
}

bool CppStackerBoxConstant::imGui() {
  bool ret = false;

  if (CppStackerBox::imGui())
    ret = true;

  if(ImGui::Combo("Type", (int*)&constantType, constantTypeUI)) {
    dataType = getDataType();
    ret = true;
  }

  if(constantType == ECT_Float) {
    assert(dataType == EDT_Float);
    if (ImGui::SliderFloat("Value", &value.x, minSlider, maxSlider))
      ret = true;
  }
  else if (constantType == ECT_ColorRGB) {
    if (ImGui::ColorEdit3("Value", &value.x, 0))
      ret = true;
  }
  else if (constantType == ECT_ColorRGBA) {
    if (ImGui::ColorEdit4("Value", &value.x, 0))
      ret = true;
  }
  else if (constantType == ECT_Float2) {
    if (ImGui::SliderFloat2("Value", &value.x, minSlider, maxSlider))
      ret = true;
  }
  else if (constantType == ECT_Float3) {
    if (ImGui::SliderFloat3("Value", &value.x, minSlider, maxSlider))
      ret = true;
  }
  else if (constantType == ECT_Float4) {
    if (ImGui::SliderFloat4("Value", &value.x, minSlider, maxSlider))
      ret = true;
  }

  ImGui::Separator();

  if (!(constantType == ECT_ColorRGB || constantType == ECT_ColorRGBA)) {
    ImGui::InputFloat("min Slider", &minSlider);
    ImGui::InputFloat("max Slider", &maxSlider);
  }

  return ret;
}

bool CppStackerBoxConstant::load(const rapidjson::Document::ValueType& doc) {
  // call parent
  if (!StackerBox::load(doc))
    return false;

  // todo: error handling
  constantType = (EConstantType)doc["constantType"].GetInt();
  value.x = doc["valueX"].GetFloat();
  value.y = doc["valueY"].GetFloat();
  value.z = doc["valueZ"].GetFloat();
  value.w = doc["valueW"].GetFloat();
  minSlider = doc["minSlider"].GetFloat();
  maxSlider = doc["maxSlider"].GetFloat();

  dataType = getDataType();
  return true;
}

void CppStackerBoxConstant::save(rapidjson::Document& d, rapidjson::Value& objValue) const {
  // call parent
  StackerBox::save(d, objValue);

  objValue.AddMember("constantType", constantType, d.GetAllocator());
  objValue.AddMember("valueX", value.x, d.GetAllocator());
  objValue.AddMember("valueY", value.y, d.GetAllocator());
  objValue.AddMember("valueZ", value.z, d.GetAllocator());
  objValue.AddMember("valueW", value.w, d.GetAllocator());
  objValue.AddMember("minSlider", minSlider, d.GetAllocator());
  objValue.AddMember("maxSlider", maxSlider, d.GetAllocator());
}

bool CppStackerBoxSwizzle::imGui() {
  bool ret = false;

  if (CppStackerBox::imGui())
    ret = true;

  ImGui::InputText("XYZW", xyzw, 5, 0);

  return ret;
}

bool CppStackerBoxSwizzle::load(const rapidjson::Document::ValueType& doc) {
  // call parent
  if (!StackerBox::load(doc))
    return false;

  const char* str = doc["xyzw"].GetString();
  if(strlen(str) > 4) {
    return false;
  }

  strcpy(xyzw, str);
  return true;
}

void CppStackerBoxSwizzle::save(rapidjson::Document& d, rapidjson::Value& objValue) const {
  // call parent
  StackerBox::save(d, objValue);

  objValue.AddMember("xyzw", xyzw, d.GetAllocator());
}

EDataType CppStackerBoxSwizzle::computeOutputDataType(const EDataType inputDataType) const {
  assert(inputDataType < EDT_MAX);

  const char* p = xyzw;

  // 0:none, 1:x, 2:xy, 3:zyw, 4:xyzw
  int32 inputComponentCount = 0;

  if(inputDataType == EDT_Int || inputDataType == EDT_Float)
    inputComponentCount = 1;
  else if (inputDataType == EDT_Float2)
    inputComponentCount = 2;
  else if (inputDataType == EDT_Float3)
    inputComponentCount = 3;
  else if (inputDataType == EDT_Float4)
    inputComponentCount = 4;

  while(char c = *p++){
    // outside of range by default
    int32 componentIndex = 4;

    c = (char)tolower(c);

    if(c == 'x' || c == 'r')
      componentIndex = 0;
    else if (c == 'y' || c == 'g')
      componentIndex = 1;
    else if (c == 'z' || c == 'b')
      componentIndex = 2;
    else if (c == 'w' || c == 'a')
      componentIndex = 3;

    if(componentIndex >= inputComponentCount)
      return EDT_Void;
  }

  EDataType ret = EDT_Void;

  if (inputComponentCount == 1)
    ret = inputDataType;
  if(inputComponentCount == 2)
    ret = EDT_Float2;
  else if (inputComponentCount == 3)
    ret = EDT_Float3;
  else if (inputComponentCount == 4)
    ret = EDT_Float4;
  else assert(0);

  return ret;
}


