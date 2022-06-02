#include "HeaderincludeguardCheck.h"

namespace myplugin {

MyHeaderGuardCheck::MyHeaderGuardCheck(clang::StringRef Name,
                                       clang::tidy::ClangTidyContext *Context)
    : clang::tidy::utils::HeaderGuardCheck(Name, Context) {}

std::string MyHeaderGuardCheck::getHeaderGuard(clang::StringRef Filename,
                                               clang::StringRef OldGuard) {
  // from llvms implementation
  std::string Guard = Filename.str();

  // When running under Windows, need to convert the path separators from
  // `\` to `/`.
  Guard = llvm::sys::path::convert_to_slash(Guard);

  // Find the project root make this configurable from settings?
  size_t PosProjectRoot = Guard.rfind("external-tidy-module");
  if (PosProjectRoot != clang::StringRef::npos)
    Guard = Guard.substr(PosProjectRoot);

  std::replace(Guard.begin(), Guard.end(), '/', '_');
  std::replace(Guard.begin(), Guard.end(), '.', '_');
  std::replace(Guard.begin(), Guard.end(), '-', '_');
  return clang::StringRef(Guard).upper();
}
} // namespace myplugin
