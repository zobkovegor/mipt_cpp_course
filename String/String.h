#include <algorithm>
#include <iostream>
#include <cstring>
#include <string>


class String {

private:
  size_t count_elem = 0;
  size_t cps = 0;
  char* s = nullptr;

  String(size_t n): count_elem(n), cps(n + 1), s(new char[cps]) {
    s[n] = '\0';
  } 

  void Swap(String& other) {
    std::swap(count_elem, other.count_elem);
    std::swap(cps, other.cps);
    std::swap(s, other.s);
  }

  void Newcps(size_t new_cps) {
    char* arr = new char[new_cps];
    memcpy(arr, s, count_elem);
    delete[] s;
    s = arr;
  }

public:
  // Constructors
  String() = default;

  String(size_t count, char c): String(count) {
    std::fill(s, s + count_elem, c);
  }

  String(const char* str) {
    size_t count = 0;
    while (str[count] != '\0' && str[count + 1] != '\n') {
      count++;
    }
    count_elem = count;
    cps = count + 1;
    s = new char[cps];
    memcpy(s, str, count_elem);
    s[count_elem] = '\0';
  }

  String(const String& other): String(other.count_elem) {
    memcpy(s, other.s, count_elem);
  }

  // operators
  String& operator=(String other) & {
    Swap(other);
    return *this;
  }

  char& operator[](size_t i) { return s[i]; }
  
  const char& operator[](size_t i) const { return s[i]; }

  String& operator+=(const String& other) {
    if (cps <= count_elem + other.count_elem) {
      while (cps <= count_elem + other.count_elem) {
        cps *= 2;
      }
      Newcps(cps);
    }
    memcpy(s + count_elem, other.s, other.count_elem);
    count_elem += other.count_elem;
    s[count_elem] = '\0';
    return *this;
  }

  String& operator+=(char other) {
    push_back(other);
    return *this;
  }
  // Destructor
  ~String() {
    delete[] s;
  }

  // Methods
  size_t length() const {
    return count_elem;
  }
  
  size_t size() const {
    return count_elem;
  }
  
  size_t capacity() const {
    return (cps);
  }

  void push_back(char new_elem) {
    count_elem++;
    if (cps <= count_elem) {
      cps *= 2;
      Newcps(cps);
    } 
    s[count_elem - 1] = new_elem;
    s[count_elem] = '\0';
  }

  void pop_back() {
    count_elem--;
    s[count_elem] = '\0';
  }

  char& front() {
    return s[0];
  }

  const char& front() const {
    return s[0];
  }

  char& back() {
    return s[count_elem - 1];
  }
  
  const char& back() const {
    return s[count_elem - 1];
  }


  size_t find(const String& substr) const {
    if (substr.length() > length()) {
      return length();
    }
    for (size_t i = 0; i != count_elem - substr.count_elem; ++i) {
      if (strncmp(this->s + i , substr.s, substr.count_elem) == 0) {
        return i;
      }
    }
    return length();
  }

  size_t rfind(const String& substr) const {
    if (substr.length() > length()) {
      return length();
    }

    for (int i = count_elem - substr.count_elem; i >= 0; --i) {
      if (strncmp(this->s + i , substr.s, substr.count_elem) == 0) {
        return i;
      }
    }
    return length();
  }

  bool isEqual(const String& other) const {
    if (length() == other.length()) {
      return memcmp(s, other.data(), length()) == 0;
    }
    return false;
  }

  bool compare(const String& other) const {
    size_t n = std::min(other.length(), length());
    if (memcmp(s, other.data(), n) < 0) {
      return true;
    } else if (memcmp(s, other.data(), n) > 0) {
      return false;
    }
    if (other.length() > length()) {
      return true;
    }
    return false;
  }

  String substr(size_t start, size_t count) const {
    String new_string("");
    if (start > count_elem) {
      return new_string;
    }
    memcpy(new_string.s, s + start, std::min(count_elem - start, count));
    new_string.count_elem = std::min(count_elem - start, count);
    return new_string;
  }

  bool empty() const { return count_elem == 0; }

  void clear() {
    count_elem = 0;
  }

  void shrink_to_fit() {
    cps = count_elem;
    Newcps(cps);
  }

  char* data() {
    return s;
  }

  const char* data() const {
    return s;
  }

};

bool operator==(const String& string, const String& other) {
  return string.isEqual(other);
}

bool operator==(const String& string, const char*& s) {
  return string.isEqual(s);
}

bool operator!=(const String& string, const String& other) {
  return !(string == other);
}

bool operator!=(const String& string, const char*& s) {
  return !(string == s);
}

bool operator<(const String& string, const String& other) {
  return string.compare(other);
}

bool operator<(const String& string, const char*& s) {
  return string.compare(s);
}

bool operator>(const String& string, const String& other) {
  return other < string;
}

bool operator>(const String& string, const char*& s) {
  return s < string;
}

bool operator>(const char*& s, const String& string) {
  return string < s;
}

bool operator<=(const String& string, const String& other) {
  return !(string > other);
}

bool operator<=(const String& string, const char*& s) {
  return !(string > s);
}

bool operator<=(const char*& s, const String& string) {
  return !(s > string);
}

bool operator>=(const String& string, const String& other) {
  return !(string < other);
}

bool operator>=(const String& string, const char*& s) {
  return !(string < s);
}

bool operator>=(const char*& s, const String& string) {
  return !(s < string);
}

String operator+(const String& s, const String& other) {
    String new_string = s;
    new_string += other;
    return new_string;
}

String operator+(const String& s, const char& other) {
    String new_string = s;
    new_string += other;
    return new_string;
}

String operator+(const char& other, const String& s) {
    String new_string(1, other);
    new_string += s;
    return new_string;
}

std::ostream& operator<<(std::ostream &os, const String& string){
  for (size_t elem = 0; elem != string.length(); ++elem) {
    os << string.data()[elem];
  }
  return os;
}
std::istream& operator>>(std::istream& in, String& string) {
  char* s = new char;
  in >> s;
  string = s;
  delete s;
  return in;
}