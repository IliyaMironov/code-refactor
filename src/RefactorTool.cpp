#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/CommandLine.h"

#include <unordered_set>
#include <cstring>

#include "RefactorTool.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

static llvm::cl::OptionCategory ToolCategory("refactor-tool options");
static llvm::cl::opt<std::string> LogFile("log",
    llvm::cl::desc("Path to log file for refactoring changes"),
    llvm::cl::init(""),
    llvm::cl::cat(ToolCategory));

// Метод run вызывается для каждого совпадения с матчем. 
// Мы проверяем тип совпадения по bind-именам и применяем рефакторинг.
void RefactorHandler::log(const SourceManager &SM, SourceLocation Loc,
                          const char *Kind, const char *Name) {
    if (!LogStream.is_open()) return;
    auto PLoc = SM.getPresumedLoc(Loc);
    if (PLoc.isValid()) {
        LogStream << PLoc.getFilename() << ":"
                  << PLoc.getLine() << ": "
                  << Kind << ": " << Name << "\n";
        LogStream.flush();
    }
}

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
    log(SM, Dtor->getLocation(), "virtual-dtor",
        Dtor->getParent()->getNameAsString().c_str());
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

    ++Buf;

    const char *Suffixes[] = {"const", "volatile", "noexcept", "&&", "&"};
    bool Found = true;
    while (Found) {
        const char *BeforeWS = Buf;
        while (*Buf == ' ' || *Buf == '\t') ++Buf;
        Found = false;
        for (const char *Suf : Suffixes) {
            size_t Len = std::strlen(Suf);
            if (std::strncmp(Buf, Suf, Len) == 0 &&
                (Len <= 2 || !std::isalnum(static_cast<unsigned char>(Buf[Len])))) {
                Buf += Len;
                Found = true;
                break;
            }
        }
        if (!Found) Buf = BeforeWS;
    }

    SourceLocation InsertLoc = SearchLoc.getLocWithOffset(Buf - SM.getCharacterData(SearchLoc));
    Rewrite.InsertText(InsertLoc, " override");
    log(SM, Method->getLocation(), "add-override",
        Method->getQualifiedNameAsString().c_str());

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
    log(SM, LoopVar->getLocation(), "add-reference",
        LoopVar->getNameAsString().c_str());
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
                    hasMethod(cxxDestructorDecl(
                        unless(isVirtual()),
                        unless(isImplicit())   // <-- добавить
                    ).bind("nonVirtualDtor"))
                )
            )
        )
    );
}

auto NoOverrideMatcher()
{
    return cxxMethodDecl(
        isOverride(),                    // <-- вместо isVirtual(), unless(isOverride())
        unless(cxxDestructorDecl())      // <-- добавить
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
ComplexConsumer::ComplexConsumer(Rewriter &Rewrite, const std::string &LogPath)
    : Handler(Rewrite, LogPath) {
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
        RewriterForCodeRefactor, LogFile);
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