/*
 * The MIT License (MIT)
 * 
 * All contributions by Krzysztof Narkiewicz (see https://github.com/ezaquarii/bison-flex-cpp-example for the original file):
 * Copyright (c) 2014 Krzysztof Narkiewicz <krzysztof.narkiewicz@ezaquarii.com>
 *
 * All other contributions:
 * Copyright (c) 2015 Lukas Huwiler <lukas.huwiler@gmx.ch>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 */


%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0"
%defines
%define parser_class_name { Parser }

%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define api.namespace { tagfilter }
%code requires
{
    #include <iostream>
    #include <string>
    #include <vector>
    #include <stdint.h>
    #include <memory>
    #include "command.h"

    using namespace std;

    namespace tagfilter {
        class Scanner;
        class Interpreter;
    }
}

// Bison calls yylex() function that must be provided by us to suck tokens
// from the scanner. This block will be placed at the beginning of IMPLEMENTATION file (cpp).
// We define this function here (function! not method).
// This function is called only inside Bison, so we make it static to limit symbol visibility for the linker
// to avoid potential linking conflicts.
%code top
{
    #include <iostream>
    #include <inttypes.h>
	  #include <cerrno>
	  #include <regex>
    #include "scanner.h"
    #include "parser.hpp"
    #include "interpreter.h"
    #include "location.hh"
    
    // yylex() arguments are defined in parser.y
    static tagfilter::Parser::symbol_type yylex(tagfilter::Scanner &scanner, tagfilter::Interpreter &driver) {
        return scanner.get_next_token();
    }

    #define YY_BUFFER_STATE yy_scan_string(const char *str)
    
    // you can accomplish the same thing by inlining the code using preprocessor
    // x and y are same as in above static function
    // #define yylex(x, y) scanner.get_next_token()
    
    using namespace tagfilter;
}

%lex-param { tagfilter::Scanner &scanner }
%lex-param { tagfilter::Interpreter &driver }
%parse-param { tagfilter::Scanner &scanner }
%parse-param { tagfilter::Interpreter &driver }
%locations
%define parse.trace
%define parse.error verbose

%define api.token.prefix {TOKEN_}

%token 			                END 		    0 	"end of file";
%token <std::string>       	STRING  		    "string";
%token 			                VAL 			      "value keyword (v)";
%token 			                KEY 			      "key keyword (k)";
%token 			                TAG 			      "tag keyword (t)";
%token                      BOUNDINGBOX     "bounding box (bb)";
%token 			                LEFTPAR 		    "left parenthesis";
%token 			                RIGHTPAR 	    	"right parenthesis";
%token 			                NOT 			      "logical NOT operator (!)";
%token 			                EQUAL 			    "comparison operator (==)";
%token                      GREATER         "greater than operator (>)";
%token                      GEQUAL          "greater than or equal operator (>=)";
%token                      LESS            "less than operator (<)";
%token                      LEQUAL          "less than or equal operator (<=)";
%token 			                CONTAINS    		"contains operator (%contains%)";
%token 			                GREPL 			    "match operator (%grepl%)";
%token 			                AND 			      "logical AND operator (&)";
%token 			                OR 			        "logical OR operator (|)";
%token 			                COMMA 			    "comma";
%token <std::string>      	ERROR			      "token error";
%token                      ID              "id keyword";
%token <int64_t>            INTEGER         "integer";
%token                      INTERROR        "integer overflow";
%token <double>             DOUBLE          "double";
%token <osmium::item_type>  ENTITYTYPE      "entity type";
%token                      HAVDIST         "haversineDistance";

%type<std::shared_ptr<tagfilter::Command>>        tagparse;
%type<std::shared_ptr<tagfilter::Command>>        atomar_tagparse;
%type<std::shared_ptr<tagfilter::Command>>        binary_connective;
%type<std::shared_ptr<tagfilter::Command>>        numeric_comparison;
%type<std::shared_ptr<tagfilter::NumericCommand>> numeric_expression;

%left NOT
%left AND
%left OR

%start tagparse 

%%

tagparse:           atomar_tagparse {
                      $$ = $1;
                    }
                    
                    | binary_connective {
                      $$ = $1;
                    }
;

numeric_expression:  DOUBLE {
                      std::shared_ptr<NumericCommand> cmd = std::make_shared<NumericIdentity>($1);
                      $$ = cmd;
                    }
                    
                    | HAVDIST LEFTPAR DOUBLE COMMA DOUBLE RIGHTPAR {
                      osmium::Location loc = osmium::Location($3, $5);
                      if(!loc.valid()) {
                        error(@$, "location with lon " + std::to_string(loc.lon_without_check()) + " and lat " + std::to_string(loc.lat_without_check()) + " is invalid");
                        YYERROR;
                      }
                      std::shared_ptr<NumericCommand> cmd = std::make_shared<HaversineDistance>(loc);
                      $$ = cmd;
                    }
;

