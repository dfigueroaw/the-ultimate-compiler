

#include "../include/parser.h"
#include "../include/scanner.h"
#include "../include/visitor.h"

#include <exception>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace {
std::expected<std::string, std::string>
readFile(const std::filesystem::path &path) {
  std::ifstream input(path);
  if (!input)
    return std::unexpected("Error: no se pudo abrir el archivo '" +
                           path.string() + "'");

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::filesystem::path outputPathFor(const std::filesystem::path &inputPath) {
  auto outputPath = inputPath;
  outputPath.replace_extension(".s");
  return outputPath;
}

std::expected<std::ofstream, std::string>
openOutputFile(const std::filesystem::path &path) {
  std::ofstream output(path);
  if (!output)
    return std::unexpected("Error: no se pudo crear el archivo de salida '" +
                           path.string() + "'");
  return output;
}

std::expected<std::pair<std::filesystem::path, std::filesystem::path>,
              std::string>
parseArguments(int argc, const char *argv[]) {
  if (argc != 2 && argc != 4)
    return std::unexpected("Uso: " + std::string(argv[0]) +
                           " <archivo_de_entrada> [-o archivo_salida]");

  std::filesystem::path inputFile;
  std::optional<std::filesystem::path> outputFile;

  for (int i = 1; i < argc; ++i) {
    const std::string arg(argv[i]);
    if (arg == "-o") {
      if (outputFile.has_value())
        return std::unexpected("Error: opción '-o' duplicada");
      if (i + 1 >= argc)
        return std::unexpected("Error: falta archivo después de '-o'");
      outputFile = std::filesystem::path(argv[++i]);
      continue;
    }

    if (!inputFile.empty())
      return std::unexpected("Error: archivo de entrada duplicado");
    inputFile = std::filesystem::path(arg);
  }

  if (inputFile.empty())
    return std::unexpected("Error: falta archivo de entrada");

  return std::pair{inputFile, outputFile.value_or(outputPathFor(inputFile))};
}
} // namespace

int main(int argc, const char *argv[]) {

  auto arguments = parseArguments(argc, argv);
  if (!arguments) {
    std::cerr << arguments.error() << "\n";
    return 1;
  }

  const auto &[inputFile, outputFile] = *arguments;
  auto input = readFile(inputFile);
  if (!input) {
    std::cerr << input.error() << "\n";
    return 1;
  }

  Scanner scanner(input->c_str());
  Parser parser(&scanner);

  std::unique_ptr<Program> program;
  try {
    program = parser.parseProgram();
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }

  auto outfile = openOutputFile(outputFile);
  if (!outfile) {
    std::cerr << outfile.error() << "\n";
    return 1;
  }

  try {
    GenCodeVisitor codegen(*outfile);
    codegen.generate(program.get());
    std::cout << "Codigo ensamblador generado en '" << outputFile << "'.\n";
    std::cout << "Compilación exitosa.\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
