#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

const uint64_t cNum = std::pow(10, 9);

enum class sign {
  positive = 0,
  negative = 1
};

class BigInteger {
 private:
  std::vector<uint64_t> BigInt;

  void Assignment(const std::string& num) {
    BigInt.resize(0);
    if (num.size() == 0) {
      currSign = sign::positive;
      BigInt.push_back(0);
      return;
    }
    int lastInd = 0;
    if (num[0] == '-') {
      currSign = sign::negative;
      lastInd = 1;
    } else {
      currSign = sign::positive;
    }
    while (num[lastInd] == '0') {
      lastInd++;
    }
    if (lastInd == static_cast<int>(num.size())) {
      BigInt.push_back(0);
      currSign = sign::positive;
      return;
    }
    int currNumSize = static_cast<int>(num.size()) - 1;
    while (currNumSize - 9 >= lastInd - 1) {
      BigInt.push_back(std::stoi(num.substr(currNumSize - 8, 9)));
      currNumSize -= 9;
    }
    if (currNumSize >= lastInd) {
      uint64_t BigIntSeg = 0;
      int currDegree = std::pow(10, currNumSize - lastInd);
      for (int currElem = lastInd; currElem <= currNumSize; ++currElem) {
        BigIntSeg += static_cast<uint64_t>(num[currElem] - '0') * currDegree;
        currDegree /= 10;
      }
      BigInt.push_back(BigIntSeg);
    }
  }

  void SamesignPlus(const BigInteger& other) {
    size_t ind = other.BigInt.size();
    if (other.BigInt.size() > BigInt.size()) {
      ind = BigInt.size();
      for (size_t segm = ind; segm != other.BigInt.size(); ++segm) {
        BigInt.push_back(other.BigInt[segm]);
      }
    }
    uint64_t remaind = 0;
    for (size_t currSegm = 0; currSegm < ind; ++currSegm) {
      uint64_t num = BigInt[currSegm] + other.BigInt[currSegm] + remaind;
      BigInt[currSegm] = num % cNum;     
      remaind = num / cNum;
    }
    while (remaind != 0 && ind < BigInt.size()) {
      uint64_t num = BigInt[ind] + remaind;
      BigInt[ind] = num % cNum;
      remaind = num / cNum;
      ++ind;
    }
    if (remaind != 0) {
      BigInt.push_back(remaind);
    }
  }

  void FirstIsGreaterMinus(const BigInteger& other) {
    for (size_t currSegm = 0; currSegm < other.BigInt.size(); ++currSegm) {
      if (BigInt[currSegm] < other.BigInt[currSegm]) {
        size_t indOfTake = currSegm + 1;
        while(true) {
          if (BigInt[indOfTake] != 0) {
            BigInt[indOfTake] -= 1;
            break;
          } else {
            BigInt[indOfTake] = cNum - 1;
          }
          ++indOfTake;
        }
        BigInt[currSegm] += cNum;
      }
      BigInt[currSegm] -= other.BigInt[currSegm];  
    }
  }

  bool FirsIsLessModul(const BigInteger& second) const {
    if (BigInt.size() != second.BigInt.size()) {
      return BigInt.size() < second.BigInt.size();
    }
    for (int currSegm = BigInt.size() - 1; currSegm >= 0; --currSegm) {
      if (BigInt[currSegm] != second.BigInt[currSegm]) {
        return BigInt[currSegm] < second.BigInt[currSegm];
      }
    }
    return 0;
  }

  bool isEqual(const BigInteger& second) const {
    if (BigInt.size() != second.BigInt.size()) {
      return false;
    }
    for (int currSegm = BigInt.size() - 1; currSegm >= 0; --currSegm) {
      if (BigInt[currSegm] != second.BigInt[currSegm]) {
        return false;
      }
    }
    return true;
  }

