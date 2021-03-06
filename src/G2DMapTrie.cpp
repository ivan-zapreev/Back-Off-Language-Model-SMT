/* 
 * File:   G2DMapTrie.cpp
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
 * Created on September 8, 2015, 11:22 AM
 */

#include "G2DMapTrie.hpp"

#include <inttypes.h>   // std::uint32_t
#include <algorithm>    // std::max

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::__G2DMapTrie;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            G2DMapTrie<MAX_LEVEL, WordIndexType>::G2DMapTrie(WordIndexType & word_index)
            : GenericTrieBase<G2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __G2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR>(word_index),
            m_1_gram_data(NULL), m_n_gram_data(NULL) {
                //Perform an error check! This container has bounds on the supported trie level
                ASSERT_CONDITION_THROW((MAX_LEVEL > M_GRAM_LEVEL_6), string("The maximum supported trie level is") + std::to_string(M_GRAM_LEVEL_6));
                ASSERT_CONDITION_THROW((!word_index.is_word_index_continuous()), "This trie can not be used with a discontinuous word index!");

                //Clear the M-Gram bucket arrays
                memset(m_m_gram_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TProbBackMap*));

                LOG_DEBUG << "sizeof(T_M_Gram_PB_Entry)= " << sizeof (T_M_Gram_PB_Entry) << END_LOG;
                LOG_DEBUG << "sizeof(T_M_Gram_Prob_Entry)= " << sizeof (T_M_Gram_Prob_Entry) << END_LOG;
                LOG_DEBUG << "sizeof(TProbBackMap)= " << sizeof (TProbBackMap) << END_LOG;
                LOG_DEBUG << "sizeof(TProbMap)= " << sizeof (TProbMap) << END_LOG;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::pre_allocate(const size_t counts[MAX_LEVEL]) {
                //Call the base-class
                BASE::pre_allocate(counts);

                //Pre-allocate the 1-Gram data
                const size_t num_words = BASE::get_word_index().get_number_of_words(counts[0]);
                m_1_gram_data = new T_M_Gram_Payload[num_words];
                memset(m_1_gram_data, 0, num_words * sizeof (T_M_Gram_Payload));

                //Insert the unknown word data into the allocated array
                T_M_Gram_Payload & pb_data = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                pb_data.m_prob = UNK_WORD_LOG_PROB_WEIGHT;
                pb_data.m_back = ZERO_BACK_OFF_WEIGHT;

                //Initialize the m-gram maps
                for (TModelLevel idx = 1; idx <= BASE::NUM_M_GRAM_LEVELS; ++idx) {
                    m_m_gram_data[idx - 1] = new TProbBackMap(__G2DMapTrie::BUCKETS_FACTOR, counts[idx]);
                }

                //Initialize the n-gram's map
                m_n_gram_data = new TProbMap(__G2DMapTrie::BUCKETS_FACTOR, counts[MAX_LEVEL - 1]);
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            G2DMapTrie<MAX_LEVEL, WordIndexType>::~G2DMapTrie() {
                //Check that the one grams were allocated, if yes then the rest must have been either
                if (m_1_gram_data != NULL) {
                    //De-allocate one grams
                    delete[] m_1_gram_data;
                    //De-allocate m-gram maps
                    for (TModelLevel idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                        delete m_m_gram_data[idx];
                    }
                    //De-allocate n-grams map
                    delete m_n_gram_data;
                }
            };

            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, BasicWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, CountingWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, HashingWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, TOptBasicWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, TOptCountWordIndex);
        }
    }
}