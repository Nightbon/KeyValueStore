// Общий набор тестов интерфейса KeyValueStore для обеих реализаций.

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <format>
#include <string>
#include <thread>
#include <vector>

#include "model/hash_table.h"
#include "model/self_balancing_binary_search_tree.h"
#include "test_helpers.h"

namespace {

using StoreTypes =
    ::testing::Types<s21::HashTable, s21::SelfBalancingBinarySearchTree>;

template <typename StoreImpl>
class KeyValueStoreTest : public ::testing::Test {
 protected:
  StoreImpl store;
};

TYPED_TEST_SUITE(KeyValueStoreTest, StoreTypes);

// ─── SET / GET ──────────────────────────────────────────────────────────────

TYPED_TEST(KeyValueStoreTest, SetThenGet) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_EQ(this->store.Get("foo"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_EQ(this->store.Get("unknown"), std::nullopt);
}

TYPED_TEST(KeyValueStoreTest, SetOverwritesValue) {
  this->store.Set("foo", tests::MakeStudent("Vas", "I", 20, "Mos", 5));
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_EQ(this->store.Size(), 1u);
  EXPECT_EQ(this->store.Get("foo"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
}

TYPED_TEST(KeyValueStoreTest, SetWithoutExResetsTtl) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55),
                  std::chrono::seconds{10});
  this->store.Set("foo",
                  tests::MakeStudent("Petrov", "Petr", 2001, "Tver", 42));
  EXPECT_EQ(this->store.Ttl("foo"), -1);
}

TYPED_TEST(KeyValueStoreTest, ValueIsCopiedOnSet) {
  s21::Student value =
      tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55);
  this->store.Set("foo", value);
  value.city = "Kazan";
  EXPECT_EQ(this->store.Get("foo"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
}

// ─── EXISTS / DEL ───────────────────────────────────────────────────────────

TYPED_TEST(KeyValueStoreTest, ExistsTrueFalse) {
  EXPECT_FALSE(this->store.Exists("foo"));
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_TRUE(this->store.Exists("foo"));
}

TYPED_TEST(KeyValueStoreTest, DelReturnsTrueOnce) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_TRUE(this->store.Del("foo"));
  EXPECT_FALSE(this->store.Del("foo"));
  EXPECT_EQ(this->store.Get("foo"), std::nullopt);
  EXPECT_EQ(this->store.Size(), 0u);
}

// ─── UPDATE ─────────────────────────────────────────────────────────────────

TYPED_TEST(KeyValueStoreTest, UpdateAllFields) {
  this->store.Set("foo", tests::MakeStudent("Vas", "I", 20, "Mos", 5));
  const s21::StudentFilter filter{"Vasilev", "Ivan", 2000, "Moscow", 55};
  EXPECT_TRUE(this->store.Update("foo", filter));
  EXPECT_EQ(this->store.Get("foo"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
}

TYPED_TEST(KeyValueStoreTest, UpdateWithDashKeepsFields) {
  this->store.Set("foo", tests::MakeStudent("Vas", "I", 20, "Mos", 5));
  const s21::StudentFilter filter{"Vasilev", std::nullopt, std::nullopt,
                                  std::nullopt, 55};
  EXPECT_TRUE(this->store.Update("foo", filter));
  EXPECT_EQ(this->store.Get("foo"),
            tests::MakeStudent("Vasilev", "I", 20, "Mos", 55));
}

TYPED_TEST(KeyValueStoreTest, UpdateMissingKeyFails) {
  const s21::StudentFilter filter{"Vasilev", std::nullopt, std::nullopt,
                                  std::nullopt, 55};
  EXPECT_FALSE(this->store.Update("foo", filter));
}

// ─── KEYS ───────────────────────────────────────────────────────────────────

TYPED_TEST(KeyValueStoreTest, KeysListsLiveKeys) {
  EXPECT_TRUE(this->store.Keys().empty());

  this->store.Set("boo",
                  tests::MakeStudent("Ivanov", "Vasily", 2000, "Moscow", 55));
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  this->store.Set("bar",
                  tests::MakeStudent("Sidorov", "Sergei", 1847, "Suzdal", 42));

  std::vector<std::string> keys = this->store.Keys();
  std::ranges::sort(keys);
  EXPECT_EQ(keys, (std::vector<std::string>{"bar", "boo", "foo"}));

  EXPECT_TRUE(this->store.Del("foo"));
  keys = this->store.Keys();
  std::ranges::sort(keys);
  EXPECT_EQ(keys, (std::vector<std::string>{"bar", "boo"}));
}

// ─── RENAME ─────────────────────────────────────────────────────────────────

TYPED_TEST(KeyValueStoreTest, RenameMovesRecord) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_TRUE(this->store.Rename("foo", "foo2"));
  EXPECT_EQ(this->store.Get("foo"), std::nullopt);
  EXPECT_EQ(this->store.Get("foo2"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_EQ(this->store.Size(), 1u);
}

TYPED_TEST(KeyValueStoreTest, RenameMissingSourceFails) {
  EXPECT_FALSE(this->store.Rename("foo", "foo2"));
}

TYPED_TEST(KeyValueStoreTest, RenameOverwritesTarget) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  this->store.Set("foo2",
                  tests::MakeStudent("Ivanov", "Vasily", 2000, "Moscow", 55));
  EXPECT_TRUE(this->store.Rename("foo", "foo2"));
  EXPECT_EQ(this->store.Size(), 1u);
  EXPECT_EQ(this->store.Get("foo2"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
}

TYPED_TEST(KeyValueStoreTest, RenameToSameKeyKeepsRecord) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_TRUE(this->store.Rename("foo", "foo"));
  EXPECT_EQ(this->store.Size(), 1u);
  EXPECT_EQ(this->store.Get("foo"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
}

// ─── TTL ────────────────────────────────────────────────────────────────────

TYPED_TEST(KeyValueStoreTest, TtlWithoutExpiryIsMinusOne) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_EQ(this->store.Ttl("foo"), -1);
}

TYPED_TEST(KeyValueStoreTest, TtlOfMissingKeyIsNull) {
  EXPECT_EQ(this->store.Ttl("unknown"), std::nullopt);
}

TYPED_TEST(KeyValueStoreTest, TtlCountsDown) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55),
                  std::chrono::seconds{2});
  const std::optional<int> first = this->store.Ttl("foo");
  ASSERT_TRUE(first.has_value());
  EXPECT_GT(*first, 0);
  EXPECT_LE(*first, 2);

  std::this_thread::sleep_for(std::chrono::milliseconds{1100});
  const std::optional<int> second = this->store.Ttl("foo");
  ASSERT_TRUE(second.has_value());
  EXPECT_LE(*second, 1);
}

TYPED_TEST(KeyValueStoreTest, ExpiredEntryVanishes) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55),
                  std::chrono::seconds{1});
  this->store.Set("bar",
                  tests::MakeStudent("Ivanov", "Vasily", 2000, "Moscow", 55));

  std::this_thread::sleep_for(std::chrono::milliseconds{1150});

  EXPECT_EQ(this->store.Get("foo"), std::nullopt);
  EXPECT_FALSE(this->store.Exists("foo"));
  EXPECT_FALSE(this->store.Del("foo"));
  EXPECT_EQ(this->store.Ttl("foo"), std::nullopt);
  EXPECT_EQ(this->store.Keys(), std::vector<std::string>{"bar"});
  ASSERT_EQ(this->store.ShowAll().size(), 1u);
  EXPECT_EQ(this->store.ShowAll().front(),
            tests::MakeStudent("Ivanov", "Vasily", 2000, "Moscow", 55));
  EXPECT_EQ(this->store.Find(s21::StudentFilter{.last_name = "Vasilev"}),
            (std::vector<std::string>{}));
  EXPECT_EQ(this->store.Size(), 1u);
}

