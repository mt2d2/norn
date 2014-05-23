#include <cstdlib> // exit

#include "main.h"

int main(int argc, char *argv[]) {
  if (argc < 2)
    raise_error("usage: norn <file>");

  Params params = parse_params(argc, argv);
  if (params.version) {
    std::cout << "Norn v0.2" << std::endl;
    exit(0);
  } else if (params.help) {
    std::cout << "norn <file>" << std::endl;
    std::cout << "Flags: " << std::endl;
    std::cout << "    -version" << std::endl;
    std::cout << "    -help" << std::endl;
    std::cout << "    -bytecode " << std::endl;
    std::cout << "    -noopt" << std::endl;
    std::cout << "    -nojit" << std::endl;
    exit(0);
  }

  // lex, parse, optimize, and execute!
  // parse the language stdlib and input
  Program program;
  {
    BuildContext build;

    std::ifstream lang("lang.norn");
    Parser(lang).parse(build, -1);
    std::ifstream source(argv[argc - 1]);
    Parser(source).parse(build, params.optimize ? 1 : 0);

    program = build.get_program();

    program.absolute_jumps();
    program.calculate_memory_slots();
  }

  if (params.bytecode)
    std::cout << program << std::endl;
  else
    Machine(program, params.debug, params.nojit).execute();
}

Params parse_params(int argc, char **argv) {
  int stop = argc - 1;
  if (argv[argc - 1][0] == '-')
    stop = argc;

  Params params;
  for (int i = 1 /*skip program*/; i < stop /*skip filename*/; i++) {
    std::string input = argv[i];
    if (input[0] == '-') {
      if (input.find("debug") != std::string::npos)
        params.debug = true;
      if (input.find("version") != std::string::npos)
        params.version = true;
      if (input.find("help") != std::string::npos)
        params.help = true;
      if (input.find("noopt") != std::string::npos)
        params.optimize = false;
      if (input.find("bytecode") != std::string::npos)
        params.bytecode = true;
      if (input.find("nojit") != std::string::npos)
        params.nojit = true;
    }
  }

  return params;
}
