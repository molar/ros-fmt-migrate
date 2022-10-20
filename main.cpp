// File lifted from /clang-tools-extra/test/clang-tidy/CTTestTidyModule.cpp
#include "HeaderincludeguardCheck.h"
#include "MoveConstantInitToDeclaration.h"
#include "ReorderCtorInitializer.h"
#include "RosstreamtofmtCheck.h"
#include "clang-tidy/ClangTidy.h"
#include "clang-tidy/ClangTidyCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang;
using namespace clang::tidy;
using namespace clang::ast_matchers;

namespace {

class CTTestModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<modernize::RosstreamtofmtCheck>(
        "mir-rosstreamfmt");
    CheckFactories.registerCheck<myplugin::MyHeaderGuardCheck>(
        "mir-headercheck");
    CheckFactories.registerCheck<modernize::ReorderCtorInitializer>(
        "mir-reorder");
    CheckFactories.registerCheck<modernize::MoveConstantInitToDeclaration>(
        "mir-move-init");
  }
};
} // namespace

namespace tidy1 {
// Register the CTTestTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<::CTTestModule> X("mytest-module",
                                                      "Adds my checks.");
} // namespace tidy1

// This anchor is used to force the linker to link in the generated object file
// and thus register the CTTestModule.
volatile int CTTestModuleAnchorSource = 0;
