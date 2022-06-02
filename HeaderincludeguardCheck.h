#ifndef HEADER_INCLUDE_GUARD_CHECK_H_
#define HEADER_INCLUDE_GUARD_CHECK_H_

#include <clang-tidy/utils/HeaderGuard.h>

namespace myplugin {
class MyHeaderGuardCheck : public clang::tidy::utils::HeaderGuardCheck {
public:
  MyHeaderGuardCheck(clang::StringRef Name,
                     clang::tidy::ClangTidyContext *Context);

  std::string
  getHeaderGuard(clang::StringRef Filename,
                 clang::StringRef OldGuard = clang::StringRef()) override;
};
} // namespace myplugin

#endif /* HEADER_INCLUDE_GUARD_CHECK_H_ */
