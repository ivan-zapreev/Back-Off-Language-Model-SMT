/* 
 * File:   W2CArrayTrie.hpp
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
 * Created on August 27, 2015, 08:33 AM
 */

#ifndef W2CORDEREDARRAYTRIE_HPP
#define	W2CORDEREDARRAYTRIE_HPP

#include <string>       // std::string
#include <cstdlib>      // std::calloc std::realloc std::free
#include <cmath>        // std::log std::log10
#include <algorithm>    // std::max

#include "Globals.hpp"
#include "Logger.hpp"

#include "LayeredTrieBase.hpp"

#include "AWordIndex.hpp"
#include "HashingWordIndex.hpp"
#include "ArrayUtils.hpp"
#include "DynamicMemoryArrays.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::utils::array;
using namespace uva::smt::tries::alloc;
using namespace uva::smt::tries::__W2CArrayTrie;

namespace uva {
    namespace smt {
        namespace tries {
            namespace __W2CArrayTrie {

                /**
                 * This template structure is used for storing trie element data
                 * Each element contains a context id of the m-gram and its payload -
                 * the probability/back-off data, the latter is the template parameter
                 */
                template<typename PAYLOAD_TYPE>
                struct S_M_GramData {
                    TShortId id;
                    PAYLOAD_TYPE payload;

                    //Stores the memory increase strategy object
                    const static MemIncreaseStrategy m_mem_strat;
                };

                template<typename PAYLOAD_TYPE>
                const MemIncreaseStrategy S_M_GramData<PAYLOAD_TYPE>::m_mem_strat =
                get_mem_incr_strat(__W2CArrayTrie::MEM_INC_TYPE,
                        __W2CArrayTrie::MIN_MEM_INC_NUM, __W2CArrayTrie::MEM_INC_FACTOR);

                typedef S_M_GramData<T_M_Gram_Payload> T_M_GramData;
                typedef S_M_GramData<TLogProbBackOff> T_N_GramData;

                /**
                 * This is the less operator implementation
                 * @param one the first object to compare
                 * @param two the second object to compare
                 * @return true if ctx_id of one is smaller than ctx_id of two, otherwise false
                 */
                inline bool operator<(const T_M_GramData& one, const T_M_GramData& two) {
                    return one.id < two.id;
                }

                /**
                 * This is the less operator implementation
                 * @param one the first object to compare
                 * @param two the second object to compare
                 * @return true if ctx_id of one is smaller than ctx_id of two, otherwise false
                 */
                inline bool operator<(const T_N_GramData& one, const T_N_GramData& two) {
                    return one.id < two.id;
                }
            }

