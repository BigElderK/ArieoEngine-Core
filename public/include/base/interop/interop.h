#pragma once
#include <cstddef>
#include <type_traits>
#include "base/compile/ct_check.h"
#include "base/interop/instance.h"
#include "core/logger/logger.h"

namespace Arieo::Core
{
    class ModuleManager;
}

namespace Arieo::Base::Interop
{
    template<typename T>
    class SharedRef
    {
    protected:
        T* m_ptr = nullptr;
        RefControlBlock* m_ref_control_block = nullptr;

    public:
        // --- Constructors ---
        SharedRef() noexcept = default;
        SharedRef(std::nullptr_t) noexcept {}

        SharedRef(T* ptr, RefControlBlock* ref_control_block) noexcept
            : m_ptr(ptr), m_ref_control_block(ref_control_block)
        {
            if (m_ref_control_block) m_ref_control_block->addRef();
        }

        // Copy
        SharedRef(const SharedRef& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            if (m_ref_control_block) m_ref_control_block->addRef();
        }

        // Move
        SharedRef(SharedRef&& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            other.m_ptr = nullptr;
            other.m_ref_control_block = nullptr;
        }

        // Aliasing constructor (like shared_ptr aliasing ctor)
        template<typename U>
        SharedRef(const SharedRef<U>& other, T* ptr) noexcept
            : m_ptr(ptr), m_ref_control_block(other.m_ref_control_block)
        {
            if (m_ref_control_block) m_ref_control_block->addRef();
        }

        // Converting constructor (implicit upcast)
        template<typename U>
            requires std::is_convertible_v<U*, T*>
        SharedRef(const SharedRef<U>& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            if (m_ref_control_block) m_ref_control_block->addRef();
        }

        template<typename U>
            requires std::is_convertible_v<U*, T*>
        SharedRef(SharedRef<U>&& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            other.m_ptr = nullptr;
            other.m_ref_control_block = nullptr;
        }

        // --- Destructor ---
        ~SharedRef()
        {
            if (m_ref_control_block) m_ref_control_block->release();
        }

        // --- Assignment ---
        SharedRef& operator=(const SharedRef& other) noexcept
        {
            if (this != &other)
            {
                SharedRef tmp(other);
                swap(tmp);
            }
            return *this;
        }

        SharedRef& operator=(SharedRef&& other) noexcept
        {
            if (this != &other)
            {
                SharedRef tmp(std::move(other));
                swap(tmp);
            }
            return *this;
        }

        template<typename U>
            requires std::is_convertible_v<U*, T*>
        SharedRef& operator=(const SharedRef<U>& other) noexcept
        {
            SharedRef tmp(other);
            swap(tmp);
            return *this;
        }

        template<typename U>
            requires std::is_convertible_v<U*, T*>
        SharedRef& operator=(SharedRef<U>&& other) noexcept
        {
            SharedRef tmp(std::move(other));
            swap(tmp);
            return *this;
        }

        SharedRef& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        // --- Modifiers ---
        void reset() noexcept
        {
            SharedRef tmp;
            swap(tmp);
        }

        void swap(SharedRef& other) noexcept
        {
            std::swap(m_ptr, other.m_ptr);
            std::swap(m_ref_control_block, other.m_ref_control_block);
        }

        // --- Observers ---
        T* get() const noexcept { return m_ptr; }

        template<typename U = T>
        U& operator*() const noexcept requires (!std::is_void_v<U>) { return *m_ptr; }
        T* operator->() const noexcept { return m_ptr; }

        uint32_t useCount() const noexcept
        {
            return m_ref_control_block ? m_ref_control_block->useCount() : 0;
        }

        explicit operator bool() const noexcept { return m_ptr != nullptr; }

