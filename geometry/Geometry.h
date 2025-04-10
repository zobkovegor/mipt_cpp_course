#include <iostream>
#include <cmath>
#include <vector>

const int cNum = 100'000'000;

class Line;

struct Point {
private:
  void Swap(Point other) {
    std::swap(x, other.x);
    std::swap(y, other.y);
  }
public:
  double x = 0;
  double y = 0;
  Point() = default;
  Point (double x, double y): x(x), y(y) {}
  Point (const Point& other): x(other.x), y(other.y) {}
  Point& operator=(const Point& other) {
    Swap(other);
    return *this;
  }
  ~Point() = default;
  bool isEqualPoint(const Point& other) const {
    return (x - other.x <= 1e-6 && other.x - x <= 1e-6) && (y - other.y <= 1e-6 && other.y - y <= 1e-6);
  }
  bool operator==(const Point& otherPoint) const {
    return isEqualPoint(otherPoint);
  }
  bool operator!=(const Point& otherPoint) const {
    return !(isEqualPoint(otherPoint));
  }
  void rotate(const Point& center , double corn);
  void reflect(const Point& center);
  void reflect(const Line& axis);
  void scale(const Point& center, double coefficient);
};

double getDistance(const Point& first, const Point& second) {
  return std::sqrt(std::pow((second.x - first.x), 2) + std::pow((second.y - first.y), 2)); 
}

struct Vector {
  Point start;
  Point end;
  double len = 0;
  Vector(const Point& first, const Point& second) : start(first), end(second), 
        len(getDistance(first, second)) {}

  double xLen() {
    return end.x - start.x;
  }
  double yLen() {
    return end.y - start.y;
  }
};

class Line {
 public:
  static bool Check(double firstVal, double secondVal, double thirdVal, double fourthVal) {
    double fVal = std::round(firstVal * secondVal * cNum);
    double sVal = std::round(thirdVal * fourthVal * cNum);
    return fVal == sVal;
  }
  void Swap(Line other) {
    std::swap(xShift, other.xShift);
    std::swap(yShift, other.yShift);
    std::swap(cShift, other.cShift);
  }
  double xShift = 0;
  double yShift = 0;
  double cShift = 0;
 public:
  Line(const Point& firstPoint, const Point& secondPoint): xShift(secondPoint.y - firstPoint.y),
                                                           yShift(firstPoint.x - secondPoint.x),
                                                           cShift((secondPoint.x - firstPoint.x)*firstPoint.y - 
                                                                  (secondPoint.y - firstPoint.y)*firstPoint.x) {}
  Line(const double tg, const double c): xShift(tg), yShift(-1), cShift(c) {}
  Line(const Point& point, const double tg): xShift(tg), yShift(-1), cShift(point.y - tg * point.x) {}
  Line& operator=(const Line& other) {
    Swap(other);
    return *this;
  }
  Point intersectionPoint(const Line& other) const {
    return Point((yShift * other.cShift - other.yShift * cShift) / (xShift * other.yShift - other.xShift * yShift),
                  (cShift * other.xShift - other.cShift * xShift) / (xShift * other.yShift - other.xShift * yShift));
  } 
  bool operator==(const Line& other) {
    if (Check(xShift, other.yShift, other.xShift, yShift) && 
        Check(xShift, other.cShift, other.xShift, cShift)) {
          return true;
    }
    return false;
  }
  bool operator!=(const Line& other) {
    return !(*this == other);
  }
  ~Line() = default;
};

class Shape {
 protected:
  Shape() = default;
 public:
  virtual double perimeter() const = 0;
  virtual double area() const = 0;
  virtual bool containsPoint(const Point& point) const = 0; 
  virtual void rotate(const Point& center, double angle) = 0;
  virtual void reflect(const Point& center) = 0;
  virtual void reflect(const Line& axis) = 0;
  virtual void scale(const Point& center, double coefficient) = 0;
  virtual bool isEqual(const Shape* other) const = 0;
  virtual bool congruent(const Shape* another) const = 0;
  virtual bool checkSimilarTo(const Shape* another) const = 0;
  virtual bool operator==(const Shape &other) const {
    return this->isEqual(&other);
    };
  virtual bool isCongruentTo(const Shape& other) const {
    return this->congruent(&other);
  }
  virtual bool isSimilarTo(const Shape& other) const {
    return this->checkSimilarTo(&other);
  }
  virtual ~Shape() = default;
};

