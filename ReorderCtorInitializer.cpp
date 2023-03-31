
//===--- ReorderCtorInitializer.cpp - clang-tidy --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ReorderCtorInitializer.h"

#include <iostream>
#include <sstream>

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Transformer/RangeSelector.h"
#include "clang/Tooling/Transformer/RewriteRule.h"
#include "clang/Tooling/Transformer/Transformer.h"
#include "utils.hpp"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace modernize {

void ReorderCtorInitializer::registerMatchers(MatchFinder *Finder) {
  auto Matcher = cxxRecordDecl(isExpansionInMainFile(),
                               hasDescendant(cxxCtorInitializer()))
                     .bind("class_with_ctor_init");
  Finder->addMatcher(Matcher, this);
}

void ReorderCtorInitializer::check(const MatchFinder::MatchResult &Result) {
  if (const clang::CXXRecordDecl *FS =
          Result.Nodes.getNodeAs<clang::CXXRecordDecl>(
              "class_with_ctor_init")) {
    clang::SourceManager &Sm = *Result.SourceManager;
    for (const auto &Ctor : FS->ctors()) {
      if (Ctor->getNumCtorInitializers() == 0) {
        continue;
      }

      std::map<int, const CXXCtorInitializer *> InitOrder;
      llvm::SmallVector<const CXXCtorInitializer *> BaseInitializers;
      for (const auto Init : Ctor->inits()) {
        if (Init->isBaseInitializer()) {
          BaseInitializers.push_back(Init);
        }
        if (Init->getSourceOrder() < 0) {
          continue;
        }
        InitOrder[Init->getSourceOrder()] = Init;
      }
      std::map<int, const CXXCtorInitializer *> FieldOrder;
      int FieldOrderIndex = 0;
      for (const auto Field : FS->fields()) {
        for (const auto &[Order, Init] : InitOrder) {
          if (Order < 0) {
            continue;
          }
          if (Field == Init->getMember()) {
            FieldOrder[FieldOrderIndex] = Init;
          }
        }
        ++FieldOrderIndex;
      }
      // compare the two maps
      if (FieldOrder != InitOrder) {
        // we have a mismatch
        // emit fixit with new order.
        // the fixit is constructed by copying source ranges in the right order
        // :)
        llvm::SmallVector<llvm::StringRef> Ordering;
        for (const auto &BaseInits : BaseInitializers) {
          Ordering.push_back(getExprAsString(Sm, *BaseInits));
        }
        for (const auto &[Order, Init] : FieldOrder) {
          Ordering.push_back(getExprAsString(Sm, *Init));
        }
        std::stringstream Ss;
        int Cnt = 0;
        for (const auto &Str : Ordering) {
          Ss << Str.str();
          ++Cnt;
          if (Cnt != Ordering.size()) {
            Ss << ", ";
          }
        }

        auto FormatString = Ss.str();
        auto Begin = InitOrder[0]->getSourceRange().getBegin();
        auto End = InitOrder[InitOrder.size() - 1]->getSourceRange().getEnd();
        auto Diag = diag(Begin, "Write in field declaration order instead",
                         DiagnosticIDs::Warning);
        Diag << FixItHint::CreateReplacement(SourceRange{Begin, End},
                                             FormatString);
      }
    }
  }
}

}  // namespace modernize
}  // namespace tidy
}  // namespace clang