        // --- Comparisons ---
        bool operator==(const SharedRef& other) const noexcept { return m_ptr == other.m_ptr; }
        bool operator!=(const SharedRef& other) const noexcept { return m_ptr != other.m_ptr; }
        bool operator<(const SharedRef& other) const noexcept { return m_ptr < other.m_ptr; }
        bool operator==(std::nullptr_t) const noexcept { return m_ptr == nullptr; }
        bool operator!=(std::nullptr_t) const noexcept { return m_ptr != nullptr; }

        // Allow SharedRef<U> to access our internals for converting ctors
        template<typename U>
        friend class SharedRef;

        template<typename U>
        friend class WeakRef;

        template <typename TInstance>
        TInstance* castToInstance()
        {
            const uint32_t hash = static_cast<uint32_t>(std::hash<std::string_view>{}(typeid(TInstance).name()));
            uint32_t* type_hash_ptr = reinterpret_cast<uint32_t*>(reinterpret_cast<std::byte*>(static_cast<TInstance*>(m_ptr)) + sizeof(TInstance));
            if(*type_hash_ptr != hash)            
            {
                Core::Logger::fatal("Type hash mismatch when casting instance. Expected: {}, Actual: {}", hash, *type_hash_ptr);
                return nullptr;
            }
            return static_cast<TInstance*>(m_ptr);
        }
    };
    static_assert(Base::ct::DLLBoundaryLayoutSafe<SharedRef<void>>, "SharedRef<T> must be DLL boundary safe");

    template<typename T>
    class WeakRef
    {
    protected:
        T* m_ptr = nullptr;
        RefControlBlock* m_ref_control_block = nullptr;
    public:
        // --- Constructors ---
        WeakRef() noexcept = default;

        // From SharedRef
        WeakRef(const SharedRef<T>& shared) noexcept
            : m_ptr(shared.m_ptr), m_ref_control_block(shared.m_ref_control_block)
        {
            if (m_ref_control_block) m_ref_control_block->addWeakRef();
        }

        // Converting from SharedRef<U>
        template<typename U>
            requires std::is_convertible_v<U*, T*>
        WeakRef(const SharedRef<U>& shared) noexcept
            : m_ptr(shared.m_ptr), m_ref_control_block(shared.m_ref_control_block)
        {
            if (m_ref_control_block) m_ref_control_block->addWeakRef();
        }

        // Copy
        WeakRef(const WeakRef& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            if (m_ref_control_block) m_ref_control_block->addWeakRef();
        }

        // Move
        WeakRef(WeakRef&& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            other.m_ptr = nullptr;
            other.m_ref_control_block = nullptr;
        }

        // Converting copy
        template<typename U>
            requires std::is_convertible_v<U*, T*>
        WeakRef(const WeakRef<U>& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            if (m_ref_control_block) m_ref_control_block->addWeakRef();
        }

        // Converting move
        template<typename U>
            requires std::is_convertible_v<U*, T*>
        WeakRef(WeakRef<U>&& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            other.m_ptr = nullptr;
            other.m_ref_control_block = nullptr;
        }

        // --- Destructor ---
        ~WeakRef()
        {
            if (m_ref_control_block) m_ref_control_block->releaseWeak();
        }

        // --- Assignment ---
        WeakRef& operator=(const WeakRef& other) noexcept
        {
            if (this != &other)
            {
                WeakRef tmp(other);
                swap(tmp);
            }
            return *this;
        }

        WeakRef& operator=(WeakRef&& other) noexcept
        {
            if (this != &other)
            {
                WeakRef tmp(std::move(other));
                swap(tmp);
            }
            return *this;
        }

        WeakRef& operator=(const SharedRef<T>& shared) noexcept
        {
            WeakRef tmp(shared);
            swap(tmp);
            return *this;
        }

        template<typename U>
            requires std::is_convertible_v<U*, T*>
        WeakRef& operator=(const SharedRef<U>& shared) noexcept
        {
            WeakRef tmp(shared);
            swap(tmp);
            return *this;
        }

