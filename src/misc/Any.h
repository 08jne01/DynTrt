#include <any>
#include "DynTrt.h"

namespace DynTrt
{
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

    

    template<typename TyStorage, typename TraitsType, typename... TyMethods>
    struct AnyValue
    {
        
        using data_pointer = void;

        template<typename TyMethod, typename T>
        using typed_method_pointer = typename TyMethod::template any_pointer<T>;

        template<typename TyMethod>
        using method_pointer = typename TyMethod::template any_pointer<data_pointer>;

        template<typename T>
        using vtable = std::tuple<typed_method_pointer<TyMethods,T>...>; //...
        
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
        requires( 
            (HasOverload<TraitsType, T, TyMethods> && ...)
        )
        AnyValue( const T& value ): 
            storage(std::move(value))
            //table(  ) // Fucking horrible why can't we just expand type_sequence...
        {
            static vtable<T> static_table = 
            std::make_tuple(
                typed_method_pointer<TyMethods,T>(&TraitsType::template Invoke<TyMethods>)... 
            );
            //static vtable static_table = std::make_tuple(method_pointer<TyMethods>(InvokeStatic<TyMethods, T>)... );
            table = reinterpret_cast<vtable<void>*>(&static_table);
        }

    private:
        template<typename Ty>
        static Ty Cast( const std::any& storage ) { std::any_cast<Ty>(storage); }

        template<typename Ty, bool check=false>
        static Ty Cast( const TyStorage& storage ) { return storage.template cast<Ty,check>(); } // not checked here since we are casting
    public:
        template<typename TyMethod, typename... Ts>
        requires (
            BoundMethod<TyMethod,TyMethods...> &&
            std::same_as<typename TyMethod::arguments, detail::type_sequence<Ts...>>
        )
        auto Call(Ts... args)
        {
            constexpr size_t n = detail::index_in_pack<TyMethod, TyMethods...>::value;
            return std::get<n>(*table)(&storage, args...);
        }

        template<typename TyMethod, typename... Ts>
        requires (
            BoundMethod<TyMethod,TyMethods...> &&
            std::same_as<typename TyMethod::arguments, detail::type_sequence<Ts...>>
        )
        auto Call(Ts... args) const
        {
            constexpr size_t n = detail::index_in_pack<TyMethod, TyMethods...>::value;
            return std::get<n>(*table)(&storage, args...);
        }

        template<typename T>
        T& Get() { return *storage.template cast<T*,true>(); }

    private:
        vtable<void>* table;
        TyStorage storage;
    };

    template<size_t N, typename TraitsType, typename... TyMethods>
    using AnySmall = AnyValue<AnyStorage<N>, TraitsType, TyMethods...>;

    template<typename TraitsType, typename... TyMethods>
    using Any = AnyValue<std::any, TraitsType, TyMethods...>;
}

