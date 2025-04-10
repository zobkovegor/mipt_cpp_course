#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <array>

const uint64_t cNum = std::pow(10, 9);

enum class sign {
  positive = 0,
  negative = 1
};

class BigInteger {
 private:
  std::vector<uint64_t> BigInt;

  friend std::ostream& operator<<(std::ostream& os, const BigInteger& currBigInt){ return os << currBigInt.toString(); }
  friend std::istream& operator>>(std::istream& in, BigInteger& currBigInt) {
    std::string s;
    in >> s;
    if (s == "-0") {
      s = "0";
    }
    currBigInt = s;
    return in;
  }

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

  friend bool FirsIsLessModul(const BigInteger& first, const BigInteger& second) {
    if (first.BigInt.size() != second.BigInt.size()) {
      return first.BigInt.size() < second.BigInt.size();
    }
    for (int currSegm = first.BigInt.size() - 1; currSegm >= 0; --currSegm) {
      if (first.BigInt[currSegm] != second.BigInt[currSegm]) {
        return first.BigInt[currSegm] < second.BigInt[currSegm];
      }
    }
    return 0;
  }

friend void Multiplicate(BigInteger& currBigInt, const BigInteger& other) {
    size_t newSize = currBigInt.BigInt.size() + other.BigInt.size();
    BigInteger newBigInt;
    newBigInt.BigInt.resize(newSize, 0);
    BigInteger remaind;
    remaind.BigInt.resize(newSize + 1, 0);
    for (size_t i = 0; i < other.BigInt.size(); ++i) {
        uint64_t carry = 0;
        uint64_t otherVal = other.BigInt[i];
        for (size_t j = 0; j < currBigInt.BigInt.size(); ++j) {
            uint64_t currVal = currBigInt.BigInt[j];
            uint64_t product = otherVal * currVal + carry + newBigInt.BigInt[i + j];
            newBigInt.BigInt[i + j] = product % cNum;
            carry = product / cNum;
        }
        if (carry > 0) {
            newBigInt.BigInt[i + currBigInt.BigInt.size()] += carry;
        }
    }
    newBigInt.Remove0();
    currBigInt = newBigInt;
}

  long BinSearch(const BigInteger& other) const {
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
    if (num < 0) {
      currSign = sign::negative;
    }
    BigInt.resize(1);
    if (static_cast<uint64_t>(std::abs(num)) >= cNum) {
      BigInt.push_back(std::abs(num) / cNum);
    }
    BigInt[0] = (std::abs(num) % cNum);
  }

  BigInteger& operator=(const BigInteger& other) & {
    Swap(other);
    return *this;
  }

  BigInteger& operator=(std::string& num) {
    if (num[0] == '-') {
      currSign = sign::negative;
    } else {
      currSign = sign::positive;
    }
    Assignment(num);
    return *this;
  }

  BigInteger& operator=(long long num) {
    if (num < 0) {
      currSign = sign::negative;
    }
    BigInt.resize(1);
    if (static_cast<uint64_t>(std::abs(num)) >= cNum) {
      BigInt.push_back(std::abs(num) / cNum);
    }
    BigInt[0] = (std::abs(num) % cNum);
    return *this;
  }

