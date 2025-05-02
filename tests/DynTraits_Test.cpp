#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "VirtualHelper.h"
#include <iostream>
#include <random>
#include <variant>

#include "DynTrt.h"

struct Circle
{
    void PrintMove(double x, double y) { printf("Circle Move, (%lf,%lf)\n", x, y); }
    void PrintDraw() const { printf("Circle Draw\n"); }

    inline void Move(double x, double y)
    {
        move += 2.0 * (x + y);
    }
    inline void Draw()
    {
        draw += 1.0;
    }

    inline void Rotate( double value ) { printf("Rotation Circle\n"); }

    double move = 0.0;
    double draw = 0.0;
};

struct Rectangle
{
    void PrintMove(double x, double y) { printf("Rectangle Move, (%lf,%lf)\n", x, y); }
    void PrintDraw() const { printf("Rectangle Draw\n"); }
    void PrintRotate(double x) { printf("Rectangle Rotate: %lf\n", x); }


    inline void Move(double x, double y)
    {
        move += x + y;
    }

    inline void Draw()
    {
        draw += 1.0;
    }

    inline void Rotate( double value ) { printf("Rotation Rectangle\n"); }

    double move = 0.0;
    double draw = 0.0;
};

struct Triangle
{
    void PrintMove(double x, double y) { printf("Triangle Move, (%lf,%lf)\n", x, y); }
    void PrintDraw() { printf("Triangle Draw\n"); }
    void PrintRotate(double x) { printf("Triangle Rotate: %lf\n", x); }
};

#define ANY_INVOKE_DEFINITION() \
    template<typename Method, typename T, typename... Ts> \
    static inline Method::return_type Invoke( T*, Ts... )


struct Shape
{
    ANY_INVOKE_DEFINITION();

    // Render
    struct Draw :   DynTrt::Method<Shape, void(DynTrt::ConstSelf)> {};
    // Transform
    struct Move :   DynTrt::Method<Shape, void(DynTrt::Self, double x, double y)> {};
    struct Scale :  DynTrt::Method<Shape, void(DynTrt::Self)> {};
    // Modfiy
    struct Slice :  DynTrt::Method<Shape, void(DynTrt::Self)> {};
    // Rotatable
    struct Rotate : DynTrt::Method<Shape, void(DynTrt::Self, double)> {};

    

    // define type which traits belong to
    using Transform =   DynTrt::AnySmall<16, Shape, Move>;
    using Modify =      DynTrt::AnySmall<16, Shape, Slice>;
    using Rotatable =   DynTrt::AnySmall<16, Shape, Rotate>;
    using Drawable =    DynTrt::AnySmall<16, Shape, Draw>;

    using Any = DynTrt::AnySmall<16, Shape, Move, Draw>;
    using Traits = DynTrt::Trait<Shape, Shape::Move, Shape::Draw>;
};

// struct Shape
// {
//     ANY_INVOKE_DEFINITION();

//     struct Move     : DynTrt::Trait<Shape,void(double x, double y)> {};
//     struct Draw     : DynTrt::Trait<Shape,void()> {};
//     struct Scale    : DynTrt::Trait<Shape,void()> {};
//     //defaulted function
//     struct Rotate   : DynTrt::Trait<Shape,void(double)> {
//         template<typename T>
//         static void Invoke(T value, double rotation) 
//         {
//             value.Rotate(rotation);
//         }
//     };

//     // define type which traits belong to
//     using Any = DynTrt::AnySmall<16, Shape, 
//         Move, 
//         Draw
//     >;

//     // you can have multiple type groupings using the same traits
//     using Transform = DynTrt::AnySmall<16, Shape, 
//         Move, 
//         Scale, 
//         Rotate
//     >;
// };

template<>
void Shape::Invoke<Shape::Draw>( const Circle* circle )
{
    //circle->draw++;
    circle->PrintDraw();
}

template<>
void Shape::Invoke<Shape::Move>( Circle* circle, double x, double y )
{
    circle->move++;
    circle->PrintMove(x, y);
}


template<>
void Shape::Invoke<Shape::Draw>( const Rectangle* rectangle )
{
    rectangle->PrintDraw();
}

template<>
void Shape::Invoke<Shape::Move>( Rectangle* rectangle, double x, double y )
{
    rectangle->move++;
    rectangle->PrintMove(x, y);
}

template<>
void Shape::Invoke<Shape::Rotate>( Rectangle* rectangle, double x )
{
    rectangle->PrintRotate(x);
}

template<>
void Shape::Invoke<Shape::Draw>( Triangle* triangle )
{
    triangle->PrintDraw();
}

template<>
void Shape::Invoke<Shape::Move>( Triangle* triangle, double x, double y )
{
    triangle->PrintMove(x, y);
}

template<>
void Shape::Invoke<Shape::Rotate>( Triangle* triangle, double x )
{
    triangle->PrintRotate(x);
}

template<typename T>
struct Invoker;

template<typename... Ts>
struct Invoker<DynTrt::detail::type_sequence<Ts...>>
{

};

