#pragma once
#include <concepts>
#include <type_traits>
#include <tuple>
#include <any>
namespace AnyTraits
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

        template<typename Ret, typename... Args>
        struct Signature<Ret(Args...)> {
            template<typename TyStorage>
            using any_pointer = Ret(*)(TyStorage&, Args...);

            template<typename TyStorage>
            using any_type = std::remove_pointer_t<any_pointer<TyStorage>>;
            //using overload_type = Ret(TAny,Args...);
            //using overload_pointer = Ret(*)(TAny,Args...);
            using return_type = Ret;
            using arguments = detail::type_sequence<Args...>;

            template<typename T>
            using full_arguments = detail::type_sequence<T, Args...>;
        };
    }   
    
    template<typename Method, typename... Methods>
    concept BoundMethod = (std::same_as<Method, Methods> || ... );

    template<typename Traits, typename Method, typename T, typename... Ts>
    concept HasMethod = requires(T value, Ts... args) {
        { Traits::template Invoke<Method>(value, args...) };
    };

    template<typename Trait, typename Ret, typename... Ts>
    concept HasOverload = requires(Ts... args) {
        { Trait::Invoke(args...) } -> std::same_as<Ret>;
    };

    // template<typename Trait, typename Ret, typename... Ts>
    // concept Invokable = requires( Ts... args )
    // {

    // }

    template<typename T>
    concept Anyable = 
        std::is_trivially_copyable_v<T> &&
        std::is_trivially_destructible_v<T>;

    template<std::size_t N = 16>
    struct AnyStorage
    {
        template<typename T>
        explicit AnyStorage(const T& value): type_hash(typeid(T).hash_code())
        {
            static_assert(sizeof(T) <= N,  "Type to big to fit into AnyStorage");
            memcpy(buffer, &value, sizeof(T));
        }

        template<typename T, bool checked=true>
        T cast() const
        {
            using V = std::remove_pointer_t<T>;

            if constexpr ( checked )
            {
                if ( typeid(V).hash_code() != type_hash )
                    throw std::bad_any_cast();
            }

            if constexpr ( std::is_same_v<T, std::remove_pointer_t<T>> )
                return *(T*)buffer;
            else
                return (V*)buffer;
        }
        alignas(std::max_align_t) std::byte buffer[N];
        std::size_t type_hash; 
    };

    

    template<typename TyStorage, typename TraitsType, typename... Methods>
    struct AnyValue
    {
        
        template<typename Method>
        using method_pointer = typename Method::template any_pointer<TyStorage>;
        using vtable = std::tuple<method_pointer<Methods>...>; //...
        
        // Just below we do p(InvokeStatic) where p is the corresponding function type (saved in Method)
        // same way we do vtable above but just passing InvokeStatic in.

        AnyValue( const AnyValue& )=default;
        AnyValue( AnyValue&& )=default;

        AnyValue& operator=( const AnyValue& )=default;
        AnyValue& operator=( AnyValue&& )=default;

        // 
        // Draw
        // Move
        //

        template<typename T>
        AnyValue( T value ): 
            storage(std::move(value)),
            table( std::make_tuple(method_pointer<Methods>(InvokeStatic<Methods, T>)... ) ) // Fucking horrible why can't we just expand type_sequence...
        {
        }
    private:
        template<typename Ty>
        static Ty Cast( const std::any& storage ) { std::any_cast<Ty>(storage); }

        template<typename Ty, bool check=false>
        static Ty Cast( const TyStorage& storage ) { return storage.template cast<Ty,check>(); } // not checked here since we are casting

        template<typename Method, typename T, typename Ret, typename Ty, typename... Ts>
        requires( HasOverload<Method, T, Ts...> )
        static Ret InvokeStatic( Ty storage, Ts... args )
        {
            T* value = Cast<T*>(storage);
            static_assert( HasMethod<TraitsType, Method, T, decltype(args)...>, "Object is missing required trait" );
            return Method::Invoke(*value, args...);
        }

        template<typename Method, typename T, typename Ret, typename Ty, typename... Ts>
        requires( ! HasOverload<Method, T, Ts...> )
        static Ret InvokeStatic( Ty storage, Ts... args )
        {
            T* value = Cast<T*>(storage);
            static_assert( HasMethod<TraitsType, Method, T, decltype(args)...>, "Object is missing required trait" );
            return TraitsType::template Invoke<Method>(*value, args...);
        }


        // template<typename Method, typename T>
        // requires( Method::defaulted )
        // auto MakeInvoke()
        // {
        //     return [](const AnyStorage<> storage, auto... args ){
        //         //T value = std::any_cast<T>(storage);
        //         T value = storage.cast<T>();
        //         static_assert( HasMethod<TraitsType, Method, T, decltype(args)...>, "Object is missing required trait" );
        //         return Method::Invoke(value, args...);
        //     };
        // }

        // template<typename Method, typename T>
        // requires( ! Method::defaulted )
        // auto MakeInvoke()
        // {
        //     return [](const AnyStorage<> storage, auto... args ){
        //         //T value = std::any_cast<T>(storage);
        //         T value = storage.cast<T>();
        //         static_assert( HasMethod<TraitsType, Method, T, decltype(args)...>, "Object is missing required trait" );
        //         //static_assert( HasOverload<Method, T, decltype(args)...>, "Object (T) is missing required trait" );
        //         return TraitsType::template Invoke<Method>(value, args...);
        //     };
        // }
    public:
        template<typename Method, typename... Ts>
        requires (
            BoundMethod<Method,Methods...> &&
            std::same_as<typename Method::arguments, detail::type_sequence<Ts...>>
        )
        auto Call(Ts... args)
        {
            constexpr size_t n = detail::index_in_pack<Method, Methods...>::value;
            return std::get<n>(table)(storage, args...);
        }

        template<typename T>
        T& Get() { return *storage.template cast<T*,true>(); }

    private:
        vtable table;
        TyStorage storage;
    };

    template<size_t N, typename TraitsType, typename... Methods>
    using AnySmall = AnyValue<AnyStorage<N>, TraitsType, Methods...>;

    template<typename TraitsType, typename... Methods>
    using Any = AnyValue<std::any, TraitsType, Methods...>;

    template<typename TraitType, typename T>
    struct Trait;

    template<typename TraitType, typename Ret, typename... Ts>
    struct Trait<TraitType, Ret(Ts...)> : detail::Signature<Ret(Ts...)>
    {
        static constexpr bool defaulted = false;
        using Method = detail::Signature<Ret(Ts...)>;

        template<typename T>
        inline static Ret Invoke( T value, Ts... args )
        {
            return TraitType::Invoke(value, args...);
        }
    };
}

//static void Shape::Invoke<Shape::Draw,AnyStorage<16>>(AnyStorage<16>)
