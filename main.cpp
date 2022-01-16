#include <fstream>
#include <iostream>
#include <string>
#include <vector>


using namespace std;

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

int main() {
  string line;
  ifstream list_file("wordlist.txt");
  string out_filter_file_name = "filter_list.txt";
  vector<string> word_list;
  uint8_t **filters;
  size_t word_count;
  const bool recompute_filters = false;

  if (list_file.is_open()) {
    while (getline(list_file, line)) {
      word_list.push_back(line);
    }
  }

  word_count = word_list.size();
  // word_count = 4;

  cout << "Read " << word_list.size() << " words" << endl;

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
  }

  // for safety make word_count small
  // word_count = 4;
  // for (int i = 0; i < word_count; i++) {
  //   for (int j = 0; j < word_count; j++) {
  //     cout << word_list[i] << " - " << word_list[j] << " = "
  //          << DecodeFilter(filters[i][j]) << endl;
  //   }
  // }


  // string filter = "21022";
  // uint8_t encoded_filter = EncodeFilter(filter);
  // cout << "Encoding of " << filter << " is " << encoded_filter << endl;
  // cout << "Decoding of " << encoded_filter << " is " << DecodeFilter(encoded_filter) << endl;

  // out_file << encoded_filter;

  cout << endl;
}