#include "BigInteger.h"

#define MID(a, b) ((a+b)/2)
#define POW(a) (a*a)

template<size_t res, size_t l = 1, size_t r = res>
struct SQRT;

template<size_t res, size_t r>
struct SQRT<res, r, r> {
  static constexpr size_t value = r;
};

template <size_t res, size_t l, size_t r>
struct SQRT {
  static constexpr size_t value = SQRT<res, (POW(MID(r, l)) >= res ? 
                  l : MID(r, l)+1), (POW(MID(r, l)) >= res ? MID(r, l) : r)>::value;
};

template<size_t N, size_t D>
struct is_primeHelper {
  static constexpr bool value = N % D == 0 && N != D ? false : is_primeHelper<N, D - 1>::value;
};

template<const size_t N>
struct is_primeHelper<N, 1> {
  static constexpr bool value = true;
};

template <size_t N>
struct is_prime {
  static constexpr bool value = is_primeHelper<N, SQRT<N>::value>::value;
};

template <>
struct is_prime<1> {
  static constexpr bool value = true;;
};

template <size_t N>
class Residue {
 private:
 static size_t gcd (size_t a, size_t b, int& x, int& y) {
    if (a == 0) {
      x = 0; y = 1;
      return b;
    }
    int x1, y1;
    size_t d = gcd(b%a, a, x1, y1);
    x = y1 - (b / a) * x1;
    y = x1;
    return d;
  } 
  size_t elem_ = 0;
 public: 
  friend std::ostream& operator<<(std::ostream& os, const Residue& res){ return os << res.elem_; }

  friend std::istream& operator>>(std::istream& in, Residue& res){ return in >> res.elem_; }

  Residue(int x): elem_(x >= 0 ? x % N : -(std::abs(x) % N) + N) {}

  Residue() = default;

  Residue<N>& operator+=(const Residue<N>& other) {
    elem_ = (elem_ + other.elem_) % N;
    return *this;
  }

  Residue<N>& operator-=(const Residue<N>& other) {
    elem_ = (static_cast<int>(elem_) - static_cast<int>(other.elem_)) >= 0 ?
            (static_cast<int>(elem_) - static_cast<int>(other.elem_)) % N : 
            -(std::abs(static_cast<int>(elem_) - static_cast<int>(other.elem_)) % N) + N;
    return *this;
  }

  Residue<N>& operator*=(const Residue<N>& other) {
    elem_ = (elem_ * other.elem_) % N;
    return *this;
  }

  Residue<N>& operator/=(const Residue<N>& other) {
    for (size_t i = 1; i != N; ++i) {
      if ((i * other.elem_) % N == elem_) {
        elem_ = i;
        return *this;
      }
    }
    elem_ = 0;
    return *this;
  }
  bool operator==(const Residue<N>& other) const {
    return elem_ == other.elem_;
  }
  bool operator!=(const Residue<N>& other) const {
    return elem_ != other.elem_;
  }
  explicit operator int() const {
    return static_cast<int>(elem_); 
  }
};
template <size_t N>
Residue<N> operator+(const Residue<N>& curr, const Residue<N>& other) {
  Residue<N> new_elem = curr;
  new_elem += other;
  return new_elem;
}

template <size_t N>
Residue<N> operator-(const Residue<N>& curr, const Residue<N>& other) {
  Residue new_elem = curr;
  new_elem -= other;
  return new_elem;
}

template <size_t N>
Residue<N> operator*(const Residue<N>& curr, const Residue<N>& other) {
  Residue<N> new_elem = curr;
  new_elem *= other;
  return new_elem;
}

template <size_t N>
Residue<N> operator/(const Residue<N>& curr, const Residue<N>& other) {
  Residue<N> new_res = curr;
  new_res /= other;
  return new_res;
}
template<size_t M, size_t N, typename Field = Rational>
class Matrix {
 private:

