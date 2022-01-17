#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

template <class T> 
string FormatWithCommas(T value) {
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << value;
    return ss.str();
}

bool ValidateFilter(string filter) {
  if (filter.size() != 5)
    return false;

  for (string::size_type i = 0; i < filter.size(); i++) {
    if (filter[i] != '0' && filter[i] != '1' && filter[i] != '2')
      return false;
  }
  return true;
}

uint8_t EncodeFilter(string filter) {
  if (filter.size() != 5)
    return 255;

  uint8_t result = 0;
  for (string::size_type i = 0; i < filter.size(); i++) {
    int current_term;
    switch (filter[i])
    {
    case '0':
        current_term = 0;
        break;
    case '1':
        current_term = 1;
        break;
    case '2':
      current_term = 2;
      break;

    default:
      return 255;
      break;
    }

    result = result * 3 + current_term;
  }

  return result;
}

string DecodeFilter(uint8_t number) {
  if (number < 0 || number > 242) {
    return "error";
  }
  char binary_result[] = {'0', '0', '0', '0', '0', '\0'}; 

  for(int i = 0; i < 5; i++){
      switch (number % 3)
      {
      case 0:
        binary_result[5 - 1 - i] = '0';
        break;
      case 1:
        binary_result[5 - 1 - i] = '1';
        break;
      case 2:
        binary_result[5 - 1 - i] = '2';
        break;

      default:
          break;
      }
      number = number / 3;
  }

  return string(binary_result);
}

string CreateFilter(string input, string target) {
  if(input.size() != 5 || target.size() != 5){
    return "string_not_valid";
  }
  char binary_result[] = {'0', '0', '0', '0', '0', '\0'};

  for (int i = 0; i < 5; i++) {
    if (input.at(i) == target.at(i))
      binary_result[i] = '2';
  }

  for (int i = 0; i < 5; i++) {
    if (binary_result[i] == '2')
      continue;
    for (int j = 0; j < 5; j++) {
      if (target.at(j) == input.at(i) && binary_result[j] != '2') {
        binary_result[i] = '1';
        break;
      }
    }
  }

  return string(binary_result);
}

uint8_t **ComputeFilters(vector<string> word_list, string filename,
                         size_t word_count) {
  cout << "Computing and writing filters for all word pairs..." << endl;
  ofstream out_file;
  out_file.open(filename.c_str(), ios::out | ios::binary);

  uint8_t *filters_pointer =
      (uint8_t *)malloc(sizeof(uint8_t) * word_count * word_count);

  uint8_t **filters = (uint8_t **)malloc(sizeof(uint8_t *) * word_count);

  for (int i = 0; i < word_count; i++) {
    filters[i] = &filters_pointer[i * word_count];
  }

  for (int i = 0; i < word_count; i++) {
    for (int j = 0; j < word_count; j++) {
      filters[i][j] = EncodeFilter(CreateFilter(word_list[i], word_list[j]));
    }
  }

  out_file.write((char *)filters_pointer,
                 sizeof(uint8_t) * word_count * word_count);
  cout << "Done! Wrote " << FormatWithCommas(word_count * word_count)
       << " filters to " << filename << endl;

  return filters;
}

bool SlowMatch(uint8_t encoded_filter, string input, string target) {
  char filter[6];
  strcpy(filter, DecodeFilter(encoded_filter).c_str());

  for (int i = 0; i < 5; i++) {
    bool found = false;
    switch (filter[i]) {
    case '2':
      if (input.at(i) != target.at(i)) {
        return false;
      }
      break;
    case '1':
      if (target.at(i) == input.at(i)) {
        return false;
      }
      for (int j = 0; j < 5; j++) {
        if (i != j && target.at(j) == input.at(i) && filter[j] != '2') {
          found = true;
          break;
        }
      }
      if (!found)
        return false;
      break;
    case '0':
      for (int j = 0; j < 5; j++) {
        if (target.at(j) == input.at(i) && filter[j] != '2') {
          return false;
        }
      }
      break;

    default:
      return false;
      break;
    }
  }

  return true;
}

