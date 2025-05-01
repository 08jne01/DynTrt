#include <catch2/catch_test_macros.hpp>

#include <iostream>

#include "AnyTraits.h"

struct Circle
{
    void PrintMove(double x, double y) { printf("Circle Move, (%lf,%lf)\n", x, y); }
    void PrintDraw() { printf("Circle Draw\n"); }
};

struct Rectangle
{
    void PrintMove(double x, double y) { printf("Rectangle Move, (%lf,%lf)\n", x, y); }
    void PrintDraw() { printf("Rectangle Draw\n"); }
};

struct Shape
{
    template<typename Method, typename T, typename... Ts>
    static Method::return_type Invoke( T, Ts... );

    struct Move : AnyTraits::Trait<Shape,void(double x, double y)> {};
    struct Draw : AnyTraits::Trait<Shape,void()> {};
    
    struct Scale : AnyTraits::Trait<Shape,void()> {};
    struct Rotate : AnyTraits::Trait<Shape,void()> {};

    using Transformable = AnyTraits::Any<Shape, Move, Draw, Scale>;

    using Any = AnyTraits::AnySmall<16, Shape, Move, Draw>;
};

template<>
void Shape::Invoke<Shape::Draw>( Circle circle )
{
    circle.PrintDraw();
}

template<>
void Shape::Invoke<Shape::Move>( Circle circle, double x, double y )
{
    circle.PrintMove(x, y);
}

template<>
void Shape::Invoke<Shape::Draw>( Rectangle rectangle )
{
    rectangle.PrintDraw();
}

template<>
void Shape::Invoke<Shape::Move>( Rectangle rectangle, double x, double y )
{
    rectangle.PrintMove(x, y);
}

template<typename T>
struct Invoker;

template<typename... Ts>
struct Invoker<AnyTraits::detail::type_sequence<Ts...>>
{

};

template<typename T, typename Ret, typename T1, typename... Ts>
static Ret Invoke( T1 storage, Ts... args )
{
    return 0.0;
}

TEST_CASE("Any Traits Basic", "[Any Traits][Basic]")
{

    Shape::Any shape = Circle{};

    shape.Call<Shape::Move>(1.5, 0.0);
    shape.Call<Shape::Draw>();

    shape = Rectangle{};

    shape.Call<Shape::Move>(0.5, 1.0);
    shape.Call<Shape::Draw>();

    Shape::Any shape2 = shape;

}

// static void Shape::Invoke<Shape::Move,AnyStorage<16>,double,double>(AnyStorage<16>,double,double)