  friend void Multiplicate(BigInteger& currBigInt, const BigInteger& other) {
    BigInteger newBigInt;
    newBigInt.BigInt.resize(currBigInt.BigInt.size() + other.BigInt.size() - 1);
    BigInteger remaind = 0;
    remaind.BigInt.resize(currBigInt.BigInt.size() + other.BigInt.size());
    for (size_t currFirstSegm = 0; currFirstSegm != other.BigInt.size(); ++currFirstSegm) {
      for (size_t currSecondSegm = 0; currSecondSegm != currBigInt.BigInt.size(); ++currSecondSegm) {
        uint64_t currSum = (other.BigInt[currFirstSegm] * currBigInt.BigInt[currSecondSegm]);
        newBigInt.BigInt[currFirstSegm + currSecondSegm] += (currSum) % cNum;
        if (newBigInt.BigInt[currFirstSegm + currSecondSegm] > cNum) {
          remaind.BigInt[currFirstSegm + currSecondSegm] += (newBigInt.BigInt[currFirstSegm + currSecondSegm] - newBigInt.BigInt[currFirstSegm + currSecondSegm] % cNum);
          size_t ind = currFirstSegm + currSecondSegm;
          while (remaind.BigInt[ind] > cNum) {
            if (remaind.BigInt.size() - 1 <= ind) {
              remaind.BigInt.push_back(0);
            }
            remaind.BigInt[ind + 1] += (remaind.BigInt[ind] / cNum);
            remaind.BigInt[ind] = remaind.BigInt[ind] % cNum;
            ++ind;
          }
          newBigInt.BigInt[currFirstSegm + currSecondSegm] = newBigInt.BigInt[currFirstSegm + currSecondSegm] % cNum;
        }
        remaind.BigInt[currFirstSegm + currSecondSegm] += (currSum - currSum % cNum);
        size_t ind = currFirstSegm + currSecondSegm;
        while (remaind.BigInt[ind] > cNum) {
          if (remaind.BigInt.size() - 1 <= ind) {
            remaind.BigInt.push_back(0);
          }
          remaind.BigInt[ind + 1] += (remaind.BigInt[ind] / cNum);
          remaind.BigInt[ind] = remaind.BigInt[ind] % cNum;
          ++ind;
        }
      }
    }
    if (remaind.BigInt[remaind.BigInt.size() - 1] == 0) {
      remaind.BigInt.pop_back();
    }
    newBigInt += remaind;
    currBigInt = newBigInt;
  }

  long BinSearch(const BigInteger& other) const;

  void Swap(const BigInteger& other) {
    BigInteger NewBigInt = other;
    std::swap(BigInt, NewBigInt.BigInt);
    std::swap(currSign, NewBigInt.currSign);
  }
 public:
  sign currSign = sign::positive;

  void Remove0() {
    if (BigInt.size() == 1) {
      return;
    }
    for (int currSegm = BigInt.size() - 1; currSegm >= 1; --currSegm) {
      if (BigInt[currSegm] != 0) {
        break;
      }
      BigInt.erase(BigInt.begin() + currSegm);
    }
  }

  BigInteger(const std::string& num) {
    Assignment(num);
  }

  BigInteger() {
    BigInt.push_back(0);
  }

  BigInteger(const BigInteger& other) : BigInt(other.BigInt), currSign(other.currSign) {
  }  

  BigInteger(long long num) {
    std::string str = std::to_string(num);
    Assignment(str);
  }

  BigInteger& operator=(const BigInteger& other) & {
    Swap(other);
    return *this;
  }

  friend bool operator<(const BigInteger& currBigInt, const BigInteger& other);

  friend bool operator==(const BigInteger& currBigInt, const BigInteger& other);

  std::string toString() const;

  BigInteger& operator+=(const BigInteger& other);

  BigInteger& operator++();

  BigInteger operator++(int);

  BigInteger& operator-=(const BigInteger& other);

  BigInteger& operator--();

  BigInteger operator--(int);

  BigInteger& operator*=(const BigInteger& other);

  BigInteger operator-();

  BigInteger& operator/=(const BigInteger& other);

  BigInteger& operator%=(const BigInteger& other);

  explicit operator bool() const;

  size_t size() const {
    return BigInt.size();
  }

  const uint64_t& operator[] (size_t i) const {
    return BigInt[i];
  }

  uint64_t& operator[] (size_t i) {
    return BigInt[i];
  }

  ~BigInteger() = default;
};

std::string BigInteger::toString() const {
  std::string strBigInteger;
  if (currSign == sign::negative) {
    strBigInteger.push_back('-');
  }
  int currPos = BigInt.size() - 1;
  while(currPos >= 0 && BigInt[currPos] == 0) {
    --currPos;
  }
  if (currPos < 0) {
    return "0";
  } else if(currPos == static_cast<int>(BigInt.size() - 1)) {
    strBigInteger += std::to_string(BigInt[BigInt.size() - 1]);
    --currPos;
  } 
  for (int currSegm = currPos; currSegm >= 0; --currSegm) {
    for (size_t j = 0; j != 9 - std::to_string(BigInt[currSegm]).size(); ++j) {
      strBigInteger.push_back('0'); 
    }
    strBigInteger += std::to_string(BigInt[currSegm]);
  }
  return strBigInteger;
}