bool Match(uint8_t encoded_filter, string query, string target,
           unordered_map<string, int> word_map, uint8_t **filters) {
  int query_index = word_map[query];
  int target_index = word_map[target];
  uint8_t filter_entry = filters[query_index][word_map[target]];
  return filter_entry == encoded_filter;
}

vector<string> FilterWords(uint8_t encoded_filter, string query,
                           vector<string> original_word_list,
                           vector<string> partial_word_list,
                           unordered_map<string, int> original_word_map,
                           unordered_map<string, int> partial_word_map,
                           uint8_t **filters,
                           vector<int> **compressed_filters) {
  string upper_query = query;
  transform(upper_query.begin(), upper_query.end(), upper_query.begin(),
            ::toupper);
  cout << endl
       << "Filtering " << FormatWithCommas(partial_word_list.size())
       << " words with guess \"" << upper_query << "\" and filter "
       << DecodeFilter(encoded_filter) << endl;
  vector<string> result;
  for (string word : partial_word_list) {
    if (Match(encoded_filter, query, word, original_word_map, filters)) {
      result.push_back(word);
    } else {
      partial_word_map.erase(word);
    }
  }

  for (int i = 0; i < 243; i++) {
    if(i != encoded_filter){
      compressed_filters[original_word_map[query]][i].clear();
    }
  }

  for (string word : result) {
    for (int i = 0; i < 243; i++) {
      int word_index = original_word_map[word];

      vector<int> filtered_vector(
          compressed_filters[word_index][i].size());

      auto it = copy_if(
          compressed_filters[word_index][i].begin(),
          compressed_filters[word_index][i].end(), filtered_vector.begin(),
          [partial_word_map, original_word_list](int sample) {
            return partial_word_map.find(original_word_list.at(sample)) !=
                   partial_word_map.end();
          });

      filtered_vector.resize(distance(filtered_vector.begin(), it));

      compressed_filters[word_index][i] = filtered_vector;
    }
  }

  cout << "Down to " << FormatWithCommas(result.size()) << " words" << endl
       << endl;
  return result;
}

int CountMatches(uint8_t encoded_filter, string query, vector<string> word_list,
                 unordered_map<string, int> word_map, uint8_t **filters) {
  int result = 0;
  for(string word : word_list) {
    if(Match(encoded_filter, query, word, word_map, filters)){
      result++;
    }
  }
  return result;
}

int CountNonMatches(uint8_t encoded_filter, string query,
                    vector<string> word_list,
                    unordered_map<string, int> word_map, uint8_t **filters) {
  int result = 0;
  for(string word : word_list) {
    if(!Match(encoded_filter, query, word, word_map, filters)){
      result++;
    }
  }
  return result;
}

string FindSuggestion(vector<string> word_list,
                      unordered_map<string, int> partial_word_map,
                      uint8_t **filters, vector<int> **compressed_filters) {
  int min_words_remaining = -1;
  string best_word = "";
  // int arose_score = 0;

  for (string query : word_list) {
    int current_word_cut = 0;
    
    for (string potential_word : word_list){
      int query_index = partial_word_map[query];
      int potential_word_index = partial_word_map[potential_word];
      uint8_t current_filter =
          filters[query_index][potential_word_index];

      current_word_cut +=
          compressed_filters[query_index][current_filter].size();
    }

    // if (query == "arose") {
    //   arose_score = current_word_cut;
    // }

    if (current_word_cut < min_words_remaining || min_words_remaining < 0) {
      min_words_remaining = current_word_cut;
      best_word = query;
    }
  }

  // cout << "Arose score: " << FormatWithCommas(arose_score) << endl;
  // cout << "best reducing word " << best_word << " score - "
  //      << FormatWithCommas(min_words_remaining) << endl;
  return best_word;
}