void Point::rotate(const Point& center, double corn) {
  Vector vect(*this, center);
  x = (vect.xLen() * std::cos(corn * M_PI / 180) - vect.yLen() * std::sin(corn * M_PI / 180) + center.x);
  y = (vect.xLen() * std::sin(corn * M_PI / 180) + vect.yLen() * std::cos(corn * M_PI / 180) + center.y);
}
void Point::reflect(const Point& center) {
  x = 2 * center.x - x;
  y = 2 * center.y - y;
}
void Point::reflect(const Line& axis) {
  double t = (-axis.cShift - axis.xShift * x - axis.yShift * y) / ((axis.xShift * axis.xShift) + (axis.yShift * axis.yShift));
  x += 2 * t * axis.xShift;
  y += 2 * t * axis.yShift;
}
void Point::scale(const Point& center, double coefficient) {
  x = (x - center.x) * coefficient + center.x;
  y = (y - center.y) * coefficient + center.y;
}

class Polygon: public virtual Shape {
 protected:
  bool getSin(const Point& firstPoint, const Point& secondPoint, const Point& thirdPoint) const {
    Vector firstV(firstPoint, secondPoint);
    Vector secondV(secondPoint, thirdPoint);
    return (firstV.xLen() * secondV.yLen() - secondV.xLen() * firstV.yLen()) > 0;
  }
  double getCos(const Point& point, const Point& first, const Point& second) const {
    Vector firstV(first, point);
    Vector secondV(second, point);
    return std::acos((firstV.xLen() * secondV.xLen() + 
                        firstV.yLen() * secondV.yLen())
                          / (firstV.len * secondV.len));
  }
  std::vector<Point> Points;
 public:
  Polygon(std::vector<Point>& points) : Points(points) {}
  Polygon(std::initializer_list<Point>& init_list): Points(init_list) {}
  template <typename... Args>
  Polygon(Args... args) {
    Points = {args...};
  }
  int verticesCount() const {
    return static_cast<int>(Points.size());
  }
  const std::vector<Point>& getVertices() {
    return Points;
  }
  bool isConvex() const {
    bool val = getSin(Points[0], Points[1], Points[2]);
    for (size_t i = 1; i < Points.size(); ++i) {
      if (val != getSin(Points[(i) % Points.size()], Points[(i + 1) % Points.size()], Points[(i + 2) % Points.size()])) {
        return false;
      }
    }
    return true;
  }
  bool containsPoint(const Point& point) const override {
    if (point == Points[0]) {
      return true;
    }
    double sumCos = 0;
    for (size_t i = 0; i != Points.size() - 1 ; ++i) {
      if (point == Points[i]) {
        return true;
      }
      sumCos += (getCos(point, Points[i], Points[(i + 1)]));
    }
    sumCos += (getCos(point, Points.back(), Points[0]));
    sumCos = std::round(std::abs(sumCos) * cNum);
    return sumCos == std::round(2 * M_PI * cNum);
  }
  double perimeter() const override {
    double perim = getDistance(Points[0], Points[Points.size() - 1]);
    for (size_t i = 1; i < Points.size(); ++i) {
      perim += getDistance(Points[i], Points[i - 1]);
    }
    return perim;
  }
  double area() const override {
    double s = 0;
    for (size_t i = 1; i <= Points.size(); ++i) {
      s += Points[i % Points.size()].x * (Points[(i + 1) % Points.size()].y - Points[(i - 1) % Points.size()].y);
    }
    s *= 0.5;
    return std::abs(s);
  }
  bool checkSimilarTo(const Shape* another) const override {
    const Polygon* other = dynamic_cast<const Polygon*>(another);
    if (other == nullptr) {
      return false;
    }    if (Points.size() != other->Points.size()) {
      return false;
    }
    double cf = other->perimeter() / perimeter();
    if (std::round(area() * cf * cf * 100) == std::round(other->area() * 100)) {
      return true;
    }
    return false;
  }
  bool isEqual(const Shape* another) const override {
    const Polygon* other = dynamic_cast<const Polygon*>(another);
    if (other == nullptr) {
      return false;
    }
    if (Points.size() != other->Points.size()) {
      return false;
    }
    for (size_t i = 0; i != Points.size(); ++i) {
      bool check = false;
      for (size_t j = 0; j != Points.size(); ++j) {
        if (Points[i] == other->Points[j]) {
          check = true;
          break;
        }
        check = false;
      }
      if (check) {
        return check;
      }
    }
    return false;
  }
  void rotate(const Point& center, double angle) override {
    for (size_t i = 0; i != Points.size(); ++i) {
      Points[i].rotate(center, angle);
    }
  }
  void reflect(const Point& center) override {
    for (size_t i = 0; i != Points.size(); ++i) {
      Points[i].reflect(center);
    }
  }
  void reflect(const Line& axis) override {
    for (size_t i = 0; i != Points.size(); ++i) {
      Points[i].reflect(axis);
    }
  }
  void scale(const Point& center, double coefficient) override {
    for (size_t i = 0; i != Points.size(); ++i) {
      Points[i].scale(center, coefficient);
    }
  }
  bool congruent(const Shape* another) const override {
    const Polygon* other = dynamic_cast<const Polygon*>(another);
    if (std::round(perimeter() * cNum) == std::round(other->perimeter() * cNum) && checkSimilarTo(other)) {
      return true;
    }
    return false;
  }
  virtual ~Polygon() = default;
};

