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

namespace {

int editorCallback(ImGuiInputTextCallbackData *callback) {
  constexpr auto allowed{[](unsigned char ch) noexcept {
    // Allow newline + tab for code editing, and printable ASCII
    return ch == '\n' || ch == '\t' || (ch >= 0x20 && ch <= 0x7E);
  }};

  switch (callback->EventFlag) {
  case ImGuiInputTextFlags_CallbackCharFilter: {
    auto const ch{gsl::narrow_cast<unsigned char>(callback->EventChar)};

    return allowed(ch) ? 0 : 1;
  }

  case ImGuiInputTextFlags_CallbackEdit: {
    // Sanitize buffer (account for pasted code)
    auto const buf{
        std::span{callback->Buf, gsl::narrow<std::size_t>(callback->BufSize)}};

    auto dst{buf.begin()};
    auto const end{buf.begin() + callback->BufTextLen};

    for (auto it{buf.begin()}; it != end; ++it) {
      if (allowed(gsl::narrow_cast<unsigned char>(*it))) {
        *dst++ = *it;
      }
    }

    *dst = '\0';

    // Update length + cursor
    callback->BufTextLen = gsl::narrow<int>(dst - buf.begin());
    callback->CursorPos = std::min(callback->CursorPos, callback->BufTextLen);
    callback->SelectionStart = callback->SelectionEnd = callback->CursorPos;
    return 0;
  }
  default:
    break;
  }

  return 0;
}

} // namespace

void uiEditor::functionEditor(AppContext &context, Raycast const &raycast,
                              gsl::not_null<ImFont *> font) {
  static constexpr std::size_t kMaxEditorTextSize{80UL * 16};
  static constexpr auto kEditorErrorMessage{
      "ERROR: Ill-formed code or expression"};

  auto &appState{context.appState};
  auto &renderState{context.renderState};
  auto const &groups{context.functionManager.getGroups()};

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
    auto const textSize{ImGui::CalcTextSize(kEditorErrorMessage)};
    ImGui::SameLine(uiWindowSize.x - textSize.x - 8);

    ImVec4 const redColor{1.0f, 0.35f, 0.35f, 1.0f};
    ImGui::TextColored(redColor, "%s", kEditorErrorMessage);
  }
  ImGui::BeginChild("##childScopes", ImVec2(0, uiWindowSize.y / 2),
                    ImGuiChildFlags_Borders |
                        ImGuiChildFlags_AlwaysUseWindowPadding);
  ImGui::TextUnformatted("Global scope:");
  ImGui::SameLine((uiWindowSize.x / 2) - 4);
  ImGui::TextUnformatted("Local scope:");

  std::string newExpression{};
  std::string newEmbeddedCodeGlobal{};
  std::string newEmbeddedCodeLocal{};

  // Code embedded in global scope
  {
    std::array<char, kMaxEditorTextSize + 1> embeddedCode{};
    auto const &code{data.codeGlobal};
    code.copy(embeddedCode.data(), std::min(code.length(), kMaxEditorTextSize));
    embeddedCode[kMaxEditorTextSize] = '\0';
    ImGui::PushFont(font);
    ImGui::InputTextMultiline("##editGlobalScope", embeddedCode.data(),
                              embeddedCode.size(),
                              ImVec2((uiWindowSize.x / 2) - 20, -1),
                              ImGuiInputTextFlags_AllowTabInput |
                                  ImGuiInputTextFlags_CallbackCharFilter |
                                  ImGuiInputTextFlags_CallbackEdit,
                              editorCallback);
    ImGui::PopFont();
    newEmbeddedCodeGlobal = embeddedCode.data();
  }

  // Code embedded in local scope
  {
    ImGui::SameLine();
    std::array<char, kMaxEditorTextSize + 1> embeddedCode{};
    auto const &code{data.codeLocal};
    code.copy(embeddedCode.data(), std::min(code.length(), kMaxEditorTextSize));
    embeddedCode[kMaxEditorTextSize] = '\0';
    ImGui::PushFont(font);
    ImGui::InputTextMultiline("##editLocalScope", embeddedCode.data(),
                              embeddedCode.size(),
                              ImVec2((uiWindowSize.x / 2) - 20, -1),
                              ImGuiInputTextFlags_AllowTabInput |
                                  ImGuiInputTextFlags_CallbackCharFilter |
                                  ImGuiInputTextFlags_CallbackEdit,
                              editorCallback);
    ImGui::PopFont();
    newEmbeddedCodeLocal = embeddedCode.data();
  }
  ImGui::EndChild();

  ImGui::TextUnformatted("Expression:");
  auto const eqIsovalue{std::format("= {:.3g}", renderState.isoValue)};
  auto const eqIsoValueSize{ImGui::CalcTextSize(eqIsovalue.c_str())};

  // Multiline text is prefilled with zeros to indicate its maximum size
  std::array<char, kMaxEditorTextSize + 1> multilineText{};
  data.expression.copy(multilineText.data(),
                       std::min(kMaxEditorTextSize, data.expression.length()));
  multilineText[kMaxEditorTextSize] = '\0';

  ImGui::PushFont(font);
  ImGui::InputTextMultiline(
      "##editExpression", multilineText.data(), multilineText.size(),
      ImVec2(uiWindowSize.x - (eqIsoValueSize.x + 25), -1),
      ImGuiInputTextFlags_AllowTabInput |
          ImGuiInputTextFlags_CallbackCharFilter |
          ImGuiInputTextFlags_CallbackEdit,
      editorCallback);
  ImGui::PopFont();

  newExpression = multilineText.data();
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
      auto const &oldFunction{
          groups.at(appState.selectedFunctionGroupIndex)
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