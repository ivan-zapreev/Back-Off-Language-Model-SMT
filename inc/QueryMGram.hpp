/* 
 * File:   QueryMGram.hpp
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
 * Created on October 28, 2015, 10:28 AM
 */

#ifndef QUERYMGRAM_HPP
#define	QUERYMGRAM_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "BaseMGram.hpp"

#include "TextPieceReader.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

#include "ByteMGramId.hpp"
#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

namespace uva {
    namespace smt {
        namespace tries {
            namespace m_grams {

                /**
                 * This class is used to represent the N-Gram that will be queried against the language model.
                 */
                template<typename WordIndexType, TModelLevel MAX_LEVEL_CAPACITY = M_GRAM_LEVEL_MAX>
                class T_Query_M_Gram : public T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY> {
                public:
                    //The type of the word id
                    typedef typename WordIndexType::TWordIdType TWordIdType;
                    //Define the corresponding M-gram id type
                    typedef m_gram_id::Byte_M_Gram_Id<TWordIdType> T_M_Gram_Id;
                    //Define the base class type
                    typedef T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY> BASE;

                    //Stores the unknown word masks for the probability computations,
                    //up to and including 8-grams. Are used to mark <unk> word bits:
                    // 0: 10000000, 1: 01000000, 2: 00100000, 3: 00010000,
                    // 4: 00001000, 5: 00000100, 6: 00000010, 7: 00000001
                    static constexpr uint8_t UNK_WORD_MASKS[M_GRAM_LEVEL_8] = {
                        0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
                    };

                    //Stores the sub-m-gram unknown word masks for checking
                    //on <unk> words, up to and including 8-grams:
                    //0: { 0: 10000000 1: 11000000 2: 11100000 3: 11110000 4: 11111000 5: 11111100 6: 11111110 7: 11111111 }
                    //1: { 0: 00000000 1: 01000000 2: 01100000 3: 01110000 4: 01111000 5: 01111100 6: 01111110 7: 01111111 }
                    //2: { 0: 00000000 1: 00000000 2: 00100000 3: 00110000 4: 00111000 5: 00111100 6: 00111110 7: 00111111 }
                    //3: { 0: 00000000 1: 00000000 2: 00000000 3: 00010000 4: 00011000 5: 00011100 6: 00011110 7: 00011111 }
                    //4: { 0: 00000000 1: 00000000 2: 00000000 3: 00000000 4: 00001000 5: 00001100 6: 00001110 7: 00001111 }
                    //5: { 0: 00000000 1: 00000000 2: 00000000 3: 00000000 4: 00000000 5: 00000100 6: 00000110 7: 00000111 }
                    //6: { 0: 00000000 1: 00000000 2: 00000000 3: 00000000 4: 00000000 5: 00000000 6: 00000010 7: 00000011 }
                    //7: { 0: 00000000 1: 00000000 2: 00000000 3: 00000000 4: 00000000 5: 00000000 6: 00000000 7: 00000001 }
                    static constexpr uint8_t SMG_UNK_WORD_MASKS[M_GRAM_LEVEL_8][M_GRAM_LEVEL_8] = {
                        { 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF},
                        { 0x00, 0x40, 0x60, 0x70, 0x78, 0x7C, 0x7E, 0x7F},
                        { 0x00, 0x00, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F},
                        { 0x00, 0x00, 0x00, 0x10, 0x18, 0x1C, 0x1E, 0x1F},
                        { 0x00, 0x00, 0x00, 0x00, 0x08, 0x0C, 0x0E, 0x0F},
                        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x06, 0x07},
                        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03},
                        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
                    };

                    /**
                     * The basic constructor, is to be used when the M-gram will
                     * actual level is not known beforehand - used e.g. in the query
                     * m-gram sub-class. The actual m-gram level is set to be
                     * undefined. Filling in the M-gram tokens is done elsewhere.
                     * @param word_index the used word index
                     */
                    T_Query_M_Gram(WordIndexType & word_index)
                    : T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY>(word_index) {
                    }