class Ellipse: public virtual Shape {
 protected: 
  Point firstFocus;
  Point secondFocus;
  double dist = 0;
  double a = 0;
  double b = 0;
  void UpdateFocus() {
    if (std::abs(firstFocus.x) < 1.0 / cNum) {
      firstFocus.x = 0;
    }
    if (std::abs(firstFocus.y) < 1.0 / cNum) {
      firstFocus.y = 0;
    }
    if (std::abs(secondFocus.x) < 1.0 / cNum) {
      secondFocus.x = 0;
    }
    if (std::abs(secondFocus.y) < 1.0 / cNum) {
      secondFocus.y = 0;
    }
  }
 public:
  Ellipse(const Point& mid, double r): firstFocus(mid), secondFocus(mid), dist(2 * r) {
    a = dist / 2;
    b = std::sqrt(a * a - (getDistance(firstFocus, secondFocus) / 2) * (getDistance(firstFocus, secondFocus) / 2));
  }
  Ellipse(const Point& first, const Point& second, double sums): firstFocus(first), secondFocus(second), dist(sums) {
    a = sums / 2;
    b = std::sqrt(a * a - (getDistance(firstFocus, secondFocus) / 2) * (getDistance(firstFocus, secondFocus) / 2));
  }
  std::pair<Point, Point> focuses() const {
    return std::make_pair(firstFocus, secondFocus);
  }
  std::pair<Line, Line> directrices() const {
    double distance = getDistance(firstFocus, secondFocus);
    double aa = dist / 2;
    double c = distance / 2;
    double e = c / aa;
    double d = aa / e;
    Point center((firstFocus.x + secondFocus.x) / 2, (firstFocus.y + secondFocus.y) / 2);
    double x = -1 / e;
    double y = center.y - (-1 / e) * center.x + d;
    double z = center.y - (-1 / e) * center.x - d;
    Line directrix1(x, y);
    Line directrix2(x, z);
    return std::make_pair(directrix1, directrix2);
  }
  double eccentricity() const {
    double distance = getDistance(firstFocus, secondFocus);
    double aa = dist / 2;
    double c = distance / 2;
    double e = c / aa;
    return e;
  }

