#include "parser.hpp"
#include <cstdarg>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

namespace sdf {

std::string token_type_to_string(TokenType type) {
  switch (type) {
  case TokenType::OPEN_CURLY:
    return "\"{\"";
  case TokenType::CLOSE_CURLY:
    return "\"}\"";
  case TokenType::OPEN_PAREN:
    return "\"(\"";
  case TokenType::CLOSE_PAREN:
    return "\")\"";
  case TokenType::COLON:
    return "\":\"";
  case TokenType::SEMICOLON:
    return "\";\"";
  case TokenType::IDENTIFIER:
    return "identifier";
  case TokenType::SCENE_IDENTIFIER:
    return "\"scene\" keyword";
  case TokenType::ASSET_IDENTIFIER:
    return "\"asset\" keyword";
  case TokenType::ENTITY_IDENTIFIER:
    return "\"entity\" keyword";
  case TokenType::STRING:
    return "string";
  case TokenType::FLOAT:
    return "float";
  case TokenType::INTEGER:
    return "integer";
  case TokenType::END:
    return "END";
  }

  return "";
}

Scanner::Scanner(const char *input) { this->input = std::string(input); }

Result<std::vector<Token>> Scanner::scan_tokens() {
  std::vector<Token> tokens;
  while (!is_at_end()) {
    auto result = this->scan_token();
    if (result) {
      Token token = result.value;
      token.line = line;
      tokens.push_back(token);
    } else {
      return result.error;
    }
  }

  return tokens;
}

char Scanner::previous() {
  if (input_pos > 0) {
    return input[input_pos - 1];
  }
  return '\0';
}

char Scanner::peek() {
  if (is_at_end())
    return '\0';
  return input[input_pos];
}

Result<Token> Scanner::scan_string() {
  size_t strpos = 0;
  String str = "";

  // @TODO: allow \" inside strings

  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n')
      line++;
    str[strpos++] = peek();
    str[strpos] = '\0';
    advance();
  }

  if (is_at_end()) {
    return Error::custom(line, "Unterminated string");
  }

  // Closing "
  advance();

  Token token;
  token.type = TokenType::STRING;
  strcpy(token.value.sval, str);

  return token;
}

Result<Token> Scanner::scan_number() {
  size_t strpos = 0;
  String str = "";
  bool is_float = false;

  str[strpos++] = previous();
  str[strpos] = '\0';

  while (is_digit(peek()) || peek() == '.') {
    str[strpos++] = peek();
    str[strpos] = '\0';

    if (peek() == '.') {
      is_float = true;
    }

    advance();
  }

  if (str[1] == '\0' && (str[0] == '-' || str[0] == '+' || str[0] == '+')) {
    // Error parsing number
    return Error::custom(line, "Expecting number");
  }

  Token token;
  if (is_float) {
    token.type = TokenType::FLOAT;
    token.value.fval = (float)atof(str);
  } else {
    token.type = TokenType::INTEGER;
    token.value.ival = (int)atoi(str);
  }

  return token;
}

Result<Token> Scanner::scan_identifier() {
  size_t strpos = 0;
  String str = "";

  str[strpos++] = previous();
  str[strpos] = '\0';

  while (is_alphanum(peek()) || peek() == '_') {
    str[strpos++] = peek();
    str[strpos] = '\0';

    advance();
  }

  Token token;
  token.type = TokenType::IDENTIFIER;
  strcpy(token.value.sval, str);

  if (strcmp(str, "scene") == 0) {
    token.type = TokenType::SCENE_IDENTIFIER;
  } else if (strcmp(str, "asset") == 0) {
    token.type = TokenType::ASSET_IDENTIFIER;
  } else if (strcmp(str, "entity") == 0) {
    token.type = TokenType::ENTITY_IDENTIFIER;
  }

  return token;
}

void Scanner::scan_comment() {
  while (peek() != '\n' && !is_at_end()) {
    advance();
  }

  if (is_at_end())
    return;

  if (peek() == '\n')
    line++;

  // Closing \n
  advance();
}

