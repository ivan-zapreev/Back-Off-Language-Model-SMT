/* 
 * File:   ATrie.hpp
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
 * Created on September 3, 2015, 2:59 PM
 */

#ifndef GENERIC_TRIE_DRIVER_HPP
#define	GENERIC_TRIE_DRIVER_HPP

#include <string>       // std::string
#include <functional>   // std::function

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "MathUtils.hpp"

#include "MGrams.hpp"
#include "ModelMGram.hpp"
#include "TextPieceReader.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "GenericTrieBase.hpp"

#include "BitmapHashCache.hpp"

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"
#include "LayeredTrieDriver.hpp"
#include "G2DHashMapTrie.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::caching;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the common generic trie base class.
             * @param N the maximum level of the considered N-gram, i.e. the N value
             * @param TrieType the type of word index to be used
             */
            template<typename TrieType>
            class GenericTrieDriver : public GenericTrieBase<TrieType::MAX_LEVEL, typename TrieType::WordIndexType> {
            public:
                static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;
                typedef typename TrieType::WordIndexType WordIndexType;
                typedef GenericTrieBase<MAX_LEVEL, WordIndexType> BASE;

                //The typedef for the retrieving function
                typedef function<void(const GenericTrieDriver&, const T_M_Gram<WordIndexType> & gram, TLogProbBackOff & total_prob) > TRetrieveDataFunct;

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit GenericTrieDriver(WordIndexType & word_index)
                : GenericTrieBase<MAX_LEVEL, WordIndexType> (word_index),
                m_trie(word_index) {
                }

                /**
                 * @see GenericTrieBase
                 */
                void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    //Pre-allocate the bitmap-hash caches if needed
                    if (TrieType::needs_bitmap_hash_cache()) {
                        for (size_t idx = 0; idx < BASE::NUM_M_N_GRAM_LEVELS; ++idx) {
                            m_bitmap_hash_cach[idx].pre_allocate(counts[idx + 1]);
                            Logger::updateProgressBar();
                        }
                    }

                    m_trie.pre_allocate(counts);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void add_1_gram(const T_Model_M_Gram<WordIndexType> &gram) {
                    m_trie.add_1_gram(gram);
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    if (TrieType::needs_bitmap_hash_cache()) {
                        //Call the super class first, is needed for caching
                        register_m_gram_cache<CURR_LEVEL>(gram);
                    }

                    m_trie.template add_m_gram<CURR_LEVEL>(gram);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void add_n_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    if (TrieType::needs_bitmap_hash_cache()) {
                        //Call the super class first, is needed for caching
                        register_m_gram_cache<MAX_LEVEL>(gram);
                    }

                    m_trie.add_n_gram(gram);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void log_trie_type_usage_info() {
                    m_trie.log_trie_type_usage_info();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline bool is_post_grams() {
                    return m_trie.template is_post_grams<CURR_LEVEL>();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void post_grams() {
                    m_trie.template post_grams<CURR_LEVEL>();
                };

                /**
                 * Allows to check if the given sub-m-gram is potentially present in the trie.
                 * @param IS_BACK_OFF true if this is for the back-off m-gram case
                 * @param CURR_LEVEL the currently considered level of the m-gram
                 * @param gram the M-gram query to be checked for its hash begin registered in the cache
                 * @return true if the sub-m-gram is potentially present, otherwise false
                 */
                template<bool IS_BACK_OFF, TModelLevel CURR_LEVEL>
                inline bool is_m_gram_hash_cached(const T_M_Gram<WordIndexType> & gram) const {
                    if (TrieType::needs_bitmap_hash_cache()) {
                        //Check on which sub-m-gram level it is
                        if (CURR_LEVEL > M_GRAM_LEVEL_1) {
                            const BitmapHashCache & ref = m_bitmap_hash_cach[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET];
                            return ref.is_m_gram_hash_cached<IS_BACK_OFF, CURR_LEVEL>(gram);
                        } else {
                            //If this is a unigram then we always check the trie
                            //as retrieving the word probability costs nothing.
                            return true;
                        }
                    } else {
                        return true;
                    }
                }

                /**
                 * Allows to check if the given sub-m-gram, defined by the BEGIN_WORD_IDX
                 * and END_WORD_IDX template parameters, is potentially present in the trie.
                 * @param BEGIN_WORD_IDX the begin word index in the given m-gram
                 * @param END_WORD_IDX the end word index in the given m-gram
                 * @param gram the m-gram to work with
                 * @return true if the sub-m-gram is potentially present, otherwise false
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                inline bool is_m_gram_hash_cached(const T_Query_M_Gram<WordIndexType> & gram) const {
                    //Check if the caching is enabled
                    if (TrieType::needs_bitmap_hash_cache()) {
                        //If the caching is enabled
                        
                        //Compute the model level
                        constexpr TModelLevel CURR_LEVEL = (END_WORD_IDX - BEGIN_WORD_IDX) + 1;
                        
                        //Check on which sub-m-gram level it is
                        if (CURR_LEVEL > M_GRAM_LEVEL_1) {
                            //The higher sub-m-gram levels always require checking
                            const BitmapHashCache & ref = m_bitmap_hash_cach[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET];
                            return ref.is_m_gram_hash_cached<BEGIN_WORD_IDX, END_WORD_IDX>(gram);
                        } else {
                            //If this is a unigram then we always check the trie
                            //as retrieving the word probability costs nothing.
                            return true;
                        }
                    } else {
                        //If caching is not enabled then we always check the trie
                        return true;
                    }
                }

                /**
                 * This method allows to get the probability and/or back off weight for the
                 * sub-m-gram defined by the BEGIN_WORD_IDX and END_WORD_IDX template parameters.
                 * @param BEGIN_WORD_IDX the begin word index in the given m-gram
                 * @param END_WORD_IDX the end word index in the given m-gram
                 * @param gram the m-gram to work with
                 * @param payload the payload structure to put the values in
                 * @return true if the payload has been found, otherwise false
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                inline bool get_payload(const T_Query_M_Gram<WordIndexType> & gram, T_M_Gram_Payload & payload) const {
                    //Check if the sub-m-gram hash has been cached
                    if( is_m_gram_hash_cached< BEGIN_WORD_IDX, END_WORD_IDX>(gram) ) {
                        //Check the trie for this m-gram's payload
                        return m_trie.template get_payload<BEGIN_WORD_IDX, END_WORD_IDX>(gram, payload);
                    } else {
                        //There is no data cached for this sub-m-gram so it is definitely not present in the trie.
                        return false;
                    }
                };

                /**
                 * Allows to retrieve the probability and back-off weight of the unknown word
                 * @param payload the unknown word payload data
                 */
                inline void get_unk_word_payload(T_M_Gram_Payload & payload) const {
                    m_trie.get_unk_word_payload(payload);
                };

                /**
                 * Allows to get the probability value also by checking the cache.
                 * If the probability is not found then the prob value is to stay intact!
                 * @param curr_level the currently considered level of the m-gram
                 * @param gram the M-gram query for a specific current level
                 * @param result the result variable to get the probability set to
                 */
                inline void get_prob_weight(const TModelLevel curr_level, const T_M_Gram<WordIndexType> & gram, TLogProbBackOff & total_prob) const {
                    get_prob_weight_func[curr_level](*this, gram, total_prob);
                }

                /**
                 * Allows to get the back-off weight value also by checking the cache.
                 * If the back-off is not found then the probability value is to stay intact.
                 * Then the back-off weight is considered to be zero!
                 * @param curr_level the currently considered level of the m-gram
                 * @param gram the M-gram query for a specific current level
                 * @param result the result variable to get the back-off weight added to
                 */
                inline void add_back_off_weight(const TModelLevel curr_level, const T_M_Gram<WordIndexType> & gram, TLogProbBackOff & total_prob) const {
                    add_back_off_weight_func[curr_level](*this, gram, total_prob);
                }

                /**
                 * Allows to get the probability value also by checking the cache.
                 * If the probability is not found then the prob value is to stay intact!
                 * 
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void get_prob_weight(const T_M_Gram<WordIndexType> & gram, TLogProbBackOff & total_prob) const {
                    LOG_DEBUG << "---> get_prob_weight(" << CURR_LEVEL << ") = " << total_prob << END_LOG;

                    //Try getting the probability value.
                    //1. If the level is one go on: we can get smth
                    //even if the 1-Gram consists of just an unknown word.
                    //2. If the context length is more then one and there is
                    //an unknown word in the gram then it makes no sense to do
                    //searching as there are no M-grams with <unk> in them
                    if ((CURR_LEVEL == M_GRAM_LEVEL_1) ||
                            ((CURR_LEVEL > M_GRAM_LEVEL_1)
                            && gram.template has_no_unk_words<false, CURR_LEVEL>()
                            && is_m_gram_hash_cached<false, CURR_LEVEL>(gram))) {
                        //Let's look further, may be we will find something!
                        LOG_DEBUG1 << "All pre-checks are passed, calling add_prob_value(level, prob)!" << END_LOG;
                        m_trie.template get_prob_weight<CURR_LEVEL>(gram, total_prob);
                    } else {

                        LOG_DEBUG << "Could try to get probs but it will not be  successful due to "
                                << "the present unk words!  Thus backing off right away!" << END_LOG;
                    }
                    LOG_DEBUG << "<--- get_prob_weight(" << CURR_LEVEL << ") = " << total_prob << END_LOG;
                }

                /**
                 * Allows to get the back-off weight value also by checking the cache.
                 * If the back-off is not found then the probability value is to stay intact.
                 * Then the back-off weight is considered to be zero!
                 * 
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                void add_back_off_weight(const T_M_Gram<WordIndexType> & gram, TLogProbBackOff & total_prob) const {
                    LOG_DEBUG << "---> add_back_off_weight(" << CURR_LEVEL << ") = " << total_prob << END_LOG;

                    //Try getting the back-off weight.
                    //1. If the context length is one go on: we can get smth
                    //even if the 1-Gram consists of just an unknown word.
                    //2. If the context length is more then one and there is
                    //an unknown word in the gram then it makes no sense to do
                    //searching as there are no M-grams with <unk> in them
                    if ((CURR_LEVEL == M_GRAM_LEVEL_1) ||
                            ((CURR_LEVEL > M_GRAM_LEVEL_1)
                            && gram.template has_no_unk_words<true, CURR_LEVEL>()
                            && is_m_gram_hash_cached<true, CURR_LEVEL>(gram))) {
                        //Let's look further, we definitely get some back-off weight or zero!
                        LOG_DEBUG1 << "All pre-checks are passed, calling add_back_off_weight(level, prob)!" << END_LOG;
                        m_trie.template add_back_off_weight<CURR_LEVEL>(gram, total_prob);
                    } else {
                        LOG_DEBUG << "Could try to back off but it will not be "
                                << "successful due to the present unk words! Thus "
                                << "the back-off weight is zero!" << END_LOG;
                    }
                    LOG_DEBUG << "<--- add_back_off_weight(" << CURR_LEVEL << ") = " << total_prob << END_LOG;
                }

                /**
                 * The basic class destructor
                 */
                virtual ~GenericTrieDriver() {
                };

            protected:
                //Stores the trie

                TrieType m_trie;

                //Stores the bitmap hash caches per M-gram level for 1 < M <= N
                BitmapHashCache m_bitmap_hash_cach[BASE::NUM_M_N_GRAM_LEVELS];

                //Declare static arrays of pointers to the template function instances 
                static const TRetrieveDataFunct get_prob_weight_func[];
                static const TRetrieveDataFunct add_back_off_weight_func[];

                /**
                 * Is to be used from the sub-classes from the add_X_gram methods.
                 * This method allows to register the given M-gram in internal high
                 * level caches if present.
                 * @param gram the M-gram to cache
                 */
                template<TModelLevel CURR_LEVEL>
                inline void register_m_gram_cache(const T_Model_M_Gram<WordIndexType> &gram) {
                    if (TrieType::needs_bitmap_hash_cache() && (CURR_LEVEL > M_GRAM_LEVEL_1)) {
                        m_bitmap_hash_cach[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET].template add_m_gram<WordIndexType, CURR_LEVEL>(gram);
                    }
                }
            };

            template<typename TrieType>
            const typename GenericTrieDriver<TrieType>::TRetrieveDataFunct GenericTrieDriver<TrieType>::get_prob_weight_func[] = {
                NULL,
                &GenericTrieDriver::get_prob_weight<M_GRAM_LEVEL_1>,
                &GenericTrieDriver::get_prob_weight<M_GRAM_LEVEL_2>,
                &GenericTrieDriver::get_prob_weight<M_GRAM_LEVEL_3>,
                &GenericTrieDriver::get_prob_weight<M_GRAM_LEVEL_4>,
                &GenericTrieDriver::get_prob_weight<M_GRAM_LEVEL_5>,
                &GenericTrieDriver::get_prob_weight<M_GRAM_LEVEL_6>,
                &GenericTrieDriver::get_prob_weight<M_GRAM_LEVEL_7>
            };

            template<typename TrieType>
            const typename GenericTrieDriver<TrieType>::TRetrieveDataFunct GenericTrieDriver<TrieType>::add_back_off_weight_func[] = {
                NULL,
                &GenericTrieDriver::add_back_off_weight<M_GRAM_LEVEL_1>,
                &GenericTrieDriver::add_back_off_weight<M_GRAM_LEVEL_2>,
                &GenericTrieDriver::add_back_off_weight<M_GRAM_LEVEL_3>,
                &GenericTrieDriver::add_back_off_weight<M_GRAM_LEVEL_4>,
                &GenericTrieDriver::add_back_off_weight<M_GRAM_LEVEL_5>,
                &GenericTrieDriver::add_back_off_weight<M_GRAM_LEVEL_6>,
                &GenericTrieDriver::add_back_off_weight<M_GRAM_LEVEL_7>
            };

#define INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, WORD_IDX_TYPE) \
            template class GenericTrieDriver<PREFIX##TRIE_NAME##WORD_IDX_TYPE>; \
            typedef GenericTrieDriver<PREFIX##TRIE_NAME##WORD_IDX_TYPE> TTrieDriver##TRIE_NAME##WORD_IDX_TYPE;

#define INSTANTIATE_TYPEDEF_TRIE_DRIVERS_PREFIX_NAME(PREFIX, TRIE_NAME) \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, Basic); \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, Count); \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, OptBasic); \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, OptCount);

#define INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(TRIE_NAME) \
INSTANTIATE_TYPEDEF_TRIE_DRIVERS_PREFIX_NAME( TLayeredTrieDriver, TRIE_NAME);         

#define INSTANTIATE_TYPEDEF_GENERIC_TRIE_DRIVERS_NAME(TRIE_NAME) \
INSTANTIATE_TYPEDEF_TRIE_DRIVERS_PREFIX_NAME( T, TRIE_NAME);         

            //Make sure that there will be templates instantiated, at least for the given parameter values
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(C2DMapTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(C2DHybridTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(C2WArrayTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(W2CHybridTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(W2CArrayTrie);
            INSTANTIATE_TYPEDEF_GENERIC_TRIE_DRIVERS_NAME(G2DMapTrie);
        }
    }
}

#endif	/* ATRIE_HPP */

