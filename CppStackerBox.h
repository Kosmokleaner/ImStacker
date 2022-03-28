#pragma once
#include <string>
#include "StackerUI.h"

// index is serialized
enum DataType {
    EDT_Unknown,
    EDT_Int,
    EDT_Float,
// --------------
    EDT_MAX,
};

// index is serialized so only add in end
enum ENodeType {
    NT_Unknown,
    NT_IntVariable, NT_FloatVariable,
    NT_Add, NT_Sub, NT_Mul, NT_Div, NT_Sin, NT_Cos, NT_Frac, NT_Saturate, NT_Lerp,
    // -----------------
    NT_NodeTypeTerminator
};

// return 0 if internal error
inline const char* enumToCStr(const ENodeType nodeType) {
    const char* tab[] = {
        "Unknown",
        "IntVariable", "FloatVariable",
        "Add", "Sub", "Mul", "Div", "Sin", "Cos", "Frac", "Saturate", "Lerp"
    };

    if(nodeType < sizeof(tab) / sizeof(tab[0]))
        return tab[nodeType];
    // sync tab[] with ENodeTab
    assert(0);
    return nullptr;
}

class CppStackerBox : public StackerBox {
public:
    // used by createNode()
    CppStackerBox();

    // for user/debugging, not needed to function
    std::string name;
    //
    DataType dataType = EDT_Float;
    //
    ENodeType nodeType = NT_Unknown;

    // interface StackerBox ---------------------------------

    virtual const char* getType() const override { return "CppStackerBox"; }
    virtual void imGui() override;
    virtual bool isVariable() const override { return false; }
    virtual bool generateCode(GenerateCodeContext& context) override;
    virtual bool load(const rapidjson::Document::ValueType& doc) override;
    virtual void save(rapidjson::Document& d, rapidjson::Value& objValue) const override;
    virtual void drawBox(const StackerUI& stackerUI, const ImVec2 minR, const ImVec2 maxR) override;
    virtual void validate() const override;

private:
    static const char* getType(DataType dataType);
};

// -------------------------------------------------------------------

class CppStackerBoxFloat : public CppStackerBox {
public:
    float value = 0;
    float minSlider = 0.0f;
    float maxSlider = 1.0f;

    // interface StackerBox ---------------------------------

    virtual const char* getType() const { return "CppStackerBoxFloat"; }
    virtual bool isVariable() const { return true; }
    virtual bool generateCode(GenerateCodeContext& context);
    virtual bool load(const rapidjson::Document::ValueType& doc);
    virtual void save(rapidjson::Document& d, rapidjson::Value& objValue) const;
    virtual void drawBox(const StackerUI& stackerUI, const ImVec2 minR, const ImVec2 maxR);
};

// -------------------------------------------------------------------

class CppAppConnection : public StackerUI::IAppConnection {
public:

    virtual void openContextMenu(StackerUI& stackerUI, const StackerBoxRect& rect);
    virtual StackerBox* createNode(const char* className);
};
