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
  // size_t word_count = word_list.size();

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
  cout << "Done!" << endl;

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

bool Match(uint8_t encoded_filter, string query, string target, unordered_map<string, int> word_map, uint8_t **filters){
  return filters[word_map[query]][word_map[target]] == encoded_filter;
}

vector<string> FilterWords(uint8_t encoded_filter, string query,
                           vector<string> word_list,
                           unordered_map<string, int> word_map,
                           uint8_t **filters) {
  for (vector<string>::iterator it = word_list.begin();
       it != word_list.end();) {
    if(!Match(encoded_filter, query, *it, word_map, filters)){
      it = word_list.erase(it);
    } else {
      ++it;
    }
  }
  return word_list;
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

int main() {
  string line;
  ifstream list_file("wordlist.txt");
  string out_filter_file_name = "filter_list.txt";

  vector<string> word_list;
  uint8_t **filters;
  size_t word_count;
  unordered_map<string, int> word_map;

  const bool recompute_filters = true;

  if (list_file.is_open()) {
    int index = 0;
    while (getline(list_file, line) 
    //&& index < 4
    ) {
      word_list.push_back(line);
      word_map[line] = index;
      index++;
    }
  }

  word_count = word_list.size();
  // word_count = 4;

  cout << "Read " << FormatWithCommas(word_list.size()) << " words" << endl;

  if (recompute_filters) {
    // Warning - may take a long time to run ComputeFilters(word_list)
    filters = ComputeFilters(word_list, out_filter_file_name, word_count);
  } else {
    ifstream filter_file(out_filter_file_name.c_str());

    uint8_t *filters_pointer =
      (uint8_t *)malloc(sizeof(uint8_t) * word_count * word_count);

    filters = (uint8_t **)malloc(sizeof(uint8_t *) * word_count);
    for (int i = 0; i < word_count; i++) {
      filters[i] = &filters_pointer[i * word_count];
    }

    filter_file.read((char *)filters_pointer,
                     sizeof(uint8_t) * word_count * word_count);
    cout << "Read "
         << FormatWithCommas(sizeof(uint8_t) * word_count * word_count)
         << " filters from " << out_filter_file_name << endl;
  }

  // for safety make word_count small
  // word_count = 4;
  for (int i = 0; i < word_count; i++) {
    for (int j = 0; j < word_count; j++) {
      cout << word_list[i] << " - " << word_list[j] << " = "
           << DecodeFilter(filters[i][j]) << endl;
    }
  }

  // cout << endl;
  // string query = "aioli";
  // string target = "aalii";
  // string filter = "21012";
  // string result = Match(EncodeFilter(filter), query, target, word_map, filters)
  //                     ? "TRUE"
  //                     : "FALSE";
  // cout << query << " - " << target << " = " << filter << " "
  //      << result << endl;

  // cout << "Word " << query << " - " << filter << " has "
  //      << CountMatches(EncodeFilter(filter), query, word_list, word_map,
  //                      filters)
  //      << " matches" << endl;

  // cout << CreateFilter(query, target) << endl;

  cout << endl;
}