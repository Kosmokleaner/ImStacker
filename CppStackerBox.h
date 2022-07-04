#pragma once
#include <string>
#include "StackerUI.h"

// index is serialized
// update CppStackerBox::getType() if you add a new entry
enum EDataType {
  EDT_Void,
  // 32bit int
  EDT_Int,
  // 32bit float
  EDT_Float,
  // two floats x,y
  EDT_Float2,
  // three floats x,y,z
  EDT_Float3,
  // four floats, x,y,z,w
  EDT_Float4,
  // --------------
  EDT_MAX,
};

// index is serialized so only add in end
// update CppAppConnection::openContextMenu()
//    and CppStackerBox::generateCode()
//    and enumToCStr()
// if you add a new entry
enum ENodeType {
  NT_Unknown,
  NT_IntVariable, NT_FloatVariable,
  NT_Add, NT_Sub, NT_Mul, NT_Div, NT_Sin, NT_Cos, NT_Frac, NT_Saturate, NT_Lerp, NT_Dot, NT_Sqrt,
  NT_Swizzle, // like UE4 ComponentMask and Append in one
  NT_Rand,
  NT_Time,
  NT_FragCoord,
  NT_ScreenUV,
  NT_RGBOutput,
  // -----------------
  NT_NodeTypeTerminator
};

// return 0 if internal error
inline const char* enumToCStr(const ENodeType nodeType) {
  const char* tab[] = {
      "Unknown",
      "IntVariable", "FloatVariable",
      "Add", "Sub", "Mul", "Div", "Sin", "Cos", "Frac", "Saturate", "Lerp", "Dot", "Sqrt",
      "Swizzle",
      "Rand",
      "Time",
      "FragCoord",
      "ScreenUV",
      "RGBOutput",
  };

  if (nodeType < sizeof(tab) / sizeof(tab[0]))
    return tab[nodeType];
  // sync tab[] with ENodeTab
  assert(0);
  return nullptr;
}

class CppStackerBox : public StackerBox {
public:
  // for user/debugging, not needed to function
  std::string name;
  //
  EDataType dataType = EDT_Float;
  //
  ENodeType nodeType = NT_Unknown;

  int32 castTo(GenerateCodeContext& context, const EDataType dstDataType) const;

  // interface StackerBox ---------------------------------

  virtual const char* getType() const override { return "CppStackerBox"; }
  virtual bool imGui() override;
  virtual bool isVariable() const override { return false; }
  virtual bool generateCode(GenerateCodeContext& context) override;
  virtual bool load(const rapidjson::Document::ValueType& doc) override;
  virtual void save(rapidjson::Document& d, rapidjson::Value& objValue) const override;
  virtual void drawBox(const StackerUI& stackerUI, const ImVec2 minR, const ImVec2 maxR) override;
  virtual void validate() const override;
};

// @return name used in generated code e.g. "vec4"
const char* getGLSLTypeName(const EDataType dataType);

// -------------------------------------------------------------------

class CppStackerBoxConstant : public CppStackerBox {
public:
  enum EConstantType {
    ECT_Float,
    ECT_Float2,
    ECT_Float3,
    ECT_Float4,
    ECT_ColorRGB,
    ECT_ColorRGBA,
  };
  static constexpr const char* constantTypeUI = (char*)"Float\0" "Float2\0" "Float3\0" "Float4\0" "ColorRGB\0" "ColorRGBA\0" ;

  EConstantType constantType = ECT_Float;

  ImVec4 value = {};
  float minSlider = 0.0f;
  float maxSlider = 2.0f;

  EDataType getDataType() const;

  // interface StackerBox ---------------------------------

  virtual const char* getType() const override { return "CppStackerBoxConstant"; }
  virtual bool imGui() override;
  virtual bool isVariable() const override { return true; }
  virtual bool canHaveInput() const override { return false; }
  virtual bool generateCode(GenerateCodeContext& context) override;
  virtual bool load(const rapidjson::Document::ValueType& doc) override;
  virtual void save(rapidjson::Document& d, rapidjson::Value& objValue) const override;
  virtual void drawBox(const StackerUI& stackerUI, const ImVec2 minR, const ImVec2 maxR) override;
};

// -------------------------------------------------------------------

class CppAppConnection : public StackerUI::IAppConnection {
public:
  std::string generatedCode;
  std::string warningsAndErrors;

  virtual void openContextMenu(StackerUI& stackerUI, const StackerBoxRect& rect);
  virtual StackerBox* createNode(const char* className);
  virtual const char* getWarningsAndErrors() { return warningsAndErrors.c_str(); }
  virtual void startCompile();
  virtual void endCompile();
  virtual void reCompile();
  virtual std::string* code() { return &generatedCode; }
};

// -------------------------------------------------------------------

class CppStackerBoxSwizzle : public CppStackerBox {
public:
  // 0 terminated, to be valid it must only use xyzw and the input must have those
  char xyzw[5] = { 'x', 'y', 0, 0, 0 };

  // @return EDT_Void means the swizzle failed
  EDataType computeOutputDataType(const EDataType inputDataType) const;

  // interface StackerBox ---------------------------------

  virtual const char* getType() const override { return "CppStackerBoxSwizzle"; }
  virtual bool imGui() override;
  virtual bool load(const rapidjson::Document::ValueType& doc) override;
  virtual void save(rapidjson::Document& d, rapidjson::Value& objValue) const override;
};
