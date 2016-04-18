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


#include "interpreter.h"
#include "command.h"

#include <sstream>
#include <cstring>
#include <boost/concept_check.hpp>

using namespace tagfilter;

Interpreter::Interpreter() :
    m_scanner(*this),
    m_parser(m_scanner, *this),
    m_location(0)
{

}

int Interpreter::parse(std::string expr) {
    m_location = 0;
    char *input = new char[expr.length() + 1];
    std::strcpy(input, expr.c_str());
    membuf sbuf(input, input + (sizeof(char) * (expr.length() + 1)));
    std::istream in(&sbuf);
    switchInputStream(&in);
    int failure = m_parser.parse();
    delete[] input;
    return failure;
}

void Interpreter::clear() {
    m_location = 0;
    m_command = nullptr;
}

std::shared_ptr<Command> Interpreter::returnAST() {
	//std::cout << m_commands.size() << " on the stack. Returning..." << std::endl;
	return m_command;
}

//std::string Interpreter::str() const {
//    std::stringstream s;
//    s << "Interpreter: " << m_commands.size() << " commands received from command line." << endl;
//    //for(int i = 0; i < m_commands.size(); i++) {
//    //    s << " * " << m_commands[i].str() << endl;
//    //}
//    return s.str();
//}

void Interpreter::switchInputStream(std::istream *is) {
    m_scanner.switch_streams(is, NULL);
    m_command = nullptr;
}

//void Interpreter::pushCommand(std::shared_ptr<Command> cmd)
//{
//    m_commands.push(cmd);
//}
//
//std::shared_ptr<Command> Interpreter::popCommand() 
//{
//	std::shared_ptr<Command> ret = m_commands.top();
//	m_commands.pop();
//	return ret;
//}

void Interpreter::increaseLocation(unsigned int loc) {
    m_location += loc;
}

unsigned int Interpreter::location() const {
    return m_location;
}
