# DynTrt

DynTrts is a very small header only library (very WIP) initialy designed to emulate the runtime-concept however the scope of that is quite wide and for my selfish use case: value semmantics (although better) are unavailable. So we have wide pointer like objects similar to rust instead.

## Installation

If you wish to use cmake add the subdirectory and then add the `DynTrt` as a link library to your target.

## Examples

### Simple Example

#### Defining a Trait

```cpp
    // assume we have some definition for these objects elsewhere
    struct Circle;
    struct Rectangle;

    // Define Traits struct, this can contain multiple traits made up of the methods
    // defined within it.
    struct Shape
    {
        // This is required in all your Traits structs, it is assumed to exist
        // in order to call the methods created. This is always the same though.
        // It can simply be pasted in. I didn't want to subject anyone to macros.
        template<typename Method, typename T, typename... Ts>
        static inline Method::return_type Invoke( T*, Ts... );

        // This is a method definition. They always inherit from a named struct. The name
        // being the name of the function you would like to call it by.
        //
        // Method definitions are made of two parts:
        //                              default function - here we have no default.
        //                             |     function prototype
        //                             |     |       you can specify either ConstSelf or Self
        //                             v     v             v 
        struct Draw : DynTrt::Method<void, void(DynTrt::ConstSelf, uint8_t r, uint8_t g, uint8_t b)> {};
        //                                         Note you can give parameters names still ^
        //
        // So the pseudo code for this function is:
        // Draw(Self,r,g,b) -> void
        // 
        // Without a default parameter every type to be used with this Method must have Shape's
        // Invoke function explicitly specialised.

        // Defining a Method with a default.
        // Instead this time the first template parameter is the one we wish to use as the default
        // DynTrt will try to find and Invoke function in there matching the parameters given.
        //                             v
        struct Move : DynTrt::Method<Move, void(DynTry::Self, double x, double y)> {
            // very important to define it as STATIC
            template<typename T>
            static void Invoke( T* self, double x, double y ) { 
                self->Move(x,y); // assuming shape has a Move member
            }
        };

        // Lastly we must define the actual Traits you can have as many as you wish, 
        // here we just define one called drawable since this is what this trait represents.
        using Drawable = DynTrt::Trait<Shape, Draw, Move>;
        //                           ^       ^
        //                           |       |.... list of Methods this trait has.
        //                           | Traits object this belongs to - just like defaults this is only 
        //                              so it can find the appropriate Invoke overload if you wanted
        //                              you could specify a different location.
        //                         
    };
```

That was defining a trait. In the example above we Draw is not defined for any of our structs. Thus we must specialised the Shape::Invoke template:

```cpp

template<>                       // v const since we specified ConstSelf
void Shape::Invoke<Shape::Draw>( const Rectangle* self, uint8_t r, uint8_t g, uint8_t b )
{
    // do something with this
}

template<> // Circle must also be defined if we plan to use it with the Shape::Trait trait.
void Shape::Invoke<Shape::Draw>( const Circle* self, uint8_t r, uint8_t g, uint8_t b )
{
    // do something with this
}
```

#### Invoking Functions

A DynTrt trait acts like a pointer to a virtual interface. It can be casted similar to dynamic cast (and it stores type information so it will return nullptr if type is incorrect). Calling a function on a DynTrt object that satisfies a trait is really straight forward:

```cpp

Circle c; // <--- assume we already have it

// Erase the type
// drawable acts a interface pointer.
Shape::Drawable drawable = &c; // stores a pointer to c

//                    v function we are calling
drawable.Call<Shape::Move>(0.5, 0.25);
drawable.Call<Shape::Draw>( 255, 0, 255 );
```

Simple as that. Worth noting calls should respect the constness of the self pointer. Meaning a `Shape::Draw` here can be called in a const context and `Shape::Move` cannot. This means if you passed const pointer to drawable you would only be able to call `Shape::Draw`.

We can reassign drawable to another object like so:
```cpp
Rectangle r;
Shape::Drawable drawable = &r;
```

If you have a function which takes a specific trait then it is enough to pass the pointer your object.

```cpp
void Draw(const Shape::Drawable drawable)
{
    drawable.Call<Shape::Draw>(255, 0, 0);
}

// we can call draw like so
Rectangle r;
Draw(&r);
```

#### Dynamic Casting

You can dynamic cast back to your type:

```cpp
Shape::Drawable drawable; // assume we have some actual instance

// this is similar to dynamic cast
Circle* circle = drawable.Get<Circle>();
// if it fails due to incorrect types then nullptr is returned -> this includes incorrect const.
```

## How it Works

Like in Rust traits here are wide pointers, they store a pointer to the variable and a pointer to the virtual function pointer table.

The virtual function table (vtable) maps the function call type `Shape::Draw`, `Shape::Move` to the function pointers `Shape::Invoke<Shape::Draw>` etc. When a type is passed into the constructor of a trait data pointer is saved and the vtable pointer is retrieved based on the type info (there is one vtable for each instantiated type in a trait).

Note the vtables are only ever created at compile time so if you don't have any instances of your traits you will not have any vtables.
