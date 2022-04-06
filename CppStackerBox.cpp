#include "CppStackerBox.h"
#include <vector>
#include <assert.h>
#include "imgui.h"

#ifdef _WIN32
  // We rely on string literate compare for simpler code.
  // warning C4130 : '==' : logical operation on address of string constant
  #pragma warning( disable : 4130 )
#else
  #define	sprintf_s(buffer, buffer_size, stringbuffer, ...) (sprintf(buffer, stringbuffer, __VA_ARGS__))
#endif

// literal (number, slider): bool, int, float, string, color, bitmap, struct? float slider
// input: time, pixelpos, camera
// conversion float->int, int->float, int->string, string->int,  ...
// + - * / pow(a, b), sin(), cos(), frac()
// get_var, set_var
// print
// composite e.g. float slider in 0..1 range

// todo: copy,paste, delete with key binding

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

    if(strcmp(className, "CppStackerBox") == 0)
        return new CppStackerBox();

    if (strcmp(className, "CppStackerBoxConstant") == 0)
        return new CppStackerBoxConstant();

    return nullptr;
}

void CppAppConnection::openContextMenu(StackerUI& stackerUI, const StackerBoxRect& rect) {

#define ENTRY(inName, tooltip) \
    if(ImGui::MenuItem(#inName, nullptr)) { \
        auto obj = new CppStackerBox(); \
        obj->nodeType = NT_##inName; \
        obj->rect = rect; \
        obj->name = "Unnamed"; \
        stackerUI.stackerBoxes.push_back(obj); \
    } \
    imguiToolTip(tooltip);

// @param inType e.g. EDT_Float
#define ENTRY_CONSTANT(inName, inType, tooltip) \
    if(ImGui::MenuItem(#inName, nullptr)) { \
        auto obj = new CppStackerBoxConstant(); \
        obj->dataType = inType;\
        obj->name = #inName; \
        obj->rect = rect; \
        stackerUI.stackerBoxes.push_back(obj); \
    } \
    imguiToolTip(tooltip);

//    ENTRY(IntVariable, "Integer variable (no fractional part)");
    ENTRY_CONSTANT(FloatConstant, EDT_Float, "Floating point constant (with fractional part)");
    ENTRY_CONSTANT(Vec4Constant, EDT_Vec4, "Vec4 constant");
    ImGui::Separator();
    ENTRY(Add, "Sum up multiple inputs of same type");
    ENTRY(Sub, "Subtract two numbers or negate one number");
    ENTRY(Mul, "Multiple multiple numbers");
    ENTRY(Div, "Divide two numbers, 1/x a single number");
    ENTRY(Sin, "Sin() in radiants");
    ENTRY(Cos, "Cos() in radiants");
    ENTRY(Frac, "like HLSL frac() = x-floor(x)");
    ENTRY(Saturate, "like HLSL saturate(), clamp betwen 0 and 1)");
    ENTRY(Lerp, "like HLSL lerp(x0,x1,a) = x0*(1-a) + x1*a, linear interpolation");
    ENTRY(FragCoord, "see OpenGL gl_FragCoord");
    ENTRY(Output, "output vec4 as postprocess");

#undef ENTRY

    if(ImGui::GetClipboardText()) {
        ImGui::Separator();
        if (ImGui::MenuItem("Paste", "CTRL+V")) {
            stackerUI.clipboardPaste();
        }
    }
}

const char* CppStackerBox::getTypeName(DataType dataType) {
    switch(dataType) {
        case EDT_Unknown : return "unknown";
        case EDT_Int: return "int";
        case EDT_Float: return "float";
        case EDT_Vec2: return "vec2";
        case EDT_Vec3: return "vec3";
        case EDT_Vec4: return "vec4";
        default:
            assert(0);
    }

    return "";
}

void CppStackerBox::imGui() {
    validate();
    ImGui::TextUnformatted(getTypeName(dataType));
    imguiInputText("name", name, 0);
    validate();
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
    if(!StackerBox::load(doc))
        return false;

    // todp
//    typeName = doc["typeName"].GetUint() != 0;
    name = doc["name"].GetString();

    // todo: consider string serialize
    nodeType = (ENodeType)doc["nodeType"].GetUint();
    if(nodeType >= (uint32)NT_NodeTypeTerminator) {
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

    if (context.params.size() == 0 && nodeType == NT_FragCoord) {
      if (context.code) {
        dataType = EDT_Vec4;
        sprintf_s(str, sizeof(str), "%s v%d = gl_FragCoord;\n",
          getTypeName(dataType),
          vIndex);
        *context.code += str;
      }
      validate();
      return true;
    }

    dataType = EDT_Unknown;  

    if (!context.params.empty()) {
        dataType = ((CppStackerBox*)context.params[0])->dataType;
        validate();

        // all same type?
        for (size_t i = 1, count = context.params.size(); i < count; ++i) {
            if (((CppStackerBox*)context.params[i])->dataType != dataType) {
                dataType = EDT_Unknown;
                validate();
                return false;
            }
            validate();
        }
        validate();
    }

    if (context.params.size() == 0 && nodeType == NT_FragCoord) {
      if (context.code) {
        assert(dataType == EDT_Vec4);
        sprintf_s(str, sizeof(str), "%s v%d = gl_FragCoord;",
          getTypeName(dataType),
          vIndex);
        *context.code += str;
      }
      validate();
      return true;
    }

    if (dataType == EDT_Unknown) {
        validate();
        return false;
    }

    // one or more inputs, all same dataType which is not EDT_Unknown

    if(nodeType == NT_Add) {
        validate();
        if (context.code) {
            sprintf_s(str, sizeof(str), "%s v%d = v%d",
                getTypeName(dataType),
                vIndex,
                context.params[0]->vIndex);
            validate();
            *context.code += str;

            for (size_t i = 1, count = context.params.size(); i < count; ++i) {
                sprintf_s(str, sizeof(str), " + v%d",
                    context.params[i]->vIndex);

                *context.code += str;
            }
            validate();
            sprintf_s(str, sizeof(str), "; // %s\n", name.c_str());
            *context.code += str;
        }
        validate();
        return true;
    }

    // unary minus to negate
    if(context.params.size() == 1 && nodeType == NT_Sub) {
        if (context.code) {
            sprintf_s(str, sizeof(str), "%s v%d = v%d; // %s\n",
                getTypeName(dataType),
                vIndex,
                context.params[0]->vIndex,
                name.c_str());
            *context.code += str;
        }
        validate();
        return true;
    }

    // a-b
    if (context.params.size() == 2 && nodeType == NT_Sub) {
        if (context.code) {
            sprintf_s(str, sizeof(str), "%s v%d = v%d - v%d; // %s\n",
                getTypeName(dataType),
                vIndex,
                context.params[0]->vIndex,
                context.params[1]->vIndex,
                name.c_str());
            *context.code += str;
        }
        validate();
        return true;
    }

    if (context.params.size() == 1 && nodeType == NT_Output) {
        if (context.code) {
            sprintf_s(str, sizeof(str), "FragColor = v%d",
                context.params[0]->vIndex);
            *context.code += str;
            sprintf_s(str, sizeof(str), "; // %s\n", name.c_str());
            *context.code += str;
        }
        validate();
        return true;
    }

    if (nodeType == NT_Mul) {
        if (context.code) {
            sprintf_s(str, sizeof(str), "%s v%d = v%d",
                getTypeName(dataType),
                vIndex,
                context.params[0]->vIndex);
            *context.code += str;

            for (size_t i = 1, count = context.params.size(); i < count; ++i) {
                sprintf_s(str, sizeof(str), " * v%d",
                    context.params[i]->vIndex);

                *context.code += str;
            }
            sprintf_s(str, sizeof(str), "; // %s\n", name.c_str());
            *context.code += str;
        }
        validate();
        return true;
    }

    // 1/b
    if (context.params.size() == 1 && nodeType == NT_Div) {
        if (context.code) {
            sprintf_s(str, sizeof(str), "%s v%d = 1.0f / v%d; // %s\n",
                getTypeName(dataType),
                vIndex,
                context.params[0]->vIndex,
                name.c_str());
            *context.code += str;
        }
        validate();
        return true;
    }

    // a/b
    if (context.params.size() == 2 && nodeType == NT_Div) {
        if (context.code) {
            sprintf_s(str, sizeof(str), "%s v%d = v%d / v%d; // %s\n",
                getTypeName(dataType),
                vIndex,
                context.params[0]->vIndex,
                context.params[1]->vIndex,
                name.c_str());
            *context.code += str;
        }
        validate();
        return true;
    }

    // Sin
    if (context.params.size() == 1 && nodeType == NT_Sin) {
        if (context.code) {
            sprintf_s(str, sizeof(str), "%s v%d = sin(v%d); // %s\n",
                getTypeName(dataType),
                vIndex,
                context.params[0]->vIndex,
                name.c_str());
            *context.code += str;
        }
        validate();
        return true;
    }

    // Cos
    if (context.params.size() == 1 && nodeType == NT_Cos) {
        if (context.code) {
            sprintf_s(str, sizeof(str), "%s v%d = cos(v%d); // %s\n",
                getTypeName(dataType),
                vIndex,
                context.params[0]->vIndex,
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
        if(dataType == EDT_Float) {
          sprintf_s(str, sizeof(str), "%s v%d = %f;\n",
              getTypeName(dataType),
              vIndex,
              value.x);
          *context.code += str;
        }
        else {
          assert(dataType == EDT_Vec4);
          sprintf_s(str, sizeof(str), "%s v%d = vec4(%f, %f, %f, %f);\n",
            getTypeName(dataType),
            vIndex,
            value.x, value.y, value.z, value.w);
          *context.code += str;
        }
    }
    return true;
}

void CppStackerBoxConstant::drawBox(const StackerUI& stackerUI, const ImVec2 minR, const ImVec2 maxR) {
    ImVec2 sizeR(maxR.x - minR.x, maxR.y - minR.y);
    ImGuiStyle& style = ImGui::GetStyle();

    float sliderSizeX = sizeR.x - stackerUI.scale * 1.0f;

    if(sliderSizeX > 0) {
        float sliderSizeY = 1.0f * style.FramePadding.y + ImGui::GetFontSize();
        float border = 2;

        // centerx, top
        if(maxR.y - minR.y > stackerUI.scale) {
          ImVec2 s = ImGui::CalcTextSize(name.c_str());
          ImGui::SetCursorScreenPos(ImVec2(minR.x + (sizeR.x - s.x) / 2, minR.y + border));
          ImGui::TextUnformatted(name.c_str());
        }

        // centerx, bottom
        ImGui::SetCursorScreenPos(ImVec2(minR.x + (sizeR.x - sliderSizeX) / 2, minR.y + sizeR.y - sliderSizeY - border));
        // centerx, centery
//        ImGui::SetCursorScreenPos(ImVec2(minR.x + (sizeR.x - sliderSizeX) / 2, minR.y + (sizeR.y - sliderSizeY) / 2));

        ImGui::SetNextItemWidth(sliderSizeX);

        if(dataType == EDT_Float) {
          char str[80];
          sprintf_s(str, 80, "%.2f", value.x);
          ImVec2 s = ImGui::CalcTextSize(str);
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderSizeX - s.x) / 2);
//          ImGui::SliderFloat("", &value.x, minSlider, maxSlider);
          ImGui::TextUnformatted(str);
        } else {
          // todo: show alpha as well
          ImVec4 col3(value.x, value.y, value.z, 1.0f);
          ImGui::PushStyleColor(ImGuiCol_Button, col3);
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col3);
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, col3);
          ImGui::Button("", ImVec2(sliderSizeX, (float)stackerUI.connectorSize));
          ImGui::PopStyleColor(3);
        }
    }
}

void CppStackerBoxConstant::imGui() {
  CppStackerBox::imGui();

  if (dataType == EDT_Float) {
    ImGui::SliderFloat("Value", &value.x, minSlider, maxSlider);
    ImGui::InputFloat("minValue", &minSlider);
    ImGui::InputFloat("maxValue", &maxSlider);
  }
  else {
    ImGui::ColorEdit4("Value", &value.x, 0);
  }
}

bool CppStackerBoxConstant::load(const rapidjson::Document::ValueType& doc) {
    // call parent
    if (!StackerBox::load(doc))
        return false;

    value.x = doc["valueX"].GetFloat();
    value.y = doc["valueY"].GetFloat();
    value.z = doc["valueZ"].GetFloat();
    value.w = doc["valueW"].GetFloat();
    minSlider = doc["minSlider"].GetFloat();
    maxSlider = doc["maxSlider"].GetFloat();
    return true;
}

void CppStackerBoxConstant::save(rapidjson::Document& d, rapidjson::Value& objValue) const {
    // call parent
    StackerBox::save(d, objValue);

    objValue.AddMember("valueX", value.x, d.GetAllocator());
    objValue.AddMember("valueY", value.y, d.GetAllocator());
    objValue.AddMember("valueZ", value.z, d.GetAllocator());
    objValue.AddMember("valueW", value.w, d.GetAllocator());
    objValue.AddMember("minSlider", minSlider, d.GetAllocator());
    objValue.AddMember("maxSlider", maxSlider, d.GetAllocator());
}
