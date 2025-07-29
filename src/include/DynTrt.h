// MIT License

// Copyright (c) 2025 Joshua Nelson

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <concepts>
#include <type_traits>
#include <tuple>
#include <any>
namespace DynTrt
{
    namespace detail
    {
        template<typename T, typename... Ts>
        struct index_in_pack;

        template<typename T, typename TyFirst, typename... TyRemaining>
        struct index_in_pack<T,TyFirst,TyRemaining...> {
            static constexpr std::size_t value = std::is_same_v<T,TyFirst> ? 0 : 1 + index_in_pack<T, TyRemaining...>::value;
        };

        template<typename T>
        struct index_in_pack<T> {
            static constexpr std::size_t value = 0;
            static_assert(sizeof(T), "Not found in pack");
        };

        template<typename... Ts>
        struct type_sequence {};

        template<typename... Ts> struct head;
        template<typename T, typename... Ts>
        struct head<type_sequence<T,Ts...>>
        {
            using type = T;
        };

        template<typename... Ts> struct tail;
        template<typename T, typename... Ts>
        struct tail<type_sequence<T,Ts...>>
        {
            using type = type_sequence<Ts...>;
        };

        template<typename T>
        using tail_t = typename tail<T>::type;
        
        // Get first in type_sequence
        template<typename T>
        using head_t = typename head<T>::type;

        template<typename T>
        struct Signature;

        template<typename Ret, typename T, typename... Args>
        requires( std::same_as<T, void> || std::same_as<T, const void> )
        struct Signature<Ret(T*, Args...)> {

            static constexpr bool is_const = std::is_const_v<T>;

            template<typename Ty>
            using const_correct = std::conditional_t<std::is_const_v<T>, std::add_const_t<Ty>, Ty>;

            template<typename TyStorage>
            using any_pointer = Ret(*)(const_correct<TyStorage>*, Args...);

            using return_type = Ret;
            using arguments = detail::type_sequence<Args...>;

            template<typename Ty>
            using full_arguments = detail::type_sequence<Ty, Args...>;

            template<typename Trait, typename Ty, typename TyMethod>
            static Ret DummyInvoke()
            {
                return Trait::template Invoke<TyMethod>((Ty*)nullptr,Args{}...);
            }
        };
    }   
    
    template<typename TyMethod, typename... TyMethods>
    concept BoundMethod = (std::same_as<TyMethod, TyMethods> || ... );

    template<typename Traits, typename TyMethod, typename T, typename... Ts>
    concept HasMethod = requires(T value, Ts... args) {
        { Traits::template Invoke<TyMethod>(value, args...) };
    };

    template<typename Trait, typename T, typename TyMethod>
    concept HasOverload = requires() {
        { TyMethod::template DummyInvoke<Trait,T,TyMethod>() } -> std::same_as<typename TyMethod::return_type>;
    };

    template<typename TraitsType, typename... TyMethods>
    struct Trait
    {
    public:
        struct TypeInfo
        {
            std::size_t type_hash;
        };
        using data_pointer = void;

        template<typename TyMethod, typename T>
        using typed_method_pointer = typename TyMethod::template any_pointer<T>;

        template<typename TyMethod>
        using method_pointer = typename TyMethod::template any_pointer<data_pointer>;

        template<typename T>
        using vtable = std::tuple<typed_method_pointer<TyMethods,T>...,TypeInfo>; //...
        
    public:
        ~Trait()=default;
        Trait( const Trait& )=default;
        Trait( Trait&& )=default;

        Trait& operator=( const Trait& )=default;
        Trait& operator=( Trait&& )=default;

        template<typename T>
        requires( 
            (HasOverload<TraitsType, T, TyMethods> && ...)
        )
        Trait( T* value ): pointer(value)
        {
            static vtable<T> static_table = 
            std::make_tuple(
                typed_method_pointer<TyMethods,T>(&TyMethods::template Invk<TraitsType, TyMethods>)..., 
                TypeInfo{typeid(T).hash_code()} 
            );
            table = reinterpret_cast<vtable<void>*>(&static_table);
        }

        template<typename T>
        requires( 
            (HasOverload<TraitsType, T, TyMethods> && ...)
        )
        Trait( const T* value ): pointer(const_cast<T*>(value))
        {
            static vtable<T> static_table = 
            std::make_tuple(
                typed_method_pointer<TyMethods,T>(&TyMethods::template Invk<TraitsType, TyMethods>)..., 
                TypeInfo{typeid(const T).hash_code()} 
            );
            table = reinterpret_cast<vtable<void>*>(&static_table);
        }
        