                    /**
                     * Allows to prepare the M-gram for being queried. 
                     * If the cumulative probability is to be computed then all
                     * word ids are needed, in case only the longest sub-m-gram
                     * conditional probability is computed, we need to start
                     * from the last word and go backwards. Then we shall stop
                     * as soon as we reach the first unknown word!
                     * @param IS_UNK_WORD_FLAGS if set to true then we also compute
                     * the <unk> word flags so that later at any time we can check
                     * whether a given sub-m-gram has <unk> words in it.
                     */
                    template<bool IS_UNK_WORD_FLAGS>
                    inline void prepare_for_querying() {
                        LOG_DEBUG1 << "Preparing for the query execution, create <unk> "
                                << "word flags = " << (IS_UNK_WORD_FLAGS ? "true" : "false") << END_LOG;

                        //Set all the "computed hash level" flags to "undefined"
                        memset(computed_hash_level, M_GRAM_LEVEL_UNDEF, MAX_LEVEL_CAPACITY * sizeof (TModelLevel));

                        //Re-set the unknown word flags, if needed
                        if (IS_UNK_WORD_FLAGS) {
                            m_unk_word_flags = 0;
                        }

                        LOG_DEBUG1 << "Start retrieving the word ids: forward" << END_LOG;
                        //Retrieve all the word ids unconditionally, as we will need all of them
                        for (TModelLevel curr_word_idx = BASE::m_actual_begin_word_idx; curr_word_idx <= BASE::m_actual_end_word_idx; ++curr_word_idx) {
                            BASE::m_word_ids[curr_word_idx] = BASE::m_word_index.get_word_id(BASE::m_tokens[curr_word_idx]);
                            LOG_DEBUG2 << "The word: '" << BASE::m_tokens[curr_word_idx] << "' is: "
                                    << SSTR(BASE::m_word_ids[curr_word_idx]) << "!" << END_LOG;
                            if (IS_UNK_WORD_FLAGS && (BASE::m_word_ids[curr_word_idx] == WordIndexType::UNKNOWN_WORD_ID)) {
                                m_unk_word_flags |= UNK_WORD_MASKS[curr_word_idx];
                            }
                        }
                        LOG_DEBUG1 << "Done preparing for the query execution!" << END_LOG;
                    }

                    /**
                     * Allows to check if the word with the given word index is an unknown word.
                     * The word flags are properly initialized iff the prepare_for_querying method
                     * was called with the IS_UNK_WORD_FLAGS template parameter set to true.
                     * @param word_idx the index of the word to be checked
                     * @return true if the word under the given index is an <unk> word
                     */
                    inline bool is_unk_word(const TModelLevel word_idx) const {
                        return m_unk_word_flags & SMG_UNK_WORD_MASKS[word_idx][word_idx];
                    }

                    /**
                     * Allows to check if the sub-m-gram defined by the method arguments has unknown words in it.
                     * The word flags are properly initialized iff the prepare_for_querying method was called with
                     * the IS_UNK_WORD_FLAGS template parameter set to true.
                     * @param begin_word_idx the first word index of the sub-m-gram
                     * @param end_word_idx the last word index of the sub-m-gram
                     * @return true if the sub-m-gram contains <unk> words, otherwise false
                     */
                    inline bool has_unk_words(const TModelLevel begin_word_idx, const TModelLevel end_word_idx) const {
                        return m_unk_word_flags & SMG_UNK_WORD_MASKS[begin_word_idx][end_word_idx];
                    }

