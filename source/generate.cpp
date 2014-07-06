#include "tree.h"

void NumberExprAST::emit_bytecode(BuildContext &out) {
  switch (type.get_primative()) {
  case INT:
    out.get_block()->add_instruction(
        Instruction(LIT_INT, Variant((int)this->val)));
    break;
  case FLOAT:
    out.get_block()->add_instruction(
        Instruction(LIT_FLOAT, Variant(((float)this->val))));
    break;
  default:
    raise_error("cannot emit NumberExprAST for non number type");
    break;
  }
}

void CharExprAST::emit_bytecode(BuildContext &out) {
  out.get_block()->add_instruction(Instruction(LIT_CHAR, Variant(this->val)));
}

void StringExprAST::emit_bytecode(BuildContext &out) {
  out.get_block()->add_instruction(Instruction(NEW_ARY, (int)val.size()));
  out.get_block()->add_instruction(
      Instruction(CPY_ARY_CHAR, out.get_program().add_string(val)));
}

void VariableExprAST::emit_bytecode(BuildContext &out) {
  if (!out.variable_exists(name))
    raise_error("variable '" + name + "' was not declared before use");

  // TODO: cludgy, should be able to declare in initalizer
  type = out.get_variable_type(this->name);

  if (type.is_primative()) {
    Opcode opcode;
    switch (type.get_primative()) {
    case BOOLEAN:
    case INT:
      opcode = LOAD_INT;
      break;
    case FLOAT:
      opcode = LOAD_FLOAT;
      break;
    case CHAR:
      opcode = LOAD_CHAR;
      break;
    case ARY:
    case CHAR_ARY:
    case INT_ARY:
    case FLOAT_ARY:
      opcode = LOAD_ARY;
      break;
    case VOID:
    default:
      opcode = LOAD_INT; // default, will never be used since error
      raise_error("cannot determine variable type in VariableExprAST");
      break;
    }

    out.get_block()->add_instruction(
        Instruction(opcode, Variant(out.get_mem_id(this->name))));
  } else {
    raise_error("cannot emit VariableExprAST for COMPLEX");
  }
}

void VariableFieldExprAST::emit_bytecode(BuildContext &out) {
  // load struct
  out.get_block()->add_instruction(
      Instruction(LOAD_INT, Variant(out.get_mem_id(this->name))));

  // load field
  auto struct_type = out.get_variable_type(name);
  auto field_type = struct_type.get_member(field);

  Opcode op = STRUCT_LOAD_INT;
  if (field_type == TypeFactory::get_instance().get(INT) ||
      field_type == TypeFactory::get_instance().get(BOOLEAN))
    op = STRUCT_LOAD_INT;
  else if (field_type == TypeFactory::get_instance().get(FLOAT))
    op = STRUCT_LOAD_FLOAT;
  else if (field_type == TypeFactory::get_instance().get(CHAR))
    op = STRUCT_LOAD_CHAR;
  else
    raise_error("unknown type in field access");

  out.get_block()->add_instruction(
      Instruction(op, struct_type.offset_of(field)));
}

void VariableAssignFieldExprAST::emit_bytecode(BuildContext &out) {
  value->emit_bytecode(out);

  // load struct
  out.get_block()->add_instruction(
      Instruction(LOAD_INT, Variant(out.get_mem_id(this->name))));

  // assign field
  auto struct_type = out.get_variable_type(name);
  auto field_type = struct_type.get_member(field);

  Opcode op = STRUCT_STORE_INT;
  if (field_type == TypeFactory::get_instance().get(INT) ||
      field_type == TypeFactory::get_instance().get(BOOLEAN))
    op = STRUCT_STORE_INT;
  else if (field_type == TypeFactory::get_instance().get(FLOAT))
    op = STRUCT_STORE_FLOAT;
  else if (field_type == TypeFactory::get_instance().get(CHAR))
    op = STRUCT_STORE_CHAR;
  else
    raise_error("unknown type in field access");

  out.get_block()->add_instruction(
      Instruction(op, struct_type.offset_of(field)));
}

void VariableAssignShortExprAST::emit_bytecode(BuildContext &out) {
  // ensure that the variable was previously declared
  if (!out.variable_exists(name))
    raise_error("variable '" + name + "' was not declared before use");
  else
    type = out.get_variable_type(name);

  // now default to standard variable declaration
  VariableAssignExprAST::emit_bytecode(out);
}

