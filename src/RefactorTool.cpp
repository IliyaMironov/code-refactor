#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/CommandLine.h"

#include <unordered_set>

#include "RefactorTool.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

static llvm::cl::OptionCategory ToolCategory("refactor-tool options");

// Метод run вызывается для каждого совпадения с матчем. 
// Мы проверяем тип совпадения по bind-именам и применяем рефакторинг.
void RefactorHandler::run(const MatchFinder::MatchResult &Result) {
    auto& Diag = Result.Context->getDiagnostics();
    auto& SM = *Result.SourceManager; // Получаем SourceManager для проверки isInMainFile
    
    if (const auto *Dtor = Result.Nodes.getNodeAs<CXXDestructorDecl>("nonVirtualDtor")) {
        handle_nv_dtor(Dtor, Diag, SM);
    }

    if (const auto *Method = Result.Nodes.getNodeAs<CXXMethodDecl>("missingOverride");
        Method && Method->size_overridden_methods() > 0 && !Method->hasAttr<OverrideAttr>()) {
        handle_miss_override(Method, Diag, SM);
    }

    if (const auto *LoopVar = Result.Nodes.getNodeAs<VarDecl>("loopVar")) {
        handle_crange_for(LoopVar, Diag, SM);
    }
}

//todo: необходимо реализовать обработку случая невиртуального деструктора
void RefactorHandler::handle_nv_dtor(const CXXDestructorDecl *Dtor,
                            DiagnosticsEngine &Diag,
                            SourceManager &SM) {
    //Реализуйте Ваш код ниже
    if (!SM.isInMainFile(Dtor->getLocation())) return;
    unsigned Offset = SM.getFileOffset(Dtor->getLocation());
    if (!virtualDtorLocations.insert(Offset).second) return;
    Rewrite.InsertTextBefore(Dtor->getBeginLoc(), "virtual ");
    const unsigned DiagID = Diag.getCustomDiagID(
            DiagnosticsEngine::Remark,
            "Объявлен деструктор"
        );
    Diag.Report(Dtor->getLocation(), DiagID);
}

//todo: необходимо реализовать обработку случая отсутствие override
void RefactorHandler::handle_miss_override(const CXXMethodDecl *Method,
                            DiagnosticsEngine &Diag,
                            SourceManager &SM) {
    //Реализуйте Ваш код ниже
    if (!SM.isInMainFile(Method->getLocation())) return;
    SourceLocation SearchLoc = Method->getNameInfo().getEndLoc();
    const char *Buf = SM.getCharacterData(SearchLoc);
    int Depth = 0;
    while (*Buf) {
        if (*Buf == '(') ++Depth;
        else if (*Buf == ')') {
            --Depth;
            if (Depth == 0) break;
        }
        ++Buf;
    }

    SourceLocation InsertLoc = SearchLoc.getLocWithOffset(Buf - SM.getCharacterData(SearchLoc) + 1);
    Rewrite.InsertText(InsertLoc, " override");

    const unsigned DiagID = Diag.getCustomDiagID(
            DiagnosticsEngine::Remark,
            "Объявлен метод"
        );
    Diag.Report(Method->getLocation(), DiagID);
}

//todo: необходимо реализовать обработку случая отсутствие & в range-for
void RefactorHandler::handle_crange_for(const VarDecl *LoopVar,
                                        DiagnosticsEngine &Diag,
                                        SourceManager &SM){
    // Реализуйте Ваш код ниже
    if (!SM.isInMainFile(LoopVar->getLocation())) return;
    QualType T = LoopVar->getType();
    if (T->isFundamentalType()) return;
    // Вставляем & после конца TypeLoc
    TypeLoc TL = LoopVar->getTypeSourceInfo()->getTypeLoc();
    SourceLocation EndLoc = Lexer::getLocForEndOfToken(TL.getEndLoc(), 0, SM, Rewrite.getLangOpts());
    Rewrite.InsertText(EndLoc, "&");
    const unsigned DiagID = Diag.getCustomDiagID(
            DiagnosticsEngine::Remark,
            "Объявлена переменная"
        );
    Diag.Report(LoopVar->getLocation(), DiagID);
}

//todo: ниже необходимо реализовать матчеры для поиска узлов AST
//note: синтаксис написания матчеров точно такой же как и для использования clang-query
/*
    Пример того, как может выглядеть реализация:
    auto AllClassesMatcher()
    {
        return cxxRecordDecl().bind("classDecl");
    }
*/
auto NvDtorMatcher()
{
    return cxxRecordDecl(
        hasAnyBase(
            hasType(
                cxxRecordDecl(
                    hasMethod(cxxDestructorDecl(unless(isVirtual())).bind("nonVirtualDtor"))
                )
            )
        )
    );
}

auto NoOverrideMatcher()
{
    //todo: замените код ниже, на свою реализацию, необходимо реализовать матчеры для поиска методов без override
    return cxxMethodDecl(
        isVirtual(),
        unless(isOverride()),
        ofClass(hasAnyBase(hasType(cxxRecordDecl())))
    ).bind("missingOverride");
}

auto NoRefConstVarInRangeLoopMatcher()
{
    //todo: замените код ниже, на свою реализацию, необходимо реализовать матчеры для поиска range-for без &
    return cxxForRangeStmt(hasLoopVariable(
        varDecl(
            hasType(isConstQualified()),
            unless(hasType(referenceType()))
        ).bind("loopVar")
    ));
}

// Конструктор принимает Rewriter для изменения кода.
ComplexConsumer::ComplexConsumer(Rewriter &Rewrite) : Handler(Rewrite) {
    // Создаем MatchFinder и добавляем матчеры.
    Finder.addMatcher(NvDtorMatcher(), &Handler);
    Finder.addMatcher(NoOverrideMatcher(), &Handler);
    Finder.addMatcher(NoRefConstVarInRangeLoopMatcher(), &Handler);
}

// Метод HandleTranslationUnit вызывается для каждого файла.
void ComplexConsumer::HandleTranslationUnit(ASTContext &Context) {
    Finder.matchAST(Context);
}


std::unique_ptr<ASTConsumer> CodeRefactorAction::CreateASTConsumer(CompilerInstance &CI,
                                                StringRef file) {
    RewriterForCodeRefactor.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<ComplexConsumer>(
        RewriterForCodeRefactor);
}

bool CodeRefactorAction::BeginSourceFileAction( CompilerInstance &CI) {
// Инициализируем Rewriter для рефакторинга.
RewriterForCodeRefactor.setSourceMgr(CI.getSourceManager(),
                                        CI.getLangOpts());
    return true;  // Возвращаем true, чтобы продолжить обработку файла.
}

void CodeRefactorAction::EndSourceFileAction() {
    // Применяем изменения в файле.
    if (RewriterForCodeRefactor.overwriteChangedFiles()) {
        llvm::errs() << "Error applying changes to files.\n";
    }
}


int main(int argc, const char **argv) {
    // Парсер опций: Обрабатывает флаги командной строки, компиляционные базы данных.
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = ExpectedParser.get();
    // Создаем ClangTool
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    // Запускаем RefactorAction.
    return Tool.run(newFrontendActionFactory<CodeRefactorAction>().get());
}