        template<typename U>
            requires std::is_convertible_v<U*, T*>
        WeakRef& operator=(const WeakRef<U>& other) noexcept
        {
            WeakRef tmp(other);
            swap(tmp);
            return *this;
        }

        template<typename U>
            requires std::is_convertible_v<U*, T*>
        WeakRef& operator=(WeakRef<U>&& other) noexcept
        {
            WeakRef tmp(std::move(other));
            swap(tmp);
            return *this;
        }

        // --- Modifiers ---
        void reset() noexcept
        {
            WeakRef tmp;
            swap(tmp);
        }

        void swap(WeakRef& other) noexcept
        {
            std::swap(m_ptr, other.m_ptr);
            std::swap(m_ref_control_block, other.m_ref_control_block);
        }

        // --- Observers ---
        uint32_t useCount() const noexcept
        {
            return m_ref_control_block ? m_ref_control_block->useCount() : 0;
        }

        bool expired() const noexcept
        {
            return useCount() == 0;
        }

        // Attempt to obtain a SharedRef. Returns empty SharedRef if expired.
        SharedRef<T> lock() const noexcept
        {
            if (m_ref_control_block && m_ref_control_block->tryAddRef())
            {
                // tryAddRef already incremented the strong count,
                // construct SharedRef without addRef
                SharedRef<T> result;
                result.m_ptr = m_ptr;
                result.m_ref_control_block = m_ref_control_block;
                return result;
            }
            return SharedRef<T>();
        }

        template<typename U>
        friend class WeakRef;

        template<typename U>
        friend class SharedRef;
    };
    static_assert(Base::ct::DLLBoundaryLayoutSafe<WeakRef<void>>, "WeakRef<T> must be DLL boundary safe");

    template<typename T>
    class UniqueRef
    {
    protected:
        T* m_ptr = nullptr;
        RefControlBlock* m_ref_control_block = nullptr;

    public:
        // --- Constructors ---
        UniqueRef() noexcept = default;
        UniqueRef(std::nullptr_t) noexcept {}

        UniqueRef(T* ptr, RefControlBlock* ref_control_block) noexcept
            : m_ptr(ptr), m_ref_control_block(ref_control_block) {}

        // No copy
        UniqueRef(const UniqueRef&) = delete;
        UniqueRef& operator=(const UniqueRef&) = delete;

        // Move
        UniqueRef(UniqueRef&& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            other.m_ptr = nullptr;
            other.m_ref_control_block = nullptr;
        }

        // Converting move
        template<typename U>
            requires std::is_convertible_v<U*, T*>
        UniqueRef(UniqueRef<U>&& other) noexcept
            : m_ptr(other.m_ptr), m_ref_control_block(other.m_ref_control_block)
        {
            other.m_ptr = nullptr;
            other.m_ref_control_block = nullptr;
        }

        // --- Destructor ---
        ~UniqueRef()
        {
            if (m_ref_control_block) m_ref_control_block->release();
        }

        // --- Assignment ---
        UniqueRef& operator=(UniqueRef&& other) noexcept
        {
            if (this != &other)
            {
                UniqueRef tmp(std::move(other));
                swap(tmp);
            }
            return *this;
        }

        template<typename U>
            requires std::is_convertible_v<U*, T*>
        UniqueRef& operator=(UniqueRef<U>&& other) noexcept
        {
            UniqueRef tmp(std::move(other));
            swap(tmp);
            return *this;
        }

        UniqueRef& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        // --- Modifiers ---
        void reset() noexcept
        {
            UniqueRef tmp;
            swap(tmp);
        }

        T* release() noexcept
        {
            T* ptr = m_ptr;
            m_ptr = nullptr;
            m_ref_control_block = nullptr;
            return ptr;
        }

        void swap(UniqueRef& other) noexcept
        {
            std::swap(m_ptr, other.m_ptr);
            std::swap(m_ref_control_block, other.m_ref_control_block);
        }

        // --- Observers ---
        T* get() const noexcept { return m_ptr; }