Result<Token> Scanner::scan_token() {
  char c = advance();
  switch (c) {
  case '(':
    return Token{TokenType::OPEN_PAREN};
  case ')':
    return Token{TokenType::CLOSE_PAREN};
  case '{':
    return Token{TokenType::OPEN_CURLY};
  case '}':
    return Token{TokenType::CLOSE_CURLY};
  case ':':
    return Token{TokenType::COLON};
  case ';':
    return Token{TokenType::SEMICOLON};

  case '\n':
    this->line++;
    // Next token
    return scan_token();

  case '\t':
  case '\r':
  case ' ':
    // Next token
    return scan_token();

  case '"':
    return scan_string();

  case '#':
    scan_comment();
    // Next token
    return scan_token();

  case '\0':
    return Token{TokenType::END};

  default:
    if (is_digit(c) || c == '+' || c == '-' || c == '.') {
      // Numbers
      return scan_number();
    } else if (is_alpha(c)) {
      // Identifiers
      return scan_identifier();
    } else {
      return Error::custom(
          line, std::string("Unexpected token ") + "\"" + c + "\"");
    }
  }
}

Result<SceneFile> Parser::postprocess(SceneFile scene) {
  std::unordered_map<std::string, size_t> asset_indices;
  for (size_t i = 0; i < scene.assets.size(); i++) {
    if (asset_indices.find(scene.assets[i].name) != asset_indices.end()) {
      return Error::custom(
          "Duplicate asset name: " + std::string(scene.assets[i].name));
    }
    scene.assets[i].index = i;
    asset_indices[scene.assets[i].name] = i;
  }

  std::unordered_set<std::string> entity_names;
  for (auto &entity : scene.entities) {
    if (strlen(entity.name) > 0) {
      if (entity_names.count(entity.name) > 0) {
        return Error::custom(
            "Duplicate entity name: " + std::string(entity.name));
      }
      entity_names.insert(entity.name);
    }

    std::unordered_set<std::string> component_names;
    for (auto &comp : entity.components) {
      if (strlen(comp.name) > 0) {
        if (component_names.count(comp.name) > 0) {
          return Error::custom(
              "Duplicate component type: " + std::string(comp.name));
        }
        component_names.insert(comp.name);
      }

      for (auto &prop : comp.properties) {
        if (strcmp(prop.name, "asset") == 0) {
          if (prop.values.size() != 1) {
            return Error::custom("Asset property must have a single value");
          }
          if (prop.values[0].type != ValueType::STRING) {
            return Error::custom("Asset property must have string type");
          }

          prop.values[0].type = ValueType::INTEGER;
          prop.values[0].value.ival = asset_indices[prop.values[0].value.sval];
          break;
        }
      }
    }
  }

  return scene;
}

Token Parser::advance() {
  if (!is_at_end()) {
    current++;
  }
  return previous();
}

bool Parser::check(TokenType type) {
  if (is_at_end())
    return false;
  return peek().type == type;
}

bool Parser::match(int n, ...) {
  va_list list;
  va_start(list, n);

  for (int i = 0; i < n; i++) {
    TokenType type = va_arg(list, TokenType);
    if (check(type)) {
      advance();
      return true;
    }
  }

  va_end(list);

  return false;
}

Result<SceneFile> Parser::parse() {
  SceneFile sceneFile;

  bool gotScene = false;

  while (!is_at_end()) {
    if (check(TokenType::SCENE_IDENTIFIER)) {
      if (gotScene)
        return Error::custom(peek(), "");
      auto result = parse_scene_block();
      if (!result) {
        return result.error;
      }
      sceneFile.scene = result.value;
      gotScene = true;
    } else if (check(TokenType::ASSET_IDENTIFIER)) {
      auto result = parse_asset_block();
      if (!result) {
        return result.error;
      }
      sceneFile.assets.push_back(result.value);
    } else if (check(TokenType::ENTITY_IDENTIFIER)) {
      auto result = parse_entity_block();
      if (!result) {
        return result.error;
      }
      sceneFile.entities.push_back(result.value);
    } else {
      return Error::expected(
          peek(), "\"scene\", \"asset\" or \"entity\" keywords");
    }
  }

  return this->postprocess(sceneFile);
}

Result<SceneBlock> Parser::parse_scene_block() {
  if (match(1, TokenType::SCENE_IDENTIFIER)) {
    if (!match(1, TokenType::OPEN_CURLY)) {
      // Fail
      return Error::expected(peek(), "\"{\" for scene scope");
    }

    std::vector<Property> properties;

    while (auto result = parse_property()) {
      properties.push_back(result.value);
    }

    if (!match(1, TokenType::CLOSE_CURLY)) {
      // Fail
      return Error::expected(peek(), "\"}\" for scene scope");
    }

    SceneBlock sceneBlock;
    sceneBlock.properties = properties;

    return sceneBlock;
  }

  return Error::expected(peek(), TokenType::SCENE_IDENTIFIER);
}

