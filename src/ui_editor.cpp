/**
 * @file ui_editor.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImVis is released under the MIT license.
 */

#include "ui_editor.hpp"

#include "appcontext.hpp"
#include "raycast.hpp"

#if defined(__EMSCRIPTEN__)
#include "ui_emscripten.hpp"
#endif

void UIEditor::functionEditor(AppContext &context, Raycast const &raycast,
                              gsl::not_null<ImFont *> font) {
  static constexpr std::size_t kMaxEditorTextSize{80 * 16};
  static constexpr std::string_view kEditorErrorMessage{
      "ERROR: Ill-formed code or expression"};

  auto &appState{context.appState};
  auto &renderState{context.renderState};
  auto &groups{context.functionManager.getGroups()};

  ImVec2 uiWindowSize{};
  ImVec2 const minUIWindowSize{250, 240};
  auto const &data{renderState.function.getData()};

  if (appState.updateFunctionEditorLayout) {
    auto const width{
        std::min(800.0f, gsl::narrow<float>(appState.windowSize.x) - 10.0f)};
    auto const height{
        std::min(600.0f, gsl::narrow<float>(appState.windowSize.y) - 10.0f)};
    ImGui::SetNextWindowPos(
        ImVec2((gsl::narrow<float>(appState.windowSize.x) - width) * 0.5f,
               (gsl::narrow<float>(appState.windowSize.y) - height) * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    appState.updateFunctionEditorLayout = false;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, minUIWindowSize);
  ImGui::Begin("Function Editor", &appState.showFunctionEditor,
               ImGuiWindowFlags_HorizontalScrollbar);

  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    SDL_SetCursor(SDL_GetDefaultCursor());
  }

  uiWindowSize = ImGui::GetWindowSize();

  ImGui::TextUnformatted("GLSL ES 3.00 embedded code:");
  if (!raycast.isProgramValid()) {
    auto const textSize{ImGui::CalcTextSize(kEditorErrorMessage.data())};
    ImGui::SameLine(uiWindowSize.x - textSize.x - 8);

    ImVec4 const redColor{1.0f, 0.35f, 0.35f, 1.0f};
    ImGui::TextColored(redColor, "%s", kEditorErrorMessage.data());
  }
  ImGui::BeginChild("##childScopes", ImVec2(0, uiWindowSize.y / 2),
                    ImGuiChildFlags_Borders |
                        ImGuiChildFlags_AlwaysUseWindowPadding);
  ImGui::TextUnformatted("Global scope:");
  ImGui::SameLine(uiWindowSize.x / 2 - 4);
  ImGui::TextUnformatted("Local scope:");

  std::string newExpression{};
  std::string newEmbeddedCodeGlobal{};
  std::string newEmbeddedCodeLocal{};

  static constexpr auto editorCallback{
      [](ImGuiInputTextCallbackData *cb) -> int {
        switch (cb->EventFlag) {
        case ImGuiInputTextFlags_CallbackCharFilter: {
          auto const c{cb->EventChar};

          // Allow newline + tab for code editing
          if (c == '\n' || c == '\t') {
            return 0;
          }

          // Printable ASCII only
          if (c >= 0x20 && c <= 0x7E) {
            return 0;
          }

          return 1; // Reject
        }

        case ImGuiInputTextFlags_CallbackEdit: {
          // Sanitize buffer (account for pasted code)
          auto src{cb->Buf};
          auto dst{cb->Buf};
          auto const end{cb->Buf + cb->BufTextLen};

          while (src < end) {
            auto const c{gsl::narrow_cast<unsigned char>(*src)};

            if (c == '\n' || c == '\t' || (c >= 0x20 && c <= 0x7E)) {
              *dst++ = *src;
            }

            src++;
          }

          *dst = '\0';

          // Update length + cursor
          cb->BufTextLen = gsl::narrow<int>(dst - cb->Buf);
          cb->CursorPos = std::min(cb->CursorPos, cb->BufTextLen);
          cb->SelectionStart = cb->SelectionEnd = cb->CursorPos;

          break;
        }
        }

        return 0;
      }};

  // Code embedded in global scope
  {
    std::array<char, kMaxEditorTextSize + 1> embeddedCode{};
    auto const &code{data.codeGlobal};
    code.copy(embeddedCode.data(), std::min(code.length(), kMaxEditorTextSize));
    ImGui::PushFont(font);
    ImGui::InputTextMultiline("##editGlobalScope", embeddedCode.data(),
                              embeddedCode.size(),
                              ImVec2(uiWindowSize.x / 2 - 20, -1),
                              ImGuiInputTextFlags_AllowTabInput |
                                  ImGuiInputTextFlags_CallbackCharFilter |
                                  ImGuiInputTextFlags_CallbackEdit,
                              editorCallback);
    ImGui::PopFont();
    newEmbeddedCodeGlobal.assign(embeddedCode.data(), kMaxEditorTextSize);
    newEmbeddedCodeGlobal.resize(newEmbeddedCodeGlobal.find('\0'));
  }

  // Code embedded in local scope
  {
    ImGui::SameLine();
    std::array<char, kMaxEditorTextSize + 1> embeddedCode{};
    auto const &code{data.codeLocal};
    code.copy(embeddedCode.data(), std::min(code.length(), kMaxEditorTextSize));
    ImGui::PushFont(font);
    ImGui::InputTextMultiline("##editLocalScope", embeddedCode.data(),
                              embeddedCode.size(),
                              ImVec2(uiWindowSize.x / 2 - 20, -1),
                              ImGuiInputTextFlags_AllowTabInput |
                                  ImGuiInputTextFlags_CallbackCharFilter |
                                  ImGuiInputTextFlags_CallbackEdit,
                              editorCallback);
    ImGui::PopFont();
    newEmbeddedCodeLocal.assign(embeddedCode.data(), kMaxEditorTextSize);
    newEmbeddedCodeLocal.resize(newEmbeddedCodeLocal.find('\0'));
  }
  ImGui::EndChild();

  ImGui::TextUnformatted("Expression:");
  auto const eqIsovalue{std::format("= {:.3g}", renderState.isoValue)};
  auto const eqIsoValueSize{ImGui::CalcTextSize(eqIsovalue.c_str())};

  // Multiline text is prefilled with zeros to indicate its maximum size
  std::string multilineText(kMaxEditorTextSize + 1, '\0');
  data.expression.copy(multilineText.data(),
                       std::min(kMaxEditorTextSize, data.expression.length()));

  ImGui::PushFont(font);
  ImGui::InputTextMultiline(
      "##editExpression", multilineText.data(), multilineText.size(),
      ImVec2(uiWindowSize.x - (eqIsoValueSize.x + 25), -1),
      ImGuiInputTextFlags_AllowTabInput |
          ImGuiInputTextFlags_CallbackCharFilter |
          ImGuiInputTextFlags_CallbackEdit,
      editorCallback);
  ImGui::PopFont();

  newExpression.assign(multilineText.data(), kMaxEditorTextSize);
  newExpression.resize(newExpression.find('\0'));
  ImGui::SameLine();
  ImGui::AlignTextToFramePadding();
  ImGui::Text("%s", eqIsovalue.c_str());

  ImGui::End();
  ImGui::PopStyleVar();

  // Poll every second to check if the embedded code or expression has changed
  static auto const pollInterval{1.0};
  static auto lastTime{ImGui::GetTime()};
  auto const currentTime{ImGui::GetTime()};
  if (currentTime - lastTime > pollInterval) {
    lastTime = currentTime;
    // Has anything changed?
    if (newExpression != data.expression ||
        newEmbeddedCodeGlobal != data.codeGlobal ||
        newEmbeddedCodeLocal != data.codeLocal) {
      auto &oldFunction{groups.at(appState.selectedFunctionGroupIndex)
                            .functions.at(appState.selectedFunctionIndex)};

      // Create a user-defined function from the current selected function
      Function::Data userDefinedData{oldFunction.getData()};
      userDefinedData.name = "User-defined";
      userDefinedData.expression = newExpression;
      userDefinedData.codeGlobal = newEmbeddedCodeGlobal;
      userDefinedData.codeLocal = newEmbeddedCodeLocal;
      userDefinedData.comment = "";

      Function userDefinedFunction{userDefinedData};
      context.functionManager.addUserDefined(userDefinedFunction);

      // Change selection to first function of the user-defined group
      appState.selectedFunctionGroupIndex = groups.size() - 1;
      appState.selectedFunctionIndex = 0;
      appState.updateFunctionTabSelection = true;

#if defined(__EMSCRIPTEN__)
      emscriptenMathJax::updateEquationName(userDefinedData.name);
      emscriptenMathJax::updateEquation(
          userDefinedFunction.getMathJaxEquation(renderState.isoValue),
          appState.overlayMathJaxComment ? userDefinedData.comment : "");
#endif

      renderState.function = userDefinedFunction;
    }
  }
}