#pragma once
#include "base/delegates/function_delegate.h"
#include <cstddef>
#include <type_traits>

#define METADATA(x) [[clang::annotate(#x)]]

namespace Arieo::Base
{
    // template <typename TInstance, typename TInterface>
    // inline TInstance* castInterfaceToInstance(TInterface* interface)
    // {
    //     return static_cast<TInstance*>(interface);
    // }

    template<typename TInterface>
    class Interface
    {
    protected:
        TInterface* m_interface = nullptr;
    public:
        Interface() = default;
        Interface(std::nullptr_t) : m_interface(nullptr) {}
        Interface(TInterface* interface) : m_interface(interface) {}

        Interface& operator=(TInterface* interface)
        {
            m_interface = interface;
            return *this;
        }

        Interface& operator=(std::nullptr_t)
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

        bool operator==(Interface<TInterface> const& other) const { return m_interface == other.m_interface; }
        bool operator!=(Interface<TInterface> const& other) const { return m_interface != other.m_interface; }
        // TInterface* get() { return m_interface; }
        // const TInterface* get() const { return m_interface; }
    };

    template<typename T>
    class InterfaceInfo
    {
    };

    template <typename TInstance, typename TInterfaceLike>
    inline TInstance* castInterfaceToInstance(TInterfaceLike interface)
    {
        using InterfaceArg = std::remove_cv_t<std::remove_reference_t<TInterfaceLike>>;
        if constexpr (std::is_pointer_v<InterfaceArg>)
        {
            return static_cast<TInstance*>(interface);
        }
        else
        {
            return static_cast<TInstance*>(interface.operator->());
        }
    }
}