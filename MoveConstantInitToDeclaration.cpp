
//===--- ReorderCtorInitializer.cpp - clang-tidy --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MoveConstantInitToDeclaration.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Transformer/RangeSelector.h"
#include "clang/Tooling/Transformer/RewriteRule.h"
#include "clang/Tooling/Transformer/Transformer.h"
#include <algorithm>
#include <experimental/map>
#include <iostream>
#include <llvm-14/llvm/Support/raw_ostream.h>
#include <sstream>
using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace modernize {

llvm::StringRef getSourceRangeAsString(SourceManager &Sm,
                                       const SourceRange &Range) {

  clang::LangOptions Lo;
  // get the text from lexer including newlines and other formatting?

  auto StartLoc = Sm.getSpellingLoc(Range.getBegin());
  auto LastTokenLoc = Sm.getSpellingLoc(Range.getEnd());
  auto EndLoc = clang::Lexer::getLocForEndOfToken(LastTokenLoc, 0, Sm, Lo);
  auto PrintableRange = clang::SourceRange{StartLoc, EndLoc};
  auto Str = clang::Lexer::getSourceText(
      clang::CharSourceRange::getCharRange(PrintableRange), Sm,
      clang::LangOptions());
  return Str;
}

void MoveConstantInitToDeclaration::registerMatchers(MatchFinder *Finder) {

  auto Matcher = cxxRecordDecl(isExpansionInMainFile(),
                               hasDescendant(cxxCtorInitializer()))
                     .bind("class_with_ctor_init");
  Finder->addMatcher(Matcher, this);
}

template <typename T> bool compare_literal(const T &a, const T &b) {
  return a.getValue() == b.getValue();
}

template <>
bool compare_literal(const clang::StringLiteral &a,
                     const clang::StringLiteral &b) {
  return a.getString() == b.getString();
}

template <typename T>
bool compare(llvm::SmallVector<const CXXCtorInitializer *> Inits) {
  auto head = llvm::cast<T>(Inits.front()->getInit());
  return std::all_of(
      std::next(Inits.begin()), Inits.end(), [head](const auto &val) {
        return compare_literal(*head, *llvm::cast<T>(val->getInit()));
      });
}

void MoveConstantInitToDeclaration::check(
    const MatchFinder::MatchResult &Result) {
  if (const clang::CXXRecordDecl *FS =
          Result.Nodes.getNodeAs<clang::CXXRecordDecl>(
              "class_with_ctor_init")) {
    clang::SourceManager &Sm = *Result.SourceManager;
    std::map<FieldDecl *, llvm::SmallVector<const CXXCtorInitializer *>>
        InitWithLiteral;
    for (const auto &Ctor : FS->ctors()) {
      if (Ctor->getNumCtorInitializers() == 0) {
        continue;
      }

      for (const auto Init : Ctor->inits()) {
        if (Init->isBaseInitializer()) {
          continue;
        }
        if (Init->getSourceOrder() < 0) {
          continue;
        }
        if (llvm::isa<clang::IntegerLiteral>(Init->getInit()) ||
            llvm::isa<clang::StringLiteral>(Init->getInit()) ||
            llvm::isa<clang::FloatingLiteral>(Init->getInit())) {
          InitWithLiteral[Init->getMember()].push_back(Init);
        }
      }
    }
    // filter out cases where there are multiple inits in different constructors
    // of the same field, and they use a different init value.
    std::experimental::erase_if(InitWithLiteral, [](const auto &kv) -> bool {
      auto &[Field, Inits] = kv;
      if (llvm::isa<IntegerLiteral>(Inits.front()->getInit())) {
        return !compare<IntegerLiteral>(Inits);
      }
      if (llvm::isa<clang::StringLiteral>(Inits.front()->getInit())) {
        return !compare<clang::StringLiteral>(Inits);
      }
      if (llvm::isa<clang::FloatingLiteral>(Inits.front()->getInit())) {
        return !compare<clang::FloatingLiteral>(Inits);
      }
      return false;
    });
    // for the remaining pairs in the map emit fixit.
    for (const auto &[Field, Inits] : InitWithLiteral) {
      std::stringstream ss;
      ss << getSourceRangeAsString(Sm, Field->getSourceRange()).str() << "{"
         << getSourceRangeAsString(Sm,
                                   (Inits.front()->getInit())->getSourceRange())
                .str()
         << "}";
      auto Diag = diag(Inits.front()->getSourceRange().getBegin(),
                       "initialise in declaration", DiagnosticIDs::Warning);
      for (const auto Init : Inits) {
        Diag << FixItHint::CreateRemoval(Init->getSourceRange());
      }
      Diag << FixItHint::CreateReplacement(Field->getSourceRange(), ss.str());
    }
  }
}

} // namespace modernize
} // namespace tidy
} // namespace clang
