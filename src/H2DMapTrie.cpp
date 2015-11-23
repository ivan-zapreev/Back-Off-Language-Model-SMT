/* 
 * File:   H2DMapTrie.cpp
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
 * Created on November 23, 2015, 12:24 AM
 */

#include "H2DMapTrie.hpp"

#include <inttypes.h>   // std::uint32_t
#include <algorithm>    // std::max

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::__H2DMapTrie;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            H2DMapTrie<MAX_LEVEL, WordIndexType>::H2DMapTrie(WordIndexType & word_index)
            : GenericTrieBase<H2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __H2DMapTrie::DO_BITMAP_HASH_CACHE>(word_index),
            m_n_gram_data(NULL) {
                //Perform an error check! This container has bounds on the supported trie level
                ASSERT_CONDITION_THROW((MAX_LEVEL > M_GRAM_LEVEL_6), string("The maximum supported trie level is") + std::to_string(M_GRAM_LEVEL_6));
                ASSERT_CONDITION_THROW((word_index.is_word_index_continuous()), "This trie can not be used with a continuous word index!");

                //Initialize the array of number of gram ids per level
                memset(m_num_buckets, 0, MAX_LEVEL * sizeof (TShortId));

                //Clear the M-Gram bucket arrays
                memset(m_m_gram_data, 0, NUM_M_GRAM_LEVELS * sizeof (TProbBackOffBucket*));

                LOG_DEBUG << "sizeof(T_M_Gram_Prob_Back_Off_Entry)= " << sizeof (__H2DMapTrie::T_M_Gram_PB_Entry) << END_LOG;
                LOG_DEBUG << "sizeof(T_M_Gram_Prob_Entry)= " << sizeof (__H2DMapTrie::T_M_Gram_Prob_Entry) << END_LOG;
                LOG_DEBUG << "sizeof(TProbBackOffBucket)= " << sizeof (TProbBackOffBucket) << END_LOG;
                LOG_DEBUG << "sizeof(TProbBucket)= " << sizeof (TProbBucket) << END_LOG;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void H2DMapTrie<MAX_LEVEL, WordIndexType>::pre_allocate(const size_t counts[MAX_LEVEL]) {
                //Call the base-class
                BASE::pre_allocate(counts);

                //Default initialize the unknown word payload data
                m_unk_word_payload.m_prob = UNK_WORD_LOG_PROB_WEIGHT;
                m_unk_word_payload.m_back = ZERO_BACK_OFF_WEIGHT;

                //Compute the number of M-Gram level buckets and pre-allocate them
                for (TModelLevel idx = 0; idx < NUM_M_GRAM_LEVELS; idx++) {
                    m_num_buckets[idx] = max(counts[idx] / __H2DMapTrie::WORDS_PER_BUCKET_FACTOR,
                            __H2DMapTrie::WORDS_PER_BUCKET_FACTOR);
                    m_m_gram_data[idx] = new TProbBackOffBucket[m_num_buckets[idx]];
                }

                //Compute the number of N-Gram level buckets and pre-allocate them
                constexpr TModelLevel MAX_LEVEL_IDX = MAX_LEVEL - LEVEL_IDX_OFFSET;
                m_num_buckets[MAX_LEVEL_IDX] = max(counts[MAX_LEVEL_IDX] / __H2DMapTrie::WORDS_PER_BUCKET_FACTOR,
                        __H2DMapTrie::WORDS_PER_BUCKET_FACTOR);
                m_n_gram_data = new TProbBucket[m_num_buckets[MAX_LEVEL_IDX]];
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            H2DMapTrie<MAX_LEVEL, WordIndexType>::~H2DMapTrie() {
                //De-allocate M-Grams
                for (TModelLevel idx = 0; idx < NUM_M_GRAM_LEVELS; idx++) {
                    delete[] m_m_gram_data[idx];
                }
                //De-allocate N-Grams
                delete[] m_n_gram_data;
            };

            INSTANTIATE_TRIE_TEMPLATE_TYPE(H2DMapTrie, M_GRAM_LEVEL_MAX, BasicWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(H2DMapTrie, M_GRAM_LEVEL_MAX, CountingWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(H2DMapTrie, M_GRAM_LEVEL_MAX, HashingWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(H2DMapTrie, M_GRAM_LEVEL_MAX, TOptBasicWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(H2DMapTrie, M_GRAM_LEVEL_MAX, TOptCountWordIndex);
        }
    }
}