  Point center() const {
    Point mid((firstFocus.x + secondFocus.x) / 2, (secondFocus.y + firstFocus.y) / 2);
    return mid;
  }
  bool containsPoint(const Point& point) const override {
    if (getDistance(firstFocus, point) + getDistance(secondFocus, point) <= dist) {
      return true;
    }
    return false;
  }
  double perimeter() const override {
    return M_PI * (3 * (a + b) - std::sqrt((3 * a + b) * (a + 3 * b)));
  }
  double area() const override {
    return std::abs(M_PI * a * b);
  }
  bool checkSimilarTo(const Shape* another) const override {
    const Ellipse* other = dynamic_cast<const Ellipse*>(another);
    if (other == nullptr) {
      return false;
    }
    double cf = other->dist / dist;
    Vector firstV(secondFocus, firstFocus);
    Vector secondV(other->firstFocus, other->secondFocus);
    if ((firstV.xLen() + firstV.yLen()) * cf == (secondV.xLen() + secondV.yLen()) &&
        a * cf == other->a && b * cf == other->b) {
      return true;
    }
    return false;
  }
  bool isEqual(const Shape* another) const override {
    const Ellipse* other = dynamic_cast<const Ellipse*>(another);
    if (other == nullptr) {
      return false;
    }
    int dist1 = std::round(dist * 10);
    int dist2 = std::round(other->dist * 10);
    if (dist1 == dist2) {
      if ((firstFocus == other->firstFocus || firstFocus == other->secondFocus) && 
          (secondFocus == other->firstFocus || secondFocus == other->secondFocus)) {
            return true;
      }
    }   
    return false;
  }
  void rotate(const Point& center, double angle) override {
    firstFocus.rotate(center, angle);
    secondFocus.rotate(center, angle);
    UpdateFocus();
  }
  void reflect(const Point& center) override {
    firstFocus.reflect(center);
    secondFocus.reflect(center);
    UpdateFocus();
  }
  void reflect(const Line& axis) override {
    firstFocus.reflect(axis);
    secondFocus.reflect(axis);
    UpdateFocus();
  }
  void scale(const Point& center, double coefficient) override {
    firstFocus.scale(center, coefficient);
    secondFocus.scale(center, coefficient);
    a *= coefficient;
    b *= coefficient;
    dist *= coefficient;
    UpdateFocus();
  }
  bool congruent(const Shape* another) const override {
    const Ellipse* other = dynamic_cast<const Ellipse*>(another);
    if (other == nullptr) {
      return false;
    }
    if (perimeter() == other->perimeter() && checkSimilarTo(other)) {
      return true;
    }
    return false;
  }
  virtual ~Ellipse() = default;
};

class Circle: public virtual Ellipse {
 public:
  Point mid;
  double R = 0;
  Circle(const Point& mid, double r): Ellipse(mid, r), mid(mid), R(r) {}
  double radius() {
    return R;
  }
  double perimeter() const override {
    return 2 * M_PI * R;
  }
  double area() const override {
    return M_PI * R * R;
  }
  void rotate(const Point& center, double angle) override {
    mid.rotate(center, angle);
    firstFocus.rotate(center, angle);
    secondFocus.rotate(center, angle);
  }
  void reflect(const Point& center) override {
    mid.reflect(center);
    firstFocus.reflect(center);
    secondFocus.reflect(center);
  }
  void reflect(const Line& axis) override {
    mid.reflect(axis);
    firstFocus.reflect(axis);
    secondFocus.reflect(axis);
  }
  void scale(const Point& center, double coefficient) override {
    firstFocus.scale(center, coefficient);
    secondFocus.scale(center, coefficient);
    mid.scale(center, coefficient);
    dist *= coefficient;
    R *= coefficient;
  }
  virtual ~Circle() = default;
};

class Rectangle : public virtual Polygon {
 protected:
  void getPoint(const Point& firstPoint, const Point& secondPoint, double cf) {
    Point helpPoint = secondPoint;
    helpPoint.rotate(firstPoint, std::atan(cf) * 180 / M_PI);
    Vector firstV(helpPoint, firstPoint);
    Point firstLPoint(firstPoint.x + (firstV.xLen()) / std::pow(1 + cf * cf, 0.5), firstPoint.y + (firstV.yLen()) / std::pow(1 + cf * cf, 0.5));
    Point firstRPoint = firstLPoint;
    Point sim((firstPoint.x + secondPoint.x) / 2.0, (firstPoint.y + secondPoint.y) / 2.0);
    firstRPoint.reflect(sim);
    Points = {firstPoint, firstLPoint, secondPoint, firstRPoint};
  }
 public:
  Rectangle() = default;
  Rectangle(const Point& first, const Point& second, const Point& third, const Point& fourth) {
    Points = {first, second, third, fourth};
  }
  Rectangle(const Point& first, const Point& second, double cf) {
    getPoint(first, second, cf);
  }
  std::pair<Line, Line> diagonals() const {
    Line firstDiagonal(Points[0], Points[2]);
    Line secondDiagonal(Points[1], Points[3]);
    return std::make_pair(firstDiagonal, secondDiagonal); 
  }
  Point center() const {
    Point mid((Points[0].x + Points[1].x + Points[2].x + Points[3].x) / 4, 
              (Points[0].y + Points[1].y + Points[2].y + Points[3].y) / 4);
    return mid;
  }
  virtual ~Rectangle() = default;
};

