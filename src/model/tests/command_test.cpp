// Тесты разбора и исполнения консольных команд (CommandProcessor).

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "model/command_processor.h"
#include "model/hash_table.h"
#include "model/response.h"
#include "model/self_balancing_binary_search_tree.h"
#include "test_helpers.h"

namespace {

using StoreTypes =
    ::testing::Types<s21::HashTable, s21::SelfBalancingBinarySearchTree>;

// Достаёт альтернативу из ответа модели.
template <typename T>
const T& As(const s21::Response& response) {
  return std::get<T>(response);
}

template <typename StoreImpl>
class CommandTest : public ::testing::Test {
 protected:
  s21::CommandProcessor processor{std::make_unique<StoreImpl>()};

  // Выполняет команду и возвращает ответ.
  s21::Response Exec(const std::string& line) {
    return *processor.Execute(line);
  }

  // Значение записи в хранилище, проверяемое через команду GET.
  std::optional<s21::Student> Stored(const std::string& key) {
    return As<s21::MaybeRecord>(Exec("GET " + key)).value;
  }
};

TYPED_TEST_SUITE(CommandTest, StoreTypes);

// ─── Выбор хранилища и работа без него ─────────────────────────────────────

TEST(ProcessorTest, SelectStoreCreatesRequestedStore) {
  for (const int kind : {1, 2}) {
    s21::CommandProcessor processor;
    EXPECT_TRUE(processor.SelectStore(kind));
    EXPECT_TRUE(processor.HasStore());
    EXPECT_TRUE(std::holds_alternative<s21::Ok>(
        *processor.Execute("SET foo Vasilev Ivan 2000 Moscow 55")));
  }
}

TEST(ProcessorTest, SelectStoreRejectsUnknownKind) {
  s21::CommandProcessor processor;
  EXPECT_FALSE(processor.SelectStore(0));
  EXPECT_FALSE(processor.SelectStore(3));
  EXPECT_FALSE(processor.HasStore());
}

TEST(ProcessorTest, ExecuteWithoutStoreErrors) {
  s21::CommandProcessor processor;
  EXPECT_EQ(
      processor.Execute("GET foo"),
      (std::optional{s21::Response{s21::Error{"storage is not selected"}}}));
  EXPECT_EQ(processor.Execute("   "), std::nullopt);
}

TEST(ProcessorTest, UploadExpandsLeadingTilde) {
  s21::CommandProcessor processor{std::make_unique<s21::HashTable>()};
  const std::string old_home =
      std::getenv("HOME") != nullptr ? std::getenv("HOME") : "";
  ::setenv("HOME", ::testing::TempDir().c_str(), 1);
  tests::WriteTempFile("cmd_home_upload.dat", "k \"A\" \"B\" 2000 \"C\" 1\n");
  const s21::Response response =
      *processor.Execute("UPLOAD ~/cmd_home_upload.dat");
  ::setenv("HOME", old_home.c_str(), 1);
  EXPECT_EQ(As<s21::Count>(response).value, 1u);
}

TEST(ProcessorTest, TildeStaysLiteralWithoutHome) {
  s21::CommandProcessor processor{std::make_unique<s21::HashTable>()};
  const std::string old_home =
      std::getenv("HOME") != nullptr ? std::getenv("HOME") : "";
  ::unsetenv("HOME");
  const s21::Response response = *processor.Execute("UPLOAD ~/nope.dat");
  ::setenv("HOME", old_home.c_str(), 1);
  EXPECT_EQ(As<s21::Error>(response).message,
            "unable to open file \"~/nope.dat\"");
}

// ─── Пустая строка и неизвестные команды ────────────────────────────────────

TYPED_TEST(CommandTest, EmptyLineGivesNullopt) {
  EXPECT_EQ(this->processor.Execute(""), std::nullopt);
  EXPECT_EQ(this->processor.Execute("   \t "), std::nullopt);
}

TYPED_TEST(CommandTest, UnknownCommand) {
  EXPECT_EQ(As<s21::Error>(this->Exec("FOO bar")).message,
            "unknown command \"FOO\"");
  EXPECT_EQ(As<s21::Error>(this->Exec("get foo")).message,
            "unknown command \"get\"");
}

// ─── SET ────────────────────────────────────────────────────────────────────

TYPED_TEST(CommandTest, SetCreatesRecord) {
  EXPECT_TRUE(std::holds_alternative<s21::Ok>(
      this->Exec("SET foo Vasilev Ivan 2000 Moscow 55")));
  EXPECT_EQ(this->Stored("foo"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
}

TYPED_TEST(CommandTest, SetWithExtraSpaces) {
  EXPECT_TRUE(std::holds_alternative<s21::Ok>(
      this->Exec("  SET   foo  Vasilev   Ivan 2000  Moscow  55 ")));
  EXPECT_EQ(this->Stored("foo"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
}

TYPED_TEST(CommandTest, SetWrongArity) {
  EXPECT_EQ(As<s21::Error>(this->Exec("SET foo")).message,
            "wrong number of arguments for \"SET\"");
  EXPECT_EQ(As<s21::Error>(this->Exec("SET foo A B 2000 C 55 X")).message,
            "wrong number of arguments for \"SET\"");
  EXPECT_EQ(As<s21::Error>(this->Exec("SET foo A B 2000 C 55 EX 10 X")).message,
            "wrong number of arguments for \"SET\"");
}

TYPED_TEST(CommandTest, SetCastErrorForYearAndCoins) {
  EXPECT_EQ(
      As<s21::Error>(this->Exec("SET foo Vasilev Ivan aaaa Moscow 55")).message,
      "unable cast value \"aaaa\" to type int");
  EXPECT_EQ(As<s21::Error>(this->Exec("SET foo Vasilev Ivan 2000 Moscow 12ab"))
                .message,
            "unable cast value \"12ab\" to type int");
}

TYPED_TEST(CommandTest, SetWithExStoresTtl) {
  EXPECT_TRUE(std::holds_alternative<s21::Ok>(
      this->Exec("SET foo Vasilev Ivan 2000 Moscow 55 EX 100")));
  EXPECT_EQ(As<s21::MaybeTtl>(this->Exec("TTL foo")).seconds, 100);
}

TYPED_TEST(CommandTest, SetExRequiresExKeyword) {
  EXPECT_EQ(
      As<s21::Error>(this->Exec("SET foo Vasilev Ivan 2000 Moscow 55 XX 100"))
          .message,
      "expected EX parameter");
}

TYPED_TEST(CommandTest, SetExSecondsMustBeNonNegativeInt) {
  EXPECT_EQ(
      As<s21::Error>(this->Exec("SET foo Vasilev Ivan 2000 Moscow 55 EX abcd"))
          .message,
      "unable cast value \"abcd\" to type int");
  EXPECT_EQ(
      As<s21::Error>(this->Exec("SET foo Vasilev Ivan 2000 Moscow 55 EX -1"))
          .message,
      "unable cast value \"-1\" to type int");
}

// ─── GET / EXISTS / DEL / TTL ───────────────────────────────────────────────

TYPED_TEST(CommandTest, GetReturnsRecordOrNull) {
  this->Exec("SET foo Vasilev Ivan 2000 Moscow 55");
  const std::optional<s21::Student> record =
      As<s21::MaybeRecord>(this->Exec("GET foo")).value;
  ASSERT_TRUE(record.has_value());
  EXPECT_EQ(*record, tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_EQ(As<s21::MaybeRecord>(this->Exec("GET bar")).value, std::nullopt);
}

TYPED_TEST(CommandTest, GetWrongArity) {
  EXPECT_EQ(As<s21::Error>(this->Exec("GET")).message,
            "wrong number of arguments for \"GET\"");
  EXPECT_EQ(As<s21::Error>(this->Exec("GET a b")).message,
            "wrong number of arguments for \"GET\"");
}

TYPED_TEST(CommandTest, ExistsAndDelFlags) {
  EXPECT_EQ(As<s21::Flag>(this->Exec("EXISTS foo")).value, false);
  this->Exec("SET foo Vasilev Ivan 2000 Moscow 55");
  EXPECT_EQ(As<s21::Flag>(this->Exec("EXISTS foo")).value, true);
  EXPECT_EQ(As<s21::Flag>(this->Exec("DEL foo")).value, true);
  EXPECT_EQ(As<s21::Flag>(this->Exec("DEL foo")).value, false);
  EXPECT_EQ(As<s21::Error>(this->Exec("EXISTS foo x")).message,
            "wrong number of arguments for \"EXISTS\"");
  EXPECT_EQ(As<s21::Error>(this->Exec("DEL")).message,
            "wrong number of arguments for \"DEL\"");
}

TYPED_TEST(CommandTest, TtlReportsRemainingSeconds) {
  this->Exec("SET foo Vasilev Ivan 2000 Moscow 55 EX 30");
  EXPECT_EQ(As<s21::MaybeTtl>(this->Exec("TTL foo")).seconds, 30);
  this->Exec("SET bar Vasilev Ivan 2000 Moscow 55");
  EXPECT_EQ(As<s21::MaybeTtl>(this->Exec("TTL bar")).seconds, -1);
  EXPECT_EQ(As<s21::MaybeTtl>(this->Exec("TTL missing")).seconds, std::nullopt);
  EXPECT_EQ(As<s21::Error>(this->Exec("TTL a b")).message,
            "wrong number of arguments for \"TTL\"");
}

// ─── RENAME ─────────────────────────────────────────────────────────────────

TYPED_TEST(CommandTest, RenameMovesRecord) {
  this->Exec("SET foo Vasilev Ivan 2000 Moscow 55");
  EXPECT_TRUE(std::holds_alternative<s21::Ok>(this->Exec("RENAME foo bar")));
  EXPECT_EQ(this->Stored("foo"), std::nullopt);
  EXPECT_EQ(this->Stored("bar"),
            tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
}

TYPED_TEST(CommandTest, RenameMissingKeyErrors) {
  EXPECT_EQ(As<s21::Error>(this->Exec("RENAME foo bar")).message,
            "key \"foo\" does not exist");
  EXPECT_EQ(As<s21::Error>(this->Exec("RENAME foo")).message,
            "wrong number of arguments for \"RENAME\"");
}

// ─── KEYS / SHOWALL ─────────────────────────────────────────────────────────

TYPED_TEST(CommandTest, KeysListsAllKeys) {
  this->Exec("SET boo Ivanov Vasily 2000 Moscow 55");
  this->Exec("SET foo Vasilev Ivan 2000 Moscow 55");
  this->Exec("SET bar Sidorov Sergei 1847 Suzdal 42");

  std::vector<std::string> keys = As<s21::KeyList>(this->Exec("KEYS")).keys;
  std::ranges::sort(keys);
  EXPECT_EQ(keys, (std::vector<std::string>{"bar", "boo", "foo"}));

  EXPECT_EQ(As<s21::Error>(this->Exec("KEYS foo")).message,
            "wrong number of arguments for \"KEYS\"");
}

TYPED_TEST(CommandTest, ShowAllReturnsAllRecords) {
  EXPECT_TRUE(As<s21::StudentTable>(this->Exec("SHOWALL")).students.empty());
  this->Exec("SET foo Vasilev Ivan 2000 Moscow 55");
  this->Exec("SET bar Sidorov Sergei 1847 Suzdal 42");
  const std::vector<s21::Student> students =
      As<s21::StudentTable>(this->Exec("SHOWALL")).students;
  EXPECT_EQ(students.size(), 2u);
  EXPECT_TRUE(std::ranges::find(students, tests::MakeStudent(
                                              "Vasilev", "Ivan", 2000, "Moscow",
                                              55)) != students.end());
  EXPECT_EQ(As<s21::Error>(this->Exec("SHOWALL x")).message,
            "wrong number of arguments for \"SHOWALL\"");
}

// ─── UPDATE ─────────────────────────────────────────────────────────────────

TYPED_TEST(CommandTest, UpdateChangesOnlyFilledFields) {
  this->Exec("SET foo Vasilev Ivan 2000 Moscow 55");
  EXPECT_TRUE(std::holds_alternative<s21::Ok>(
      this->Exec("UPDATE foo Petrov - - Kazan -")));
  EXPECT_EQ(this->Stored("foo"),
            tests::MakeStudent("Petrov", "Ivan", 2000, "Kazan", 55));
}

TYPED_TEST(CommandTest, UpdateMissingKeyErrors) {
  EXPECT_EQ(As<s21::Error>(this->Exec("UPDATE foo Petrov - - Kazan -")).message,
            "key \"foo\" does not exist");
  EXPECT_EQ(As<s21::Error>(this->Exec("UPDATE foo Petrov - - Kazan")).message,
            "wrong number of arguments for \"UPDATE\"");
}

TYPED_TEST(CommandTest, UpdateCastErrors) {
  this->Exec("SET foo Vasilev Ivan 2000 Moscow 55");
  EXPECT_EQ(As<s21::Error>(this->Exec("UPDATE foo - - abcd - -")).message,
            "unable cast value \"abcd\" to type int");
  EXPECT_EQ(As<s21::Error>(this->Exec("UPDATE foo - - - - abcd")).message,
            "unable cast value \"abcd\" to type int");
}

// ─── FIND ───────────────────────────────────────────────────────────────────

TYPED_TEST(CommandTest, FindByFilledFields) {
  this->Exec("SET boo Ivanov Vasily 2000 Moscow 55");
  this->Exec("SET foo Vasilev Ivan 2000 Tver 42");

  std::vector<std::string> keys =
      As<s21::KeyList>(this->Exec("FIND - - 2000 - -")).keys;
  std::ranges::sort(keys);
  EXPECT_EQ(keys, (std::vector<std::string>{"boo", "foo"}));

  EXPECT_EQ(As<s21::KeyList>(this->Exec("FIND Vasilev - - - -")).keys,
            (std::vector<std::string>{"foo"}));
  EXPECT_TRUE(As<s21::KeyList>(this->Exec("FIND - - 1999 - -")).keys.empty());
}

TYPED_TEST(CommandTest, FindArityAndCastErrors) {
  EXPECT_EQ(As<s21::Error>(this->Exec("FIND - - 2000 -")).message,
            "wrong number of arguments for \"FIND\"");
  EXPECT_EQ(As<s21::Error>(this->Exec("FIND - - 20x0 - -")).message,
            "unable cast value \"20x0\" to type int");
}

// ─── UPLOAD / EXPORT ────────────────────────────────────────────────────────

TYPED_TEST(CommandTest, UploadReadsDataFile) {
  const std::string path =
      tests::WriteTempFile("cmd_upload.dat",
                           "key1 \"Vasilev\" \"Ivan\" 2001 \"Rostov\" 55\n"
                           "key2 \"Petrov\" \"Petr\" 2002 \"Tver\" 42\n");
  EXPECT_EQ(As<s21::Count>(this->Exec("UPLOAD " + path)).value, 2u);
  EXPECT_EQ(this->Stored("key2"),
            tests::MakeStudent("Petrov", "Petr", 2002, "Tver", 42));
}

TYPED_TEST(CommandTest, UploadMissingFileErrors) {
  const std::string path = ::testing::TempDir() + "cmd_no_such_file.dat";
  EXPECT_EQ(As<s21::Error>(this->Exec("UPLOAD " + path)).message,
            "unable to open file \"" + path + "\"");
  EXPECT_EQ(As<s21::Error>(this->Exec("UPLOAD")).message,
            "wrong number of arguments for \"UPLOAD\"");
}

TYPED_TEST(CommandTest, ExportWritesDataFile) {
  this->Exec("SET foo Vasilev Ivan 2000 Moscow 55");
  this->Exec("SET bar Sidorov Sergei 1847 Suzdal 42");

  const std::string path = ::testing::TempDir() + "cmd_export.dat";
  EXPECT_EQ(As<s21::Count>(this->Exec("EXPORT " + path)).value, 2u);

  std::vector<std::string> lines = tests::SplitLines(tests::ReadFile(path));
  while (!lines.empty() && lines.back().empty()) {
    lines.pop_back();
  }
  std::ranges::sort(lines);
  EXPECT_EQ(lines, (std::vector<std::string>{
                       "bar \"Sidorov\" \"Sergei\" 1847 \"Suzdal\" 42",
                       "foo \"Vasilev\" \"Ivan\" 2000 \"Moscow\" 55"}));
}

}  // namespace
