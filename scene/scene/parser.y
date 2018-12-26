%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.2"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
#include <string>
#include <cstring>
#include "scene.hpp"

using namespace scene;

namespace scene {
class Driver;
}
}

%param { Driver& drv }

%locations

%define parse.trace
%define parse.error verbose

%code {
#include "driver.hpp"
}

%define api.token.prefix {TOK_}

%token END 0 "end of file"
%token SCENE_HEADER ASSETS_HEADER ENTITIES_HEADER
%token <int> INTEGER
%token <float> FLOAT
%token <std::string> NAME STRING
%token OPEN_CURLY CLOSE_CURLY OPEN_SQUARE CLOSE_SQUARE POUND
%token EOL SPACE

%type <int> id

%printer { yyo << $$; } <*>;

%%

scene:
  | EOL
  | scene scene_section
  | scene assets_section
  | scene entities_section
  ;

scene_section:
  SCENE_HEADER { drv.setSection(Driver::Section::eScene); }
  | scene_section EOL
  | scene_section EOL property
  ;

assets_section:
  ASSETS_HEADER { drv.setSection(Driver::Section::eAssets); }
  | assets_section EOL
  | assets_section EOL asset_name
  | assets_section EOL property
  ;

asset_name: id NAME { drv.addAsset($1, $2); };

entities_section:
  ENTITIES_HEADER { drv.setSection(Driver::Section::eEntities); }
  | entities_section EOL
  | entities_section EOL entity_id
  | entities_section EOL component
  ;

entity_id: id { drv.addEntity($1); };

component:
  component_name OPEN_CURLY CLOSE_CURLY
  | component_name OPEN_CURLY EOL CLOSE_CURLY
  | component_name OPEN_CURLY component_properties CLOSE_CURLY
  | component_name OPEN_CURLY EOL component_properties CLOSE_CURLY
  | component_name OPEN_CURLY EOL component_properties EOL CLOSE_CURLY
  ;

component_name: NAME { drv.addComponent($1); };

component_properties:
  property
  | component_properties EOL property
  ;

id:
  POUND INTEGER { $$ = $2; }
  | POUND { $$ = -1; }
  ;

property:
  property_name
  | OPEN_SQUARE {
    throw yy::parser::syntax_error(drv.m_location,
                                   "bracketed values must start at "
                                   "the same line as the property name");
  }
  | property_name property_name {
    throw yy::parser::syntax_error(drv.m_location,
                                   "property name cannot be used as a value");
  }
  | property_name values
  | property_name OPEN_SQUARE CLOSE_SQUARE
  | property_name OPEN_SQUARE EOL CLOSE_SQUARE
  | property_name OPEN_SQUARE bracketed_values CLOSE_SQUARE
  | property_name OPEN_SQUARE EOL bracketed_values CLOSE_SQUARE
  | property_name OPEN_SQUARE EOL bracketed_values EOL CLOSE_SQUARE
  ;

property_name: NAME { drv.addProperty($1); };

bracketed_values:
  value
  | bracketed_values EOL value
  | bracketed_values value
  ;

values:
  value
  | values value
  ;

value:
  int_value
  | float_value
  | string_value
  ;

int_value: INTEGER { drv.addValue($1); };
float_value: FLOAT { drv.addValue($1); };
string_value: STRING { drv.addValue($1); };

%%

void yy::parser::error (const location_type& l, const std::string& m) {
  std::cerr << l << ": " << m << '\n';
}
