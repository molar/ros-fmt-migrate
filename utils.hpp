#ifndef CLANG_TIDY_EXTERNAL_MODULE_UTILS_HPP_
#define CLANG_TIDY_EXTERNAL_MODULE_UTILS_HPP_
#include "clang/Basic/SourceManager.h"
namespace clang::tidy::modernize {

template <typename T>
llvm::StringRef getExprAsString(clang::SourceManager &Sm, const T &Ex) {
  clang::LangOptions Lo;
  // get the text from lexer including newlines and other formatting?
  auto Str = clang::Lexer::getSourceText(
      clang::CharSourceRange::getTokenRange(Ex.getSourceRange()), Sm,
      clang::LangOptions());
  return Str;
}

}  // namespace clang::tidy::modernize
#endif
