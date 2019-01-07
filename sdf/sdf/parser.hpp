#pragma once

#include "scene.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace sdf {

enum struct TokenType {
  OPEN_PAREN,
  CLOSE_PAREN,
  OPEN_CURLY,
  CLOSE_CURLY,
  COLON,
  SEMICOLON,

  IDENTIFIER,
  SCENE_IDENTIFIER,
  ASSET_IDENTIFIER,
  ENTITY_IDENTIFIER,

  // Values
  STRING,
  INTEGER,
  FLOAT,

  END
};

struct Token {
  TokenType type;
  union {
    int ival;
    float fval;
    String sval;
  } value{0};
  size_t line = -1;
};

std::string token_type_to_string(TokenType type);

struct Error {
  Token token;
  std::string message;

  static Error expected(Token token, TokenType expected) {
    Error error;
    error.token = token;
    error.message = "[line " + std::to_string(token.line) + "] Expected " +
                    token_type_to_string(expected) + ", but instead got " +
                    token_type_to_string(token.type);
    return error;
  }

  static Error expected(Token token, const std::string &expected) {
    Error error;
    error.token = token;
    error.message = "[line " + std::to_string(token.line) + "] Expected " +
                    expected + ", but instead got " +
                    token_type_to_string(token.type);
    return error;
  }

  static Error custom(Token token, const std::string &message) {
    Error error;
    error.token = token;
    error.message = "[line " + std::to_string(token.line) + "] " + message;
    return error;
  }

  static Error custom(size_t line, const std::string &message) {
    Error error;
    error.message = "[line " + std::to_string(line) + "] " + message;
    return error;
  }

  static Error custom(const std::string &message) {
    Error error;
    error.message = message;
    return error;
  }
};

template <typename T> struct Result {
  Result(Error error) : error(error) {}
  Result(T t) : value(t) {}

  operator bool() { return error.message.length() == 0; }

  T value;
  Error error;
};

inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

inline bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_alphanum(char c) { return is_digit(c) || is_alpha(c); }

class Scanner {
public:
  Scanner(const char *input);

  Result<std::vector<Token>> scan_tokens();

private:
  std::string input;
  size_t input_pos = 0;
  size_t line = 1;

  inline bool is_at_end() { return input_pos > input.length(); }
  inline char advance() { return input[input_pos++]; }
  char previous();
  char peek();

  Result<Token> scan_string();
  Result<Token> scan_number();
  Result<Token> scan_identifier();
  void scan_comment();

  Result<Token> scan_token();
};

/*
  scene        -> (scene_block| asset_block | entity_block)*
  scene_block  -> SCENE_IDENT OPEN_CURLY (property)* CLOSE_CURLY
  asset_block  -> ASSET_IDENT IDENT string OPEN_CURLY (property)* CLOSE_CURLY
  entity_block -> ENTITY_IDENT string? OPEN_CURLY (component)* CLOSE_CURLY
  component    -> IDENT OPEN_CURLY (property)* CLOSE_CURLY
  property     -> (IDENT | SCENE_IDENT | ASSET_IDENT | ENTITY_IDENT) (value)*
  value        -> INTEGER FLOAT STRING
 */

class Parser {
public:
  Parser(const std::vector<Token> tokens) : tokens(tokens) {}

  Result<SceneFile> parse();

private:
  std::vector<Token> tokens;
  size_t current = 0;

  Result<SceneFile> postprocess(SceneFile scene);

  inline bool is_at_end() { return tokens.size() <= current + 1; }
  inline Token peek() { return tokens[current]; }
  inline Token previous() { return tokens[current - 1]; }
  Token advance();
  bool check(TokenType type);
  bool match(int n, ...);

  Result<SceneBlock> parse_scene_block();
  Result<AssetBlock> parse_asset_block();
  Result<EntityBlock> parse_entity_block();
  Result<Component> parse_component();
  Result<Property> parse_property();
  Result<Value> parse_value();
};

char *read_entire_file(FILE *file);

Result<SceneFile> parse_file(FILE *file);

Result<SceneFile> parse_string(const char *input);

} // namespace sdf
