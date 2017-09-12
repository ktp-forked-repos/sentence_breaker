#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <tuple>
#include <exception>
#include <cctype>
#include <boost/container/flat_map.hpp>

namespace
{

namespace EXCEPTIONS
{
	class NON_ALPHABETICAL_EXCEPTION : public std::exception
	{

	};

	class IMPOSSIBLE_MATCH_EXCEPTION : public std::exception
	{

	};
};

// Dictionary class that quickly finds out whether a sequence of characters represents an English word, or the prefix of an English word,
// or both
// Implementation with a prefix tree, which allows linear time prefix feasibility test and low memory footprint
// (Memory: O(width * height), width is no more than 26 and height is the number of characters of an average english word - 5 to 10 maybe?).
class DICTIONARY
{
public:
	// A dictionary file that could be useful: http://www-01.sil.org/linguistics/wordlists/english/wordlist/wordsEn.txt
	// Just over 1 megabyte, most computers should handle.
	DICTIONARY(const char * filename)
	{
		load(filename);
	}

	// Read comment for PREFIX_TREE::prefix_match
	std::pair<bool, bool> prefix_match(std::string::const_iterator begin_prefix, std::string::const_iterator end_prefix) const
	{
		return m_prefix_tree.prefix_match(begin_prefix, end_prefix);
	}

	void load(const char * filename)
	{
		std::ifstream ifs(filename);
		std::string word;
		while (!ifs.eof())
		{
			word.clear();
			ifs >> word;
			m_prefix_tree.add_word(word);
		}
	}

private:
	class PREFIX_TREE
	{
		class TREE_NODE
		{
		public:
			static char sanitize_key(char c)
			{
				return std::tolower(c);
			}

			TREE_NODE()
			:
				m_children(),
				m_is_word(false)
			{

			}

			auto add_or_find_child(char c)
			{
				auto emplace_result = m_children.emplace(std::make_pair(sanitize_key(c), TREE_NODE()));
				auto emplace_iter = emplace_result.first;

				return emplace_iter;
			}

			void set_is_word()
			{
				m_is_word = true;
			}

			auto find_child(char c)
			{
				auto iter = m_children.find(sanitize_key(c));
				return (iter == m_children.end()) ? nullptr : &(*iter);
			}

			auto num_children() const
			{
				return m_children.size();
			}
		private:
			boost::container::flat_map<char, TREE_NODE> m_children; // Sorted vector
			bool m_is_word;
		};

	public:
		// TODO: As an optimization, take positional hint to speed up ordered loading.
		void add_word(const std::string & new_word)
		{
			// TODO: Construct the tree branch for the word
			TREE_NODE * last_node = &m_root;
			for (char c : new_word)
			{
				last_node = &((last_node->add_or_find_child(c))->second);
			}
			last_node->set_is_word();
		}

		// Match prefix to words in a dictionary represented by tree of characters (prefix as root),
		// This implementation would be quick to determine the following two boolean return values that caller needs.
		//
		// Returns: bool 1: whether this substring is a word
		//          bool 2: whether this substring is a prefix of one or more other words, excluding the substring itself
		//
		// A worthy optimization: Could take a hint iterator to limit the search to a subtree to speed up the search.
		// Oftentimes we know which subtree we should look at (e.g. when we want to test whether “apple” is a word after knowing
		//  “appl” is a prefix but not a word - we should just start from the “l” node instead of redundantly going through
		// the a-p-p-l-e path).
		//
		// * Input case insensitive.
		//
		// Complexity: O of
		//    length of character sequence under test *
		//    linear or binary search of a tree edge under a tree node
		//          (Assuming English, 26 at most, so it’s arguably constant. Should be much fewer than 26 in practice)
		std::pair<bool, bool> prefix_match(std::string::const_iterator begin_prefix, std::string::const_iterator end_prefix) const
		{
			// TODO: Find whether the character sequence has a matching word (matched to a tree branch with no child)
			// or a matching prefix (matched to a tree branch with children) or both
			return std::make_pair(false, false);
		}
	private:


		TREE_NODE m_root;
	};