  void Print() const {
    for (size_t i = 0; i < M; ++i) {
      for (size_t j = 0; j < N; ++j) {
        std::cout << elems[i][j] << " ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  void Swap(Matrix other) {
    std::swap(elems, other.elems);
  }

  std::pair<size_t, size_t> FirstNoZero(size_t curr_i, size_t curr_j) const {
    for (size_t j = curr_j; j < N; ++j) {
      for (size_t i = curr_i; i < M; ++i) {
        if (elems[i][j] != Field(0)) {
          return {i, j};
        }
      }
    }
    return {M + 1, N + 1};
  }

    Matrix<M, N, Field> downTriangleWithOutUnity(Field& coefficient) const {
      Matrix<M, N, Field> new_matrix = *this; 
      for (size_t i = 0, j = 0; j < N && i < M; ++i, ++j) {
        std::pair<size_t, size_t> currNoZero = new_matrix.FirstNoZero(i, j);
        if (currNoZero.first == M + 1 && currNoZero.second == N + 1) {
          return new_matrix;
        }
        if (i != currNoZero.first) {
          std::swap(new_matrix[i], new_matrix[currNoZero.first]);
          coefficient *= -1;
        }
        j = currNoZero.second;
        Field curr_coefficient = new_matrix[i][j];
        coefficient *= curr_coefficient;
        for (size_t k = 0; k < N; ++k) {
          new_matrix[i][k] /= curr_coefficient;
        }
        for (size_t underStr = i + 1; underStr < M; ++underStr) {
          Field new_coefficient = new_matrix[underStr][j];
          if (new_coefficient != Field(0)) {
            for (size_t t = 0; t < N; ++t) {
              new_matrix[underStr][t] -= new_coefficient * new_matrix[i][t];
            }
          }
        }
      }
      return new_matrix;
    }
  
  void downTriangleWithUnity(Matrix<M, N, Field>& unity) {
    for (size_t i = 0, j = 0; j < N && i < M; ++i, ++j) {
      std::cout << i << std::endl;
      std::pair<size_t, size_t> currNoZero = FirstNoZero(i, j);
      if (currNoZero.first == M + 1 && currNoZero.second == N + 1) {
        return;
      }
      if (i != currNoZero.first) {
        std::swap(elems[i], elems[currNoZero.first]);
        std::swap(unity[i], unity[currNoZero.first]);
      }
      j = currNoZero.second;
      Field curr_coefficient = elems[i][j];
      if (curr_coefficient != Field(0)) {
        for (size_t k = 0; k != N; ++k) {
          elems[i][k] /= curr_coefficient;
          unity[i][k] /= curr_coefficient;
        }
        for (size_t underStr = i + 1; underStr < M; ++underStr) {
          Field new_coefficient = elems[underStr][j];
          if (new_coefficient != Field(0)) {
            for (size_t t = 0; t < N; ++t) {
              elems[underStr][t] -= elems[i][t] * new_coefficient;
              unity[underStr][t] -= unity[i][t] * new_coefficient;
            }
          }
        }      
      }
    }
    std::cout << unity << std::endl;
    for (size_t i = N - 1; i != 0; --i) {
      std::cout << i << std::endl;
      if ((*this)[i][i] != 0) {
        for (size_t next_str = 0; next_str != i; ++next_str) {
          Field coeff = (*this)[next_str][i]; 
          if (coeff != Field(0)) {
            (*this)[next_str][i] = 0;
            for (size_t k = 0; k != N; ++k) {
              unity[next_str][k] -= unity[i][k] * coeff;
            }
          }
        }
      }
    }
    return;
  }

  std::array<std::array<Field, N>, M> elems;
 
 public:
  Matrix() = default;

  Matrix(const std::array<std::array<Field, M>, N>& elems): elems(elems) {}

  Matrix(const std::initializer_list<std::initializer_list<Field>>& init) {
    auto rowIt = init.begin();
    for (size_t i = 0; i < M && rowIt != init.end(); ++i, ++rowIt) {
        auto colIt = rowIt->begin();
        for (size_t j = 0; j < N && colIt != rowIt->end(); ++j, ++colIt) {
            elems[i][j] = Field(*colIt);
        }
    }
  }

  Matrix(const Matrix<M, N, Field>& other): elems(other.elems) {}

  Matrix<M, N, Field>& operator=(const Matrix<M, N, Field>& other) {
    Swap(other);
    return *this;
  }

  ~Matrix() = default;

  friend std::ostream& operator<<(std::ostream& os, const Matrix<M, N, Field>& res) { 
    res.Print(); 
    return os; 
  }

  Matrix<M, N, Field>& operator+=(const Matrix<M, N, Field>& other) {
    for (size_t m = 0; m != M; ++m) {
      for (size_t n = 0; n != N; ++n) {
        elems[m][n] += other.elems[m][n];
      }
    }
    return *this;  
  }

  Matrix<M, N, Field>& operator-=(const Matrix<M, N, Field>& other) {
    for (size_t m = 0; m != M; ++m) {
      for (size_t n = 0; n != N; ++n) {
        elems[m][n] -= other.elems[m][n];
      }
    }
    return *this;  
  }

  Matrix<N, N, Field>& operator*=(const Matrix<N, N, Field>& other) {
    static_assert(M == N);
    Matrix<M, N, Field> newMatrix;
    for (size_t m = 0; m != N; ++m) {
      for (size_t k = 0; k != N; ++k) {
        for (size_t n = 0; n != N; ++n) {
          newMatrix[m][k] += (*this)[m][n] * other[n][k];
        }
      }
    }
    (*this) = newMatrix;
    return (*this);
  }

  std::array<Field, N>& operator[](size_t i) { return elems[i]; }
  
  const std::array<Field, N>& operator[](size_t i) const { return elems[i]; }

  Matrix<M, N, Field>& operator*=(const Field& other) {
    for (size_t m = 0; m != M; ++m) {
      for (size_t n = 0; n != N; ++n) {
        elems[m][n] *= other;
      }
    }
    return *this; 
  }

  Matrix<N, M, Field> transposed() const {
    Matrix<N, M, Field> new_matrix;
    for (size_t i = 0; i != M; ++i) {
      for (size_t j = 0; j != N; ++j) {
        new_matrix[j][i] = elems[i][j];
      }
    }
    return new_matrix;
  }

  Field trace() const {
    static_assert(N == M);
    Field sums = 0;
    for (size_t i = 0; i != N; ++i) {
      sums += elems[i][i];
    }
    return sums;
  }
  
  const std::array<Field, N> getRow(size_t i) const {
    return elems[i];
  }

  const std::array<Field, M> getColumn(size_t k) const {
    std::array<Field, M> newArray;
    for (size_t i = 0; i != M; ++i) {
      newArray[i] = elems[i][k];
    }
    return newArray;
  }

  std::array<Field, N> getRow(size_t i) {
    return elems[i];
  }

  std::array<Field, M> getColumn(size_t k) {
    std::array<Field, M> newArray;
    for (size_t i = 0; i != M; ++i) {
      newArray[i] = elems[i][k];
    }
    return newArray;
  }

  Field det() const {
    static_assert(M == N);
    Field coefficient = Field(1);
    Matrix<M, N, Field> Gausse = downTriangleWithOutUnity(coefficient);
    Field ans = Gausse[0][0];
    for (size_t i = 1; i != N; ++i) {
      ans *= Gausse[i][i];
    }
    ans *= coefficient;
    return ans;
  }

  size_t rank() const {
    Field coefficient = 1;
    Matrix<M, N, Field> Gausse = downTriangleWithOutUnity(coefficient);
    size_t rank = 0;
    for (size_t i = 0; i != M; ++i) {
      for (size_t j = 0; j != N; ++j) {
        if (Gausse[i][j] != 0) {
          ++rank;
          break;
        }
      }
    }
    return rank;
  }

  Matrix<M, N, Field> unityMatrix() {
    static_assert(M == N);
    Matrix<N, M, Field> unity;
    for (size_t i = 0; i != N; ++i) {
      unity[i][i] = 1;
    }
    return unity;
  }

  void invert() {
    static_assert(M == N);
    Matrix<M, N, Field> unity = unityMatrix();
    downTriangleWithUnity(unity);
    *this = unity;
  }

  Matrix<M, N, Field> inverted() {
    static_assert(M == N);
    Matrix<M, N, Field> new_matrix = *this;
    new_matrix.invert();
    return new_matrix;
  }
};

template <size_t M, size_t N, typename Field> 
Matrix<M, N, Field> operator+(const Matrix<M, N, Field>& curr, const Matrix<M, N, Field>& other) {
  Matrix<M, N, Field> currNew = curr;
  currNew += other;
  return currNew;
}
  
template <size_t M, size_t N, typename Field> 
Matrix<M, N, Field> operator-(const Matrix<M, N, Field>& curr, const Matrix<M, N, Field>& other) {
  Matrix<M, N, Field> currNew = curr;
  currNew -= other;
  return currNew;
}

template<size_t M, size_t N, size_t K, typename Field>
Matrix<M, K, Field> operator*(const Matrix<M, N, Field>& curr, const Matrix<N, K, Field>& other) {
  Matrix<M, K, Field> new_matrix;
  for (size_t m = 0; m != M; ++m) {
    for (size_t k = 0; k != K; ++k) {
      for (size_t n = 0; n != N; ++n) {
        new_matrix[m][k] += curr[m][n] * other[n][k];
      }
    }
  }
  return new_matrix;
}

template <size_t M, size_t N, typename Field> 
Matrix<M, N, Field> operator*(const Field& other, const Matrix<M, N, Field>& curr) {
  Matrix<M, N, Field> new_matrix = curr;
  for (size_t m = 0; m != M; ++m) {
    for (size_t n = 0; n != N; ++n) {
      new_matrix[m][n] *= other;
    }
  }
  return new_matrix; 
}

template<size_t N, typename Field = Rational>
using SquareMatrix = Matrix<N, N, Field>;

template <size_t M, size_t N, typename Field> 
bool operator==(const Matrix<M, N, Field>& curr, const Matrix<M, N, Field>& other) {
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      if (curr[i][j] != other[i][j]) {
        return false;
      }
    }
  }
  return true;
}

template <size_t M, size_t N, typename Field> 
bool operator!=(const Matrix<M, N, Field>& curr, const Matrix<M, N, Field>& other) {
  return !(curr == other);
}

template <size_t M, size_t N, typename Field>
std::istream& operator>>(std::istream& in, Matrix<M, N, Field>& matrix) {
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      in >> matrix[i][j];
    }
  }
  return in;
}