        template<typename U = T>
        U& operator*() const noexcept requires (!std::is_void_v<U>) { return *m_ptr; }
        T* operator->() const noexcept { return m_ptr; }

        explicit operator bool() const noexcept { return m_ptr != nullptr; }

        // --- Comparisons ---
        bool operator==(const UniqueRef& other) const noexcept { return m_ptr == other.m_ptr; }
        bool operator!=(const UniqueRef& other) const noexcept { return m_ptr != other.m_ptr; }
        bool operator==(std::nullptr_t) const noexcept { return m_ptr == nullptr; }
        bool operator!=(std::nullptr_t) const noexcept { return m_ptr != nullptr; }

        template<typename U>
        friend class UniqueRef;
    };
    static_assert(Base::ct::DLLBoundaryLayoutSafe<UniqueRef<void>>, "UniqueRef<T> must be DLL boundary safe");

    template<typename T>
    class RawRef
    {
    protected:
        T* m_ptr = nullptr;

    public:
        RawRef() noexcept = default;
        RawRef(std::nullptr_t) noexcept {}
        RawRef(T* ptr) noexcept : m_ptr(ptr) {}

        RawRef(const RawRef&) noexcept = default;
        RawRef(RawRef&&) noexcept = default;
        RawRef& operator=(const RawRef&) noexcept = default;
        RawRef& operator=(RawRef&&) noexcept = default;
        ~RawRef() = default;

        template<typename U>
            requires std::is_convertible_v<U*, T*>
        RawRef(const RawRef<U>& other) noexcept : m_ptr(other.get()) {}

        RawRef& operator=(std::nullptr_t) noexcept { m_ptr = nullptr; return *this; }
        RawRef& operator=(T* ptr) noexcept { m_ptr = ptr; return *this; }

        void reset() noexcept { m_ptr = nullptr; }

        T* get() const noexcept { return m_ptr; }
        template<typename U = T>
        U& operator*() const noexcept requires (!std::is_void_v<U>) { return *m_ptr; }
        T* operator->() const noexcept { return m_ptr; }

        explicit operator bool() const noexcept { return m_ptr != nullptr; }

        bool operator==(const RawRef& other) const noexcept { return m_ptr == other.m_ptr; }
        bool operator!=(const RawRef& other) const noexcept { return m_ptr != other.m_ptr; }
        bool operator<(const RawRef& other) const noexcept { return m_ptr < other.m_ptr; }
        bool operator==(std::nullptr_t) const noexcept { return m_ptr == nullptr; }
        bool operator!=(std::nullptr_t) const noexcept { return m_ptr != nullptr; }

        template<typename U>
        friend class RawRef;
    };
    static_assert(Base::ct::DLLBoundaryLayoutSafe<RawRef<void>>, "RawRef<T> must be DLL boundary safe");

    template<typename TInterface, typename TInstance, typename... Args>
    static SharedRef<TInterface> createInstance(Args&&... args)
    {
        // Allocate memory for T + type hash
        Instance<TInstance>* new_instance = new (Base::Memory::malloc(sizeof(Instance<TInstance>))) Instance<TInstance>(std::forward<Args>(args)...);
        RefControlBlock* ref_block = new (Base::Memory::malloc(sizeof(RefControlBlock))) RefControlBlock(
            new_instance,
            [](void* context) -> void{
                auto* instance = static_cast<Instance<TInstance>*>(context);
                instance->~Instance<TInstance>();
                Base::Memory::free(instance);
            },
            [](RefControlBlock* this_block) -> void{
                this_block->~RefControlBlock();
                Base::Memory::free(this_block);
            }
        );

        return SharedRef<TInterface>(static_cast<TInterface*>(new_instance->operator->()), ref_block);
    }
}

namespace Arieo::Base
{
    class IReferenceCounted
    {
    public:        
        virtual void addRef() = 0;
        virtual void release() = 0;
    };