void VariableAssignExprAST::emit_bytecode(BuildContext &out) {
  out.set_variable_type(this->name, this->type);

  if (type.is_primative()) {
    Opcode opcode;
    switch (type.get_primative()) {
    case BOOLEAN:
    case INT:
      opcode = STORE_INT;
      break;
    case FLOAT:
      opcode = STORE_FLOAT;
      break;
    case CHAR:
      opcode = STORE_CHAR;
      break;
    case ARY:
    case CHAR_ARY:
      opcode = STORE_ARY;
      break;
    case VOID:
    default:
      opcode = STORE_INT; // default, will never be used since error
      raise_error("cannot emit VariableAssignExprAST for no type");
      break;
    }

    value->emit_bytecode(out);
    out.get_block()->add_instruction(
        Instruction(opcode, Variant(out.get_mem_id(this->name))));
  } else {
    value->emit_bytecode(out);
    out.get_block()->add_instruction(
        Instruction(STORE_INT, Variant(out.get_mem_id(this->name))));
  }
}

void emit_cast(BuildContext &out, ExprAST *left, ExprAST *right) {
  if (left->type == TypeFactory::get_instance().get(VOID)) {
    if (VariableExprAST *var = dynamic_cast<VariableExprAST *>(left))
      left->type = out.get_variable_type(var->name);
    else if (CallExprAST *c = dynamic_cast<CallExprAST *>(left))
      left->type = c->resolve_return_type(out);
    else
      raise_error("unknown cast on left");
  }

  if (right->type == TypeFactory::get_instance().get(VOID)) {
    if (VariableExprAST *var = dynamic_cast<VariableExprAST *>(right))
      right->type = out.get_variable_type(var->name);
    else if (CallExprAST *c = dynamic_cast<CallExprAST *>(right))
      right->type = c->resolve_return_type(out);
    else
      raise_error("unknown cast on right");
  }

  right->emit_bytecode(out);
  if (right->type == TypeFactory::get_instance().get(INT) &&
      left->type == TypeFactory::get_instance().get(FLOAT))
    out.get_block()->add_instruction(Instruction(I2F));

  left->emit_bytecode(out);
  if (left->type == TypeFactory::get_instance().get(INT) &&
      right->type == TypeFactory::get_instance().get(FLOAT))
    out.get_block()->add_instruction(Instruction(I2F));
}

void BinaryExprAST::emit_bytecode(BuildContext &out) {
  emit_cast(out, left, right);

  if (right->type == TypeFactory::get_instance().get(FLOAT) ||
      left->type == TypeFactory::get_instance().get(FLOAT))
    this->type = TypeFactory::get_instance().get(FLOAT);

  switch (type.get_primative()) {
  case INT:
    switch (this->op) {
    case '+':
      out.get_block()->add_instruction(Instruction(ADD_INT));
      break;
    case '-':
      out.get_block()->add_instruction(Instruction(SUB_INT));
      break;
    case '*':
      out.get_block()->add_instruction(Instruction(MUL_INT));
      break;
    case '/':
      out.get_block()->add_instruction(Instruction(DIV_INT));
      break;
    case '%':
      out.get_block()->add_instruction(Instruction(MOD_INT));
      break;
    case '&':
      out.get_block()->add_instruction(Instruction(AND_INT));
      break;
    case '|':
      out.get_block()->add_instruction(Instruction(OR_INT));
      break;
    case '<':
      out.get_block()->add_instruction(Instruction(LE_INT));
      break;
    case '>':
      out.get_block()->add_instruction(Instruction(GE_INT));
      break;
    case '=':
      out.get_block()->add_instruction(Instruction(EQ_INT));
      break;
    case '!':
      out.get_block()->add_instruction(Instruction(NEQ_INT));
      break;
    case 'l':
      out.get_block()->add_instruction(Instruction(LEQ_INT));
      break;
    case 'g':
      out.get_block()->add_instruction(Instruction(GEQ_INT));
      break;
    case 'a':
      out.get_block()->add_instruction(Instruction(LOGICAL_AND));
      break;
    case 'o':
      out.get_block()->add_instruction(Instruction(LOGICAL_OR));
      break;
    }
    break;
  case FLOAT:
    switch (this->op) {
    case '+':
      out.get_block()->add_instruction(Instruction(ADD_FLOAT));
      break;
    case '-':
      out.get_block()->add_instruction(Instruction(SUB_FLOAT));
      break;
    case '*':
      out.get_block()->add_instruction(Instruction(MUL_FLOAT));
      break;
    case '/':
      out.get_block()->add_instruction(Instruction(DIV_FLOAT));
      break;
    case '%':
      out.get_block()->add_instruction(Instruction(MOD_FLOAT));
      break;
    case '&':
      // out.get_block()->add_instruction(Instruction(AND_FLOAT));
      raise_error("cannot AND floats");
      break;
    case '|':
      raise_error("cannot OR floats");
      // out.get_block()->add_instruction(Instruction(OR_FLOAT));
      break;
    case '<':
      out.get_block()->add_instruction(Instruction(LE_FLOAT));
      break;
    case '>':
      out.get_block()->add_instruction(Instruction(GE_FLOAT));
      break;
    case '=':
      out.get_block()->add_instruction(Instruction(EQ_FLOAT));
      break;
    case '!':
      out.get_block()->add_instruction(Instruction(NEQ_FLOAT));
      break;
    case 'l':
      out.get_block()->add_instruction(Instruction(LEQ_FLOAT));
      break;
    case 'g':
      out.get_block()->add_instruction(Instruction(GEQ_FLOAT));
      break;
    }
    break;
  default:
    raise_error("cannot emit BinaryExprAST for non number type");
    break;
  }
}