	// Data Members
	PREFIX_TREE m_prefix_tree;
};


// Complexity:
// Assume I modify prefix_match to take subtree_hint iterator to limit the search scope and get rid of redundant work
// O of input string length * linear or binary search of an edge under a dictionary tree node (which is arguably constant for English)
//   = effectively linear
void break_sentence(std::vector<std::string> & word_breakdown, const std::string & in_sentence, const DICTIONARY & dict)
{
	// Robustness consideration (not all are implemented)
	// 1) Spaces? Handled by main string reader already. But if still exists,
	//    just jump over them and continue matching at next real character
	// 2) Segments of non alphabetical? Consider as one word
	// 3) Cases? Preserve, but assume prefix_match is case insensitive
	// 4) What if it gets stuck?

	// Clear the output buffer that stores the result
	word_breakdown.clear();

	// Iterator pairs for the whole input sentences
	const auto begin_iter = in_sentence.cbegin();
	const auto end_iter   = in_sentence.cend();

	// Iterators that stores the progress of the current word being worked on.
	auto round_begin = begin_iter;
	auto round_curr = begin_iter;

	// The longest match found so far for the current word being worked on.
	std::string last_exact_match; // Always clear it when move on to a new word
	std::string::const_iterator last_exact_match_end_iter = end_iter; // Always set to end_iter when move on to a new word

	// Loop to greedily find the longest match and add it to the result vector, then start at next character and repeat.
	while (round_begin != end_iter)
	{
		bool is_word, is_prefix;
		std::tie(is_word, is_prefix) = dict.prefix_match(round_begin, round_curr);

		if (is_word)
		{
			if (!is_prefix || (round_curr == end_iter))
			{
				// Perfect. This is the longest possible solution for the current word.
				// There’s no chance that keep appending on the current word can give us a longer result.
				// Action: Add the word to buffer and set up everything for a new round (new word).

				word_breakdown.emplace_back(round_begin, round_curr);
				++round_curr;
				round_begin = round_curr;
				last_exact_match.clear();
				last_exact_match_end_iter = end_iter;
			}
			else
			{
				// This is a good match, but there might be more possibilities if we appending to the word under test.
				// Action: keep trying to match more characters, but save the current word and char iterator just in case
				// All subsequent matches fail - we’ll revert to this solution by then.
				last_exact_match.assign(round_begin, round_curr);
				last_exact_match_end_iter = round_curr;
				++round_curr;
			}

		}
		else // !is_word
		{
			if (is_prefix)
			{
				// Not a word, but could be part of a word.
				// Still there’s chances to find a word if we keep appending new characters.
				// Action: Stay in current round, but expands the prefix under test.
				++round_curr;
			}
			else
			{
				// We failed by either overmatching (if last exact match was set), or by impossible input (if otherwise)
				// Action:
				// In case of overmatching, we just revert to the last matched word, and roll back iterators for new round.
				// Otherwise, throw an exception to signify an impossible match.
				if (last_exact_match.empty())
				{
					throw EXCEPTIONS::IMPOSSIBLE_MATCH_EXCEPTION();
				}
				else
				{
					// Check for mismatch between last_exact_match and last_exact_match_end_iter
					assert(last_exact_match_end_iter != end_iter);

					word_breakdown.push_back(std::move(last_exact_match));
					last_exact_match.clear();
					last_exact_match_end_iter = end_iter;
					round_begin = last_exact_match_end_iter + 1;
					round_curr  = round_begin;
				}
			}
		} // end if-else (is_word)
	}
}

} // End Namespace


int main()
{
	std::cout << "Hello world!" << std::endl;
	std::vector<std::string> out_words;
	std::string in_sentence;
	DICTIONARY dict("merriam-webster.dict");
	// Simple string reader...
	while (std::cin.eof()) // TODO(bxing): API call correct?
	{
		in_sentence.clear();
		std::cin >> in_sentence; // Reads until space. Kinda what we want.
		break_sentence(out_words, in_sentence, dict);
		for (const auto & word : out_words)
		{
			std::cout << word << std::endl;
		}
	}
	return 0;
}