    class IBufferView
    {
    public:
        virtual void* getBuffer() const = 0;
        virtual size_t getSize() const = 0;
    };

    class IBuffer
        : IReferenceCounted
    {
    public:
        virtual void* getBuffer() const = 0;
        virtual size_t getSize() const = 0;
    };
}

namespace Arieo::Base
{
    template<typename T>
    class InteropOld;

    template<>
    class InteropOld<std::string_view>
    {
    private:
        void* buf = nullptr;
        size_t size = 0;
    public:
        InteropOld() = delete;
        InteropOld(const InteropOld&) noexcept = delete;
        InteropOld(InteropOld&&) noexcept = delete;
        InteropOld& operator=(const InteropOld&) noexcept = delete;
        InteropOld& operator=(InteropOld&&) noexcept = delete;
        ~InteropOld() = default;

        InteropOld(const std::string_view& str)
            : buf((void*)str.data()), size(str.size()) {}

        InteropOld(const char str[])
            : buf((void*)str), size(std::strlen(str)) {}

        std::string getString() const
        {
            return std::string((char*)buf, size);
        }
    };
    static_assert(Base::ct::DLLBoundarySafeCheck<InteropOld<std::string_view>>, "InteropOld<std::string_view> must be DLL boundary safe");

    template<>
    class InteropOld<IBufferView>
    {
    private:
        void* m_buf = nullptr;
        size_t m_size = 0;
    public:
        InteropOld() = delete;
        InteropOld(const InteropOld&) noexcept = delete;
        InteropOld(InteropOld&&) noexcept = delete;
        InteropOld& operator=(const InteropOld&) noexcept = delete;
        InteropOld& operator=(InteropOld&&) noexcept = delete;
        ~InteropOld() = default;

        InteropOld(void* data, size_t len) : m_buf(data), m_size(len) {}

        void* getBuffer() const { return m_buf; }
        size_t getSize() const { return m_size; }
    };
    static_assert(Base::ct::DLLBoundarySafeCheck<InteropOld<IBufferView>>, "InteropOld<IBufferView> must be DLL boundary safe");

    template<>
    class InteropOld<IBuffer>
    {
    private:
        void* m_buf = nullptr;
        size_t m_size = 0;
        void (*m_release_func)(void*, size_t) = nullptr;

    public:
        InteropOld() = delete;
        InteropOld(const InteropOld&) noexcept = default;
        InteropOld(InteropOld&&) noexcept = default;
        InteropOld& operator=(const InteropOld&) noexcept = default;
        InteropOld& operator=(InteropOld&&) noexcept = default;
        ~InteropOld() = default;

        InteropOld(void* data, size_t len, void (*release_func)(void*, size_t)) 
            : m_buf(data), m_size(len), m_release_func(release_func) {}

        void* getBuffer() const { return m_buf; }
        size_t getSize() const { return m_size; }

        void release()
        {
            if(m_release_func)
            {
                m_release_func(m_buf, m_size);
                m_buf = nullptr;
                m_size = 0;
            }
        }
    };
    static_assert(Base::ct::DLLBoundarySafeCheck<InteropOld<IBufferView>>, "InteropOld<IBufferView> must be DLL boundary safe");

    template<class TInterface>
    class InteropOld    
    {
    private:
    
    private:
        TInterface* m_interface = nullptr;
        InteropOld(TInterface* interface) : m_interface(interface) {}
        friend class Arieo::Core::ModuleManager;

        template<typename T>
        friend class Instance;
    public:
        InteropOld() = default;
        InteropOld(std::nullptr_t) : m_interface(nullptr) {}
        InteropOld(const InteropOld&) noexcept = default;
        InteropOld(InteropOld&&) noexcept = default;
        InteropOld& operator=(const InteropOld&) noexcept = default;
        InteropOld& operator=(InteropOld&&) noexcept = default;
        ~InteropOld() = default;

        InteropOld& operator=(std::nullptr_t)
        {
            m_interface = nullptr;
            return *this;
        }