TYPED_TEST(KeyValueStoreTest, ReSetAfterExpiryWorks) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55),
                  std::chrono::seconds{1});
  std::this_thread::sleep_for(std::chrono::milliseconds{1150});
  EXPECT_FALSE(this->store.Exists("foo"));

  this->store.Set("foo",
                  tests::MakeStudent("Petrov", "Petr", 2001, "Tver", 42));
  EXPECT_EQ(this->store.Get("foo"),
            tests::MakeStudent("Petrov", "Petr", 2001, "Tver", 42));
}

// ─── FIND ───────────────────────────────────────────────────────────────────

TYPED_TEST(KeyValueStoreTest, FindByAllFields) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  this->store.Set("boo",
                  tests::MakeStudent("Vasilev", "Anton", 1997, "Tver", 55));

  const s21::StudentFilter filter{"Vasilev", "Ivan", 2000, "Moscow", 55};
  EXPECT_EQ(this->store.Find(filter), (std::vector<std::string>{"foo"}));
}

TYPED_TEST(KeyValueStoreTest, FindByPartialMatch) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  this->store.Set("boo",
                  tests::MakeStudent("Vasilev", "Anton", 1997, "Tver", 55));
  this->store.Set("bar",
                  tests::MakeStudent("Ivanov", "Vasily", 2000, "Moscow", 42));

  const s21::StudentFilter filter{.last_name = "Vasilev", .coins = 55};
  std::vector<std::string> keys = this->store.Find(filter);
  std::ranges::sort(keys);
  EXPECT_EQ(keys, (std::vector<std::string>{"boo", "foo"}));
}

