//===--- RosstreamtofmtCheck.cpp - clang-tidy -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RosstreamtofmtCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Transformer/RangeSelector.h"
#include "clang/Tooling/Transformer/RewriteRule.h"
#include "clang/Tooling/Transformer/Transformer.h"
#include <sstream>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace modernize {
inline auto getRosLoggers() {
  return clang::ast_matchers::anyOf(
      clang::ast_matchers::isExpandedFromMacro("ROS_DEBUG_STREAM"),
      clang::ast_matchers::isExpandedFromMacro("ROS_INFO_STREAM"),
      clang::ast_matchers::isExpandedFromMacro("ROS_WARN_STREAM"),
      clang::ast_matchers::isExpandedFromMacro("ROS_ERROR_STREAM"),
      clang::ast_matchers::isExpandedFromMacro("ROS_FATAL_STREAM"));
}

template <typename T> inline auto getLogExpression(T Matchers) {
  using namespace clang::ast_matchers;
  return compoundStmt(
      Matchers,
      hasDescendant(cxxOperatorCallExpr(hasOperatorName("<<")).bind("logcode")),
      isExpansionInMainFile());
}

class FormatStringBuilder {
public:
  FormatStringBuilder(clang::SourceManager &Sm) : Sm(Sm) {}
  void addStringLiteral(const clang::StringLiteral &Sl) {
    auto Str = Sl.getString();
    FmtStringComponents.push_back(Str.str());
  }
  void addIntegerLiteral(const clang::IntegerLiteral &Il) {
    FmtStringComponents.push_back(getExprAsString(Il).str());
  }

  void addFormatExpr(const clang::Expr &Ex) {
    // copy from start to end loc
    FmtStringComponents.push_back("{}");
    FmtArgsComponents.push_back(getExprAsString(Ex).str());
  }

  std::string getFormatString() {
    std::stringstream Ss;
    Ss << "fmt::format(\"";
    for (const auto &Fmt : FmtStringComponents) {
      Ss << Fmt;
    }
    Ss << "\"";
    for (const auto &Arg : FmtArgsComponents) {
      Ss << "," << Arg;
    }
    Ss << ")";
    return Ss.str();
  }

private:
  llvm::StringRef getExprAsString(const clang::Expr &Ex) {

    auto Range = Ex.getSourceRange();

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
  clang::SourceManager &Sm;
  llvm::SmallVector<std::string> FmtStringComponents;
  llvm::SmallVector<std::string> FmtArgsComponents;
};

void visitCallExpr(const clang::Expr &A0, const clang::Expr &A1,
                   FormatStringBuilder &FSB);

void visitArg(const clang::Expr &A, FormatStringBuilder &FSB) {
  if (const clang::CXXOperatorCallExpr *Oper =
          llvm::dyn_cast<const clang::CXXOperatorCallExpr>(
              &A)) // CXXOperatorCall("<<")
  {
    if (Oper->getOperator() == clang::OverloadedOperatorKind::OO_LessLess) {
      visitCallExpr(*Oper->getArg(0), *Oper->getArg(1), FSB);
    } else {
      FSB.addFormatExpr(A);
    }
  } else if (const clang::ImplicitCastExpr *Cast =
                 llvm::dyn_cast<const clang::ImplicitCastExpr>(&A)) {
    // check if it is a string literal or 'ss' or
    // or 'endl'
    if (const clang::StringLiteral *Lit =
            llvm::dyn_cast<const clang::StringLiteral>(Cast->getSubExpr())) {
      FSB.addStringLiteral(*Lit);
    } else if (const auto *DeclRef = llvm::dyn_cast<const clang::DeclRefExpr>(
                   Cast->getSubExpr())) {
      if (DeclRef->getNameInfo().getName().getAsString() == "endl") {
        return;
      }
      FSB.addFormatExpr(A);
    } else {
      FSB.addFormatExpr(A);
    }
  } else if (const clang::IntegerLiteral *Lit =
                 llvm::dyn_cast<const clang::IntegerLiteral>(&A)) {
    FSB.addIntegerLiteral(*Lit);
  } else {
    if (const auto *DeclRef = llvm::dyn_cast<const clang::DeclRefExpr>(&A)) {
      if (DeclRef->getNameInfo().getName().getAsString() == "ss") {
        return;
      }
    }
    FSB.addFormatExpr(A);
  }
}

void visitCallExpr(const clang::Expr &A0, const clang::Expr &A1,
                   FormatStringBuilder &FSB) {
  visitArg(A0, FSB);
  visitArg(A1, FSB);
}

void RosstreamtofmtCheck::registerMatchers(MatchFinder *Finder) {
  auto Matcher = getLogExpression(getRosLoggers());
  clang::ast_matchers::StatementMatcher MyMatch = Matcher.bind("log");
  Finder->addMatcher(Matcher, this);
}

void RosstreamtofmtCheck::check(const MatchFinder::MatchResult &Result) {
  if (const clang::CXXOperatorCallExpr *FS =
          Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("logcode")) {
    const auto *Arg0 = FS->getArg(0);
    const auto *Arg1 = FS->getArg(1);
    FormatStringBuilder FSB(*Result.SourceManager);
    visitCallExpr(*Arg0, *Arg1, FSB);
    auto FormatString = FSB.getFormatString();
    auto Diag = diag(FS->getBeginLoc(), "Rewrite to use format style instead",
                     DiagnosticIDs::Warning);
    auto Expand = Result.SourceManager->getExpansionRange(FS->getSourceRange());
    Diag << FixItHint::CreateReplacement(Expand.getAsRange(), FormatString);
  }
}

} // namespace modernize
} // namespace tidy
} // namespace clang