Type CallExprAST::resolve_return_type(BuildContext &out) {
  return out.get_block_type(callee_signature(out));
}

void CallExprAST::emit_bytecode(BuildContext &out) {
  int block_id = this->get_block_id(out);
  // std::cout << "Calling: " <<  out.get_block_ptr(block_id)->get_name() << "
  // block_id: " << block_id << std::endl;

  for (auto i = Args.rbegin(); i != Args.rend(); ++i)
    (*i)->emit_bytecode(out);

  Block *callee = out.get_program().get_block_ptr(block_id);
  out.get_block()->add_instruction(
      Instruction(CALL, reinterpret_cast<Variant *>(callee)));
}

void PrototypeAST::emit_bytecode(BuildContext &out) {
  out.set_block(
      out.get_program().get_block_ptr(out.get_program().get_block_id(name)));

  // handle args
  for (auto &elem : args) {
    if (elem.second.is_primative()) {
      Opcode opcode;
      switch (elem.second.get_primative()) {
      case BOOLEAN:
      case INT:
        opcode = STORE_INT;
        break;
      case FLOAT:
        opcode = STORE_FLOAT;
        break;
      case CHAR:
        opcode = STORE_CHAR;
        break;
      case ARY:
      case CHAR_ARY:
      case INT_ARY:
      case FLOAT_ARY:
        opcode = STORE_ARY;
        break;
      case VOID:
      default:
        opcode = STORE_INT; // default, will never be used since error
        raise_error("cannot emit argument store for void type");
        break;
      }

      out.set_variable_type(elem.first, elem.second);
      out.get_block()->add_instruction(
          Instruction(opcode, out.get_mem_id(elem.first)));
    } else {
      raise_error("cannot emit argument in PrototypeAST for COMPLEX");
    }
  }
}

void FunctionAST::emit_bytecode(BuildContext &out) {
  this->proto->emit_bytecode(out);

  for (auto &elem : body)
    (elem)->emit_bytecode(out);

  if (out.get_block()->get_instruction(out.get_block()->get_size() - 1)->op !=
      RTRN)
    out.get_block()->add_instruction(Instruction(RTRN));
}

void IfExprAST::emit_bytecode(BuildContext &out) {
  int true_jmp = out.get_and_increment_seed();
  int false_jmp = out.get_and_increment_seed();

  // emit the condition
  // jump to true if true, proceed to then block
  this->cond->emit_bytecode(out);
  out.get_block()->add_instruction(Instruction(TJMP, true_jmp));

  // emit the otherwise block, jmp to end
  for (auto &elem : otherwise)
    (elem)->emit_bytecode(out);
  out.get_block()->add_instruction(Instruction(UJMP, false_jmp));

  // emit the then block, jump to end
  out.get_block()->add_instruction(Instruction(LBL, true_jmp));
  for (auto &elem : then)
    (elem)->emit_bytecode(out);

  // mark end
  out.get_block()->add_instruction(Instruction(LBL, false_jmp));
}

void VMBuiltinExprAST::emit_bytecode(BuildContext &out) {
  out.get_block()->add_instruction(Instruction(op));
}

void ForExprAST::emit_bytecode(BuildContext &out) {
  int step_label = out.get_and_increment_seed();
  int end_label = out.get_and_increment_seed();

  // starting label
  Start->emit_bytecode(out);

  // End and JMP if true
  out.get_block()->add_instruction(Instruction(LBL, step_label));
  out.get_block()->add_instruction(
      Instruction(LOOP, out.get_and_increment_loop_count()));
  End->emit_bytecode(out);
  out.get_block()->add_instruction(Instruction(FJMP, end_label));

  // main body, looping point
  for (auto *elem : body)
    elem->emit_bytecode(out);

  // Step++ and jump back
  Step->emit_bytecode(out);
  out.get_block()->add_instruction(Instruction(UJMP, step_label));

  // all done
  out.get_block()->add_instruction(Instruction(LBL, end_label));
}

