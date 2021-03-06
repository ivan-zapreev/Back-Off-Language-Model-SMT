/* 
 * File:   NGramBuilder.cpp
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
 * Created on April 18, 2015, 12:02 PM
 */
#include "ARPAGramBuilder.hpp"

#include <string>    // std::string
#include <vector>    // std::vector
#include <stdexcept> // std::invalid_argument
#include <algorithm> // std::min

#include "Logger.hpp"
#include "StringUtils.hpp"
#include "Exceptions.hpp"

using std::invalid_argument;
using namespace uva::smt::logging;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                template<typename WordIndexType, TModelLevel CURR_LEVEL>
                const unsigned short int ARPAGramBuilder<WordIndexType, CURR_LEVEL>::MIN_NUM_TOKENS_NGRAM_STR = 2;
                template<typename WordIndexType, TModelLevel CURR_LEVEL>
                const unsigned short int ARPAGramBuilder<WordIndexType, CURR_LEVEL>::MAX_NUM_TOKENS_NGRAM_STR = 3;

                template<typename WordIndexType, TModelLevel CURR_LEVEL>
                ARPAGramBuilder<WordIndexType, CURR_LEVEL>::ARPAGramBuilder(WordIndexType & word_index, typename TAddGramFunct<WordIndexType>::func addGarmFunc)
                : m_add_garm_func(addGarmFunc), m_token(), m_m_gram(word_index, CURR_LEVEL) {
                    LOG_DEBUG2 << "Constructing ARPANGramBuilder(" << CURR_LEVEL << ", trie)" << END_LOG;
                }

                template<typename WordIndexType, TModelLevel CURR_LEVEL>
                ARPAGramBuilder<WordIndexType, CURR_LEVEL>::ARPAGramBuilder(const ARPAGramBuilder<WordIndexType, CURR_LEVEL>& orig)
                : m_add_garm_func(orig.m_add_garm_func), m_token(), m_m_gram(orig.m_m_gram.get_word_index(), CURR_LEVEL) {
                }

                template<typename WordIndexType, TModelLevel CURR_LEVEL>
                ARPAGramBuilder<WordIndexType, CURR_LEVEL>::~ARPAGramBuilder() {
                }

                template<typename WordIndexType, TModelLevel CURR_LEVEL>
                bool ARPAGramBuilder<WordIndexType, CURR_LEVEL>::parse_to_gram(TextPieceReader &line) {
                    //Read the first element until the tab, we read until the tab because it should be the probability
                    if (line.get_first_tab(m_token)) {
                        //Try to parse it float
                        if (fast_s_to_f(m_m_gram.m_payload.m_prob, m_token.get_rest_c_str())) {
                            LOG_DEBUG2 << "Parsed the N-gram probability: " << m_m_gram.m_payload.m_prob << END_LOG;

                            //Start the new m-gram
                            m_m_gram.start_new_m_gram();

                            //Read the first N-1 tokens of the N-gram - space separated
                            for (int i = 0; i < (CURR_LEVEL - 1); i++) {
                                if (!line.get_first_space(m_m_gram.get_next_new_token())) {
                                    LOG_WARNING << "An unexpected end of line '" << line.str()
                                            << "' when reading the " << (i + 1)
                                            << "'th " << CURR_LEVEL << "-gram token!" << END_LOG;
                                    //The unexpected end of file, broken file format (?)
                                    return false;
                                }
                            }

                            //Read the last token of the N-gram, which is followed by the new line or a tab
                            if (!line.get_first_tab(m_m_gram.get_next_new_token())) {
                                LOG_WARNING << "An unexpected end of line '" << line.str()
                                        << "' when reading the " << CURR_LEVEL << "'th "
                                        << CURR_LEVEL << "-gram token!" << END_LOG;
                                //The unexpected end of file, broken file format (?)
                                return false;
                            }

                            //Now if there is something left it should be the back-off weight, otherwise we are done
                            if (line.has_more()) {
                                //Take the remainder of the line and try to parse it!
                                if (!fast_s_to_f(m_m_gram.m_payload.m_back, line.get_rest_c_str())) {
                                    LOG_WARNING << "Could not parse the remainder of the line '" << line.str()
                                            << "' as a back-off weight!" << END_LOG;
                                    //The first token was not a float, need to skip to another N-Gram section(?)
                                    return false;
                                }
                                LOG_DEBUG2 << "Parsed the N-gram back-off weight: " << m_m_gram.m_payload.m_back << END_LOG;
                            } else {
                                //There is no back-off so set it to zero
                                m_m_gram.m_payload.m_back = ZERO_BACK_OFF_WEIGHT;
                                LOG_DEBUG2 << "The parsed N-gram '" << line.str()
                                        << "' does not have back-off using: " << m_m_gram.m_payload.m_back << END_LOG;
                            }
                            return true;
                        } else {
                            //NOTE: Do it as a debug3 level and not a warning because 
                            //this will happen each time we need to move on to a new section!
                            LOG_DEBUG3 << "Could not parse the the string '" << m_token.str()
                                    << "' as a probability!" << END_LOG;
                            //The first token was not a float, need to skip to another N-Gram section(?)

                            //Take the line and convert it into a string, then trim it.
                            string line = m_token.str();
                            trim(line);
                            //Skip to the next section only if we are dealing with a non-empty line!
                            return (line == "");
                        }
                    } else {
                        LOG_DEBUG3 << "An unexpected end of line '" << line.str()
                                << "', an empty line detected!" << END_LOG;
                        //The unexpected end of line, it is an empty line so we skip it but keep reading this N-gram section
                        return true;
                    }
                }

                template<typename WordIndexType, TModelLevel CURR_LEVEL>
                bool ARPAGramBuilder<WordIndexType, CURR_LEVEL>::parse_line(TextPieceReader & line) {
                    LOG_DEBUG << "Processing the " << CURR_LEVEL << "-Gram (?) line: '" << line << "'" << END_LOG;
                    //We expect a good input, so the result is set to false by default.
                    bool result = false;

                    //First tokenize as a pattern "prob \t gram \t back-off"
                    if (parse_to_gram(line)) {
                        //Prepare the N-gram and for being added to the trie
                        m_m_gram.prepare_for_adding();

                        LOG_DEBUG << "Adding a " << SSTR(CURR_LEVEL) << "-Gram "
                                << (string) m_m_gram << " to the Trie" << END_LOG;

                        //Add the obtained N-gram data to the Trie
                        m_add_garm_func(m_m_gram);
                    } else {
                        //If we could not parse the line to gram then it should
                        //be the beginning of the next m-gram section
                        result = true;
                    }

                    LOG_DEBUG << "Finished processing the " << CURR_LEVEL << "-Gram (?) line: '"
                            << line << "', it is " << (result ? "NOT " : "") << "accepted" << END_LOG;

                    return result;
                }

                //Make sure that there will be templates instantiated, at least for the given parameter values

#define INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(LEVEL) \
                template class ARPAGramBuilder<BasicWordIndex, LEVEL>; \
                template class ARPAGramBuilder<CountingWordIndex, LEVEL>; \
                template class ARPAGramBuilder<HashingWordIndex, LEVEL>; \
                template class ARPAGramBuilder<TOptBasicWordIndex, LEVEL>; \
                template class ARPAGramBuilder<TOptCountWordIndex, LEVEL>;

                INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_1);
                INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_2);
                INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_3);
                INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_4);
                INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_5);
                INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_6);
                INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_7);
            }
        }
    }
}