BigInteger& BigInteger::operator+=(const BigInteger& other) {
  if (currSign == other.currSign) {
    SamesignPlus(other);
  } else if (currSign == sign::negative) {
    if (!FirsIsLessModul(other)) {
      FirstIsGreaterMinus(other);
    } else {
      BigInteger newBigInt = other;
      newBigInt.FirstIsGreaterMinus(*this);
      (*this) = newBigInt;
      currSign = sign::positive;
    }
  } else {
    if (!FirsIsLessModul(other)) {
      FirstIsGreaterMinus(other);
    } else {
      BigInteger newBigInt = other;
      newBigInt.FirstIsGreaterMinus(*this);
      (*this) = newBigInt;
      currSign = sign::negative;  
    } 
  }
  Remove0();
  if (BigInt[BigInt.size() - 1] == 0) {
    currSign = sign::positive;    
  }
  return (*this);
}

BigInteger operator+(BigInteger currBigInt, const BigInteger& other) {
  return (currBigInt += other);
}  

BigInteger& BigInteger::operator++() {
  *this += 1;
  return *this;
}

BigInteger BigInteger::operator++(int) {
  BigInteger newBigInt = *this;
  *this += 1;
  return newBigInt;
}

BigInteger& BigInteger::operator-=(const BigInteger& other) {
  if (currSign == sign::positive && other.currSign == sign::positive) {
    if (!FirsIsLessModul(other)) {
      FirstIsGreaterMinus(other);
    } else {
      BigInteger newBigInt = other;
      newBigInt.FirstIsGreaterMinus((*this));
      (*this) = newBigInt;
      currSign = sign::negative; 
    }
  } else if (currSign == sign::negative && other.currSign == sign::positive) {
    SamesignPlus(other);
  } else if (currSign == sign::negative && other.currSign == sign::negative) {
    if (!FirsIsLessModul(other)) {
      FirstIsGreaterMinus(other);
    } else {
      BigInteger newBigInt = other;
      newBigInt.FirstIsGreaterMinus(*this);
      (*this) = newBigInt;
      currSign = sign::positive;     
    }
  } else {
    SamesignPlus(other);
    currSign = sign::positive;
  }
  Remove0();
  if (BigInt[BigInt.size() - 1] == 0) {
    currSign = sign::positive;
  } 
  return (*this);
}

BigInteger operator-(BigInteger currBigInt, const BigInteger& other) {
  return (currBigInt -= other);
}

BigInteger& BigInteger::operator--() {
  *this -= 1;
  return *this;
}

BigInteger BigInteger::operator--(int) {
  BigInteger newBigInt = *this;
  *this -= 1  ;
  return newBigInt;
}

BigInteger BigInteger::operator-() {
  if (BigInt.size() == 1 && BigInt[0] == 0) {
    return 0;
  }
  BigInteger newBigInt = *this;
  if (newBigInt.currSign == sign::positive) {
    newBigInt.currSign = sign::negative;
    return newBigInt;
  }
  newBigInt.currSign = sign::positive;
  return newBigInt;
}

BigInteger& BigInteger::operator*=(const BigInteger& other) {
  sign newSign = sign::positive;
  if (other.currSign != currSign) {
    newSign = sign::negative;
  }
  Multiplicate(*this, other);
  currSign = newSign;
  Remove0();
  if (BigInt[BigInt.size() - 1] == 0) {
    currSign = sign::positive;
  } 
  return *this;
}

BigInteger operator*(const BigInteger& currBigInt, const BigInteger& other) {
  BigInteger newBigInt = currBigInt;
  return newBigInt *= other;
}

bool operator==(const BigInteger& currBigInt, const BigInteger& other) {
  return (currBigInt.currSign == other.currSign) && (currBigInt.isEqual(other));
}

bool operator!=(const BigInteger& currBigInt, const BigInteger& other) {
  return !(currBigInt == other);
}

bool operator<(const BigInteger& currBigInt, const BigInteger& other) {
  if ((other.size() == 1 && other[0] == 0) && currBigInt.size() == 1 && currBigInt[0] == 0) {
    return false;
  }
  if (currBigInt.currSign == sign::positive) {
    if (other.currSign == sign::negative) {
      return false;
    }
    return currBigInt.FirsIsLessModul(other);
  }
  if (other.currSign == sign::positive) {
    return true;
  }
  return other.FirsIsLessModul(currBigInt);
}

bool operator<=(const BigInteger& currBigInt, const BigInteger& other) {
  return !(other < currBigInt);
}

