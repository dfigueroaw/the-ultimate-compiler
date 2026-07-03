#include "ast_printer.h"

#include "asm.h"
#include "parser.h"
#include "scanner.h"
#include "visitor.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

static std::string jsonEscape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 2);
  for (char c : s) {
    switch (c) {
    case '"': out += "\\\""; break;
    case '\\': out += "\\\\"; break;
    case '\n': out += "\\n"; break;
    case '\r': out += "\\r"; break;
    case '\t': out += "\\t"; break;
    default: out += c;
    }
  }
  return out;
}

struct CompileResult {
  std::string ast;
  std::string assembly;
  std::string error;
};

static CompileResult compileSource(const std::string &source) {
  CompileResult result;

  try {
    Scanner scanner(source.c_str());
    Parser parser(&scanner);
    std::unique_ptr<Program> program(parser.parseProgram());

    // Print AST
    AstPrinter printer;
    program->accept(&printer);
    result.ast = printer.result();

    // Generate assembly
    std::ostringstream asmOut;
    GenCodeVisitor codegen(asmOut);
    codegen.generate(program.get());
    result.assembly = asmOut.str();

  } catch (const std::exception &e) {
    result.error = e.what();
  } catch (...) {
    result.error = "Unknown error during compilation";
  }

  return result;
}

extern "C" {

char *compile_code(const char *source) {
  std::string src(source);
  auto res = compileSource(src);

  std::ostringstream json;
  json << "{";
  json << "\"ast\":\"" << jsonEscape(res.ast) << "\",";
  json << "\"assembly\":\"" << jsonEscape(res.assembly) << "\",";
  json << "\"error\":\"" << jsonEscape(res.error) << "\"";
  json << "}";

  std::string jsonStr = json.str();
  char *out = static_cast<char *>(std::malloc(jsonStr.size() + 1));
  if (out) {
    std::memcpy(out, jsonStr.c_str(), jsonStr.size() + 1);
  }
  return out;
}

void free_allocated(char *ptr) { std::free(ptr); }
}

// Force linking of exception runtime functions needed by Emscripten JS glue
extern "C" {
  void __cxa_increment_exception_refcount(void *);
  void __cxa_decrement_exception_refcount(void *);
  int __cxa_can_catch(void *, void *, void *);
  void *__cxa_get_exception_ptr(void *);
}
[[maybe_unused]] static auto _cxa_force_link = [] {
  (void)&__cxa_increment_exception_refcount;
  (void)&__cxa_decrement_exception_refcount;
  (void)&__cxa_can_catch;
  (void)&__cxa_get_exception_ptr;
  return 0;
}();

// Dummy main — Emscripten runtime needs it for full C++ init
int main(int, char *[]) { return 0; }