        TInterface* operator->() { return m_interface; }
        const TInterface* operator->() const { return m_interface; }

        // TInterface& operator*() { return *m_interface; }
        // const TInterface& operator*() const { return *m_interface; }

        // operator TInterface*() { return m_interface; }
        // operator const TInterface*() const { return m_interface; }

        explicit operator bool() const { return m_interface != nullptr; }

        bool operator==(std::nullptr_t) const { return m_interface == nullptr; }
        bool operator!=(std::nullptr_t) const { return m_interface != nullptr; }

        bool operator==(InteropOld<TInterface> const& other) const { return m_interface == other.m_interface; }
        bool operator!=(InteropOld<TInterface> const& other) const { return m_interface != other.m_interface; }

        template<typename TInstance, typename... Args>
        static InteropOld<TInterface> createAs(Args&&... args)
        {
            Instance<TInstance>* new_instance = new (Base::Memory::malloc(sizeof(Instance<TInstance>))) Instance<TInstance>(std::forward<Args>(args)...);
            return new_instance->template queryInterface<TInterface>();
        }

        template<typename TInstance>
        static void destroyAs(InteropOld<TInterface>&& InteropOld)
        {
            const uint32_t hash = static_cast<uint32_t>(std::hash<std::string_view>{}(typeid(TInstance).name()));
            Instance<TInstance>* instance = reinterpret_cast<Instance<TInstance>*>(InteropOld.m_interface);
            
            if(instance->m_type_hash != hash)
            {
                Core::Logger::fatal("Type hash mismatch when destroying instance. Expected: {}, Actual: {}", hash, instance->m_type_hash);
                return;
            }

            instance->~Instance<TInstance>();
            Arieo::Base::Memory::free(InteropOld.m_interface);

            InteropOld.m_interface = nullptr;
        }

        template <typename TInstance>
        TInstance* castTo()
        {
            const uint32_t hash = static_cast<uint32_t>(std::hash<std::string_view>{}(typeid(TInstance).name()));
            Instance<TInstance>* instance = reinterpret_cast<Instance<TInstance>*>(m_interface);

            if(instance->m_type_hash != hash)         
            {
                Core::Logger::fatal("Type hash mismatch when casting instance. Expected: {}, Actual: {}", hash, instance->m_type_hash);
                return nullptr;
            }
            return static_cast<TInstance*>(m_interface);
        }
    };
    static_assert(Base::ct::DLLBoundarySafeCheck<InteropOld<void>>, "InteropOld<T> must be DLL boundary safe");
}

// Hash and equality for Interface so it can be used in unordered_set/map
namespace std {
    template <typename TInterface>
    struct hash<Arieo::Base::InteropOld<TInterface>> {
        std::size_t operator()(const Arieo::Base::InteropOld<TInterface>& iface) const noexcept {
            return reinterpret_cast<std::size_t>(iface.operator->());
        }
    };
    template <typename TInterface>
    struct equal_to<Arieo::Base::InteropOld<TInterface>> {
        bool operator()(const Arieo::Base::InteropOld<TInterface>& lhs, const Arieo::Base::InteropOld<TInterface>& rhs) const noexcept {
            return lhs.operator->() == rhs.operator->();
        }
    };

    // Hash and equality for SharedRef so it can be used in unordered_set/map
    template <typename T>
    struct hash<Arieo::Base::Interop::SharedRef<T>> {
        std::size_t operator()(const Arieo::Base::Interop::SharedRef<T>& ref) const noexcept {
            return reinterpret_cast<std::size_t>(ref.get());
        }
    };
    template <typename T>
    struct equal_to<Arieo::Base::Interop::SharedRef<T>> {
        bool operator()(const Arieo::Base::Interop::SharedRef<T>& lhs, const Arieo::Base::Interop::SharedRef<T>& rhs) const noexcept {
            return lhs.get() == rhs.get();
        }
    };
}