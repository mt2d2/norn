#ifndef MAIN_H
#define MAIN_H

class Params {
public:
  Params()
      : debug(false), version(false), help(false), optimize(true),
        bytecode(false), nojit(false) {}
  bool debug;
  bool version;
  bool help;
  bool optimize;
  bool bytecode;
  bool nojit;
};

int main(int argc, char *argv[]);
Params parse_params(int argc, char **argv);

#endif