template<typename T>
static void Draw( T shape )
{
    Shape::Invoke<Shape::Draw>(shape);
}

TEST_CASE("Any Traits Basic", "[Any Traits][Basic]")
{
    Shape::Transform shape = Circle{};
    shape.Call<Shape::Move>(1.5, 0.0);

    Shape::Drawable draw_shape = shape.Get<Circle>();
    //draw_shape.Call<Shape::Draw>();

    using T = Shape::Drawable::typed_method_pointer<Shape::Draw, Circle>;

    Shape::Rotatable rotate_shape = Rectangle{};

    Circle& s1 = shape.Get<Circle>();

    Shape::Transform shape2 = shape;
    //shape2.Call<Shape::Draw>();
    Circle& s2 = shape2.Get<Circle>();

    shape = Rectangle{};
    shape.Call<Shape::Move>(0.5, 1.0);
    //shape.Call<Shape::Draw>();

    shape = Triangle{};
    shape.Call<Shape::Move>(1.0, 0.69);
    //shape.Call<Shape::Draw>();

    if constexpr ( false )
    {
        static std::mt19937 generator;
        std::uniform_int_distribution<int> distribution{10, 1000};
        for ( size_t i = 0; i < 10000; i++ )
        {
            new char[distribution(generator)];
        }

        BENCHMARK( "Any Basic" )
        {
            Shape::Any circ = Circle{};
            Shape::Any rect = Rectangle{};

            for ( size_t i = 0; i < 10'000; i++ )
            {
                circ.Call<Shape::Move>( 0.5, 1.0 );
                rect.Call<Shape::Draw>();
                circ.Call<Shape::Draw>();
                rect.Call<Shape::Move>(0.5, 1.0);
            }

            return circ.Get<Circle>();
        };

        BENCHMARK( "Virtual Basic" )
        {

            std::unique_ptr<VirtualBenchmark::Shape> circ = std::make_unique<VirtualBenchmark::Circle>(VirtualBenchmark::Circle{});
            std::unique_ptr<VirtualBenchmark::Shape> rect = std::make_unique<VirtualBenchmark::Rectangle>(VirtualBenchmark::Rectangle{});

            for ( size_t i = 0; i < 10'000; i++ )
            {
                circ->Move( 0.5, 1.0 );
                rect->Draw();
                circ->Draw();
                rect->Move(0.5, 1.0);
            }

            return circ;
        };



        BENCHMARK( "Any Vector" )
        {
            
            std::uniform_int_distribution<int> dist{0, 1};

            std::vector<Shape::Any> shapes;
            for ( size_t i = 0; i < 10000; i++ )
            {
                if ( dist(generator) )
                {
                    shapes.emplace_back( Circle{} );
                }
                else
                {
                    shapes.emplace_back( Rectangle{} );
                }
            }

            for ( size_t i = 0; i < 1'000; i++ )
            {
                for ( auto& shape : shapes )
                {
                    shape.Call<Shape::Move>(0.5, 1.0);
                    shape.Call<Shape::Draw>();
                }
            }

            return shapes;
        };

        BENCHMARK( "Virtual Vector" )
        {
            std::uniform_int_distribution<int> dist{0, 1};

            std::vector<std::unique_ptr<VirtualBenchmark::Shape>> shapes;
            for ( size_t i = 0; i < 10000; i++ )
            {
                if ( dist(generator) )
                {
                    shapes.emplace_back( std::make_unique<VirtualBenchmark::Circle>(VirtualBenchmark::Circle{}) );
                }
                else
                {
                    shapes.emplace_back( std::make_unique<VirtualBenchmark::Rectangle>(VirtualBenchmark::Rectangle{}) );
                }
            }

            for ( size_t i = 0; i < 1'000; i++ )
            {
                for ( auto& shape : shapes )
                {
                    shape->Move(0.5, 1.0);
                    shape->Draw();
                }
            }

            return shapes;
        };
    }
}

inline void Drawa( const Shape::Traits shape )
{
    //.Call<Shape::Draw>();
}

TEST_CASE("Section 2")
{
    Circle c{};
    const Circle* p = &c;

    const Shape::Traits traits(p);
    traits.Call<Shape::Draw>();
    traits.Call<Shape::Move>(0.5, 1.0);



    //Shape::Move::DummyInvoke<Shape,Circle,Shape::Move>();

    //traits.Call<Shape::Move>(0.5, 1.0);
    //traits.Call<Shape::Draw>();

    //Draw(&c);


    //using p1 = Shape::Draw::Signature::const_correct<void>;
    // using p2 = Shape::Traits::method_pointer<Shape::Move>;

    // auto p = (Shape::Traits::method_pointer<Shape::Move>)Shape::Traits::typed_method_pointer<Shape::Move,Circle>(Shape::Invoke<Shape::Move,Circle>);
    // using p3 = Shape::Traits::vtable<void>;
}

// static void Shape::Invoke<Shape::Move,AnyStorage<16>,double,double>(AnyStorage<16>,double,double)