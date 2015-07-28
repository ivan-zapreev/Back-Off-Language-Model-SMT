/* 
 * File:   StringUtils.hpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.#
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created on July 27, 2015, 2:21 PM
 */

#ifndef STRINGUTILS_HPP
#define	STRINGUTILS_HPP

#include <string>  //std::string
#include <vector>  //std::vector
#include <sstream> //std::stringstream

#include "Logger.hpp"
#include "Exceptions.hpp"
#include "Globals.hpp"

using uva::smt::logging::Logger;
using uva::smt::tries::TModelLevel;

namespace uva {
    namespace smt {
        namespace utils {
            namespace text {

                /**
                 * Tokenise a given string into avector of strings
                 * @param s the string to tokenise
                 * @param delim the delimiter
                 * @param elems the output array
                 */
                static inline void tokenize(const std::string &data, const char delim, vector<string> & elems) {
                    stringstream stream(data);
                    string token;

                    //Read from the string stream
                    while (getline(stream, token, delim)) {
                        elems.push_back(token);
                    }
                }

                /**
                 * This method build an N-Gram from a string, which is nothing more than
                 * just taking a string and tokenizing it with the given delimiter. In
                 * addition this method will test if the resulting N-gram has exactly
                 * the specified number of elements. Will also clean the ngram vector
                 * before filling it in. The order in which the N-gram elements are stored
                 * are the same in which they are present in the given line.
                 * @param line the line of code to convert into an N-gram
                 * @param n the expected value of N
                 * @param delim the delimiter to parse the string into
                 * @param ngram the output parameter that will be filled in with the N-gram values
                 * @throws Exception in case the resulting N-gram has the number elements other than expected
                 */
                static inline void buildNGram(const string & line, const TModelLevel n, const char delim, vector<string> & ngram) throw (Exception) {
                    //First clean the vector
                    ngram.clear();
                    //Tokenise the line
                    tokenize(line, delim, ngram);
                    //Check that the number of words in the N-gram is proper
                    if (ngram.size() != n) {
                        stringstream msg;
                        msg << "The line '" << line << "' is not a " << n << "-gram as expected!";
                        throw Exception(msg.str());
                    }
                }

                /**
                 * This function can be used to trim the string
                 * @param str the string to be trimmed, it is an in/out parameter
                 * @param whitespace the white spaces to be trimmed, the default value is " \t" 
                 */
                inline void trim(std::string& str,
                        const std::string& whitespace = " \t") {
                    LOG_DEBUG2 << "Trimming the string '" << str << "', with white spaces '" << whitespace << "'" << END_LOG;
                    if (str != "") {
                        const auto strBegin = str.find_first_not_of(whitespace);
                        if (strBegin == std::string::npos) {
                            str = ""; // no content
                        } else {
                            const auto strEnd = str.find_last_not_of(whitespace);
                            const auto strRange = strEnd - strBegin + 1;

                            str = str.substr(strBegin, strRange);
                        }
                    }
                    LOG_DEBUG2 << "The trimmed result is '" << str << "'" << END_LOG;
                }

                /**
                 * This is a reduce function that first will trim the string and then
                 * reduce the sub-ranges within the string.
                 * @param str the string to be reduced, is an in/out parameter
                 * @param fill the filling symbol to be used within the string instead of ranges, by default " "
                 * @param whitespace the white spaces to be reduced, by default " \t"
                 */
                inline void reduce(std::string& str,
                        const std::string& fill = " ",
                        const std::string& whitespace = " \t") {
                    LOG_DEBUG2 << "Reducing the string '" << str << "', with white spaces '" << whitespace << "'" << END_LOG;
                    if (str != "") {
                        // trim first
                        trim(str, whitespace);

                        // replace sub ranges
                        auto beginSpace = str.find_first_of(whitespace);
                        while (beginSpace != std::string::npos) {
                            const auto endSpace = str.find_first_not_of(whitespace, beginSpace);
                            const auto range = endSpace - beginSpace;

                            str.replace(beginSpace, range, fill);

                            const auto newStart = beginSpace + fill.length();
                            beginSpace = str.find_first_of(whitespace, newStart);
                        }
                    }
                    LOG_DEBUG2 << "The reduced result is '" << str << "'" << END_LOG;
                }
            }
        }
    }
}

#endif	/* STRINGUTILS_HPP */