            /**
             * This is the Context to word array memory trie implementation class.
             * 
             * @param MAX_LEVEL the maximum number of levels in the trie.
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class W2CArrayTrie : public LayeredTrieBase<W2CArrayTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __W2CArrayTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> {
            public:
                typedef LayeredTrieBase<W2CArrayTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __W2CArrayTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> BASE;

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit W2CArrayTrie(WordIndexType & word_index);

                /**
                 * Computes the M-Gram context using the previous context and the current word id
                 * @see LayeredTrieBese
                 */
                inline bool get_ctx_id(const TModelLevel level_idx, const TShortId word_id, TLongId & ctx_id) const {
                    //Compute back the current level pure for debug purposes.
                    const TModelLevel curr_level = level_idx + BASE::MGRAM_IDX_OFFSET;

                    ASSERT_SANITY_THROW((curr_level == MAX_LEVEL) || (level_idx < 0),
                            string("Unsupported level id: ") + std::to_string(curr_level));

                    LOG_DEBUG2 << "Searching next ctx_id for " << SSTR(curr_level)
                            << "-gram with word_id: " << SSTR(word_id) << ", ctx_id: "
                            << SSTR(ctx_id) << END_LOG;

                    //First get the sub-array reference. 
                    const T_M_GramWordEntry & ref = m_m_gram_word_2_data[level_idx][word_id];
                    
                    if (DO_SANITY_CHECKS && ref.has_data()) {
                        LOG_DEBUG3 << "ref.size: " << SSTR(ref.size()) << ", ref.cio: "
                                << SSTR(ref.cio) << ", ctx_id range: [" << SSTR(ref[0].id) << ", "
                                << SSTR(ref[ref.size() - 1].id) << "]" << END_LOG;
                    }
 
                    //Check that if this is the 2-Gram case and the previous context
                    //id is 0 then it is the unknown word id, at least this is how it
                    //is now in ATrie implementation, so we need to do a warning!
                    if (DO_SANITY_CHECKS && (curr_level == M_GRAM_LEVEL_2) && (ctx_id < WordIndexType::MIN_KNOWN_WORD_ID)) {
                        LOG_WARNING << "Perhaps we are being paranoid but there "
                                << "seems to be a problem! The " << SSTR(curr_level) << "-gram ctx_id: "
                                << SSTR(ctx_id) << " is equal to an undefined(" << SSTR(WordIndexType::UNDEFINED_WORD_ID)
                                << ") or unknown(" << SSTR(WordIndexType::UNKNOWN_WORD_ID) << ") word ids!" << END_LOG;
                    }

                    //Get the local entry index and then use it to compute the next context id
                    typename T_M_GramWordEntry::TIndexType local_idx;
                    //If the local entry index could be found then compute the next ctx_id
                    if (get_m_n_gram_local_entry_idx(ref, ctx_id, local_idx)) {
                        LOG_DEBUG2 << "Got context mapping for ctx_id: " << SSTR(ctx_id)
                                << ", size = " << SSTR(ref.size()) << ", localIdx = "
                                << SSTR(local_idx) << ", resulting ctx_id = "
                                << SSTR(ref.cio + local_idx) << END_LOG;

                        //The next ctx_id is the sum of the local index and the context index offset
                        ctx_id = ref.cio + local_idx;
                        return true;
                    } else {
                        //The local index could not be found
                        return false;
                    }
                }

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() const {
                    LOG_USAGE << "Using the <" << __FILE__ << "> model." << END_LOG;
                    LOG_INFO << "Using the " << T_M_GramData::m_mem_strat.get_strategy_info()
                            << "' memory allocation strategy." << END_LOG;
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see LayeredTrieBase
                 */
                virtual void pre_allocate(const size_t counts[MAX_LEVEL]);

                /**
                 * This method allows to check if post processing should be called after
                 * all the X level grams are read. This method is virtual.
                 * For more details @see WordIndexTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                bool is_post_grams() const {
                    //Check the base class and we need to do post actions
                    //for all the M-grams with 1 < M <= N. The M-grams level
                    //data has to be ordered per word by context id, see
                    //post_M_Grams, and post_N_Grams methods below.

                    return (CURR_LEVEL > M_GRAM_LEVEL_1) || BASE::template is_post_grams<CURR_LEVEL>();
                }

                /**
                 * This method should be called after all the X level grams are read.
                 * For more details @see WordIndexTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void post_grams() {
                    //Call the base class method first
                    if (BASE::template is_post_grams<CURR_LEVEL>()) {
                        BASE::template post_grams<CURR_LEVEL>();
                    }

                    //Do the post actions here
                    if (CURR_LEVEL == MAX_LEVEL) {
                        post_n_grams();
                    } else {
                        if (CURR_LEVEL > M_GRAM_LEVEL_1) {
                            post_m_grams<CURR_LEVEL>();
                        }
                    }
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see LayeredTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    const TShortId word_id = gram.get_end_word_id();
                    if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                        //Store the payload
                        m_1_gram_data[word_id] = gram.m_payload;
                    } else {
                        //Register the m-gram in the hash cache
                        this->register_m_gram_cache(gram);

                        //Define the context id variable
                        TLongId ctx_id = WordIndexType::UNKNOWN_WORD_ID;
                        //Obtain the m-gram context id
                        __LayeredTrieBase::get_context_id<W2CArrayTrie<MAX_LEVEL, WordIndexType>, CURR_LEVEL, DebugLevelsEnum::DEBUG2>(*this, gram, ctx_id);

                        //Store the payload
                        if (CURR_LEVEL == MAX_LEVEL) {
                            //Get the sub-array reference. 
                            typename T_N_GramWordEntry::TElemType & ref = make_m_n_gram_entry<T_N_GramWordEntry>(m_n_gram_word_2_data, word_id);
                            //Store the context and word ids
                            ref.id = ctx_id;
                            //Return the reference to the probability
                            ref.payload = gram.m_payload.m_prob;
                        } else {
                            //Get the sub-array reference. 
                            typename T_M_GramWordEntry::TElemType & ref = make_m_n_gram_entry<T_M_GramWordEntry>(m_m_gram_word_2_data[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET], word_id);
                            //Store the context and word ids
                            ref.id = ctx_id;
                            //Return the reference to the newly allocated element
                            ref.payload = gram.m_payload;
                        }
                    }
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==1.
                 * The retrieval of a uni-gram data is always a success
                 * @see GenericTrieBase
                 */
                inline void get_unigram_payload(typename BASE::T_Query_Exec_Data & query) const {
                    //Get the word index for convenience
                    const TModelLevel & word_idx = query.m_begin_word_idx;

                    LOG_DEBUG << "Getting the payload for sub-uni-gram : [" << SSTR(word_idx)
                            << "," << SSTR(word_idx) << "]" << END_LOG;

                    //The data is always present.
                    query.m_payloads[word_idx][word_idx] = &m_1_gram_data[query.m_gram[word_idx]];
                };

                /**
                 * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                 * For more details @see LayeredTrieBase
                 */
                inline void get_m_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    LOG_DEBUG << "Getting the payload for sub-m-gram : [" << SSTR(query.m_begin_word_idx)
                            << ", " << SSTR(query.m_end_word_idx) << "]" << END_LOG;

                    //First ensure the context of the given sub-m-gram
                    LAYERED_BASE_ENSURE_CONTEXT(query, status);

                    //If the context is successfully ensured, then move on to the m-gram and try to obtain its payload
                    if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                        //Store the shorthand for the context and end word id
                        TLongId & ctx_id = query.m_last_ctx_ids[query.m_begin_word_idx];
                        const TShortId & word_id = query.m_gram[query.m_end_word_idx];

                        LOG_DEBUG2 << "Getting m-gram with word_id: " << SSTR(word_id)
                                << ", ctx_id: " << SSTR(ctx_id) << END_LOG;

                        //Get the entry
                        const typename T_M_GramWordEntry::TElemType * entry_ptr;
                        const TModelLevel level_idx = (query.m_end_word_idx - query.m_begin_word_idx) + 1 - BASE::MGRAM_IDX_OFFSET;
                        const T_M_GramWordEntry * ptr = m_m_gram_word_2_data[level_idx];
                        if (get_m_n_gram_entry<T_M_GramWordEntry>(ptr, word_id, ctx_id, &entry_ptr)) {
                            //Return the data
                            query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &entry_ptr->payload;
                            LOG_DEBUG << "The payload is retrieved: " << (string) entry_ptr->payload << END_LOG;
                        } else {
                            //The payload could not be found
                            LOG_DEBUG1 << "Unable to find m-gram data for ctx_id: " << SSTR(ctx_id)
                                    << ", word_id: " << SSTR(word_id) << END_LOG;
                            status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                        }
                    }
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==n
                 * @see GenericTrieBase
                 */
                inline void get_n_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    //First ensure the context of the given sub-m-gram
                    LAYERED_BASE_ENSURE_CONTEXT(query, status);