Result<AssetBlock> Parser::parse_asset_block() {
  if (match(1, TokenType::ASSET_IDENTIFIER)) {
    Token assetTypeToken = peek();
    if (!match(1, TokenType::IDENTIFIER)) {
      // Fail
      return Error::expected(peek(), "asset type");
    }

    Token assetNameToken = peek();
    if (!match(1, TokenType::STRING)) {
      // Fail
      return Error::expected(peek(), "asset name (string)");
    }

    if (!match(1, TokenType::OPEN_CURLY)) {
      // Fail
      return Error::expected(peek(), "\"{\" for asset scope");
    }

    std::vector<Property> properties;

    while (auto result = parse_property()) {
      properties.push_back(result.value);
    }

    if (!match(1, TokenType::CLOSE_CURLY)) {
      // Fail
      return Error::expected(peek(), "\"}\" for asset scope");
    }

    AssetBlock assetBlock;
    strcpy(assetBlock.type, assetTypeToken.value.sval);
    strcpy(assetBlock.name, assetNameToken.value.sval);
    assetBlock.properties = properties;

    return assetBlock;
  }

  return Error::expected(peek(), TokenType::ASSET_IDENTIFIER);
}

Result<EntityBlock> Parser::parse_entity_block() {
  if (match(1, TokenType::ENTITY_IDENTIFIER)) {
    String entityName = "";
    if (match(1, TokenType::STRING)) {
      Token entityNameToken = previous();
      strcpy(entityName, entityNameToken.value.sval);
    }

    if (!match(1, TokenType::OPEN_CURLY)) {
      // Fail
      return Error::expected(peek(), "\"{\" for entity scope");
    }

    std::vector<Component> components;

    while (auto result = parse_component()) {
      components.push_back(result.value);
    }

    if (!match(1, TokenType::CLOSE_CURLY)) {
      // Fail
      return Error::expected(peek(), "\"}\" for entity scope");
    }

    EntityBlock entityBlock;
    strcpy(entityBlock.name, entityName);
    entityBlock.components = components;

    return entityBlock;
  }

  return Error::expected(peek(), TokenType::ENTITY_IDENTIFIER);
}

Result<Component> Parser::parse_component() {
  if (match(1, TokenType::IDENTIFIER)) {
    Token ident = previous();

    if (!match(1, TokenType::OPEN_CURLY)) {
      // Fail
      return Error::expected(peek(), "\"{\" for component scope");
    }

    std::vector<Property> properties;

    while (auto result = parse_property()) {
      properties.push_back(result.value);
    }

    if (!match(1, TokenType::CLOSE_CURLY)) {
      // Fail
      return Error::expected(peek(), "\"}\" for component scope");
    }

    Component component;
    strcpy(component.name, ident.value.sval);
    component.properties = properties;

    return component;
  }

  return Error::expected(peek(), "component identifier");
}

Result<Property> Parser::parse_property() {
  if (match(
          4,
          TokenType::SCENE_IDENTIFIER,
          TokenType::ASSET_IDENTIFIER,
          TokenType::ENTITY_IDENTIFIER,
          TokenType::IDENTIFIER)) {
    Token ident = previous();

    std::vector<Value> values;

    while (auto result = parse_value()) {
      values.push_back(result.value);
    }

    Property property;
    strcpy(property.name, ident.value.sval);
    property.values = values;

    return property;
  }

  return Error::expected(peek(), "property identifier");
}

Result<Value> Parser::parse_value() {
  if (match(3, TokenType::INTEGER, TokenType::FLOAT, TokenType::STRING)) {
    Token valToken = previous();

    switch (valToken.type) {
    case TokenType::INTEGER:
      return Value{valToken.value.ival};
      break;
    case TokenType::FLOAT:
      return Value{valToken.value.fval};
      break;
    case TokenType::STRING:
      return Value{valToken.value.sval};
      break;
    default:
      break;
    }
  }

  return Error::expected(peek(), "value");
}

char *read_entire_file(FILE *file) {
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *storage = new char[size];
  fread(storage, sizeof(char), size, file);

  return storage;
}

Result<SceneFile> parse_file(FILE *file) {
  const char *input = read_entire_file(file);
  Scanner scanner(input);
  auto scannerResult = scanner.scan_tokens();
  delete[] input;

  if (scannerResult) {
    Parser parser(scannerResult.value);
    return parser.parse();
  } else {
    return scannerResult.error;
  }
}

Result<SceneFile> parse_string(const char *input) {
  Scanner scanner(input);
  auto scannerResult = scanner.scan_tokens();

  if (scannerResult) {
    Parser parser(scannerResult.value);
    return parser.parse();
  } else {
    return scannerResult.error;
  }
}
} // namespace sdf