  std::string toString() const {
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

  friend bool operator==(const BigInteger& currBigInt, const BigInteger& other) {
    return (currBigInt.currSign == other.currSign) && !(FirsIsLessModul(currBigInt, other) || FirsIsLessModul(other, currBigInt));
  }

  friend bool operator!=(const BigInteger& currBigInt, const BigInteger& other) {
    return !(currBigInt == other);
  }

  friend bool operator<(const BigInteger& currBigInt, const BigInteger& other) {
    if ((other.BigInt.size() == 1 && other.BigInt[0] == 0) && currBigInt.BigInt.size() == 1 && currBigInt.BigInt[0] == 0) {
      return false;
    }
    if (currBigInt.currSign == sign::positive) {
      if (other.currSign == sign::negative) {
        return false;
      }
      return FirsIsLessModul(currBigInt, other);
    }
    if (other.currSign == sign::positive) {
      return true;
    }
    return FirsIsLessModul(other, currBigInt);
  }

  friend bool operator<=(const BigInteger& currBigInt, const BigInteger& other) {
    return (currBigInt < other) || (currBigInt == other);
  }

  friend bool operator>(const BigInteger& currBigInt, const BigInteger& other) {
    return (other < currBigInt);
  }

  friend bool operator>=(const BigInteger& currBigInt, const BigInteger& other) {
    return (other <= currBigInt);
  }

  friend BigInteger& operator+=(BigInteger& currBigInt, const BigInteger& other) {
    if (currBigInt.currSign == other.currSign) {
      currBigInt.SamesignPlus(other);
    } else if (currBigInt.currSign == sign::negative) {
      if (!FirsIsLessModul(currBigInt, other)) {
        currBigInt.FirstIsGreaterMinus(other);
      } else {
        BigInteger newBigInt = other;
        newBigInt.FirstIsGreaterMinus(currBigInt);
        currBigInt = newBigInt;
        currBigInt.currSign = sign::positive;
      }
    } else {
      if (!FirsIsLessModul(currBigInt, other)) {
        currBigInt.FirstIsGreaterMinus(other);
      } else {
        BigInteger newBigInt = other;
        newBigInt.FirstIsGreaterMinus(currBigInt);
        currBigInt = newBigInt;
        currBigInt.currSign = sign::negative;  
      } 
    }
    currBigInt.Remove0();
    if (currBigInt.BigInt[currBigInt.BigInt.size() - 1] == 0) {
      currBigInt.currSign = sign::positive;    
    }
    return currBigInt;
  }

  friend BigInteger operator+(BigInteger currBigInt, const BigInteger& other) {
    return (currBigInt += other);
  }  

  BigInteger& operator++() {
    *this += 1;
    return *this;
  }

  BigInteger operator++(int) {
    BigInteger newBigInt = *this;
    *this += 1;
    return newBigInt;
  }

  friend BigInteger& operator-=(BigInteger& currBigInt, const BigInteger& other) {
    if (currBigInt.currSign == sign::positive && other.currSign == sign::positive) {
      if (!FirsIsLessModul(currBigInt, other) || currBigInt == other) {
        currBigInt.FirstIsGreaterMinus(other);
      } else {
        BigInteger newBigInt = other;
        newBigInt.FirstIsGreaterMinus(currBigInt);
        currBigInt = newBigInt;
        currBigInt.currSign = sign::negative; 
      }
    } else if (currBigInt.currSign == sign::negative && other.currSign == sign::positive) {
      currBigInt.SamesignPlus(other);
    } else if (currBigInt.currSign == sign::negative && other.currSign == sign::negative) {
      if (!FirsIsLessModul(currBigInt, other)) {
        currBigInt.FirstIsGreaterMinus(other);
      } else {
        BigInteger newBigInt = other;
        newBigInt.FirstIsGreaterMinus(currBigInt);
        currBigInt = newBigInt;
        currBigInt.currSign = sign::positive;     
      }
    } else {
      currBigInt.SamesignPlus(other);
      currBigInt.currSign = sign::positive;
    }
    currBigInt.Remove0();
    if (currBigInt.BigInt.size() > 0 && currBigInt.BigInt[currBigInt.BigInt.size() - 1] == 0) {
      currBigInt.currSign = sign::positive;
    } 
    return currBigInt;
  }

  void Print() {
    for (size_t i = 0; i != BigInt.size(); ++i) {
      std::cout << BigInt[i] << " ";     }
  }

  friend BigInteger operator-(BigInteger currBigInt, const BigInteger& other) {
    return (currBigInt -= other);
  }

  BigInteger operator-() {
    if (*this == 0) {
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
  
  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }

  BigInteger operator--(int) {
    BigInteger newBigInt = *this;
    *this -= 1  ;
    return newBigInt;
  }

  BigInteger& operator*=(const BigInteger& other) {
    sign newSign = sign::positive;
    if (other.currSign != currSign) {
      newSign = sign::negative;
    }
    Multiplicate(*this, other);
    currSign = newSign;
    Remove0();
    if (BigInt.size() > 0 && BigInt[BigInt.size() - 1] == 0) {
      currSign = sign::positive;
    } 
    return *this;
  }
  
  friend BigInteger operator*(BigInteger currBigInteger, const BigInteger& other) {
    return currBigInteger *= other;
  }

  BigInteger& operator/=(const BigInteger& other) {
    if (FirsIsLessModul(*this, other) && *this != other) {
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
    newBigInt.currSign = sign::positive;
    for (size_t i = other.BigInt.size(); i != 0; --i) {
      newBigInt.BigInt[other.BigInt.size() - i] = BigInt[BigInt.size() - i];
    }
    size_t index = BigInt.size() - other.BigInt.size();
    if (FirsIsLessModul(*this, other)) {
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
      while (index > 0 && FirsIsLessModul(newBigInt, other)) {
        newBigInt *= cNum;
        if (newBigInt.BigInt.size() == 0) {
          newBigInt.BigInt.resize(1);
        }
        newBigInt.BigInt[0] = BigInt[index - 1];
        --index;
        ++cnt;
      }
      if (FirsIsLessModul(newBigInt, other)) {
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


  friend BigInteger operator/(BigInteger currBigInt, const BigInteger& other) {
    return currBigInt /= other;
  }

  BigInteger& operator%=(const BigInteger& other) {
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

  friend BigInteger operator%(BigInteger currBigInt, const BigInteger& other) {
    return currBigInt %= other;
  }

  explicit operator bool() const {
    return (*this) != 0;
  }

  ~BigInteger() = default;
};

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


class Rational {
 private:
  BigInteger numerator = 0;
  BigInteger denominator = 0;

  friend std::ostream& operator<<(std::ostream& os, const Rational& currRational){ 
    return os << currRational.toString(); 
  }

  void Reduction() {
    if (denominator == 1) {
      return;
    }
    sign currSign = numerator.currSign;
    BigInteger currNumerator = numerator;
    BigInteger currGcd = denominator;
    currGcd.currSign = sign::positive;
    currNumerator.currSign = sign::positive;
    if (currNumerator < currGcd) {
      std::swap(currNumerator, currGcd);
    }
    while (currGcd > 1 && currNumerator > 1) {
      currNumerator %= currGcd;
      std::swap(currNumerator, currGcd);
    }
    if (currGcd != 1) {
      numerator /= currNumerator;
      denominator /= currNumerator;
    }
    numerator.currSign = currSign;
  }
 public:
  Rational(const BigInteger& numerat, const BigInteger& denominat) : numerator(numerat), denominator(denominat) {
    SwapSign();
    Reduction();
  }
  Rational(const BigInteger& numerat) : numerator(numerat), denominator(1) {}
  Rational(const long long numerat, const long long denominat) : numerator(numerat), denominator(denominat) {
    SwapSign();
    Reduction();
  }
  Rational(const long long numerat) : numerator(numerat), denominator(1) {}
  Rational() : numerator(0), denominator(1) {}
  
  Rational& operator+=(const Rational& other) {
    numerator *= other.denominator;
    numerator += (other.numerator * denominator);
    denominator *= other.denominator;
    Reduction();
    return *this;
  }

  friend Rational operator+(Rational currRational, const Rational& other) {
    return currRational += other;
  }

  Rational& operator-=(const Rational& other) {
    numerator *= other.denominator;
    numerator -= (other.numerator * denominator);
    denominator *= other.denominator;
    Reduction();
    return *this;
  }
  
  friend Rational operator-(Rational currRational, const Rational& other) {
    return currRational -= other;
  }

  Rational operator-() {
    Rational newRational = *this;
    if (newRational.numerator.currSign == sign::positive) {
      newRational.numerator.currSign = sign::negative;
      return newRational;
    }
    newRational.numerator.currSign = sign::positive;
    return newRational;
  }

  Rational& operator*=(const Rational& other) {
    if (other.numerator == 0 || numerator == 0) {
      numerator = 0;
      numerator.currSign = sign::positive;
      denominator = 1;
      denominator.currSign = sign::positive;
    } else {
      numerator *= other.numerator;
      denominator *= other.denominator;
      SwapSign();
      Reduction();
      numerator.Remove0();
      denominator.Remove0();
    }
    return *this;
  }

  friend Rational operator*(Rational currRational, const Rational& other) {
    return currRational *= other;
  }

  Rational& operator/=(const Rational& other) {
    if (other.numerator == 0 || numerator == 0) {
      numerator = 0;
      numerator.currSign = sign::positive;
      denominator = 1;
      denominator.currSign = sign::positive;
    } else {
      numerator *= other.denominator;
      denominator *= other.numerator;
      SwapSign();
      Reduction();
      numerator.Remove0();
      denominator.Remove0();
    }
    return *this;
  }

  friend Rational operator/(Rational currRational, const Rational& other) {
    return currRational /= other;
  }

  void SwapSign() {
    if (denominator.currSign == sign::negative) {
      if (numerator.currSign == sign::positive) {
        numerator.currSign = sign::negative;
      } else {
        numerator.currSign = sign::positive;
      }
      denominator.currSign = sign::positive;
    }
  }

  std::string toString() const {
    if (denominator == 1) {
      return numerator.toString();
    }
    return numerator.toString() + '/' + denominator.toString();
  }

  friend bool operator==(const Rational& first, const Rational& second) {
    return (first.numerator == second.numerator) && (second.denominator == first.denominator);
  }

  friend bool operator!=(const Rational& first, const Rational& second) {
    return !(first == second);
  }

  friend bool operator<(const Rational& first, const Rational& second) {
    return (first.numerator * second.denominator) < (second.numerator * first.denominator);
  }

  friend bool operator<=(const Rational& first, const Rational& second) {
    return (first == second) || (first < second);
  }

  friend bool operator>(const Rational& first, const Rational& second) {
    return second < first;
  }

  friend bool operator>=(const Rational& first, const Rational& second) {
    return (second == first) || (second < first);
  }


  std::string asDecimal(size_t precision) const {
    std::string s;
    if (numerator.currSign == sign::negative) {
      s += '-';
    }
    BigInteger newBigInt = numerator / denominator;
    if (precision == 0) {
      return newBigInt.toString();
    }
    BigInteger newBigDecimal = numerator % denominator;
    newBigDecimal.currSign = sign::positive;
    s += newBigInt.toString() + ".";
    for (size_t i = 0; i != precision; ++i) {
      newBigDecimal *= 10;
    } 
    newBigDecimal /= denominator;
    for (size_t i = 0 ; i != precision - newBigDecimal.toString().size(); ++i) {
      s += "0";
    }
    s += newBigDecimal.toString();
    return s;
  }

  explicit operator double() const {
    return std::stod(this->asDecimal(10));
  }
};

std::istream& operator>>(std::istream& in, Rational& other) {
  BigInteger s;
  in >> s;
  other = s;
  return in;
}
