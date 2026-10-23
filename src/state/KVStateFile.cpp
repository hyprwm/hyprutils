#include "KVStateFile.hpp"

#include <hyprutils/memory/Casts.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace Hyprutils::State;
using namespace Hyprutils::Memory;

namespace {
    constexpr std::string_view MAGIC{"HYPRKV\0\0", 8};
    constexpr uint64_t         VERSION = 1;

    class CReader {
      public:
        explicit CReader(std::string_view input) : m_input(input) {}

        std::optional<uint64_t> u64() {
            if (m_input.size() < sizeof(uint64_t))
                return std::nullopt;

            uint64_t value = 0;
            for (unsigned i = 0; i < sizeof(uint64_t); ++i)
                value |= uint64_t{sc<unsigned char>(m_input[i])} << (i * 8);
            m_input.remove_prefix(sizeof(uint64_t));
            return value;
        }

        std::optional<std::string_view> string() {
            const auto length = u64();
            if (!length || *length > m_input.size())
                return std::nullopt;

            const auto value = m_input.substr(0, *length);
            m_input.remove_prefix(*length);
            return value;
        }

        size_t remaining() const {
            return m_input.size();
        }

      private:
        std::string_view m_input;
    };
}

static void appendU64(std::string& output, uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8)
        output.push_back(sc<char>((value >> shift) & 0xFF));
}

static void appendString(std::string& output, std::string_view value) {
    appendU64(output, value.size());
    output.append(value);
}

struct CKVStateFile::Impl {
    Impl(CUniquePointer<IKVStateFileBackend> backend, KVStateClock clock) : backend(std::move(backend)), clock(std::move(clock)) {}

    bool load(CKVStateFile& file) {
        const auto contents = backend->read();
        if (!contents)
            return false;
        if (!contents->has_value())
            return true;

        const std::string_view bytes = **contents;
        if (!bytes.starts_with(MAGIC))
            return false;

        CReader    reader(bytes.substr(MAGIC.size()));
        const auto version = reader.u64();
        const auto count   = reader.u64();
        // Every entry needs at least a key length, timestamp, and property count.
        if (!version || *version != VERSION || !count || *count > reader.remaining() / 24)
            return false;

        EntryMap loaded;
        for (uint64_t i = 0; i < *count; ++i) {
            const auto key      = reader.string();
            const auto modified = reader.u64();
            const auto fields   = reader.u64();
            if (!key || !modified || !fields || *fields > reader.remaining() / 16)
                return false;

            std::unordered_map<std::string, std::string> values;
            for (uint64_t j = 0; j < *fields; ++j) {
                const auto name  = reader.string();
                const auto value = reader.string();
                if (!name || !value || !values.emplace(*name, *value).second)
                    return false;
            }

            const CKVStateEntry::TimePoint timestamp{std::chrono::milliseconds{bc<int64_t>(*modified)}};
            auto                           entry = CSharedPointer<CKVStateEntry>(new CKVStateEntry(file, std::move(values), timestamp));
            if (!loaded.emplace(*key, std::move(entry)).second)
                return false;
        }

        if (reader.remaining() != 0)
            return false;

        for (const auto& [key, entry] : loaded) {
            if (!latest || entry->m_modified > *latest)
                latest = entry->m_modified;
        }
        entries.swap(loaded);
        return true;
    }

    std::optional<CKVStateEntry::TimePoint> nextModified() {
        auto now = clock();
        if (latest && now <= *latest) {
            if (*latest == CKVStateEntry::TimePoint::max()) {
                healthy = false;
                return std::nullopt;
            }
            now = *latest + std::chrono::milliseconds{1};
        }
        return now;
    }

    bool save(const EntryMap& candidate) {
        std::string bytes{MAGIC};
        appendU64(bytes, VERSION);
        appendU64(bytes, candidate.size());
        for (const auto& [key, entry] : candidate) {
            appendString(bytes, key);
            appendU64(bytes, bc<uint64_t>(sc<int64_t>(entry->m_modified.time_since_epoch().count())));
            appendU64(bytes, entry->m_vals.size());
            for (const auto& [name, value] : entry->m_vals) {
                appendString(bytes, name);
                appendString(bytes, value);
            }
        }

        healthy = backend->write(bytes).has_value();
        return healthy;
    }

    bool set(CKVStateEntry& entry, std::string_view what, std::string_view to) {
        if (!healthy)
            return false;

        const std::string name{what};
        const auto        field = entry.m_vals.find(name);
        if (field != entry.m_vals.end() && field->second == to)
            return true;

        const auto modified = nextModified();
        if (!modified)
            return false;

        auto values = entry.m_vals;
        values.insert_or_assign(name, std::string{to});
        auto       replacement = CSharedPointer<CKVStateEntry>(new CKVStateEntry(entry.m_file, std::move(values), *modified));
        auto       candidate   = entries;
        const auto owner       = std::ranges::find_if(candidate, [&entry](const auto& pair) { return pair.second.get() == &entry; });
        owner->second          = replacement;

        // Prepare any allocation before writing, and preserve views of unrelated metadata values.
        auto* previous = field == entry.m_vals.end() ? nullptr : &field->second;
        if (!previous)
            entry.m_vals.reserve(entry.m_vals.size() + 1);
        if (!save(candidate))
            return false;

        if (previous)
            previous->swap(replacement->m_vals.at(name));
        else
            entry.m_vals.insert(replacement->m_vals.extract(name));
        entry.m_modified = *modified;
        latest           = *modified;
        return true;
    }

