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
  EDT_Vec2,
  // three floats x,y,z
  EDT_Vec3,
  // four floats, x,y,z,w
  EDT_Vec4,
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
  NT_Add, NT_Sub, NT_Mul, NT_Div, NT_Sin, NT_Cos, NT_Frac, NT_Saturate, NT_Lerp,
  NT_Swizzle, // like UE4 ComponentMask and Append in one
  NT_Rand,
  NT_FragCoord,
  NT_Output,
  // -----------------
  NT_NodeTypeTerminator
};

// return 0 if internal error
inline const char* enumToCStr(const ENodeType nodeType) {
  const char* tab[] = {
      "Unknown",
      "IntVariable", "FloatVariable",
      "Add", "Sub", "Mul", "Div", "Sin", "Cos", "Frac", "Saturate", "Lerp",
      "Swizzle",
      "Rand",
      "FragCoord",
      "Output",
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
const char* getTypeName(const EDataType dataType);

// -------------------------------------------------------------------

class CppStackerBoxConstant : public CppStackerBox {
public:
  ImVec4 value = {};
  float minSlider = 0.0f;
  float maxSlider = 1.0f;

  // interface StackerBox ---------------------------------

  virtual const char* getType() const { return "CppStackerBoxConstant"; }
  virtual bool imGui() override;
  virtual bool isVariable() const { return true; }
  virtual bool generateCode(GenerateCodeContext& context);
  virtual bool load(const rapidjson::Document::ValueType& doc);
  virtual void save(rapidjson::Document& d, rapidjson::Value& objValue) const;
  virtual void drawBox(const StackerUI& stackerUI, const ImVec2 minR, const ImVec2 maxR);
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
