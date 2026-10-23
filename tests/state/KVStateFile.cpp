#include <hyprutils/memory/Casts.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <hyprutils/state/KVStateFile.hpp>
#include <state/KVStateFile.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <expected>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace Hyprutils::Memory;
using namespace Hyprutils::State;
using namespace std::string_literals;

namespace {
    using TimePoint     = CKVStateEntry::TimePoint;
    using Iterator      = CKVStateFile::Iterator;
    using ConstIterator = CKVStateFile::ConstIterator;

    static_assert(std::is_same_v<TimePoint::duration, std::chrono::milliseconds>);
    static_assert(std::forward_iterator<Iterator>);
    static_assert(std::forward_iterator<ConstIterator>);
    static_assert(std::is_same_v<decltype(std::declval<CKVStateFile&>().get(std::string_view{})), CKVStateEntry*>);
    static_assert(std::is_same_v<decltype(std::declval<const CKVStateFile&>().get(std::string_view{})), const CKVStateEntry*>);
    static_assert(std::is_same_v<decltype(std::declval<const CKVStateEntry&>().get(std::string_view{})), std::optional<std::string_view>>);
    static_assert(std::is_same_v<decltype(std::declval<CKVStateEntry&>().set(std::string_view{}, std::string_view{})), bool>);
    static_assert(std::is_same_v<decltype(*std::declval<const Iterator&>()), std::pair<const std::string&, CKVStateEntry&>>);
    static_assert(std::is_same_v<decltype(*std::declval<const ConstIterator&>()), std::pair<const std::string&, const CKVStateEntry&>>);
    static_assert(std::is_same_v<decltype(std::declval<Iterator>()->first), const std::string&>);
    static_assert(std::is_same_v<decltype(std::declval<Iterator>()->second), CKVStateEntry&>);
    static_assert(std::is_same_v<decltype(std::declval<ConstIterator>()->first), const std::string&>);
    static_assert(std::is_same_v<decltype(std::declval<ConstIterator>()->second), const CKVStateEntry&>);
    static_assert(std::is_same_v<decltype(std::declval<const CKVStateFile&>().begin()), ConstIterator>);
    static_assert(std::is_same_v<decltype(std::declval<const CKVStateFile&>().end()), ConstIterator>);
    static_assert(std::is_same_v<decltype(std::declval<CKVStateFile&>().cbegin()), ConstIterator>);
    static_assert(std::is_same_v<decltype(std::declval<CKVStateFile&>().cend()), ConstIterator>);
    static_assert(std::is_convertible_v<Iterator, ConstIterator>);
    static_assert(!std::is_convertible_v<ConstIterator, Iterator>);

    TimePoint at(int64_t milliseconds) {
        return TimePoint{std::chrono::milliseconds{milliseconds}};
    }

    struct SBackendState {
        std::optional<std::string> bytes;
        bool                       failRead  = false;
        bool                       failWrite = false;
        size_t                     reads     = 0;
        size_t                     writes    = 0;
    };

    class CMemoryBackend final : public IKVStateFileBackend {
      public:
        explicit CMemoryBackend(CSharedPointer<SBackendState> state) : m_state(std::move(state)) {}

        std::expected<std::optional<std::string>, std::error_code> read() override {
            ++m_state->reads;
            if (m_state->failRead)
                return std::unexpected(std::make_error_code(std::errc::io_error));
            return m_state->bytes;
        }

        std::expected<void, std::error_code> write(std::string_view contents) override {
            ++m_state->writes;
            if (m_state->failWrite)
                return std::unexpected(std::make_error_code(std::errc::no_space_on_device));
            m_state->bytes = std::string{contents};
            return {};
        }

      private:
        CSharedPointer<SBackendState> m_state;
    };

    struct SFixtureEntry {
        std::string                                      key;
        int64_t                                          modified;
        std::vector<std::pair<std::string, std::string>> properties;
    };