                    //If the context is successfully ensured, then move on to the m-gram and try to obtain its payload
                    if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                        //Store the shorthand for the context and end word id
                        TLongId & ctx_id = query.m_last_ctx_ids[query.m_begin_word_idx];
                        const TShortId & word_id = query.m_gram[query.m_end_word_idx];

                        LOG_DEBUG2 << "Getting " << SSTR(MAX_LEVEL) << "-gram with word_id: "
                                << SSTR(word_id) << ", ctx_id: " << SSTR(ctx_id) << END_LOG;

                        //Get the entry
                        const typename T_N_GramWordEntry::TElemType * entry_ptr;
                        if (get_m_n_gram_entry<T_N_GramWordEntry>(m_n_gram_word_2_data, word_id, ctx_id, &entry_ptr)) {
                            //Return the data
                            query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &entry_ptr->payload;
                            LOG_DEBUG << "The payload is retrieved: " << entry_ptr->payload << END_LOG;
                        } else {
                            //The payload could not be found
                            LOG_DEBUG1 << "Unable to find " << SSTR(MAX_LEVEL) << "-gram data for ctx_id: "
                                    << SSTR(ctx_id) << ", word_id: " << SSTR(word_id) << END_LOG;
                            status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                        }
                    }
                }

                /**
                 * The basic destructor
                 */
                virtual ~W2CArrayTrie();

            protected:

                /**
                 * This class is to store the word mapping to the data for
                 * the 1< M <= N grams. Demending on whether M == N or not this
                 * structure is to be instantiated with a different template
                 * parameter - defines the stored data.
                 * @param ptr the pointer to the storage array
                 * @param capacity the number of allocated elements
                 * @param size the number of used elements
                 * @param cio the context index offset for computing the next contex index.
                 */
                template<typename ARRAY_ELEM_TYPE>
                class WordDataEntry : public DynamicStackArray<ARRAY_ELEM_TYPE, uint32_t> {
                public:
                    TShortId cio;
                };

                //The entries for the M-grams to store information about the end words
                typedef WordDataEntry<T_M_GramData> T_M_GramWordEntry;
                //The entries for the N-grams to store information about the end words
                typedef WordDataEntry<T_N_GramData> T_N_GramWordEntry;