int main() {
  string line;
  ifstream list_file("wordlist.txt");
  string out_filter_file_name = "filter_list.bin";

  vector<string> partial_word_list;
  vector<string> original_word_list;
  uint8_t **filters;
  vector<int> **compressed_filters;
  size_t word_count;
  unordered_map<string, int> original_word_map;
  unordered_map<string, int> partial_word_map;

  const bool recompute_filters = false;

  if (list_file.is_open()) {
    int index = 0;
    while (getline(list_file, line)) {
      partial_word_list.push_back(line);
      original_word_map[line] = index;
      partial_word_map[line] = index;
      index++;
    }
  }

  original_word_list = partial_word_list;

  word_count = original_word_list.size();

  cout << "Read " << FormatWithCommas(original_word_list.size()) << " words"
       << endl;

  if (recompute_filters) {
    // Warning - may take a long time to run ComputeFilters(word_list)
    filters =
        ComputeFilters(original_word_list, out_filter_file_name, word_count);
  } else {
    uint8_t *filters_pointer =
        (uint8_t *)malloc(sizeof(uint8_t) * word_count * word_count);

    FILE *filter_file;
    filter_file = fopen(out_filter_file_name.c_str(), "rb");
    fread(filters_pointer, sizeof(uint8_t), word_count * word_count,
          filter_file);
    fclose(filter_file);

    filters = (uint8_t **)malloc(sizeof(uint8_t *) * word_count);
    for (int i = 0; i < word_count; i++) {
      filters[i] = &filters_pointer[i * word_count];
    }

    cout << "Read "
         << FormatWithCommas(sizeof(uint8_t) * word_count * word_count)
         << " filters from " << out_filter_file_name << endl;
  }

  cout << endl
       << "Initializing program and fetching suggestion... " << endl
       << endl;

  vector<int> *compressed_filter_pointer =
      (vector<int> *)malloc(word_count * 243 * sizeof(vector<int>));
  compressed_filters =
      (vector<int> **)malloc(sizeof(vector<int> *) * word_count);

  for (int i = 0; i < word_count; i++) {
    compressed_filters[i] = &compressed_filter_pointer[i * 243];
  }

  // filling in the compressed filter data structure from the filter list
  for (int i = 0; i < word_count; i++) {
    for (int j = 0; j < word_count; j++) {
      compressed_filters[i][filters[i][j]].push_back(j);
    }
  }

  // FindSuggestion(partial_word_list, original_word_map, filters,
  //                compressed_filters);

  // First suggestion is hardcoded to the word list. Refer to previous commit or
  // the commented call above to change the call order and calculate it for a
  // possible new list. For Scrabble, "lares" will narrow down, on average, to
  // 25% less words than "arose", even though "arose" uses the top 5 letters as
  // frequency, whereas "lares" only uses the first 4 and the 7th most frequent
  // letter, ('l'). The expected number of words that "arose" can narrow down to
  // is 2,566,195/9330 words (or 275.04 words) whereas "lares" will be expected
  // to narrow down to 1,913,617/9330 words (or 205.1 words). That is a 25%
  // increase in efficiency. This may be due to the fact that even though the
  // letter 'o' is more common, it turns out to be less unique among different
  // words and more common to repeat within the same word (e.g. via the 'oo'
  // character group). Similar with 'i', the 6th most frequent letter.
  string suggestion = "lares";

  while (partial_word_list.size() > 1) {
    string upper_suggestion = suggestion;
    transform(upper_suggestion.begin(), upper_suggestion.end(),
              upper_suggestion.begin(), ::toupper);

    cout << "Suggested word: " << upper_suggestion << endl;
    cout << "Please enter the filter value from website: ";
    
    string current_filter = "";
    while (!ValidateFilter(current_filter)) {
      cin >> current_filter;
      if (!ValidateFilter(current_filter)) {
        cout << "Invalid filtered entered. Try again. > ";
      }
    }

    partial_word_list =
        FilterWords(EncodeFilter(current_filter), suggestion,
                    original_word_list, partial_word_list, original_word_map,
                    partial_word_map, filters, compressed_filters);

    suggestion = FindSuggestion(partial_word_list, partial_word_map, filters,
                                compressed_filters);
  }

  if (partial_word_list.size() == 1) {
    string upper_solution = partial_word_list.front();
    transform(upper_solution.begin(), upper_solution.end(),
              upper_solution.begin(), ::toupper);
    cout << "Final word: " << upper_solution << endl;
  } else {
    cout << "No words remaining... " << endl;
  }

  cout << endl;
  system("pause");
}