class Square: public virtual Rectangle {
 public:
  Square(const Point& first, const Point& second) {
    getPoint(first, second, 1);
  }
  Circle circumscribedCircle() const {
    double r = getDistance(center(), Points[0]);
    Circle circle(center(), r);
    return circle;
  }
  Circle inscribedCircle() const {
    Circle circle(center(), getDistance(Points[0], Points[1]) / 2);
    return circle;
  }
};

class Triangle: public virtual Polygon {
  Point getCenterOfcircumscribedCircle() const {
    Point firstPoint = Points[0]; 
    Point secondPoint = Points[1]; 
    Point thirdPoint = Points[2]; 
    Vector firstV(secondPoint, firstPoint);
    Vector secondV(thirdPoint, secondPoint);
    Vector thirdV(firstPoint, thirdPoint);
    double z1 = firstPoint.x * firstPoint.x + firstPoint.y * firstPoint.y; 
    double z2 = secondPoint.x * secondPoint.x + secondPoint.y * secondPoint.y; 
    double z3 = thirdPoint.x * thirdPoint.x + thirdPoint.y * thirdPoint.y; 
    double delta_x = 
        firstV.yLen() * z3 + secondV.yLen() * z1 + thirdV.yLen() * z2; 
    double delta_y = 
        firstV.xLen() * z3 + secondV.xLen() * z1 + thirdV.xLen() * z2; 
    double delta = firstV.xLen() * thirdV.yLen() - firstV.yLen() * thirdV.xLen(); 
    Point center = {-delta_x / (2 * delta), delta_y / (2 * delta)};
    return center;
  }
 public:
  Triangle(const Point& first, const Point& second, const Point& third) {
    Points = {first, second, third};
  }
  Point centroid() const {
    return Point((Points[0].x + Points[1].x + Points[2].x) / 3, (Points[0].y + Points[1].y + Points[2].y) / 3);
  }
  Circle circumscribedCircle() const { 
    Point center = getCenterOfcircumscribedCircle();
    return Circle(center, getDistance(center, Points[0])); 
  }
  Point orthocenter() const {
    Point centerCircle = getCenterOfcircumscribedCircle();
    Point bari = baricenter();
    return Point(-2 * centerCircle.x + 3 * bari.x, -2 * centerCircle.y + 3 * bari.y);
  }
  Point baricenter() const {
    Line first(Points[0], Point((Points[1].x + Points[2].x) / 2, (Points[1].y + Points[2].y) / 2));
    Line second(Points[1], Point((Points[0].x + Points[2].x) / 2, (Points[0].y + Points[2].y) / 2));
    return first.intersectionPoint(second);
  }
  Circle ninePointsCircle() const {
    Triangle newTriangle(Point((Points[0].x + Points[1].x) / 2, (Points[0].y + Points[1].y) / 2),
                         Point((Points[1].x + Points[2].x) / 2, (Points[1].y + Points[2].y) / 2),
                         Point((Points[0].x + Points[2].x) / 2, (Points[0].y + Points[2].y) / 2));
    return newTriangle.circumscribedCircle();
  }
  Line EulerLine() const {
    return Line(baricenter(), getCenterOfcircumscribedCircle());
  }
  Circle inscribedCircle() {
    double cfFirst = getDistance(Points[0], Points[1]) / getDistance(Points[0], Points[2]);
    double cfSecond = getDistance(Points[2], Points[1]) / getDistance(Points[0], Points[1]);
    Line firstBissetrice(Points[0], Point((Points[1].x + cfFirst * Points[2].x) / (1 + cfFirst), (Points[1].y + cfFirst * Points[2].y) / (1 + cfFirst)));
    Line secondBissetrice(Points[1], Point((Points[2].x + cfSecond * Points[0].x) / (1 + cfSecond), (Points[2].y + cfSecond * Points[0].y) / (1 + cfSecond)));
    Point center = firstBissetrice.intersectionPoint(secondBissetrice);
    return Circle(center, area() * 2 / perimeter());
  }
  ~Triangle() = default;
};