TYPED_TEST(KeyValueStoreTest, FindNoMatchGivesEmpty) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  const s21::StudentFilter filter{.last_name = "Sidorov"};
  EXPECT_TRUE(this->store.Find(filter).empty());
}

TYPED_TEST(KeyValueStoreTest, FindReflectsUpdatedValue) {
  this->store.Set("foo", tests::MakeStudent("Vas", "I", 20, "Mos", 5));
  EXPECT_TRUE(
      this->store.Update("foo", s21::StudentFilter{.last_name = "Vasilev"}));
  EXPECT_EQ(this->store.Find(s21::StudentFilter{.last_name = "Vasilev"}),
            (std::vector<std::string>{"foo"}));
  EXPECT_TRUE(this->store.Find(s21::StudentFilter{.last_name = "Vas"}).empty());
}

// ─── SHOWALL ────────────────────────────────────────────────────────────────

TYPED_TEST(KeyValueStoreTest, ShowAllReturnsAllRecords) {
  EXPECT_TRUE(this->store.ShowAll().empty());

  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  this->store.Set("boo",
                  tests::MakeStudent("Ivanov", "Vasily", 2000, "Moscow", 55));

  std::vector<s21::Student> students = this->store.ShowAll();
  std::ranges::sort(students, {}, &s21::Student::last_name);
  ASSERT_EQ(students.size(), 2u);
  EXPECT_EQ(students[0],
            tests::MakeStudent("Ivanov", "Vasily", 2000, "Moscow", 55));
  EXPECT_EQ(students[1],
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
}

// ─── UPLOAD / EXPORT ────────────────────────────────────────────────────────

TYPED_TEST(KeyValueStoreTest, UploadReadsQuotedFields) {
  const std::string path = tests::WriteTempFile(
      "upload.dat",
      "key1 \"Vasilev\" \"Ivan\" 2001 \"Rostov\" 55\n"
      "key2 \"Ivanov\" \"Vasiliy\" 2000 \"New York\" 42\n");
  EXPECT_EQ(this->store.Upload(path), 2u);
  EXPECT_EQ(this->store.Get("key1"),
            tests::MakeStudent("Vasilev", "Ivan", 2001, "Rostov", 55));
  EXPECT_EQ(this->store.Get("key2"),
            tests::MakeStudent("Ivanov", "Vasiliy", 2000, "New York", 42));
}

TYPED_TEST(KeyValueStoreTest, UploadSkipsBrokenLines) {
  const std::string path =
      tests::WriteTempFile("upload_broken.dat",
                           "key1 \"Vasilev\" \"Ivan\" 2001 \"Rostov\" 55\n"
                           "key2 \"Ivanov\" \"Vasiliy\" abcd \"Moscow\" 55\n"
                           "key3 \"Sidorov\" \"Sergei\"\n");
  EXPECT_EQ(this->store.Upload(path), 1u);
  EXPECT_EQ(this->store.Size(), 1u);
}

TYPED_TEST(KeyValueStoreTest, UploadMissingFileThrows) {
  EXPECT_THROW(
      static_cast<void>(this->store.Upload("no_such_dir/no_such_file.dat")),
      std::runtime_error);
}

TYPED_TEST(KeyValueStoreTest, ExportWritesExpectedFormat) {
  this->store.Set("key1",
                  tests::MakeStudent("Vasilev", "Ivan", 2001, "Rostov", 55));
  this->store.Set("key2",
                  tests::MakeStudent("Ivanov", "Vasiliy", 2000, "Москва", 55));

  const std::string path = tests::WriteTempFile("export.dat", "");
  EXPECT_EQ(this->store.Export(path), 2u);

  std::vector<std::string> lines = tests::SplitLines(tests::ReadFile(path));
  std::ranges::sort(lines);
  EXPECT_EQ(lines, (std::vector<std::string>{
                       "key1 \"Vasilev\" \"Ivan\" 2001 \"Rostov\" 55",
                       "key2 \"Ivanov\" \"Vasiliy\" 2000 \"Москва\" 55",
                   }));
}

TYPED_TEST(KeyValueStoreTest, ExportUploadRoundTrip) {
  this->store.Set("foo",
                  tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  this->store.Set("boo",
                  tests::MakeStudent("Ivanov", "Vasily", 1997, "Tver", 55));
  this->store.Set("bar",
                  tests::MakeStudent("Sidorov", "Sergei", 1847, "Suzdal", 42));

  const std::string path = tests::WriteTempFile("roundtrip.dat", "");
  ASSERT_EQ(this->store.Export(path), 3u);

  TypeParam restored;
  EXPECT_EQ(restored.Upload(path), 3u);
  EXPECT_EQ(restored.Get("foo"), this->store.Get("foo"));
  EXPECT_EQ(restored.Get("boo"), this->store.Get("boo"));
  EXPECT_EQ(restored.Get("bar"), this->store.Get("bar"));
}

TYPED_TEST(KeyValueStoreTest, ExportMissingPathThrows) {
  EXPECT_THROW(static_cast<void>(this->store.Export("no_such_dir/export.dat")),
               std::runtime_error);
}

// ─── Стресс: рехэш цепочек и перебалансировка дерева ───────────────────────

TYPED_TEST(KeyValueStoreTest, ManyKeysSurviveGrowthAndShrink) {
  for (int i = 0; i < 300; ++i) {
    this->store.Set(std::format("key{:03}", i),
                    tests::MakeStudent("Last", "First", 2000, "City", i));
  }
  ASSERT_EQ(this->store.Size(), 300u);
  EXPECT_EQ(this->store.Keys().size(), 300u);

  for (int i = 0; i < 300; i += 7) {
    EXPECT_EQ(this->store.Get(std::format("key{:03}", i)),
              tests::MakeStudent("Last", "First", 2000, "City", i));
  }

  for (int i = 0; i < 300; i += 2) {
    EXPECT_TRUE(this->store.Del(std::format("key{:03}", i)));
  }
  EXPECT_EQ(this->store.Size(), 150u);
  EXPECT_EQ(this->store.Get("key002"), std::nullopt);
  EXPECT_EQ(this->store.Get("key003"),
            tests::MakeStudent("Last", "First", 2000, "City", 3));
}

}  // namespace
