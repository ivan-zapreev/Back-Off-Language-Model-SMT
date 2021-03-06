/* 
 * File:   MGramCumulativeQuery.hpp
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
 * Created on November 5, 2015, 3:59 PM
 */

#ifndef MGRAMCUMULATIVEQUERY_HPP
#define	MGRAMCUMULATIVEQUERY_HPP

#include <string>       //std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "QueryMGram.hpp"
#include "TextPieceReader.hpp"
#include "MGramQuery.hpp"

#include "AWordIndex.hpp"
#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;
using namespace uva::smt::exceptions;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * Stores the query and its internal for the sake of re-usability and
             * independency from the Tries and executor. Allows to compute 
             *      \Sum_{1}^{5}log_10(Prob(w_{i}|w_{1}...w_{i-1}))
             * where log_10(Prob(w_{i}|w_{1}...w_{i-1})) is > ZERO_LOG_PROB_WEIGHT
             * 
             * Note that, here 5 is taken just as an example.
             */
            template<typename TrieType>
            class T_M_Gram_Cumulative_Query : public T_M_Gram_Query<TrieType> {
            public:
                //The word index type
                typedef typename TrieType::WordIndexType WordIndexType;
                //Define the base class type
                typedef T_M_Gram_Query<TrieType> BASE;

                //Define the maximum level constant
                static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;

                /**
                 * The basic constructor for the structure
                 * @param trie the reference to the trie object
                 */
                T_M_Gram_Cumulative_Query(TrieType & trie) : T_M_Gram_Query<TrieType>(trie) {
                }

                /**
                 * Allows to log the query results after its execution.
                 * Different logging is done based on enabled logging level
                 * and the class template parameters.
                 */
                inline void log_results() const {
                    //Initialize the current index, with the proper start value
                    TModelLevel curr_idx = BASE::m_query.m_gram.get_begin_word_idx();
                    TLogProbBackOff cumulative_prob = ZERO_PROB_WEIGHT;

                    //Print the intermediate results
                    for (; curr_idx <= BASE::m_query.m_gram.get_end_word_idx(); ++curr_idx) {
                        const string gram_str = BASE::m_query.m_gram.get_mgram_prob_str(curr_idx + 1);
                        LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                << " ) ) = " << SSTR(BASE::m_query.m_probs[curr_idx]) << END_LOG;
                        LOG_INFO << "  Prob( " << gram_str << " ) = "
                                << SSTR(pow(LOG_PROB_WEIGHT_BASE, BASE::m_query.m_probs[curr_idx])) << END_LOG;
                        if (BASE::m_query.m_probs[curr_idx] > ZERO_LOG_PROB_WEIGHT) {
                            cumulative_prob += BASE::m_query.m_probs[curr_idx];
                        }
                    }
                    LOG_RESULT << "---" << END_LOG;

                    //Print the total cumulative probability if needed
                    const string gram_str = BASE::m_query.m_gram.get_mgram_prob_str();
                    LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                            << " ) ) = " << SSTR(cumulative_prob) << END_LOG;
                    LOG_INFO << "  Prob( " << gram_str << " ) = "
                            << SSTR(pow(LOG_PROB_WEIGHT_BASE, cumulative_prob)) << END_LOG;

                    LOG_RESULT << "-------------------------------------------" << END_LOG;
                }

                /**
                 * Allows to execute m-gram the query
                 * @param text the piece containing the m-gram query
                 */
                inline void execute(TextPieceReader &text) {
                    LOG_DEBUG << "Starting to execute:" << (string) BASE::m_query.m_gram << END_LOG;

                    //Set the text piece into the m-gram
                    BASE::m_query.m_gram.set_m_gram_from_text(text);

                    //Clean the probability entries
                    memset(BASE::m_query.m_probs, 0, sizeof (TLogProbBackOff) * MAX_LEVEL);
                    //Clean the payload pointer entries
                    memset(BASE::m_query.m_payloads, 0, sizeof (void*) * MAX_LEVEL * MAX_LEVEL);

                    //If this trie needs getting context ids then clean the data as well
                    if (BASE::m_trie.is_need_getting_ctx_ids()) {
                        //Clean the payload pointer entries
                        memset(BASE::m_query.m_last_ctx_ids, WordIndexType::UNDEFINED_WORD_ID, sizeof (TLongId) * MAX_LEVEL);
                    }

                    //Execute the query
                    BASE::m_trie.template execute<true>(BASE::m_query);

                    LOG_DEBUG << "Finished executing:" << (string) BASE::m_query.m_gram << END_LOG;
                }
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, WORD_INDEX_TYPE); \
            template class T_M_Gram_Cumulative_Query<C2DHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<C2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<C2WArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<W2CArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<W2CHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<G2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<H2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>;

#define INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL(M_GRAM_LEVEL); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, BasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, CountingWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptBasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptCountWordIndex);

            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL(M_GRAM_LEVEL_MAX);
        }
    }
}

#endif	/* MGRAMCUMULATIVEQUERY_HPP */

