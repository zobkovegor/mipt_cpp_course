#include <iostream>

long long sum;

void Count(int**& arrays, int*& index, int*& argv, int index_count, const int all_index, long long prev_sum);

int main(int argc, char* argv[]) {
  int* arrays_length = new int[argc - 1];
  int** arrays = new int*[argc - 1];
  for (int i = 0; i != argc - 1; ++i) {
    arrays_length[i] = std::atoi(argv[i + 1]);
  }

  for (int i = 0; i != (argc - 1); ++i) {
    arrays[i] = new int[std::atoi(argv[i + 1])];
    for (int j = 0; j != std::atoi(argv[i + 1]); ++j) {
      std::cin >> arrays[i][j];
    }
  }
  for (int i = 0; i != std::atoi(argv[1]); ++i) {
    int* indexes = new int[argc - 1];
    indexes[0] = i;
    Count(arrays, indexes, arrays_length, 1, argc - 1, arrays[0][i]);
    delete []indexes;
  }
  for (int i = 0; i != (argc - 1); ++i) {
    delete  []arrays[i];
  }
  delete []arrays_length;
  delete []arrays;
  std::cout << sum << std::endl;
}

void Count(int**& arrays, int*& index, int*& argv, int index_count, const int all_index, long long prev_sum) {
  if (index_count == all_index) {
    sum += prev_sum;
    return;
  }
  int count_next = 0;
  for (int i = 0; i != argv[index_count]; ++i) {
    bool check = true;
    for (int j = 0; j != index_count; ++j) {
      if (i == index[j]) {
        check = false;
        break;
      }
    }
    if (check == true) {
      count_next++;
      index[index_count] = i;
      Count(arrays, index, argv, index_count + 1, all_index, prev_sum * arrays[index_count][i]);
    }
  }
  if (count_next == 0) {
    return;
  }
}