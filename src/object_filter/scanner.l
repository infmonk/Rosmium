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

%{
	#include <iostream>
	#include <cstdlib>
	#include <osmium/osm/item_type.hpp>
	#include <stdlib.h>
	#include "scanner.h"
	#include "interpreter.h"
	#include "parser.hpp"
	#include "location.hh"

	using namespace std;

	// Original yyterminate() macro returns int. Since we're using Bison 3 variants
	// as tokens, we must redefine it to change type from `int` to `Parser::semantic_type`
	#define yyterminate() tagfilter::Parser::make_END(tagfilter::location());

	// This will track current scanner location.
	// Action is called when length of the token is known.
	#define YY_USER_ACTION m_driver.increaseLocation(yyleng);
	
	// !!!WARNING!!!
	// Location API is used, but the location is not initialized, 'cause I'm lazy. When making
	// a token with make_{something} method you can pass detailed token location. Current location
	// is accessible with m_driver.location() method. All puzzle elements are there - just
	// pass location value in every action code block below. I'm going to waste more time writing
	// this excuse than putting this boilerplate below...
	//
	// Location class can be found in location.hh and posistion.hh files. It's just a bit too much
	// boilerplate for this small example. Bummer.
%}

%option nodefault
%option noyywrap
%option c++
%option yyclass="Scanner"
%option prefix="tagfilter_"

%%


\"[^"]*\"   	        { 
                        std::string content(yytext);
                        content.erase(content.end() - 1);
                        content.erase(content.begin());
                        return tagfilter::Parser::make_STRING(content, tagfilter::location( /* put location data here if you want */ )); 
            	        }
        
\'[^']*\'   	        { 
                        std::string content(yytext);
                        content.erase(content.end() - 1);
                        content.erase(content.begin());
                        return tagfilter::Parser::make_STRING(content, tagfilter::location( /* put location data here if you want */ )); 
            	        }
            	
[+-]?[0-9]+([.][0-9]+)?([eE][+-][0-9]+)? {
                          return tagfilter::Parser::make_DOUBLE(atof(yytext), tagfilter::location());
                       }
            	
EntityBits[.]node     {
                        return tagfilter::Parser::make_ENTITYTYPE(osmium::item_type::node, tagfilter::location());
                      }
                      
EntityBits[.]way      {
                        return tagfilter::Parser::make_ENTITYTYPE(osmium::item_type::way, tagfilter::location());
                      }
                      
EntityBits[.]relation {
                        return tagfilter::Parser::make_ENTITYTYPE(osmium::item_type::relation, tagfilter::location());
                      }

haversineDistance     {
                        return tagfilter::Parser::make_HAVDIST(tagfilter::location());
                      }
              
id                    {
                        return tagfilter::Parser::make_ID(tagfilter::location());
                      }

v|value	    	        {	
	    	                return tagfilter::Parser::make_VAL(tagfilter::location()); 
	    	              }

k|key	    	          {
	    	                return tagfilter::Parser::make_KEY(tagfilter::location());
	    	              }

t|tag	    	          {
	    	                return tagfilter::Parser::make_TAG(tagfilter::location());
	    	              }
	    	              
bb|boundingBox        {
                        return tagfilter::Parser::make_BOUNDINGBOX(tagfilter::location());
                      }

\(          	        {
            	          return tagfilter::Parser::make_LEFTPAR(tagfilter::location());
            	        }
            	
\)          	        { 
            	          return tagfilter::Parser::make_RIGHTPAR(tagfilter::location());
            	        }

"!"		                {
		                    return tagfilter::Parser::make_NOT(tagfilter::location());
		                  }

"=="	    	          {
	    	                return tagfilter::Parser::make_EQUAL(tagfilter::location());
	    	              }
	    	          
">="                  {
                        return tagfilter::Parser::make_GEQUAL(tagfilter::location());
                      }
	    	              
">"                   {
                        return tagfilter::Parser::make_GREATER(tagfilter::location());
                      }
                      
"<="                  {
                        return tagfilter::Parser::make_LEQUAL(tagfilter::location()); 
                      }
                      
"<"                   {
                        return tagfilter::Parser::make_LESS(tagfilter::location());
                      }

"%contains%"	        {
	    	                return tagfilter::Parser::make_CONTAINS(tagfilter::location());
	    	              }

"%grepl%"   	        {
	    	                return tagfilter::Parser::make_GREPL(tagfilter::location());
	    	              }

[&]{1,2}    	        {
	    	                return tagfilter::Parser::make_AND(tagfilter::location());
	    	              }

[|]{1,2}    	        {
	    	                return tagfilter::Parser::make_OR(tagfilter::location());
	    	              }
            	
,           	        {
                        return tagfilter::Parser::make_COMMA(tagfilter::location());
            	        }
            	
[\0\n\t ]           	{
            	        }

[A-z]+		            {
			                  return tagfilter::Parser::make_ERROR(yytext, tagfilter::location());		
		                  }

.           	        { 
			                  return tagfilter::Parser::make_ERROR(yytext, tagfilter::location());
            	        }
            	
<<EOF>>     	        { 
                        return yyterminate(); 
                      }


%%
