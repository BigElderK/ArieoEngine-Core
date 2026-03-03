#pragma once
#include "base/delegates/function_delegate.h"
#include "core/logger/logger.h"
#include <cstddef>
#include <type_traits>

namespace Arieo::Base
{
    // Forward declaration for code-generated InterfaceInfo specializations
    template<typename T>
    class InterfaceInfo;
}

#define METADATA(x) [[clang::annotate(#x)]]

namespace Arieo::Base
{
    template<typename T>
    class IDataArray
    {
    public:
        virtual const T* getData() = 0;
        virtual size_t getItemCount() = 0;
        virtual size_t getDataSize() = 0;
    };

    using IBuffer = IDataArray<void>;

    template<typename T>
    class IDataList
    {
    public:
        virtual size_t getItemCount() = 0;
        virtual const T getItem(size_t index) = 0;
    };
}

namespace Arieo::Base
{
    template<typename T>
    class DataArray final
        : public Base::IDataArray<T>
    {
    private:
        static_assert(std::is_void_v<T> || ct::PLAIN_DATA_CHECK<T> || ct::DLLBoundarySafeCheck<T>, "DataArray requires plain data or DLL-safe types");
    private:
        T* m_data = nullptr;
        size_t m_count = 0;
        std::function<void(T*, size_t)> m_destroy_delegate;
    public:
        DataArray() = delete;
        DataArray(
             T* data, 
             size_t count,
             std::function<void(T*, size_t)>&& destroy_delegate)
            : m_data(data), 
            m_count(count), 
            m_destroy_delegate(std::move(destroy_delegate))
        {
        }

        ~DataArray()
        {
            if (m_destroy_delegate)
            {
                m_destroy_delegate(m_data, m_count);
            }
        }

        const T* getData() override { return m_data; }
        size_t getItemCount() override { return m_count; }
        size_t getDataSize() override
        {
            if constexpr (std::is_void_v<T>) return m_count * sizeof(std::byte);
            else return m_count * sizeof(T);
        }
    };

    using Buffer = DataArray<void>;

    template<typename T>
    class DataList final
        : public Base::IDataList<T>
    {
    private:
        static_assert(std::is_void_v<T> || ct::PLAIN_DATA_CHECK<T> || ct::DLLBoundarySafeCheck<T>, "DataArray requires plain data or DLL-safe types");

        std::function<size_t()> m_get_count_delegate;
        std::function<T(size_t)> m_get_item_delegate;
        std::function<void()> m_destroy_delegate;
    private:
    public:
        DataList() = delete;
        DataList(const DataList&) = delete;
        DataList& operator=(const DataList&) = delete;

        template<typename TContainer, typename TGetter>
            requires requires(TContainer c, TGetter g, size_t i) { c.size(); c[i]; g(c, i); }
        DataList(TContainer&& container, TGetter&& get_item_delegate)
        {
            size_t count = container.size();
            m_get_count_delegate = [count]() { return count; };
            m_get_item_delegate = [v_container = std::move(container), v_get_item_delegate = std::move(get_item_delegate)](size_t index) mutable
            { 
                return v_get_item_delegate(v_container, index);
            };
        }

        DataList(std::function<size_t()>&& get_count_delegate, std::function<T(size_t)>&& get_item_delegate, std::function<void()>&& destroy_delegate)
        {
            m_get_count_delegate = std::move(get_count_delegate);
            m_get_item_delegate = std::move(get_item_delegate);
            m_destroy_delegate = std::move(destroy_delegate);
        }

        size_t getItemCount() override { return m_get_count_delegate ? m_get_count_delegate() : 0; }
        const T getItem(size_t index) override 
        { 
            if(index >= getItemCount())
            {
                Core::Logger::fatal("getItem called on DataList without a valid index: {} (item count: {})", index, getItemCount());
            }
            return (index < getItemCount() && m_get_item_delegate) ? m_get_item_delegate(index) : T{}; 
        }
        ~DataList()
        {
            if (m_destroy_delegate) m_destroy_delegate();
        }
    };
}