                /**
                 * The purpose of this local function is three fold:
                 * 1. First we compute the context index offset values.
                 * 2. Second we re-order the context data arrays per word.
                 * 3. Free the unneeded memory allocated earlier.
                 * @param WORD_ENTRY_TYPE word array element type
                 * @param wordsArray the word array to work with
                 */
                template<typename WORD_ENTRY_TYPE>
                void post_M_N_Grams(WORD_ENTRY_TYPE * wordsArray) {
                    //Define and initialize the current context index offset.
                    //The initial value is 1, although in this Trie it should
                    //not matter much, but it is better to reserve 0 for
                    //an undefined context value
                    TShortId cio = BASE::FIRST_VALID_CTX_ID;

                    //Iterate through all the word_id sub-array mappings in the level and sort sub arrays
                    for (TShortId word_id = WordIndexType::UNDEFINED_WORD_ID; word_id < m_num_word_ids; word_id++) {
                        //First get the sub-array reference. 
                        WORD_ENTRY_TYPE & ref = wordsArray[word_id];

                        //Assign the context index offset
                        ref.cio = cio;
                        //Compute the next context index offset, for the next word
                        cio += ref.size();

                        //Reduce capacity if there is unused memory
                        ref.shrink();

                        //Order the N-gram array as it is unordered and we will binary search it later!
                        ref.sort();
                    }
                }

                template<TModelLevel level>
                inline void post_m_grams() {
                    //Sort the level's data
                    post_M_N_Grams<T_M_GramWordEntry>(m_m_gram_word_2_data[level - BASE::MGRAM_IDX_OFFSET]);
                }

                inline void post_n_grams() {
                    //Sort the level's data
                    post_M_N_Grams<T_N_GramWordEntry>(m_n_gram_word_2_data);
                };

            private:

                //Stores the number of used word ids
                TShortId m_num_word_ids;

                //Stores the 1-gram data
                T_M_Gram_Payload * m_1_gram_data;

                //Stores the M-gram word to data mappings for: 1 < M < N
                //This is a two dimensional array
                T_M_GramWordEntry * m_m_gram_word_2_data[BASE::NUM_M_GRAM_LEVELS];

                //Stores the M-gram word to data mappings for: 1 < M < N
                //This is a one dimensional array
                T_N_GramWordEntry * m_n_gram_word_2_data;

                /**
                 * For a M-gram allows to create a new context entry for the given word id.
                 * This method words for 1 < M <= N.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordsArray the array where the word entries of this level are stored
                 * @param word_id the word id we need the new context entry from
                 */
                template<typename WORD_ENTRY_TYPE>
                static inline typename WORD_ENTRY_TYPE::TElemType & make_m_n_gram_entry(WORD_ENTRY_TYPE* wordsArray, const TShortId & word_id) {
                    LOG_DEBUG2 << "Making entry for M-gram with word_id:\t" << SSTR(word_id) << END_LOG;

                    //Return the next new element new/free!
                    return wordsArray[word_id].allocate();
                };

                /**
                 * For the given M-gram defined by the word id and a context id it allows to retrieve local index where the m-gram's entry is stored.
                 * This method words for 1 < M <= N.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordEntry the word entry to search in
                 * @param ctx_id the context id we are after
                 * @param localIdx the [out] parameter which is the found local array index
                 * @return true if the index was found otherwise false
                 * @throw nothing
                 */
                template<typename WORD_ENTRY_TYPE>
                bool get_m_n_gram_local_entry_idx(const WORD_ENTRY_TYPE & ref, const TLongId ctx_id, typename WORD_ENTRY_TYPE::TIndexType & local_idx) const {
                    LOG_DEBUG2 << "Searching word data entry for ctx_id: " << SSTR(ctx_id) << END_LOG;

                    //Check if there is data to search in
                    if (ref.has_data()) {
                        //The data is available search for the word index in the array
                        //WQRNING: Switching to linear search here significantly worsens
                        //the performance!
                        if (my_bsearch_id<typename WORD_ENTRY_TYPE::TElemType, typename WORD_ENTRY_TYPE::TIndexType > (ref.data(), 0, ref.size() - 1, ctx_id, local_idx)) {
                            LOG_DEBUG2 << "Found sub array local index = " << SSTR(local_idx) << END_LOG;
                            return true;
                        } else {
                            LOG_DEBUG1 << "Unable to find M-gram context id for a word, prev ctx_id: "
                                    << SSTR(ctx_id) << ", ctx_id range: [" << SSTR(ref[0].id)
                                    << ", " << SSTR(ref[ref.size() - 1].id) << "]" << END_LOG;
                            return false;
                        }
                    } else {
                        LOG_DEBUG1 << "Unable to find M-gram word id data for a word, nothing is present!" << END_LOG;

                        return false;
                    }
                }

