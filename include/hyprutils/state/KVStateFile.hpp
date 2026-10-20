#pragma once

#include "../memory/SharedPtr.hpp"
#include "../memory/UniquePtr.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace Hyprutils::State {

    class CKVStateFile;
    struct SKVStateFileAccess;

    /*
     * Entries are owned by their file and become invalid on removal, pruning, or file destruction.
     */
    class CKVStateEntry {
      public:
        using TimePoint = std::chrono::sys_time<std::chrono::milliseconds>;

        ~CKVStateEntry() = default;

        // The returned view is invalidated when its value is changed or the entry is destroyed.
        std::optional<std::string_view> get(std::string_view what) const;
        bool                            set(std::string_view what, std::string_view to);
        TimePoint                       lastModified() const;

        CKVStateEntry(const CKVStateEntry&)            = delete;
        CKVStateEntry(CKVStateEntry&&)                 = delete;
        CKVStateEntry& operator=(const CKVStateEntry&) = delete;
        CKVStateEntry& operator=(CKVStateEntry&&)      = delete;

      private:
        CKVStateEntry(CKVStateFile& file, std::unordered_map<std::string, std::string>&& values, TimePoint modified);

        std::unordered_map<std::string, std::string> m_vals;
        CKVStateFile&                                m_file;
        TimePoint                                    m_modified;

        friend class CKVStateFile;
    };

    /*
     * KV State File
     * This is a persistent, on-disk file that can be accessed.
     * Creating it will populate it with data if it already exists
     * A missing file is an empty store. A failed load leaves an empty, bad store.
     * Mutations persist immediately. A failed save preserves the previous state and marks the store bad.
     * Bad stores reject further mutations. Only one writer per file is supported; access is not thread-safe.
     */
    class CKVStateFile {
      private:
        using EntryMap = std::unordered_map<std::string, Memory::CSharedPointer<CKVStateEntry>>;

      public:
        template <bool IsConst>
        class CIterator {
          private:
            using BaseIterator = std::conditional_t<IsConst, EntryMap::const_iterator, EntryMap::iterator>;
            using Entry        = std::conditional_t<IsConst, const CKVStateEntry, CKVStateEntry>;

          public:
            using ValueType        = std::pair<const std::string&, Entry&>;
            using Reference        = ValueType;
            using DifferenceType   = std::ptrdiff_t;
            using IteratorCategory = std::input_iterator_tag;
            using IteratorConcept  = std::forward_iterator_tag;

            CIterator() = default;

            template <bool OtherConst>
                requires(IsConst && !OtherConst)
            CIterator(const CIterator<OtherConst>& other) : m_iter(other.m_iter) {}

            Reference operator*() const {
                return {m_iter->first, *m_iter->second};
            }

            auto operator->() const {
                return SArrow{**this};
            }

            CIterator& operator++() {
                ++m_iter;
                return *this;
            }

            CIterator operator++(int) {
                auto previous = *this;
                ++*this;
                return previous;
            }

            template <bool OtherConst>
            bool operator==(const CIterator<OtherConst>& other) const {
                return m_iter == other.m_iter;
            }

          private:
            struct SArrow {
                Reference        value;
                const Reference* operator->() const {
                    return &value;
                }
            };

            explicit CIterator(BaseIterator iter) : m_iter(iter) {}

            BaseIterator m_iter;

            template <bool>
            friend class CIterator;
            friend class CKVStateFile;
        };

        using Iterator      = CIterator<false>;
        using ConstIterator = CIterator<true>;

        explicit CKVStateFile(std::filesystem::path where);
        ~CKVStateFile();

        CKVStateFile(const CKVStateFile&)                   = delete;
        CKVStateFile(CKVStateFile&&)                        = delete;
        CKVStateFile&        operator=(const CKVStateFile&) = delete;
        CKVStateFile&        operator=(CKVStateFile&&)      = delete;

        bool                 good() const;
        size_t               size() const;

        CKVStateEntry*       get(std::string_view key);
        const CKVStateEntry* get(std::string_view key) const;
        bool                 insert(std::string_view key, std::unordered_map<std::string, std::string>&& data);
        bool                 remove(std::string_view key);

        // Order is unspecified. Structural changes invalidate iterators, but not surviving entry pointers.
        Iterator      begin();
        Iterator      end();
        ConstIterator begin() const;
        ConstIterator end() const;
        ConstIterator cbegin() const;
        ConstIterator cend() const;

        // Returns the number removed, or zero on failure (check good()). Saves once, without touching survivors.
        size_t pruneOld(size_t untilMax);

      private:
        struct Impl;
        explicit CKVStateFile(Memory::CUniquePointer<Impl> impl);

        Memory::CUniquePointer<Impl> m_impl;

        friend class CKVStateEntry;
        friend struct SKVStateFileAccess;
    };
}

template <bool IsConst>
struct std::iterator_traits<Hyprutils::State::CKVStateFile::CIterator<IsConst>> {
    using Iterator          = Hyprutils::State::CKVStateFile::CIterator<IsConst>;
    using value_type        = typename Iterator::ValueType;
    using reference         = typename Iterator::Reference;
    using pointer           = decltype(std::declval<Iterator>().operator->());
    using difference_type   = typename Iterator::DifferenceType;
    using iterator_category = typename Iterator::IteratorCategory;
    using iterator_concept  = typename Iterator::IteratorConcept;
};