numeric_comparison: numeric_expression GREATER numeric_expression {
                      std::shared_ptr<Command> cmd = std::make_shared<CommandGreater>($1, $3);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    | numeric_expression EQUAL numeric_expression {
                      std::shared_ptr<Command> cmd = std::make_shared<CommandEqual>($1, $3);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    } 
                    
                    | numeric_expression GEQUAL numeric_expression {
                      std::shared_ptr<Command> cmd = std::make_shared<CommandGEqual>($1, $3);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    | numeric_expression LESS numeric_expression {
                      std::shared_ptr<Command> cmd = std::make_shared<CommandLess>($1, $3);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    | numeric_expression LEQUAL numeric_expression {
                      std::shared_ptr<Command> cmd = std::make_shared<CommandLEqual>($1, $3);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    } 
;

atomar_tagparse:		LEFTPAR tagparse RIGHTPAR { 
                      driver.setCommand($2);
                      $$ = $2;
                    }	
                    
                    | numeric_comparison {
                      $$ = $1;
                    }
                    
                    | ID LEFTPAR STRING COMMA ENTITYTYPE RIGHTPAR {
                      if(!std::regex_match($3, std::regex("[-]?[0-9]+"))) {
                        error(@$, "id does not match regex pattern [-]?[0-9]+");
                        YYERROR;
                      }
                      errno = 0;
                      const char* id = $3.c_str();
                      int64_t i = strtoll(id, NULL, 10);
                      if(errno == ERANGE) {
                        error(@$, "integer overflow for id " + $3);
                        YYERROR;
                      } 
                      std::shared_ptr<Command> cmd = std::make_shared<CommandCompareId>(i, $5);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    } 
                    
                    |	NOT tagparse {
                      std::shared_ptr<Command> cmd = std::make_shared<CommandNot>($2);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    |	VAL EQUAL STRING { 
                      std::shared_ptr<Command> cmd = std::make_shared<CommandEqualValue>($3);
                      driver.setCommand(cmd);
                      $$ = cmd; 
                    }
                  
                    |	KEY EQUAL STRING { 
                      std::shared_ptr<Command> cmd = std::make_shared<CommandEqualKey>($3);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    |	VAL CONTAINS STRING {
                      std::string pattern = ".*" + $3 + ".*";
                      std::shared_ptr<Command> cmd;
                      try {
                        cmd = std::make_shared<CommandMatchesValue>(pattern);
                      } catch (exception& e) {
                        error(@$, e.what());
                        YYERROR;
                      }
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                  
                    |	KEY CONTAINS STRING {
                      std::string pattern = ".*" + $3 + ".*";
                      std::shared_ptr<Command> cmd;
                      try {
                        cmd = std::make_shared<CommandMatchesKey>(pattern);
                      } catch (exception& e) {
                        error(@$, e.what());
                        YYERROR;
                      }
                      
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    |	VAL GREPL STRING {
                      std::shared_ptr<Command> cmd;
                      try {
                        cmd = std::make_shared<CommandMatchesValue>($3);
                      } catch(exception& e) {
                        error(@$, e.what());
                        YYERROR;
                      }
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    |	KEY GREPL STRING {
                      std::shared_ptr<Command> cmd;
                      try {
                        cmd = std::make_shared<CommandMatchesKey>($3);
                      } catch(exception& e) {
                        error(@$, e.what());
                        YYERROR;
                      }
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    |	TAG LEFTPAR STRING COMMA STRING RIGHTPAR { 
                      std::shared_ptr<Command> cmd = std::make_shared<CommandIdenticalTag>($3, $5);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    | BOUNDINGBOX LEFTPAR DOUBLE COMMA DOUBLE COMMA DOUBLE COMMA DOUBLE RIGHTPAR {
                      std::shared_ptr<Command> cmd = std::make_shared<CommandBoundingBox>($3, $5, $7, $9);
                      driver.setCommand(cmd);
                      $$ = cmd;
                    }
                    
                    |	ERROR {
                      error(@$, "Unknown token '" + $1 + "'");
                      YYERROR;
                    }
;

binary_connective:	tagparse AND atomar_tagparse {
						          std::shared_ptr<Command> cmd = std::make_shared<CommandAnd>($1,$3); 
						          driver.setCommand(cmd);
						          $$ = cmd;
					          }

				            |	tagparse OR atomar_tagparse {
						          std::shared_ptr<Command> cmd = std::make_shared<CommandOr>($1,$3); 
						          driver.setCommand(cmd);
						          $$ = cmd;
					          }
;
    
%%

// Bison expects us to provide implementation - otherwise linker complains
void tagfilter::Parser::error(const location &loc , const std::string &message) {
        
        // Location should be initialized inside scanner action, but is not in this example.
        // Let's grab location directly from driver class.
    std::string err_msg = std::string("At Position ") + std::to_string(driver.location()) + ": " + message;
    driver.setError(err_msg);
}