                    /**
                     * Allows to retrieve the hash value for the sub-m-gram 
                     * defined by the template parameters
                     * @return the hash value for the given sub-m-gram
                     */
                    inline uint64_t get_hash(const TModelLevel begin_word_idx, const TModelLevel end_word_idx) const {
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * Allows to retrieve the hash value for the sub-m-gram 
                     * defined by the template parameters
                     * @return the hash value for the given sub-m-gram
                     */
                    template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                    inline uint64_t get_hash() const {
                        LOG_DEBUG1 << "Getting hash values for begin/end index: " << SSTR(BEGIN_WORD_IDX)
                                << "/" << SSTR(END_WORD_IDX) << ", the previous computed begin level "
                                << "is: " << SSTR(computed_hash_level[END_WORD_IDX]) << END_LOG;

                        //The column has not been processed before, we need to iterate and incrementally compute hashes
                        uint64_t(& hash_column)[MAX_LEVEL_CAPACITY] = const_cast<uint64_t(&)[MAX_LEVEL_CAPACITY]> (m_hash_matrix[END_WORD_IDX]);

                        //Check if the given column has already been processed.
                        //This is not an exact check, as not all the rows of the
                        //column could have been assigned with hashes. However, in
                        //case of proper use of the class this is the only check we need.
                        if (computed_hash_level[END_WORD_IDX] == M_GRAM_LEVEL_UNDEF) {
                            //Start iterating from the end of the sub-m-gram
                            TModelLevel curr_idx = END_WORD_IDX;
                            //If the word is not unknown then the first hash, the word's hash is its id
                            hash_column[curr_idx] = BASE::m_word_ids[curr_idx];

                            LOG_DEBUG1 << "hash[" << SSTR(curr_idx) << "] = " << hash_column[curr_idx] << END_LOG;

                            //Iterate through the word ids, and build-up hashes
                            do {
                                //Decrement the word id
                                curr_idx--;

                                //Incrementally build up hash, using the previous hash value and the next word id
                                hash_column[curr_idx] = combine_hash(BASE::m_word_ids[curr_idx], hash_column[curr_idx + 1]);

                                LOG_DEBUG1 << "word[" << SSTR(curr_idx) << "] = " << BASE::m_word_ids[curr_idx]
                                        << ", hash[" << SSTR(curr_idx) << "] = " << hash_column[curr_idx] << END_LOG;

                                //Stop iterating if the reached the beginning of the m-gram
                            } while (curr_idx != BEGIN_WORD_IDX);

                            //Cast the const modifier away to set the internal flag
                            const_cast<TModelLevel&> (computed_hash_level[END_WORD_IDX]) = BEGIN_WORD_IDX;

                            LOG_DEBUG1 << "compute_hash_level[" << SSTR(END_WORD_IDX) << "] = "
                                    << computed_hash_level[END_WORD_IDX] << END_LOG;
                        }

                        //Perform the sanity check if needed
                        if (DO_SANITY_CHECKS && (BEGIN_WORD_IDX < computed_hash_level[END_WORD_IDX])) {
                            stringstream msg;
                            msg << "The sub-m-gram [" << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << "]: "
                                    << (string) * this << ", hash has not been computed! The hash is only there for "
                                    << "[" << SSTR(computed_hash_level[END_WORD_IDX]) << ", " << SSTR(END_WORD_IDX) << "]";
                            throw Exception(msg.str());
                        }

                        LOG_DEBUG1 << "Resulting hash value: " << hash_column[BEGIN_WORD_IDX] << END_LOG;

                        //Return the hash value that must have been pre-computed
                        return hash_column[BEGIN_WORD_IDX];
                    }

                    /**
                     * For the given N-gram, for some level M <=N , this method
                     * allows to give the string of the object for which the
                     * probability is computed, e.g.:
                     * N-gram = "word1" -> result = "word1"
                     * N-gram = "word1 word2 word3" -> result = "word3 | word1  word2"
                     * for the first M tokens of the N-gram
                     * @param level the level M of the sub-m-gram prefix to work with
                     * @return the resulting string
                     */
                    inline string get_mgram_prob_str(const TModelLevel level) const {
                        if (level == M_GRAM_LEVEL_UNDEF) {
                            return "<none>";
                        } else {
                            if (level == M_GRAM_LEVEL_1) {
                                const TextPieceReader & token = BASE::m_tokens[BASE::m_actual_begin_word_idx];
                                return token.str().empty() ? "<empty>" : token.str();
                            } else {
                                const TModelLevel end_word_idx = (level - 1);
                                string result = BASE::m_tokens[end_word_idx].str() + " |";
                                for (TModelLevel idx = BASE::m_actual_begin_word_idx; idx != end_word_idx; idx++) {
                                    result += string(" ") + BASE::m_tokens[idx].str();
                                }
                                return result;
                            }
                        }
                    }

                    /**
                     * For the given N-gram, this method allows to give the string 
                     * of the object for which the probability is computed, e.g.:
                     * N-gram = "word1" -> result = "word1"
                     * N-gram = "word1 word2 word3" -> result = "word3 | word1  word2"
                     * @return the resulting string
                     */
                    inline string get_mgram_prob_str() const {
                        if (BASE::m_actual_level == M_GRAM_LEVEL_UNDEF) {
                            return "<none>";
                        } else {
                            if (BASE::m_actual_level == M_GRAM_LEVEL_1) {
                                const TextPieceReader & token = BASE::m_tokens[BASE::m_actual_begin_word_idx];
                                return token.str().empty() ? "<empty>" : token.str();
                            } else {
                                string result;
                                for (TModelLevel idx = BASE::m_actual_begin_word_idx; idx <= BASE::m_actual_end_word_idx; idx++) {
                                    result += BASE::m_tokens[idx].str() + string(" ");
                                }
                                return result.substr(0, result.length() - 1);
                            }
                        }
                    }

                    /**
                     * Tokenise a given piece of text into a space separated list of text pieces.
                     * @param text the piece of text to tokenise
                     * @param gram the gram container to put data into
                     */
                    inline void set_m_gram_from_text(TextPieceReader &text) {
                        BASE::m_actual_end_word_idx = BASE::m_actual_begin_word_idx;

                        //Read the tokens one by one backwards and decrement the index
                        while (text.get_first_space(BASE::m_tokens[BASE::m_actual_end_word_idx++]));

                        //Adjust the end word index, in the loop above we always
                        //increment for the one extra time, for the false result.
                        //So after the loop the end word index is equal to the word
                        //count + 1. Now set it to the proper value, subtracting 2
                        BASE::m_actual_end_word_idx -= 2;

                        //Set the actual level value to be the number of read words
                        BASE::m_actual_level = BASE::m_actual_end_word_idx + 1;

                        //Do the sanity check if needed!
                        if (DO_SANITY_CHECKS && (
                                (BASE::m_actual_level < M_GRAM_LEVEL_1) ||
                                (BASE::m_actual_level > MAX_LEVEL_CAPACITY))) {
                            stringstream msg;
                            msg << "A broken N-gram query: " << (string) * this
                                    << ", level: " << SSTR(BASE::m_actual_level);
                            throw Exception(msg.str());
                        }
                    }

                private:
                    //Stores the first known word index following the last unknown word. For non cumulative queries.
                    TModelLevel m_flk_word_idx;
                    //Stores the hash computed flags
                    TModelLevel computed_hash_level[MAX_LEVEL_CAPACITY];
                    //Stores the computed hash values
                    uint64_t m_hash_matrix[MAX_LEVEL_CAPACITY][MAX_LEVEL_CAPACITY];
                    //Unknown word bit flags
                    uint8_t m_unk_word_flags = 0;

                    /**
                     * This constructor is made private as it is not to be used
                     */
                    T_Query_M_Gram(WordIndexType & word_index, TModelLevel actual_level)
                    : T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY>(word_index, actual_level) {
                    }
                };

                template<typename WordIndexType, TModelLevel MAX_LEVEL_CAPACITY>
                constexpr uint8_t T_Query_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY>::UNK_WORD_MASKS[M_GRAM_LEVEL_8];

                template<typename WordIndexType, TModelLevel MAX_LEVEL_CAPACITY>
                constexpr uint8_t T_Query_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY>::SMG_UNK_WORD_MASKS[M_GRAM_LEVEL_8][M_GRAM_LEVEL_8];

            }
        }
    }
}

#endif	/* QUERYMGRAM_HPP */

