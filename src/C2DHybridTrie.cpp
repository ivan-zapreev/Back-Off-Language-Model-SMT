/* 
 * File:   C2DHybridTrie.cpp
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
 * Created on September 1, 2015, 15:15 PM
 */
#include "C2DHybridTrie.hpp"

#include <stdexcept> //std::exception
#include <sstream>   //std::stringstream
#include <algorithm> //std::fill

#include "Logger.hpp"
#include "StringUtils.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            C2DHybridTrie<MAX_LEVEL, WordIndexType>::C2DHybridTrie(WordIndexType & word_index,
                    const float mram_mem_factor,
                    const float ngram_mem_factor)
            : LayeredTrieBase<C2DHybridTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __C2DHybridTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR>(word_index),
            m_mgram_mem_factor(mram_mem_factor),
            m_ngram_mem_factor(ngram_mem_factor),
            m_1_gram_data(NULL) {

                //Perform an error check! This container has bounds on the supported trie level
                ASSERT_CONDITION_THROW((MAX_LEVEL < M_GRAM_LEVEL_2), string("The minimum supported trie level is") + std::to_string(M_GRAM_LEVEL_2));
                ASSERT_CONDITION_THROW((!word_index.is_word_index_continuous()), "This trie can not be used with a discontinuous word index!");

                //Memset the M grams reference and data arrays
                memset(m_m_gram_alloc_ptrs, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TMGramAllocator *));
                memset(m_m_gram_map_ptrs, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TMGramsMap *));
                memset(m_m_gram_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (T_M_Gram_Payload *));

                //Initialize the array of counters
                memset(m_M_gram_num_ctx_ids, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TShortId));
                memset(m_M_gram_next_ctx_id, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TShortId));

                //Initialize the N-gram level data
                m_n_gram_alloc_ptr = NULL;
                m_n_gram_map_ptr = NULL;
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void C2DHybridTrie<MAX_LEVEL, WordIndexType>::preAllocateOGrams(const size_t counts[MAX_LEVEL]) {
                //Compute the number of words to be stored
                const size_t num_word_ids = BASE::get_word_index().get_number_of_words(counts[0]);

                //Pre-allocate the 1-Gram data
                m_1_gram_data = new T_M_Gram_Payload[num_word_ids];
                memset(m_1_gram_data, 0, num_word_ids * sizeof (T_M_Gram_Payload));


                //Record the dummy probability and back-off values for the unknown word
                T_M_Gram_Payload & pbData = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                pbData.m_prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.m_back = ZERO_BACK_OFF_WEIGHT;
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void C2DHybridTrie<MAX_LEVEL, WordIndexType>::preAllocateMGrams(const size_t counts[MAX_LEVEL]) {
                //Pre-allocate for the M-grams with 1 < M < N
                for (int idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                    //Get the number of elements to pre-allocate

                    //Get the number of the M-grams on this level
                    const uint num_grams = counts[idx + 1];

                    //Reserve the memory for the map
                    reserve_mem_unordered_map<TMGramsMap, TMGramAllocator>(&m_m_gram_map_ptrs[idx], &m_m_gram_alloc_ptrs[idx], num_grams, "M-Grams", m_mgram_mem_factor);

                    //Get the number of M-gram indexes on this level
                    const uint num_ngram_idx = m_M_gram_num_ctx_ids[idx];

                    m_m_gram_data[idx] = new T_M_Gram_Payload[num_ngram_idx];
                    memset(m_m_gram_data[idx], 0, num_ngram_idx * sizeof (T_M_Gram_Payload));
                }
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void C2DHybridTrie<MAX_LEVEL, WordIndexType>::preAllocateNGrams(const size_t counts[MAX_LEVEL]) {
                //Get the number of elements to pre-allocate

                const size_t numEntries = counts[MAX_LEVEL - 1];

                //Reserve the memory for the map
                reserve_mem_unordered_map<TNGramsMap, TNGramAllocator>(&m_n_gram_map_ptr, &m_n_gram_alloc_ptr, numEntries, "N-Grams", m_ngram_mem_factor);
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void C2DHybridTrie<MAX_LEVEL, WordIndexType>::pre_allocate(const size_t counts[MAX_LEVEL]) {
                //Call the super class pre-allocator!
                BASE::pre_allocate(counts);

                //Compute and store the M-gram level sizes in terms of the number of M-gram indexes per level
                //Also initialize the M-gram index counters, for issuing context indexes
                for (TModelLevel i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                    //The index counts must start with one as zero is reserved for the UNDEFINED_ARR_IDX
                    m_M_gram_next_ctx_id[i] = BASE::FIRST_VALID_CTX_ID;
                    //Due to the reserved first index, make the array sizes one element larger, to avoid extra computations
                    m_M_gram_num_ctx_ids[i] = counts[i + 1] + BASE::FIRST_VALID_CTX_ID;
                }

                //Pre-allocate 0-Grams
                preAllocateOGrams(counts);

                //Pre-allocate M-Grams
                preAllocateMGrams(counts);

                //Pre-allocate N-Grams
                preAllocateNGrams(counts);
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            C2DHybridTrie<MAX_LEVEL, WordIndexType>::~C2DHybridTrie() {
                //Deallocate One-Grams
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                }

                //Deallocate M-Grams there are N-2 M-gram levels in the array
                for (int idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                    deallocate_container<TMGramsMap, TMGramAllocator>(&m_m_gram_map_ptrs[idx], &m_m_gram_alloc_ptrs[idx]);
                    delete[] m_m_gram_data[idx];
                }

                //Deallocate N-Grams
                deallocate_container<TNGramsMap, TNGramAllocator>(&m_n_gram_map_ptr, &m_n_gram_alloc_ptr);
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, BasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, CountingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, HashingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, TOptBasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, TOptCountWordIndex);
        }
    }
}