        template<typename TyMethod, typename... Ts>
        requires (
            BoundMethod<TyMethod,TyMethods...> &&
            ! TyMethod::is_const &&
            std::same_as<typename TyMethod::arguments, detail::type_sequence<Ts...>>
        )
        inline auto Call(Ts... args)
        {
            constexpr size_t n = detail::index_in_pack<TyMethod, TyMethods...>::value;
            return std::get<n>(*table)(pointer, args...);
        }

        template<typename TyMethod, typename... Ts>
        requires (
            TyMethod::is_const &&
            BoundMethod<TyMethod,TyMethods...> &&
            std::same_as<typename TyMethod::arguments, detail::type_sequence<Ts...>>
        )
        inline auto Call(Ts... args) const
        {
            constexpr size_t n = detail::index_in_pack<TyMethod, TyMethods...>::value;
            return std::get<n>(*table)(pointer, args...);
        }

        template<typename T>
        inline T* Get() 
        {
            constexpr size_t n = detail::index_in_pack<TypeInfo, TyMethods...>::value;
            const std::size_t type_hash = std::get<n>(*table ).type_hash;
            if ( type_hash != typeid(T).hash_code() )
                return nullptr;

            return reinterpret_cast<T*>(pointer);
        }

        template<typename T>
        inline const T* Get() const
        {
            constexpr size_t n = detail::index_in_pack<TypeInfo, TyMethods...>::value;
            const std::size_t type_hash = std::get<n>(*table).type_hash;
            if ( type_hash != typeid(T).hash_code() )
                return nullptr;

            return reinterpret_cast<const T*>(pointer);
        }

    public:
        vtable<void>* table = nullptr;
        void* pointer = nullptr;
    };

    using Self = void*;
    using ConstSelf = const void*;

    /// TyDefaultMethod : Method or void. If void no default. If method will try to call Method::Invoke(self,args...) (usually templated)
    /// TyFunction : Function prototype including Self. Usually ReturnType(DynTrt::Self, Type1, Type2... TypeN);
    /// Note: self can be either DynTrt::Self or DynTrt::ConstSelf for const functions.
    template<typename TyDefaultMethod, typename TyFunction>
    struct Method;

    template<typename Ret, typename Ty, typename... Ts>
    struct Method<void, Ret(Ty, Ts...)> : detail::Signature<Ret(Ty, Ts...)>
    {
        static constexpr bool defaulted = false;
        template<typename Trait, typename TyMethod, typename T>
        static inline Ret Invk( T* value, Ts... args )
        {
            return Trait::template Invoke<TyMethod>(value, args...);
        }
    };

    template<typename TyDefaultMethod, typename Ret, typename Ty, typename... Ts>
    struct Method<TyDefaultMethod, Ret(Ty, Ts...)> : detail::Signature<Ret(Ty, Ts...)>
    {
        static constexpr bool defaulted = true;
        using TyMethod = detail::Signature<Ret(Ts...)>;

        template<typename Trait, typename TyMethod, typename T>
        requires( std::derived_from<TyDefaultMethod, Method> )
        static inline Ret Invk( T* value, Ts... args )
        {
            return TyDefaultMethod::Invoke(value, args...);
        }
    };

    // New Method of doing Traits
    template<typename... Ts>
    struct Traits
    {
        template<typename Method, typename T>
        using method_pointer = DynTrt::detail::Signature<Method>::template any_pointer<T>;

        template<typename T>
        using vtable = std::tuple<method_pointer<Ts,T>...>;

        template<auto... methods, typename T>
        void make_trait(T* ptr)
        {
            static vtable<T> vtable_pointer = std::make_tuple(
                methods...
            );

            pointer = ptr;
            table = reinterpret_cast<vtable<void>*>(&vtable_pointer);
        }

        template<size_t function, typename... TyArgs>
        auto call( TyArgs... args )
        {
            return std::get<function>( *table )( pointer, args... );
        }

        template<size_t function, typename... TyArgs>
        auto call( TyArgs... args ) const
        {
            return std::get<function>( *table )( pointer, args... );
        }

        vtable<void>* table;
        void* pointer; 
    };

    // // You can inherit from this instead of using the below declaration however
    // // it will limit your definitions to only the global namespace so it's not advised.
    // struct Traits
    // {
    //     // Default template to specialise with the respective Methods and Types.
    //     template<typename Method, typename T, typename... Ts>
    //     static inline Method::return_type Invoke( T*, Ts... );
    // };
}

//static void Shape::Invoke<Shape::Draw,AnyStorage<16>>(AnyStorage<16>)
