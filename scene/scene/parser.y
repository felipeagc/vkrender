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
%token OPEN_CURLY CLOSE_CURLY POUND

%type <int> id

%printer { yyo << $$; } <*>;

%%

scene:
  | scene scene_section
  | scene assets_section
  | scene entities_section
  ;

scene_section:
  SCENE_HEADER { drv.setSection(Driver::Section::eScene); }
  | scene_section property
  ;

assets_section:
  ASSETS_HEADER { drv.setSection(Driver::Section::eAssets); }
  | assets_section asset_name
  | assets_section property
  ;

asset_name: id NAME { drv.addAsset($1, $2); };

entities_section:
  ENTITIES_HEADER { drv.setSection(Driver::Section::eEntities); }
  | entities_section entity_id
  | entities_section component
  ;

entity_id: id { drv.addEntity($1); };

component:
  component_name OPEN_CURLY CLOSE_CURLY
  | component_name OPEN_CURLY component_properties CLOSE_CURLY
  ;

component_name: NAME { drv.addComponent($1); };

component_properties:
  property
  | component_properties property
  ;

id:
  POUND INTEGER { $$ = $2; }
  | POUND { $$ = -1; }
  ;

property:
  property_name
  | property_name values
  ;

property_name: NAME { drv.addProperty($1); };

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
