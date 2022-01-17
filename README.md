# wordle-solver
C++ program that provides an optimal solution to wordle

## How to use
Build and run the program with the `recompute_filters` variable set to `true`. `filter_list.bin` will be created locally with some precomputed data, after which you'll be prompted to submit a suggested word to wordle and input 5 numbers representing the colors you got back from the website. For example, for 🟩⬜🟨🟨🟨 you can insert **20111**. Rinse and repeat until you're left with one word.

**Don't forget to reset the `recompute_filters` variable back to `false` and rebuild the program** This way, the program will skip the lenghty step of recomputing the filter list, so you'll be asked for a suggestion much faster.

## How it works

The algorithm this program uses is as follows: It reads a list of words from wordlist.txt which contains all 9330, 5 letter words in Scrabble. Let's call this total N.
Then it creates a NxN (9330x9330) byte array and fills up its contents with the filter value between any two words.

In other words, if the final word is "**SOLAR**" and a guess would be "**SNARL**", the website would output **🟩⬜🟨🟨🟨**. I encode this into a base 3 number as **20111**. Or in decimal, 175. All possible values range from "**00000**" to "**22222**" which, in decimal, translates from 0 to 242. Given this range of valid consecutive values, this means that the possible values for a filter fit in one 8-bit unsigned number.

After creating the NxN array, I compute the optimal guess for the current word list in its filtered state. For this, I compute the expected value of the words that will have been narrowed down with each guess. So, for each word in the filtered list, I take every word in the list as a possible final word, and based on those two words (and their corresponding filter in the table), I calculate the number of elements that this possible guess would narrow down the word list to. 

What I end up with is the lowest expected value for the remaining words should I guess the right word. This value is based on the fact that each word is equally likely to be shown. If there is a different probability for a word to be chosen than others, those can be easily calculated and a weighted average can be determined.

### First optimal guess
As an aside, the first guess given this Scrabble set turns out to be "**LARES**". It will narrow down the words to an expected value of 205.1 words. One may think that the best first guess is "**AROSE**" since it uses the first 5 most likely letters encountered in this set of words, whereas, **L** turns out to be th 7th most frequent word in the word list. However, "**AROSE**" will narrow down the main list to an average of 275.04 words, almost a quarter less efficient than "**LARES**". This may be due to the fact that although **O** is more frequent than **L**, it is less useful as it tends to repeat in the same word multiple times, making its capacity for reducing entropy less potent. Similar for **I**, the 6th most frequent letter.

After the program comes up with the optimal suggestion for its given set of words, it asks the user to insert the filter result they got from the website. It then filters the list of words given the user's guess and the given filter and starts looking for a suggestion corresponding to the new set of words. Rinse and repeat until there is 0 or 1 words left.

## Notes on execution time:

The program runs on an O(N^2) complexity, which means about 87 million operation points. This can take a long time at program initialization, which is why there are some ways to bypass this effort.

The NxN filter table is the same for the same word list, so it only needs to be calculated once. As a result, as long as the `recompute_filters` variable is set to `true`, the program will spend about a minute computing all possible filters and then writing them into a file named **filter_list.bin**. After the first execution, this step is not necessary anymore, so the variable can be turned back to `false`, the program will look for those values in **filter_list.bin** and will initialize much faster. Another step is bypassed by hard coding the first optimal guess, "**LARES**" as it is not worth doing that every time the program starts.

## Example of run
![This is an image](https://i.imgur.com/GTjquXB.png)
