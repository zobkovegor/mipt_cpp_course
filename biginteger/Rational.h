#include "BigInteger.h"

class Rational {
  private:
   BigInteger numerator = 0;
 
   BigInteger denominator = 0;
 
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
 
   Rational& operator+=(const Rational& other);
 
   Rational& operator-=(const Rational& other);
 
   Rational operator-();
 
   Rational& operator*=(const Rational& other);
 
   Rational& operator/=(const Rational& other);
 
   void SwapSign();
 
   std::string toString() const;
 
   friend bool operator==(const Rational& first, const Rational& second);
 
   friend bool operator<(const Rational& first, const Rational& second);
 
   std::string asDecimal(size_t precision) const;
 
   explicit operator double() const;
 };
 
 void Rational::SwapSign() {
   if (denominator.currSign == sign::negative) {
     if (numerator.currSign == sign::positive) {
       numerator.currSign = sign::negative;
     } else {
       numerator.currSign = sign::positive;
     }
     denominator.currSign = sign::positive;
   }
 }
 
 Rational& Rational::operator+=(const Rational& other) {
   numerator *= other.denominator;
   numerator += (other.numerator * denominator);
   denominator *= other.denominator;
   Reduction();
   return *this;
 }
 
 Rational operator+(Rational currRational, const Rational& other) {
   return currRational += other;
 }
 
 Rational& Rational::operator-=(const Rational& other) {
   numerator *= other.denominator;
   numerator -= (other.numerator * denominator);
   denominator *= other.denominator;
   Reduction();
   return *this;
 }
 
 Rational operator-(Rational currRational, const Rational& other) {
   return currRational -= other;
 }
 
 Rational Rational::operator-() {
   Rational newRational = *this;
   if (newRational.numerator.currSign == sign::positive) {
     newRational.numerator.currSign = sign::negative;
     return newRational;
   }
   newRational.numerator.currSign = sign::positive;
   return newRational;
 }
 
 Rational& Rational::operator*=(const Rational& other) {
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
 
 Rational operator*(Rational currRational, const Rational& other) {
   return currRational *= other;
 }
 
 Rational& Rational::operator/=(const Rational& other) {
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
 
 Rational operator/(Rational currRational, const Rational& other) {
   return currRational /= other;
 }
 
 bool operator==(const Rational& first, const Rational& second) {
   return (first.numerator == second.numerator) && (second.denominator == first.denominator);
 }
 
 bool operator!=(const Rational& first, const Rational& second) {
   return !(first == second);
 }
 
 bool operator<(const Rational& first, const Rational& second) {
   return (first.numerator * second.denominator) < (second.numerator * first.denominator);
 }
 
 bool operator<=(const Rational& first, const Rational& second) {
   return !(second < first);
 }
 
 bool operator>(const Rational& first, const Rational& second) {
   return second < first;
 }
 
 bool operator>=(const Rational& first, const Rational& second) {
   return !(first < second);
 }
 
 std::string Rational::toString() const {
   if (denominator == 1) {
     return numerator.toString();
   }
   return numerator.toString() + '/' + denominator.toString();
 }
 
 std::string Rational::asDecimal(size_t precision) const {
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
 
 Rational::operator double() const {
   return std::stod(this->asDecimal(5));
 }
 
 std::ostream& operator<<(std::ostream& os, const Rational& currRational){ 
   return os << currRational.toString(); 
 }