                /**
                 * For the given M-gram defined by the word id and a context id it allows to retrieve the data entry.
                 * This method words for 1 < M <= N. The level parameter is only used for debugging.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordsArray the array where the word entries of this level are stored
                 * @param word_id the word id we need the to find the context entry by
                 * @param ctx_id the context id we are after
                 * @return true if the data was found, otherwise false
                 * @throw nothing
                 */
                template<typename WORD_ENTRY_TYPE>
                inline bool get_m_n_gram_entry(const WORD_ENTRY_TYPE* wordsArray,
                        const TShortId word_id, TLongId & ctx_id,
                        const typename WORD_ENTRY_TYPE::TElemType **ppData) const {
                    LOG_DEBUG2 << "Getting sub arr data for an m/n-gram with word_id: " << SSTR(word_id) << END_LOG;
                    //Get the sub-array reference. 
                    const WORD_ENTRY_TYPE & ref = wordsArray[word_id];

                    //Get the local entry index
                    typename WORD_ENTRY_TYPE::TIndexType local_idx;
                    if (get_m_n_gram_local_entry_idx<WORD_ENTRY_TYPE>(ref, ctx_id, local_idx)) {
                        //Return the pointer to the data located by the local index
                        *ppData = &ref[local_idx];

                        //The next ctx_id is the sum of the local index and the context index offset
                        ctx_id = ref.cio + local_idx;

                        return true;
                    } else {
                        LOG_DEBUG2 << "Unable to find data entry for an m/n-gram ctx_id: "
                                << SSTR(ctx_id) << ", word_id: " << SSTR(word_id) << END_LOG;

                        return false;
                    }
                }

                /**
                 * Allows to allocate word related data per word for the given M/N gram level
                 * Depends on the global __W2COrderedArrayTrie::PRE_ALLOCATE_MEMORY if true
                 * then will preallocate some memory for each word bucket! Default is false.
                 * Depending on the number of words and n-grams in the given Trie level,
                 * plus the value of:
                 *      __W2COrderedArrayTrie::INIT_MEM_ALLOC_PRCT
                 * This can have a drastic influence on MAX RSS.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordsArray the reference to the word entry array to initialize
                 * @param numMGrams the number of M-grams on this level
                 * @param numWords the number of words in the Trie
                 */
                template<typename WORD_ENTRY_TYPE>
                void preAllocateWordsData(WORD_ENTRY_TYPE* & wordsArray, const TShortId numMGrams, const TShortId numWords) {
                    //Allocate dynamic array entries and initialize it with the memory increase strategy
                    wordsArray = new WORD_ENTRY_TYPE[m_num_word_ids];

                    //Compute the initial capacity
                    size_t capacity = 0;
                    if (__W2CArrayTrie::PRE_ALLOCATE_MEMORY) {
                        //Compute the average number of data elements per word on the given M-gram level
                        const float avg_num_elems = ((float) numMGrams) / ((float) numWords);
                        //Compute the corrected number of elements to preallocate, minimum __W2COrderedArrayTrie::MIN_MEM_INC_NUM.
                        capacity = max(static_cast<size_t> (avg_num_elems * __W2CArrayTrie::INIT_MEM_ALLOC_PRCT),
                                __W2CArrayTrie::MIN_MEM_INC_NUM);

                        //Pre-allocate capacity
                        for (TShortId word_id = WordIndexType::MIN_KNOWN_WORD_ID; word_id < m_num_word_ids; word_id++) {
                            wordsArray[word_id].pre_allocate(capacity);
                        }
                    }
                }
            };

            typedef W2CArrayTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > TW2CArrayTrieBasic;
            typedef W2CArrayTrie<M_GRAM_LEVEL_MAX, CountingWordIndex > TW2CArrayTrieCount;
            typedef W2CArrayTrie<M_GRAM_LEVEL_MAX, TOptBasicWordIndex > TW2CArrayTrieOptBasic;
            typedef W2CArrayTrie<M_GRAM_LEVEL_MAX, TOptCountWordIndex > TW2CArrayTrieOptCount;
            typedef W2CArrayTrie<M_GRAM_LEVEL_MAX, HashingWordIndex > TW2CArrayTrieHashing;
        }
    }
}


#endif	/* CONTEXTTOWORDHYBRIDMEMORYTRIE_HPP */