void WhileExprAST::emit_bytecode(BuildContext &out) {
  int start_label = out.get_and_increment_seed();
  int end_label = out.get_and_increment_seed();

  // End and JMP if true
  out.get_block()->add_instruction(Instruction(LBL, start_label));
  cond->emit_bytecode(out);
  out.get_block()->add_instruction(Instruction(FJMP, end_label));

  // main body, looping point
  for (auto &elem : body)
    (elem)->emit_bytecode(out);

  // unconditionally jump back
  out.get_block()->add_instruction(Instruction(UJMP, start_label));

  // all done
  out.get_block()->add_instruction(Instruction(LBL, end_label));
}

void ReturnExprAST::emit_bytecode(BuildContext &out) {
  this->val->emit_bytecode(out);
  out.get_block()->add_instruction(Instruction(RTRN));
}

void ArrayDeclarationExprAST::emit_bytecode(BuildContext &out) {
  out.set_variable_type(this->name, this->type);
  out.get_block()->add_instruction(Instruction(NEW_ARY, this->size));
  out.get_block()->add_instruction(
      Instruction(STORE_ARY, out.get_mem_id(this->name)));
}

void ArrayIndexAssignExprAST::emit_bytecode(BuildContext &out) {
  value->emit_bytecode(out);

  if (value->type == TypeFactory::get_instance().get(VOID)) {
    Type var_type = out.get_variable_type(this->name);
    if (var_type == TypeFactory::get_instance().get("CharAry"))
      var_type = TypeFactory::get_instance().get("Char");
    else if (var_type == TypeFactory::get_instance().get("IntAry"))
      var_type = TypeFactory::get_instance().get("Int");
    else if (var_type == TypeFactory::get_instance().get("FloatAry"))
      var_type = TypeFactory::get_instance().get("Float");

    value->type = var_type;
  }

  if (value->type.is_primative()) {
    Opcode opcode;
    switch (value->type.get_primative()) {
    case BOOLEAN:
    case INT:
      opcode = STORE_ARY_ELM_INT;
      break;
    case FLOAT:
      opcode = STORE_ARY_ELM_FLOAT;
      break;
    case CHAR:
      opcode = STORE_ARY_ELM_CHAR;
      break;
    case ARY:
    case CHAR_ARY:
    case INT_ARY:
    case FLOAT_ARY:
      opcode = STORE_ARY_ELM_INT; // default, will never be used since error
      raise_error("array access for 'Ary' unimplemented");
      break;
    case VOID:
    default:
      opcode = LOAD_INT; // default, will never be used since error
      raise_error("cannot access array of no type");
      break;
    }

    index->emit_bytecode(out);
    out.get_block()->add_instruction(
        Instruction(opcode, Variant(out.get_mem_id(this->name))));
  } else {
    raise_error("cannot emit ArrayIndexAssignExprAST for COMPLEX");
  }
}

void ArrayIndexAccessExprAST::emit_bytecode(BuildContext &out) {
  Type var_type = out.get_variable_type(this->name);
  if (var_type == TypeFactory::get_instance().get("CharAry"))
    var_type = TypeFactory::get_instance().get("Char");
  else if (var_type == TypeFactory::get_instance().get("IntAry"))
    var_type = TypeFactory::get_instance().get("Int");
  else if (var_type == TypeFactory::get_instance().get("FloatAry"))
    var_type = TypeFactory::get_instance().get("Float");

  if (var_type.is_primative()) {
    Opcode opcode;
    switch (var_type.get_primative()) {
    case BOOLEAN:
    case INT:
      opcode = LOAD_ARY_ELM_INT;
      break;
    case FLOAT:
      opcode = LOAD_ARY_ELM_FLOAT;
      break;
    case CHAR:
      opcode = LOAD_ARY_ELM_CHAR;
      break;
    case ARY:
    case CHAR_ARY:
      opcode = LOAD_ARY_ELM_INT; // default, will never be used since error
      raise_error("array access for 'Ary' unimplemented");
      break;
    case VOID:
    default:
      opcode = LOAD_ARY_ELM_INT; // default, will never be used since error
      raise_error("cannot access array with no type");
      break;
    }

    index->emit_bytecode(out);
    out.get_block()->add_instruction(
        Instruction(opcode, Variant(out.get_mem_id(this->name))));
  } else {
    raise_error("cannot emit ArrayIndexAccessExprAST for COMPLEX");
  }
}