bool operator>(const BigInteger& currBigInt, const BigInteger& other) {
  return (other < currBigInt);
}

bool operator>=(const BigInteger& currBigInt, const BigInteger& other) {
  return !(currBigInt < other);
}

long BigInteger::BinSearch(const BigInteger& other) const {
  long left = 1;
  long right = cNum;
  while (right - left > 1) {
    long mid = (left + right) / 2;
    if (mid * other > (*this)) {
      right = mid;
    } else {
      left = mid;
    }
  }
  if (left * other > (*this)) {
    return left - 1;
  }
  return left;
}

BigInteger& BigInteger::operator/=(const BigInteger& otherBigIntger) {
  BigInteger other = otherBigIntger;
  if (FirsIsLessModul(other) && *this != other) {
    *this = 0;
    return *this;
  }
  if (currSign != other.currSign) {
    currSign = sign::negative;
  } else {
    currSign = sign::positive;
  }
  std::string answer;
  BigInteger newBigInt = other;
  other.currSign = sign::positive;
  newBigInt.currSign = sign::positive;
  for (size_t i = other.BigInt.size(); i != 0; --i) {
    newBigInt.BigInt[other.BigInt.size() - i] = BigInt[BigInt.size() - i];
  }
  size_t index = BigInt.size() - other.BigInt.size();
  if (FirsIsLessModul(other)) {
    newBigInt *= cNum;
    newBigInt.BigInt[0] = BigInt[index - 1];
    --index;
  }
  long ind = newBigInt.BinSearch(other);
  answer += std::to_string(ind);
  if (other.currSign == sign::negative) {
    newBigInt += ind * other;
  } else {
    newBigInt -= ind * other;
  }
  while (index != 0) {
    int cnt = 0;
    while (index > 0 && newBigInt.FirsIsLessModul(other)) {
      newBigInt *= cNum;
      if (newBigInt.BigInt.size() == 0) {
        newBigInt.BigInt.resize(1);
      }
      newBigInt.BigInt[0] = BigInt[index - 1];
      --index;
      ++cnt;
    }
    if (newBigInt.FirsIsLessModul(other)) {
      for (int j = 0; j != cnt; ++j) {
        for (int i = 0; i != 9; ++i) {
          answer.push_back('0');
        }
        newBigInt.BigInt.pop_back();
      }
      break;
    }
    long ind = newBigInt.BinSearch(other);
    for (int i = 0; i != cnt * 9 - static_cast<int>(std::to_string(ind).size()); ++i) {
      answer += '0';
    }
    answer += std::to_string(ind);
    newBigInt -= ind * other;
  }
  BigInt = BigInteger(answer).BigInt;
  if (currSign != other.currSign) {
    currSign = sign::negative;
  }
  Remove0();
  if (BigInt[BigInt.size() - 1] == 0) {
    currSign = sign::positive;
  } 
  return *this;
}

BigInteger operator/(const BigInteger& currBigInt, const BigInteger& other) {
  BigInteger newBigInt = currBigInt;
  return newBigInt /= other;
}

BigInteger& BigInteger::operator%=(const BigInteger& other) {
  BigInteger newBigInt = *this;
  newBigInt.currSign = sign::positive;
  newBigInt /= other;
  if (currSign == sign::negative) {
    *this += newBigInt * other;  
  } else {
    *this -= newBigInt * other;
  }
  Remove0();
  return *this;
}

BigInteger operator%(const BigInteger& currBigInt, const BigInteger& other) {
  BigInteger newBigInt = currBigInt;
  return newBigInt %= other;
}

BigInteger::operator bool() const {
  return (*this) != 0;
}

std::ostream& operator<<(std::ostream& os, const BigInteger& currBigInt){ return os << currBigInt.toString(); }

std::istream& operator>>(std::istream& in, BigInteger& currBigInt) {
  std::string s;
  in >> s;
  if (s == "-0") {
    s = "0";
  }
  currBigInt = s;
  return in;
}

BigInteger operator "" _bi(const char* s) {
  std::string str = s;
  BigInteger newBigInt = str;
  return newBigInt;
}

BigInteger operator "" _bi(unsigned long long s) {
  std::string str = std::to_string(s);
  BigInteger newBigInt = str;
  return newBigInt;
}

BigInteger operator "" _bi(const char* s, size_t n) {
  std::string str = "";
  for (size_t i = 0; i != n; ++i) {
    str += s[i];
  }
  BigInteger newBigInt = str;
  return newBigInt;
}