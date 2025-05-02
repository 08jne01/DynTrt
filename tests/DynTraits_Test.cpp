#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "DynTrt.h"

enum class Colour : uint8_t
{
    red,
    green,
    blue
};

struct Circle
{
    double x = 0.0;
    double y = 0.0;
    double r = 1.0;
    Colour colour = Colour::red;
    int draw_count = 0;
};

struct Rectangle
{
    double x = 0.0;
    double y = 0.0;
    double rotation = 0.0;
    double width = 1.0;
    double height = 1.0;
    Colour colour = Colour::red;
    int draw_count = 0;
};

struct Shape
{
    // Default template to specialise with the respective Methods and Types.
    template<typename Method, typename T, typename... Ts>
    static inline Method::return_type Invoke( T*, Ts... );

    // Default these
    struct Draw      : DynTrt::Method<Draw,     Colour(DynTrt::ConstSelf)> {
        template<typename T>
        static Colour Invoke(const T* self) { return self->colour; }
    };
    struct SetColour : DynTrt::Method<SetColour, void(DynTrt::Self, Colour colour)> {
        template<typename T>
        static void Invoke( T* self, Colour colour ) { self->colour = colour; }
    };
    // Default Move
    struct Move     : DynTrt::Method<Move,      void(DynTrt::Self, double x, double y)> {
        template<typename T>
        static void Invoke( T* self, double x, double y ) {
            self->x = x;
            self->y = y;
        }
    };
    // Non-default Scale/Rotate
    struct Scale    : DynTrt::Method<void, void(DynTrt::Self, double scale)> {};
    struct Rotate   : DynTrt::Method<void, void(DynTrt::Self, double rotation)> {};

    // Define Traits
    using Drawable =        DynTrt::Trait<Shape, Draw, SetColour>;
    using Transformable =   DynTrt::Trait<Shape, Move, Rotate, Scale>;
    using Moveable =        DynTrt::Trait<Shape, Move, Scale>;
};

template<>
void Shape::Invoke<Shape::Rotate>( Rectangle* self, double rotation ) 
{
    self->rotation = rotation;
}

// Note below the different implementations
template<>
void Shape::Invoke<Shape::Scale>( Circle* self, double scale )
{
    self->r *= scale;
}

// Template Specialisation of Shape::Move for Circle.
template<>
void Shape::Move::Invoke(Circle* self, double x, double y)
{
    self->x = x;
    self->y = y;
}

template<>
void Shape::Invoke<Shape::Scale>( Rectangle* self, double scale )
{
    self->width  *= scale;
    self->height *= scale;
}



TEST_CASE("DynTrait Basic", "[Basic]")
{
    SECTION("Circle")
    {
        Circle c;

        SECTION("Drawable")
        {
            Shape::Drawable drawable = &c;
            drawable.Call<Shape::SetColour>(Colour::green);
            REQUIRE(c.colour == Colour::green);
            REQUIRE( drawable.Call<Shape::Draw>() == Colour::green );
        }

        SECTION("Moveable")
        {
            Shape::Moveable moveable = &c;
            moveable.Call<Shape::Move>( 0.5, 0.25 );
            REQUIRE( c.x == 0.5 );
            REQUIRE( c.y == 0.25 );

            moveable.Call<Shape::Scale>(0.5);
            REQUIRE(c.r == 0.5);
        }

        SECTION("Transformable") 
        { // assing a Transformable with a Circle will result in linker error
          // as the function is not defined anywhere.
            //Shape::Transformable moveable = &c;
        }
    }

    SECTION("Rectangle")
    {
        Rectangle r;

        SECTION("Drawable")
        {
            Shape::Drawable drawable = &r;
            drawable.Call<Shape::SetColour>(Colour::green);
            REQUIRE(r.colour == Colour::green);
            REQUIRE( drawable.Call<Shape::Draw>() == Colour::green );
        }

        SECTION("Moveable")
        {
            Shape::Moveable moveable = &r;
            moveable.Call<Shape::Move>( 0.5, 0.25 );
            REQUIRE( r.x == 0.5 );
            REQUIRE( r.y == 0.25 );

            moveable.Call<Shape::Scale>(0.5);
            REQUIRE(r.width == 0.5);
            REQUIRE(r.height == 0.5);
        }

        SECTION("Transformable") 
        { // assing a Transformable with a Circle will result in linker error
          // as the function is not defined anywhere.
            //Shape::Transformable moveable = &c;
        }

    }
}