    // Build the version-1 wire format independently: timestamps are signed int64 milliseconds.
    // Vectors deliberately allow duplicate keys for malformed fixtures.
    void appendU64(std::string& bytes, uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8)
            bytes.push_back(sc<char>((value >> shift) & 0xFF));
    }

    void appendString(std::string& bytes, std::string_view value) {
        appendU64(bytes, value.size());
        bytes.append(value);
    }

    std::string fixtureBytes(const std::vector<SFixtureEntry>& entries, uint64_t version = 1) {
        std::string bytes{std::string_view{"HYPRKV\0\0", 8}};
        appendU64(bytes, version);
        appendU64(bytes, entries.size());
        for (const auto& entry : entries) {
            appendString(bytes, entry.key);
            appendU64(bytes, bc<uint64_t>(entry.modified));
            appendU64(bytes, entry.properties.size());
            for (const auto& [key, value] : entry.properties) {
                appendString(bytes, key);
                appendString(bytes, value);
            }
        }
        return bytes;
    }

    std::string replaceU64(std::string bytes, size_t offset, uint64_t value) {
        std::string encoded;
        appendU64(encoded, value);
        bytes.replace(offset, encoded.size(), encoded);
        return bytes;
    }

    std::vector<SFixtureEntry> specialEntries() {
        return {
            {"", 100, {{"", ""}, {"empty-value", ""}}},
            {"entry\0suffix"s, 200, {{"property\0suffix"s, "value\0suffix"s}, {"\0"s, "\0\0"s}}},
            {"日本語-λ-🙂", 300, {{"clé-雪", "café-世界-🙂"}}},
            {"\"entry\"\nnext\r\t\\", 400, {{"\"property\"\n", "\"value\"\n\r\t\\"}}},
            {"no-properties", 500, {}},
        };
    }

    void expectEntries(const CKVStateFile& file, const std::vector<SFixtureEntry>& entries) {
        EXPECT_EQ(file.size(), entries.size());
        for (const auto& expected : entries) {
            SCOPED_TRACE(expected.key);
            const auto* entry = file.get(expected.key);
            ASSERT_NE(entry, nullptr);
            EXPECT_EQ(entry->lastModified(), at(expected.modified));
            for (const auto& [key, value] : expected.properties) {
                SCOPED_TRACE(key);
                EXPECT_EQ(entry->get(key), std::optional<std::string_view>{value});
            }
        }
    }

    class KVStateFileTest : public ::testing::Test {
      protected:
        CSharedPointer<SBackendState> backend = makeShared<SBackendState>();
        CSharedPointer<TimePoint>     clock   = makeShared<TimePoint>(at(100));

        CUniquePointer<CKVStateFile>  open() {
            return SKVStateFileAccess::create(makeUnique<CMemoryBackend>(backend), [clock = clock] { return *clock; });
        }

        void expectRejectedBytes(const std::string& bytes) {
            backend->bytes          = bytes;
            const auto readsBefore  = backend->reads;
            const auto writesBefore = backend->writes;
            auto       file         = open();
            ASSERT_NE(file, nullptr);
            EXPECT_FALSE(file->good());
            EXPECT_EQ(file->size(), 0U);
            EXPECT_EQ(file->begin(), file->end());
            EXPECT_EQ(file->cbegin(), file->cend());
            EXPECT_EQ(file->get("first"), nullptr);
            EXPECT_EQ(std::as_const(*file).get("second"), nullptr);
            EXPECT_EQ(backend->reads, readsBefore + 1);

            EXPECT_FALSE(file->insert("replacement", {{"value", "must not overwrite corrupt bytes"}}));
            EXPECT_FALSE(file->remove("first"));
            EXPECT_EQ(file->pruneOld(0), 0U);
            EXPECT_FALSE(file->good());
            EXPECT_EQ(file->size(), 0U);
            file.reset();
            EXPECT_EQ(backend->writes, writesBefore);
            EXPECT_EQ(backend->bytes, std::optional<std::string>{bytes});
        }
    };

    TEST_F(KVStateFileTest, MissingStoreIsHealthyAndEmptyWithoutWriting) {
        auto file = open();
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(file->good());
        EXPECT_EQ(file->size(), 0U);
        EXPECT_EQ(file->get("missing"), nullptr);
        EXPECT_EQ(std::as_const(*file).get("missing"), nullptr);
        EXPECT_EQ(file->begin(), file->end());
        EXPECT_EQ(std::as_const(*file).begin(), std::as_const(*file).end());
        EXPECT_EQ(file->cbegin(), file->cend());
        EXPECT_FALSE(file->remove("missing"));
        EXPECT_EQ(file->pruneOld(0), 0U);
        EXPECT_EQ(file->pruneOld(100), 0U);
        EXPECT_TRUE(file->good());
        EXPECT_EQ(backend->reads, 1U);
        EXPECT_EQ(backend->writes, 0U);
        EXPECT_FALSE(backend->bytes.has_value());
        file.reset();
        EXPECT_EQ(backend->writes, 0U);
        EXPECT_FALSE(backend->bytes.has_value());
    }

    TEST_F(KVStateFileTest, ExistingEncodedEmptyStoreIsHealthyWithoutRewriting) {
        backend->bytes    = fixtureBytes({});
        const auto before = backend->bytes;
        auto       file   = open();
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(file->good());
        EXPECT_EQ(file->size(), 0U);
        EXPECT_EQ(file->begin(), file->end());
        file.reset();
        EXPECT_EQ(backend->writes, 0U);
        EXPECT_EQ(backend->bytes, before);
    }

    TEST_F(KVStateFileTest, LoadsIndependentFixtureWithBinaryAndTextKeysAndValues) {
        const auto entries = specialEntries();
        backend->bytes     = fixtureBytes(entries);
        const auto before  = backend->bytes;
        auto       file    = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        expectEntries(*file, entries);
        ASSERT_NE(file->get(""), nullptr);
        EXPECT_EQ(file->get(""), std::as_const(*file).get(""));
        EXPECT_EQ(file->get("")->get("missing-property"), std::nullopt);
        EXPECT_EQ(file->get("entry"), nullptr);
        ASSERT_NE(file->get("entry\0suffix"s), nullptr);
        EXPECT_EQ(file->get("entry\0suffix"s)->get("property"), std::nullopt);
        file.reset();
        EXPECT_EQ(backend->writes, 0U);
        EXPECT_EQ(backend->bytes, before);
    }

    TEST_F(KVStateFileTest, DecodesSignedMillisecondTimestampsWithoutChangingThemOnLoad) {
        const std::vector<SFixtureEntry> entries = {
            {"minimum", std::numeric_limits<int64_t>::min(), {}},
            {"negative", -0x0102030405060708LL, {}},
            {"minus-one", -1, {}},
            {"epoch", 0, {}},
            {"positive", 0x0102030405060708LL, {}},
            {"maximum", std::numeric_limits<int64_t>::max(), {}},
        };
        backend->bytes = fixtureBytes(entries);
        auto file      = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        expectEntries(*file, entries);
        EXPECT_EQ(backend->writes, 0U);
    }

    TEST_F(KVStateFileTest, InsertsRoundTripBinaryAndTextBytesBeforeWriterDestruction) {
        auto file = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        std::vector<SFixtureEntry> inserted;
        for (const auto& entry : specialEntries()) {
            SCOPED_TRACE(entry.key);
            *clock = at(entry.modified);
            ASSERT_TRUE(file->insert(entry.key, std::unordered_map<std::string, std::string>{entry.properties.begin(), entry.properties.end()}));
            inserted.push_back(entry);
            EXPECT_EQ(backend->writes, inserted.size());
            ASSERT_TRUE(backend->bytes.has_value());
            expectEntries(*file, inserted);

            auto reopened = open();
            ASSERT_NE(reopened, nullptr);
            ASSERT_TRUE(reopened->good());
            expectEntries(*reopened, inserted);
            EXPECT_EQ(backend->writes, inserted.size());
        }
        file.reset();
        EXPECT_EQ(backend->writes, inserted.size());
        auto reopened = open();
        ASSERT_NE(reopened, nullptr);
        ASSERT_TRUE(reopened->good());
        expectEntries(*reopened, inserted);
    }

    TEST_F(KVStateFileTest, SingleEntrySerializationMatchesIndependentWireFixture) {
        const SFixtureEntry entry = {"entry\0\n\""s, 0x0102030405060708LL, {{"property\0\"\n"s, "雪\0value\n"s}}};
        *clock                    = at(entry.modified);
        auto file                 = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        ASSERT_TRUE(file->insert(entry.key, std::unordered_map<std::string, std::string>{entry.properties.begin(), entry.properties.end()}));
        EXPECT_EQ(backend->writes, 1U);
        ASSERT_TRUE(backend->bytes.has_value());
        // One entry and one property make this comparison independent of map iteration order.
        EXPECT_EQ(*backend->bytes, fixtureBytes({entry}));
        expectEntries(*file, {entry});
    }

    TEST_F(KVStateFileTest, SetAndRemoveSaveImmediatelyAndPreserveUnrelatedEntries) {
        std::vector<SFixtureEntry> entries = {{"first", 100, {{"value", "old"}}}, {"second", 200, {{"value", "untouched"}}}};
        backend->bytes                     = fixtureBytes(entries);
        auto file                          = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        auto* first  = file->get("first");
        auto* second = file->get("second");
        ASSERT_NE(first, nullptr);
        ASSERT_NE(second, nullptr);

        *clock = at(300);
        ASSERT_TRUE(first->set("value", "new\0value\n\"雪"s));
        ASSERT_EQ(file->get("first"), first);
        entries[0].properties[0].second = "new\0value\n\"雪"s;
        entries[0].modified             = 300;
        EXPECT_EQ(backend->writes, 1U);
        expectEntries(*file, entries);
        {
            auto reopened = open();
            ASSERT_NE(reopened, nullptr);
            ASSERT_TRUE(reopened->good());
            expectEntries(*reopened, entries);
        }

        *clock = at(400);
        ASSERT_TRUE(first->set("", ""));
        entries[0].properties.emplace_back("", "");
        entries[0].modified = 400;
        EXPECT_EQ(backend->writes, 2U);
        ASSERT_EQ(file->get("first"), first);
        EXPECT_EQ(file->get("second"), second);
        expectEntries(*file, entries);
        {
            auto reopened = open();
            ASSERT_NE(reopened, nullptr);
            ASSERT_TRUE(reopened->good());
            expectEntries(*reopened, entries);
        }

        *clock = at(500);
        ASSERT_TRUE(file->remove("first"));
        entries.erase(entries.begin());
        EXPECT_EQ(backend->writes, 3U);
        EXPECT_EQ(file->get("first"), nullptr);
        EXPECT_EQ(file->get("second"), second);
        expectEntries(*file, entries);
        {
            auto reopened = open();
            ASSERT_NE(reopened, nullptr);
            ASSERT_TRUE(reopened->good());
            EXPECT_EQ(reopened->get("first"), nullptr);
            expectEntries(*reopened, entries);
        }

        ASSERT_TRUE(file->remove("second"));
        EXPECT_EQ(backend->writes, 4U);
        EXPECT_EQ(file->size(), 0U);
        ASSERT_TRUE(backend->bytes.has_value());
        EXPECT_EQ(*backend->bytes, fixtureBytes({}));
        auto reopened = open();
        ASSERT_NE(reopened, nullptr);
        EXPECT_TRUE(reopened->good());
        EXPECT_EQ(reopened->size(), 0U);
    }

    TEST_F(KVStateFileTest, NoOpSetDuplicateInsertMissingRemoveAndNoOpPruneDoNotSaveOrRetimestamp) {
        const std::vector<SFixtureEntry> entries = {{"entry", 100, {{"value", "same"}, {"empty", ""}}}};
        backend->bytes                           = fixtureBytes(entries);
        const auto before                        = backend->bytes;
        auto       file                          = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        auto* entry = file->get("entry");
        ASSERT_NE(entry, nullptr);
        *clock             = at(10000);
        backend->failWrite = true;

        EXPECT_TRUE(entry->set("value", "same"));
        EXPECT_TRUE(entry->set("empty", ""));
        EXPECT_FALSE(file->insert("entry", {{"value", "replacement"}, {"extra", "must not appear"}}));
        EXPECT_FALSE(file->remove("missing"));
        EXPECT_EQ(file->pruneOld(1), 0U);
        EXPECT_EQ(file->pruneOld(100), 0U);
        EXPECT_TRUE(file->good());
        ASSERT_EQ(file->get("entry"), entry);
        EXPECT_EQ(entry->get("extra"), std::nullopt);
        expectEntries(*file, entries);
        EXPECT_EQ(backend->writes, 0U);
        EXPECT_EQ(backend->bytes, before);
        file.reset();
        EXPECT_EQ(backend->writes, 0U);
        EXPECT_EQ(backend->bytes, before);
    }

    TEST_F(KVStateFileTest, MetadataViewsSurviveOtherFieldAndEntryMutationsAndFailedSave) {
        const std::array<std::pair<std::string, std::string>, 2> values   = {{{"short", "tiny"}, {"heap", std::string(256, 'L')}}};
        SFixtureEntry                                            expected = {"anchor", 100, {values[0], values[1], {"editable", "old"}}};
        backend->bytes                                                    = fixtureBytes({expected, {"old", 50, {}}});
        auto file                                                         = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        auto* anchor = file->get("anchor");
        ASSERT_NE(anchor, nullptr);
        std::vector<std::string_view> views;
        for (const auto& [key, value] : values) {
            const auto view = anchor->get(key);
            ASSERT_TRUE(view.has_value());
            ASSERT_EQ(*view, value);
            views.push_back(*view);
        }
        const auto expectRetainedViews = [&] {
            ASSERT_EQ(file->get("anchor"), anchor);
            for (size_t i = 0; i < values.size(); ++i) {
                SCOPED_TRACE(values[i].first);
                const auto current = anchor->get(values[i].first);
                ASSERT_TRUE(current.has_value());
                // Check storage identity before dereferencing a potentially invalidated view.
                ASSERT_EQ(current->data(), views[i].data());
                EXPECT_EQ(*current, values[i].second);
                EXPECT_EQ(views[i], values[i].second);
            }
        };

        *clock = at(1000);
        ASSERT_TRUE(anchor->set("editable", "changed"));
        expected.properties[2].second = "changed";
        ASSERT_NO_FATAL_FAILURE(expectRetainedViews());
        for (int i = 0; i < 32; ++i) {
            SCOPED_TRACE(i);
            *clock         = at(1001 + i);
            const auto key = "added-" + std::to_string(i);
            ASSERT_TRUE(anchor->set(key, "new field"));
            expected.properties.emplace_back(key, "new field");
            ASSERT_NO_FATAL_FAILURE(expectRetainedViews());
        }
        expected.modified = anchor->lastModified().time_since_epoch().count();
        ASSERT_TRUE(file->insert("unrelated", {{"value", "temporary"}}));
        ASSERT_NO_FATAL_FAILURE(expectRetainedViews());
        ASSERT_TRUE(file->remove("unrelated"));
        ASSERT_NO_FATAL_FAILURE(expectRetainedViews());
        ASSERT_EQ(file->pruneOld(1), 1U);
        ASSERT_NO_FATAL_FAILURE(expectRetainedViews());
        expectEntries(*file, {expected});

        const auto before       = backend->bytes;
        const auto writesBefore = backend->writes;
        backend->failWrite      = true;
        EXPECT_FALSE(anchor->set("heap", "unsaved"));
        EXPECT_FALSE(file->good());
        ASSERT_NO_FATAL_FAILURE(expectRetainedViews());
        backend->failWrite = false;
        EXPECT_FALSE(anchor->set("editable", "retry"));
        EXPECT_EQ(backend->writes, writesBefore + 1);
        EXPECT_EQ(backend->bytes, before);
        expectEntries(*file, {expected});

        auto reopened = open();
        ASSERT_NE(reopened, nullptr);
        ASSERT_TRUE(reopened->good());
        expectEntries(*reopened, {expected});
    }

    TEST_F(KVStateFileTest, SetAcceptsNamesAndValuesBorrowedFromExistingMetadata) {
        const std::string payload = std::string(80, 'a') + "::middle::" + std::string(80, 'z');
        const std::string selfName(64, 'k');
        SFixtureEntry     expected = {"entry", 100, {{"name", "destination"}, {"source", payload}, {selfName, selfName}}};
        backend->bytes             = fixtureBytes({expected});
        auto file                  = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        auto* entry = file->get("entry");
        ASSERT_NE(entry, nullptr);

        *clock = at(200);
        {
            const auto name  = entry->get("name");
            const auto value = entry->get("source");
            ASSERT_TRUE(name.has_value());
            ASSERT_TRUE(value.has_value());
            ASSERT_TRUE(entry->set(*name, *value));
        }
        ASSERT_EQ(file->get("entry"), entry);
        expected.properties.emplace_back("destination", payload);

        *clock = at(300);
        {
            const auto value = entry->get("source");
            ASSERT_TRUE(value.has_value());
            ASSERT_TRUE(entry->set("source", value->substr(78, 13)));
        }
        ASSERT_EQ(file->get("entry"), entry);
        expected.properties[1].second = payload.substr(78, 13);

        *clock = at(400);
        {
            const auto name  = entry->get(selfName);
            const auto value = entry->get("source");
            ASSERT_TRUE(name.has_value());
            ASSERT_TRUE(value.has_value());
            // Updating this field invalidates the storage supplying the field name itself.
            ASSERT_TRUE(entry->set(*name, *value));
        }
        expected.properties[2].second = payload.substr(78, 13);
        expected.modified             = 400;
        EXPECT_EQ(backend->writes, 3U);
        expectEntries(*file, {expected});
        auto reopened = open();
        ASSERT_NE(reopened, nullptr);
        ASSERT_TRUE(reopened->good());
        expectEntries(*reopened, {expected});
    }

    TEST_F(KVStateFileTest, TimestampsFollowClockAndAreStrictlyMonotonicAcrossTiesAndBackwardsTime) {
        *clock    = at(1000);
        auto file = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->insert("first", {{"value", "one"}}));
        auto* first = file->get("first");
        ASSERT_NE(first, nullptr);
        const auto initial = first->lastModified();
        EXPECT_EQ(initial, *clock);

        ASSERT_TRUE(file->insert("second", {{"value", "two"}}));
        ASSERT_EQ(file->get("first"), first);
        auto* second = file->get("second");
        ASSERT_NE(second, nullptr);
        const auto tied = second->lastModified();
        EXPECT_EQ(tied, initial + std::chrono::milliseconds{1});
        EXPECT_EQ(first->lastModified(), initial);

        *clock = at(-500);
        ASSERT_TRUE(first->set("value", "backwards"));
        ASSERT_EQ(file->get("first"), first);
        ASSERT_EQ(file->get("second"), second);
        const auto backwards = first->lastModified();
        EXPECT_EQ(backwards, tied + std::chrono::milliseconds{1});
        EXPECT_EQ(second->lastModified(), tied);

        *clock = at(1000000);
        ASSERT_TRUE(second->set("value", "forward"));
        ASSERT_EQ(file->get("first"), first);
        ASSERT_EQ(file->get("second"), second);
        const auto forward = second->lastModified();
        EXPECT_EQ(forward, *clock);
        EXPECT_GT(forward, backwards);
        EXPECT_EQ(first->lastModified(), backwards);
        EXPECT_EQ(backend->writes, 4U);

        file.reset();
        *clock = at(1);
        file   = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        ASSERT_NE(file->get("first"), nullptr);
        ASSERT_NE(file->get("second"), nullptr);
        EXPECT_EQ(file->get("first")->lastModified(), backwards);
        EXPECT_EQ(file->get("second")->lastModified(), forward);
        ASSERT_TRUE(file->insert("after-reopen", {}));
        ASSERT_NE(file->get("after-reopen"), nullptr);
        const auto afterReopen = file->get("after-reopen")->lastModified();
        EXPECT_EQ(afterReopen, forward + std::chrono::milliseconds{1});

        *clock = afterReopen;
        ASSERT_TRUE(file->get("first")->set("value", "tie after reopen"));
        EXPECT_EQ(file->get("first")->lastModified(), afterReopen + std::chrono::milliseconds{1});
        EXPECT_EQ(file->get("second")->lastModified(), forward);
        EXPECT_EQ(backend->writes, 6U);
    }

    TEST_F(KVStateFileTest, ReopenClampsAgainstLargestPersistedTimestampRegardlessOfFileOrder) {
        const std::vector<SFixtureEntry> entries = {{"first", 50, {}}, {"newest", 9000, {}}, {"last", 100, {}}};
        backend->bytes                           = fixtureBytes(entries);
        *clock                                   = at(1);
        auto file                                = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        expectEntries(*file, entries);
        ASSERT_TRUE(file->insert("inserted", {}));
        ASSERT_NE(file->get("inserted"), nullptr);
        const auto inserted = file->get("inserted")->lastModified();
        EXPECT_EQ(inserted, at(9001));
        expectEntries(*file, {{"first", 50, {}}, {"newest", 9000, {}}, {"last", 100, {}}, {"inserted", inserted.time_since_epoch().count(), {}}});
        ASSERT_NE(file->get("first"), nullptr);
        ASSERT_TRUE(file->get("first")->set("value", "edited"));
        EXPECT_EQ(file->get("first")->lastModified(), inserted + std::chrono::milliseconds{1});
        EXPECT_EQ(backend->writes, 2U);
    }

    TEST_F(KVStateFileTest, TimestampExhaustionRejectsChangesWithoutWritingAndIsSticky) {
        constexpr auto maximum = std::numeric_limits<int64_t>::max();
        backend->bytes         = fixtureBytes({{"entry", maximum - 1, {{"value", "before"}}}});
        *clock                 = at(1);
        auto file              = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        auto* entry = file->get("entry");
        ASSERT_NE(entry, nullptr);
        EXPECT_EQ(entry->lastModified(), at(maximum - 1));

        ASSERT_TRUE(entry->set("value", "at limit"));
        ASSERT_EQ(file->get("entry"), entry);
        const std::vector<SFixtureEntry> expected = {{"entry", maximum, {{"value", "at limit"}}}};
        expectEntries(*file, expected);
        EXPECT_EQ(backend->bytes, std::optional<std::string>{fixtureBytes(expected)});
        EXPECT_TRUE(entry->set("value", "at limit"));
        EXPECT_TRUE(file->good());
        EXPECT_EQ(backend->writes, 1U);

        const auto before = backend->bytes;
        EXPECT_FALSE(entry->set("value", "overflow"));
        EXPECT_FALSE(file->good());
        EXPECT_EQ(file->get("entry"), entry);
        EXPECT_FALSE(file->remove("entry"));
        expectEntries(*file, expected);
        EXPECT_EQ(backend->writes, 1U);
        EXPECT_EQ(backend->bytes, before);

        auto reopened = open();
        ASSERT_NE(reopened, nullptr);
        ASSERT_TRUE(reopened->good());
        expectEntries(*reopened, expected);
        EXPECT_EQ(backend->writes, 1U);
    }

    TEST_F(KVStateFileTest, PruneKeepsHundredMostRecentlyEditedEntriesAndPreservesSurvivorsAcrossReopen) {
        auto file = open();
        ASSERT_NE(file, nullptr);
        std::vector<SFixtureEntry> entries;
        for (int i = 0; i < 105; ++i) {
            *clock = at(100 + i);
            entries.push_back({"entry-" + std::to_string(i), 100 + i, {{"value", std::to_string(i)}}});
            ASSERT_TRUE(file->insert(entries.back().key, {{"value", std::to_string(i)}}));
        }
        ASSERT_EQ(file->size(), 105U);
        ASSERT_EQ(backend->writes, 105U);
        ASSERT_NE(file->get("entry-0"), nullptr);
        *clock = at(10000);
        ASSERT_TRUE(file->get("entry-0")->set("value", "recently edited"));
        entries[0].modified             = 10000;
        entries[0].properties[0].second = "recently edited";
        ASSERT_EQ(backend->writes, 106U);

        file.reset();
        *clock = at(-1000);
        file   = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        expectEntries(*file, entries);
        // Reading an old entry must not make it more recent for pruning.
        ASSERT_NE(file->get("entry-1"), nullptr);
        EXPECT_EQ(file->get("entry-1")->get("value"), std::optional<std::string_view>{"1"});

        entries.erase(entries.begin() + 1, entries.begin() + 6);
        std::vector<CKVStateEntry*> survivors;
        for (const auto& entry : entries) {
            ASSERT_NE(file->get(entry.key), nullptr);
            survivors.push_back(file->get(entry.key));
        }
        const auto before = backend->bytes;
        EXPECT_EQ(file->pruneOld(100), 5U);
        EXPECT_TRUE(file->good());
        EXPECT_EQ(backend->writes, 107U);
        EXPECT_NE(backend->bytes, before);
        expectEntries(*file, entries);
        for (size_t i = 0; i < entries.size(); ++i)
            EXPECT_EQ(file->get(entries[i].key), survivors[i]);
        for (int i = 1; i <= 5; ++i)
            EXPECT_EQ(file->get("entry-" + std::to_string(i)), nullptr);

        {
            auto reopened = open();
            ASSERT_NE(reopened, nullptr);
            ASSERT_TRUE(reopened->good());
            expectEntries(*reopened, entries);
            for (int i = 1; i <= 5; ++i)
                EXPECT_EQ(reopened->get("entry-" + std::to_string(i)), nullptr);
        }
        EXPECT_EQ(file->pruneOld(100), 0U);
        EXPECT_EQ(file->pruneOld(200), 0U);
        EXPECT_EQ(backend->writes, 107U);
        expectEntries(*file, entries);

        EXPECT_EQ(file->pruneOld(0), 100U);
        EXPECT_TRUE(file->good());
        EXPECT_EQ(file->size(), 0U);
        EXPECT_EQ(backend->writes, 108U);
        auto reopened = open();
        ASSERT_NE(reopened, nullptr);
        EXPECT_TRUE(reopened->good());
        EXPECT_EQ(reopened->size(), 0U);
    }

    TEST_F(KVStateFileTest, IteratorCopiesPostfixConversionAndEqualityReferToTheSameEntries) {
        const std::vector<SFixtureEntry> entries = {{"alpha", 100, {}}, {"beta", 200, {}}, {"gamma", 300, {}}};
        backend->bytes                           = fixtureBytes(entries);
        auto file                                = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        ASSERT_EQ(file->size(), 3U);
        const auto& constFile = *file;
        auto        iter      = file->begin();
        auto        copy      = iter;
        Iterator    assigned;
        assigned                = iter;
        ConstIterator converted = iter;
        const auto    constCopy = converted;
        ConstIterator constAssigned;
        constAssigned = iter;

        ASSERT_NE(iter, file->end());
        EXPECT_EQ(iter, copy);
        EXPECT_EQ(assigned, iter);
        EXPECT_EQ(converted, iter);
        EXPECT_EQ(iter, converted);
        EXPECT_EQ(constAssigned, converted);
        EXPECT_EQ(constFile.begin(), converted);
        EXPECT_EQ(constFile.cbegin(), converted);
        EXPECT_EQ(constFile.end(), file->end());
        EXPECT_EQ(file->end(), constFile.cend());
        EXPECT_EQ(&(*iter).first, &iter->first);
        EXPECT_EQ(&iter->first, &converted->first);
        EXPECT_EQ(&iter->second, file->get(iter->first));
        EXPECT_EQ(&converted->second, constFile.get(converted->first));

        const auto previous = iter++;
        EXPECT_EQ(previous, copy);
        EXPECT_EQ(assigned, previous);
        EXPECT_NE(previous, iter);
        EXPECT_NE(converted, iter);
        EXPECT_NE(iter, converted);
        EXPECT_EQ(&++copy, &copy);
        EXPECT_EQ(copy, iter);
        const auto constPrevious = converted++;
        EXPECT_EQ(constPrevious, previous);
        EXPECT_EQ(constCopy, constPrevious);
        EXPECT_NE(constCopy, converted);
        EXPECT_EQ(converted, iter);
        EXPECT_EQ(&++constAssigned, &constAssigned);
        EXPECT_EQ(constAssigned, converted);

        std::set<std::string> visited;
        for (auto cursor = constFile.cbegin(); cursor != constFile.cend(); cursor++) {
            EXPECT_TRUE(visited.insert(cursor->first).second);
            EXPECT_EQ(&(*cursor).second, constFile.get(cursor->first));
        }
        EXPECT_EQ(visited, (std::set<std::string>{"alpha", "beta", "gamma"}));
        EXPECT_EQ(backend->writes, 0U);
        expectEntries(*file, entries);
    }

    TEST_F(KVStateFileTest, RangeForUsesReferencesAndAllowsEntryMutationWhileIterating) {
        backend->bytes = fixtureBytes({{"alpha", 100, {{"value", "old"}}}, {"beta", 200, {{"value", "old"}}}, {"gamma", 300, {{"value", "old"}}}});
        *clock         = at(1000);
        auto file      = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        std::set<std::string>      visited;
        std::vector<SFixtureEntry> expected;
        for (auto&& [key, entry] : *file) {
            static_assert(std::is_same_v<decltype(key), const std::string&>);
            static_assert(std::is_same_v<decltype(entry), CKVStateEntry&>);
            EXPECT_TRUE(visited.insert(key).second);
            EXPECT_EQ(&entry, file->get(key));
            *clock += std::chrono::milliseconds{1};
            ASSERT_TRUE(entry.set("value", key));
            EXPECT_EQ(entry.lastModified(), *clock);
            *clock += std::chrono::milliseconds{1};
            ASSERT_TRUE(entry.set("added", "during iteration"));
            EXPECT_EQ(entry.lastModified(), *clock);
            EXPECT_EQ(&entry, file->get(key));
            EXPECT_EQ(backend->writes, visited.size() * 2);
            expected.push_back({key, entry.lastModified().time_since_epoch().count(), {{"value", key}, {"added", "during iteration"}}});
        }
        EXPECT_EQ(visited, (std::set<std::string>{"alpha", "beta", "gamma"}));
        visited.clear();
        for (auto&& [key, entry] : std::as_const(*file)) {
            static_assert(std::is_same_v<decltype(key), const std::string&>);
            static_assert(std::is_same_v<decltype(entry), const CKVStateEntry&>);
            EXPECT_TRUE(visited.insert(key).second);
            EXPECT_EQ(&entry, std::as_const(*file).get(key));
            EXPECT_EQ(entry.get("value"), std::optional<std::string_view>{key});
        }
        EXPECT_EQ(visited.size(), 3U);
        expectEntries(*file, expected);
        auto reopened = open();
        ASSERT_NE(reopened, nullptr);
        ASSERT_TRUE(reopened->good());
        expectEntries(*reopened, expected);
        EXPECT_EQ(backend->writes, 6U);
    }

    TEST_F(KVStateFileTest, EntryPointersRemainStableAcrossInsertionsAndUpdates) {
        auto file = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->insert("anchor", {{"value", "original"}}));
        auto*       anchor      = file->get("anchor");
        const auto* constAnchor = std::as_const(*file).get("anchor");
        ASSERT_NE(anchor, nullptr);
        ASSERT_EQ(anchor, constAnchor);
        const auto originalTime = anchor->lastModified();
        for (int i = 0; i < 256; ++i) {
            SCOPED_TRACE(i);
            ASSERT_TRUE(file->insert("other-" + std::to_string(i), {{"value", std::to_string(i)}}));
            ASSERT_EQ(file->get("anchor"), anchor);
            ASSERT_EQ(std::as_const(*file).get("anchor"), constAnchor);
        }
        EXPECT_EQ(anchor->lastModified(), originalTime);
        auto* other = file->get("other-0");
        ASSERT_NE(other, nullptr);
        ASSERT_TRUE(other->set("value", "updated elsewhere"));
        ASSERT_EQ(file->get("anchor"), anchor);
        ASSERT_EQ(file->get("other-0"), other);
        EXPECT_EQ(anchor->lastModified(), originalTime);
        EXPECT_EQ(constAnchor->get("value"), std::optional<std::string_view>{"original"});

        ASSERT_TRUE(anchor->set("value", "updated through saved pointer"));
        ASSERT_EQ(file->get("anchor"), anchor);
        ASSERT_EQ(file->get("other-0"), other);
        EXPECT_EQ(constAnchor->get("value"), std::optional<std::string_view>{"updated through saved pointer"});
        EXPECT_EQ(other->get("value"), std::optional<std::string_view>{"updated elsewhere"});
        ASSERT_TRUE(file->remove("other-255"));
        EXPECT_EQ(file->get("anchor"), anchor);
        EXPECT_EQ(file->get("other-0"), other);
    }

    enum class eFailingMutation {
        INSERT,
        SET_EXISTING,
        SET_NEW,
        REMOVE,
        PRUNE,
    };

    class KVStateMutationFailureTest : public KVStateFileTest, public ::testing::WithParamInterface<eFailingMutation> {};

    TEST_P(KVStateMutationFailureTest, FailedSaveRollsBackMemoryAndTimestampsAndRejectsAllLaterMutations) {
        const std::vector<SFixtureEntry> entries = {
            {"first", 100, {{"value", "original"}}}, {"second", 200, {{"value", "second value"}}}, {"third", 300, {{"value", "third value"}}}};
        backend->bytes    = fixtureBytes(entries);
        const auto before = backend->bytes;
        auto       file   = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        std::vector<CKVStateEntry*> pointers;
        for (const auto& entry : entries) {
            ASSERT_NE(file->get(entry.key), nullptr);
            pointers.push_back(file->get(entry.key));
        }
        *clock             = at(1000);
        backend->failWrite = true;
        switch (GetParam()) {
            case eFailingMutation::INSERT: EXPECT_FALSE(file->insert("new-entry", {{"value", "new"}})); break;
            case eFailingMutation::SET_EXISTING: EXPECT_FALSE(pointers[0]->set("value", "changed")); break;
            case eFailingMutation::SET_NEW: EXPECT_FALSE(pointers[0]->set("new-property", "new")); break;
            case eFailingMutation::REMOVE: EXPECT_FALSE(file->remove("first")); break;
            case eFailingMutation::PRUNE: EXPECT_EQ(file->pruneOld(1), 0U); break;
        }
        EXPECT_FALSE(file->good());
        EXPECT_EQ(backend->writes, 1U);
        EXPECT_EQ(backend->bytes, before);
        expectEntries(*file, entries);
        for (size_t i = 0; i < entries.size(); ++i)
            ASSERT_EQ(file->get(entries[i].key), pointers[i]);
        EXPECT_EQ(file->get("new-entry"), nullptr);
        EXPECT_EQ(pointers[0]->get("new-property"), std::nullopt);

        backend->failWrite = false;
        *clock             = at(2000);
        EXPECT_FALSE(file->insert("later-entry", {{"value", "later"}}));
        EXPECT_FALSE(pointers[0]->set("value", "later change"));
        EXPECT_FALSE(pointers[0]->set("new-property", "later change"));
        EXPECT_FALSE(file->remove("second"));
        EXPECT_EQ(file->pruneOld(0), 0U);
        EXPECT_FALSE(file->good());
        EXPECT_EQ(backend->writes, 1U);
        EXPECT_EQ(backend->reads, 1U);
        EXPECT_EQ(backend->bytes, before);
        expectEntries(*file, entries);
        EXPECT_EQ(file->get("later-entry"), nullptr);
        for (size_t i = 0; i < entries.size(); ++i)
            ASSERT_EQ(file->get(entries[i].key), pointers[i]);
        EXPECT_EQ(pointers[0]->get("new-property"), std::nullopt);

        file.reset();
        EXPECT_EQ(backend->writes, 1U);
        EXPECT_EQ(backend->bytes, before);
        auto reopened = open();
        ASSERT_NE(reopened, nullptr);
        ASSERT_TRUE(reopened->good());
        expectEntries(*reopened, entries);
        EXPECT_EQ(reopened->get("new-entry"), nullptr);
        EXPECT_EQ(reopened->get("later-entry"), nullptr);
        EXPECT_EQ(backend->writes, 1U);
    }

    INSTANTIATE_TEST_SUITE_P(KVState, KVStateMutationFailureTest,
                             ::testing::Values(eFailingMutation::INSERT, eFailingMutation::SET_EXISTING, eFailingMutation::SET_NEW, eFailingMutation::REMOVE,
                                               eFailingMutation::PRUNE),
                             [](const ::testing::TestParamInfo<eFailingMutation>& info) {
                                 switch (info.param) {
                                     case eFailingMutation::INSERT: return "Insert";
                                     case eFailingMutation::SET_EXISTING: return "SetExistingProperty";
                                     case eFailingMutation::SET_NEW: return "SetNewProperty";
                                     case eFailingMutation::REMOVE: return "Remove";
                                     case eFailingMutation::PRUNE: return "Prune";
                                 }
                                 return "Unknown";
                             });

    TEST_F(KVStateFileTest, FailedFirstInsertLeavesStoreMissingAndBad) {
        auto file = open();
        ASSERT_NE(file, nullptr);
        ASSERT_TRUE(file->good());
        backend->failWrite = true;
        EXPECT_FALSE(file->insert("first", {{"value", "unsaved"}}));
        EXPECT_FALSE(file->good());
        EXPECT_EQ(file->size(), 0U);
        EXPECT_EQ(file->get("first"), nullptr);
        EXPECT_EQ(backend->writes, 1U);
        EXPECT_FALSE(backend->bytes.has_value());
        backend->failWrite = false;
        EXPECT_FALSE(file->insert("retry", {}));
        EXPECT_FALSE(file->remove("first"));
        EXPECT_EQ(file->pruneOld(0), 0U);
        file.reset();
        EXPECT_EQ(backend->writes, 1U);
        EXPECT_FALSE(backend->bytes.has_value());
        auto reopened = open();
        ASSERT_NE(reopened, nullptr);
        EXPECT_TRUE(reopened->good());
        EXPECT_EQ(reopened->size(), 0U);
    }

    TEST_F(KVStateFileTest, ReadFailuresAreStickyAndDistinctFromAMissingStore) {
        const std::array<std::optional<std::string>, 2> persisted = {std::nullopt, fixtureBytes({{"first", 100, {{"value", "stored"}}}})};
        for (const auto& bytes : persisted) {
            SCOPED_TRACE(bytes.has_value() ? "existing bytes" : "missing bytes");
            backend           = makeShared<SBackendState>();
            backend->bytes    = bytes;
            backend->failRead = true;
            auto file         = open();
            ASSERT_NE(file, nullptr);
            EXPECT_FALSE(file->good());
            EXPECT_EQ(file->size(), 0U);
            EXPECT_EQ(file->get("first"), nullptr);
            EXPECT_EQ(file->begin(), file->end());
            backend->failRead = false;
            EXPECT_FALSE(file->insert("replacement", {{"value", "must not save"}}));
            EXPECT_FALSE(file->remove("first"));
            EXPECT_EQ(file->pruneOld(0), 0U);
            EXPECT_FALSE(file->good());
            EXPECT_EQ(file->size(), 0U);
            EXPECT_EQ(backend->reads, 1U);
            file.reset();
            EXPECT_EQ(backend->writes, 0U);
            EXPECT_EQ(backend->bytes, bytes);
        }
    }

    struct SInvalidFile {
        std::string name;
        std::string bytes;
    };

    std::vector<SInvalidFile> invalidFiles() {
        const std::string         key      = "first";
        const std::string         property = "property";
        const auto                valid    = fixtureBytes({{key, 123, {{property, "value"}}}});
        std::vector<SInvalidFile> cases    = {
            {"EmptyExistingFile", ""},
            {"VersionZero", fixtureBytes({}, 0)},
            {"VersionTwo", fixtureBytes({}, 2)},
            {"VersionOverflow", fixtureBytes({}, std::numeric_limits<uint64_t>::max())},
            {"DuplicateEntries", fixtureBytes({{"first", 100, {{"a", "one"}}}, {"first", 200, {{"b", "two"}}}})},
            {"DuplicateEmptyEntryKeys", fixtureBytes({{"", 100, {}}, {"", 200, {}}})},
            {"DuplicateProperties", fixtureBytes({{"first", 100, {{"property", "one"}, {"property", "two"}}}})},
            {"DuplicateEmptyPropertyKeys", fixtureBytes({{"first", 100, {{"", "one"}, {"", "two"}}}})},
            {"TrailingBytes", valid + "trailing"},
            {"TrailingNul", valid + '\0'},
            {"TrailingNulAfterEmptyStore", fixtureBytes({}) + '\0'},
            {"EntryCountWithoutEntries", replaceU64(fixtureBytes({}), 16, 1)},
            {"EntriesAfterZeroCount", replaceU64(valid, 16, 0)},
        };
        for (size_t i = 0; i < 8; ++i) {
            auto badMagic = valid;
            badMagic[i] ^= 0x01;
            cases.push_back({"BadMagicByte" + std::to_string(i), std::move(badMagic)});
        }

        constexpr size_t                                           entryKeyLengthOffset    = 8 + 8 + 8;
        const size_t                                               timestampOffset         = entryKeyLengthOffset + 8 + key.size();
        const size_t                                               propertyCountOffset     = timestampOffset + 8;
        const size_t                                               propertyKeyLengthOffset = propertyCountOffset + 8;
        const size_t                                               valueLengthOffset       = propertyKeyLengthOffset + 8 + property.size();
        const std::array<std::pair<std::string_view, size_t>, 5>   fields                  = {{
            {"EntryCount", 16},
            {"EntryKeyLength", entryKeyLengthOffset},
            {"PropertyCount", propertyCountOffset},
            {"PropertyKeyLength", propertyKeyLengthOffset},
            {"ValueLength", valueLengthOffset},
        }};
        const std::array<std::pair<std::string_view, uint64_t>, 4> invalidSizes            = {{
            {"ExceedsRemainingBytes", valid.size() + 1},
            {"Overflows32Bits", uint64_t{1} << 32},
            {"HighBit", uint64_t{1} << 63},
            {"MaximumU64", std::numeric_limits<uint64_t>::max()},
        }};
        for (const auto& [field, offset] : fields) {
            for (const auto& [kind, value] : invalidSizes)
                cases.push_back({std::string{field} + std::string{kind}, replaceU64(valid, offset, value)});
        }
        cases.push_back({"PropertiesAfterZeroCount", replaceU64(valid, propertyCountOffset, 0)});
        return cases;
    }

    class KVStateMalformedFileTest : public KVStateFileTest, public ::testing::WithParamInterface<SInvalidFile> {};

    TEST_P(KVStateMalformedFileTest, RejectsMalformedBytesAndNeverOverwritesThem) {
        expectRejectedBytes(GetParam().bytes);
    }

    INSTANTIATE_TEST_SUITE_P(KVState, KVStateMalformedFileTest, ::testing::ValuesIn(invalidFiles()),
                             [](const ::testing::TestParamInfo<SInvalidFile>& info) { return info.param.name; });

    TEST_F(KVStateFileTest, RejectsEveryTruncatedPrefixIncludingAfterACompleteEntry) {
        const auto complete = fixtureBytes({{"first", 100, {{"property", "value"}}}, {"second", -200, {{"another", "payload"}, {"", ""}}}});
        for (size_t length = 0; length < complete.size(); ++length) {
            SCOPED_TRACE(length);
            expectRejectedBytes(complete.substr(0, length));
        }
    }
}
