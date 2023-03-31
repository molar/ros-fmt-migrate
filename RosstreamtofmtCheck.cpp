//===--- RosstreamtofmtCheck.cpp - clang-tidy -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RosstreamtofmtCheck.h"

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
// from ASTMatchers Internal

inline auto getRosLoggers() {
  return anyOf(
      compoundStmt(isExpandedFromMacro("ROS_DEBUG_STREAM")).bind("debug"),
      compoundStmt(isExpandedFromMacro("ROS_INFO_STREAM")).bind("info"),
      compoundStmt(isExpandedFromMacro("ROS_WARN_STREAM")).bind("warn"),
      compoundStmt(isExpandedFromMacro("ROS_ERROR_STREAM")).bind("error"),
      compoundStmt(isExpandedFromMacro("ROS_FATAL_STREAM")).bind("fatal"));
}

template <typename T>
inline auto getLogExpression(T Matchers) {
  using namespace clang::ast_matchers;
  return compoundStmt(
             Matchers,
             hasDescendant(
                 cxxOperatorCallExpr(hasOperatorName("<<")).bind("logcode")),
             isExpansionInMainFile())
      .bind("logexpr");
}

class FormatStringBuilder {
 public:
  FormatStringBuilder(clang::SourceManager &Sm, std::string LoggerName)
      : LoggerTo{LoggerName}, Sm(Sm) {}
  void addStringLiteral(const clang::StringLiteral &Sl) {
    auto Str = Sl.getString();
    FmtStringComponents.push_back(Str.str());
  }
  void addIntegerLiteral(const clang::IntegerLiteral &Il) {
    FmtStringComponents.push_back(
        clang::tidy::modernize::getExprAsString(Sm, Il).str());
  }

  void addFormatExpr(const clang::Expr &Ex) {
    // copy from start to end loc
    FmtStringComponents.push_back("{}");
    FmtArgsComponents.push_back(
        clang::tidy::modernize::getExprAsString(Sm, Ex).str());
  }

  std::string getFormatString() {
    // ignore first entry which is the target string stream
    std::vector<std::string> Strings{FmtStringComponents.begin() + 1,
                                     FmtStringComponents.end()};
    std::vector<std::string> Args{FmtArgsComponents.begin() + 1,
                                  FmtArgsComponents.end()};
    std::stringstream Ss;
    Ss << LoggerTo << "(\"";
    for (const auto &Fmt : Strings) {
      Ss << Fmt;
    }
    Ss << "\"";
    for (const auto &Arg : Args) {
      Ss << "," << Arg;
    }
    Ss << ")";
    return Ss.str();
  }

 private:
  std::string LoggerTo;
  clang::SourceManager &Sm;
  llvm::SmallVector<std::string> FmtStringComponents;
  llvm::SmallVector<std::string> FmtArgsComponents;
};

void visitCallExpr(const clang::Expr &A0, const clang::Expr &A1,
                   FormatStringBuilder &FSB);

void visitArg(const clang::Expr &A, FormatStringBuilder &FSB) {
  if (const clang::CXXOperatorCallExpr *Oper =
          llvm::dyn_cast<const clang::CXXOperatorCallExpr>(
              &A))  // CXXOperatorCall("<<")
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
  Finder->addMatcher(Matcher, this);
}

void RosstreamtofmtCheck::check(const MatchFinder::MatchResult &Result) {
  std::string logger_detected = "unknown";
  const auto &map = Result.Nodes.getMap();
  for (const std::string &logger :
       {"debug", "info", "warn", "error", "fatal"}) {
    if (map.find(logger) != map.end()) {
      logger_detected = logger;
      break;
    }
  }

  std::stringstream Ss;
  std::transform(logger_detected.begin(), logger_detected.end(),
                 logger_detected.begin(), ::toupper);
  Ss << "ROSFMT_" << logger_detected;

  if (const clang::CXXOperatorCallExpr *FS =
          Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("logcode")) {
    const auto *Arg0 = FS->getArg(0);
    const auto *Arg1 = FS->getArg(1);

    FormatStringBuilder FSB(*Result.SourceManager, Ss.str());
    visitCallExpr(*Arg0, *Arg1, FSB);
    auto FormatString = FSB.getFormatString();
    auto Expand = Result.SourceManager->getExpansionRange(FS->getSourceRange());
    Ss.str("");
    Ss << "Rewrite to use format style instead " << FormatString;
    auto Diag = diag(Expand.getBegin(), Ss.str(), DiagnosticIDs::Warning);
    Diag << FixItHint::CreateReplacement(Expand.getAsRange(), FormatString);
  }
}

}  // namespace modernize
}  // namespace tidy
}  // namespace clang