    // Snapshots share unchanged entries, keeping their addresses stable across commits.
    EntryMap                                entries;
    CUniquePointer<IKVStateFileBackend>     backend;
    KVStateClock                            clock;
    std::optional<CKVStateEntry::TimePoint> latest;
    bool                                    healthy = true;
};

CKVStateEntry::CKVStateEntry(CKVStateFile& file, std::unordered_map<std::string, std::string>&& values, TimePoint modified) :
    m_vals(std::move(values)), m_file(file), m_modified(modified) {}

std::optional<std::string_view> CKVStateEntry::get(std::string_view what) const {
    const auto field = m_vals.find(std::string{what});
    if (field == m_vals.end())
        return std::nullopt;
    return field->second;
}

bool CKVStateEntry::set(std::string_view what, std::string_view to) {
    return m_file.m_impl->set(*this, what, to);
}

CKVStateEntry::TimePoint CKVStateEntry::lastModified() const {
    return m_modified;
}

CKVStateFile::CKVStateFile(std::filesystem::path where) :
    CKVStateFile(
        makeUnique<Impl>(makeKVStateFileBackend(std::move(where)), [] { return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()); })) {}

CKVStateFile::CKVStateFile(CUniquePointer<Impl> impl) : m_impl(std::move(impl)) {
    m_impl->healthy = m_impl->load(*this);
}

CKVStateFile::~CKVStateFile() = default;

bool CKVStateFile::good() const {
    return m_impl->healthy;
}

size_t CKVStateFile::size() const {
    return m_impl->entries.size();
}

CKVStateEntry* CKVStateFile::get(std::string_view key) {
    const auto entry = m_impl->entries.find(std::string{key});
    return entry == m_impl->entries.end() ? nullptr : entry->second.get();
}

const CKVStateEntry* CKVStateFile::get(std::string_view key) const {
    const auto entry = m_impl->entries.find(std::string{key});
    return entry == m_impl->entries.end() ? nullptr : entry->second.get();
}

bool CKVStateFile::insert(std::string_view key, std::unordered_map<std::string, std::string>&& data) {
    if (!good() || m_impl->entries.contains(std::string{key}))
        return false;

    const auto modified = m_impl->nextModified();
    if (!modified)
        return false;

    auto candidate = m_impl->entries;
    candidate.emplace(key, CSharedPointer<CKVStateEntry>(new CKVStateEntry(*this, std::move(data), *modified)));
    if (!m_impl->save(candidate))
        return false;

    m_impl->entries.swap(candidate);
    m_impl->latest = *modified;
    return true;
}

bool CKVStateFile::remove(std::string_view key) {
    if (!good() || !m_impl->entries.contains(std::string{key}))
        return false;

    auto candidate = m_impl->entries;
    candidate.erase(std::string{key});
    if (!m_impl->save(candidate))
        return false;

    m_impl->entries.swap(candidate);
    return true;
}

size_t CKVStateFile::pruneOld(size_t untilMax) {
    if (!good() || size() <= untilMax)
        return 0;

    std::vector<std::pair<CKVStateEntry::TimePoint, std::string_view>> ordered;
    ordered.reserve(size());
    for (const auto& [key, entry] : m_impl->entries)
        ordered.emplace_back(entry->m_modified, key);
    std::ranges::sort(ordered);

    const auto removed   = size() - untilMax;
    auto       candidate = m_impl->entries;
    for (size_t i = 0; i < removed; ++i)
        candidate.erase(std::string{ordered[i].second});
    if (!m_impl->save(candidate))
        return 0;

    m_impl->entries.swap(candidate);
    return removed;
}

CKVStateFile::Iterator CKVStateFile::begin() {
    return Iterator{m_impl->entries.begin()};
}

CKVStateFile::Iterator CKVStateFile::end() {
    return Iterator{m_impl->entries.end()};
}

CKVStateFile::ConstIterator CKVStateFile::begin() const {
    return cbegin();
}

CKVStateFile::ConstIterator CKVStateFile::end() const {
    return cend();
}

CKVStateFile::ConstIterator CKVStateFile::cbegin() const {
    return ConstIterator{m_impl->entries.cbegin()};
}

CKVStateFile::ConstIterator CKVStateFile::cend() const {
    return ConstIterator{m_impl->entries.cend()};
}

CUniquePointer<CKVStateFile> SKVStateFileAccess::create(CUniquePointer<IKVStateFileBackend> backend, KVStateClock clock) {
    return CUniquePointer<CKVStateFile>(new CKVStateFile(makeUnique<CKVStateFile::Impl>(std::move(backend), std::move(clock))));
}
