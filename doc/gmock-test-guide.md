# Google Mock テストの書き方ガイド

GMock（Google Mock）を使ったユニットテストの基本的な書き方を説明します。

## 目次
1. [基本構造](#基本構造)
2. [モッククラスの作成](#モッククラスの作成)
3. [テストの基本フォーマット](#テストの基本フォーマット)
4. [EXPECT_CALLの使い方](#expect_callの使い方)
5. [引数のマッチャー](#引数のマッチャー)
6. [実行回数の指定](#実行回数の指定)
7. [カスタム動作の定義](#カスタム動作の定義)
8. [実践例](#実践例)

---

## 基本構造

GMockテストは以下の3ステップで構成されます：

```cpp
TEST_F(TestFixture, TestName) {
    // 1. テストデータの準備
    // 2. モックの期待動作を設定（EXPECT_CALL）
    // 3. テスト対象のメソッドを実行
}
```

---

## モッククラスの作成

### インターフェースの定義

```cpp
// 実際のインターフェース（プリンターの例）
class IPrinter {
  public:
    virtual ~IPrinter() = default;
    virtual void Print(const std::string& text) = 0;
    virtual bool IsConnected() const = 0;
    virtual int GetPageCount() const = 0;
};
```

### モッククラスの実装

```cpp
#include <gmock/gmock.h>

class MockPrinter : public IPrinter {
  public:
    MOCK_METHOD(void, Print, (const std::string& text), (override));
    MOCK_METHOD(bool, IsConnected, (), (const, override));
    MOCK_METHOD(int, GetPageCount, (), (const, override));
};
```

#### MOCK_METHODの構文

```cpp
MOCK_METHOD(戻り値の型, メソッド名, (引数リスト), (修飾子));
```

| 要素 | 説明 | 例 |
|------|------|-----|
| 戻り値の型 | メソッドの戻り値 | `void`, `int`, `bool` |
| メソッド名 | 元のメソッド名 | `Clear`, `DrawLine` |
| 引数リスト | 元の引数を括弧で囲む | `(int x, int y)` |
| 修飾子 | `override`, `const` など | `(override)`, `(const, override)` |

---

## テストの基本フォーマット

### テストフィクスチャの定義

```cpp
class DocumentTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // 各テスト実行前に呼ばれる
        // 初期化処理を記述
    }

    void TearDown() override {
        // 各テスト実行後に呼ばれる
        // クリーンアップ処理を記述
    }

    // テストで使う共通のメンバ変数
    MockPrinter mock_printer;
    Document document{mock_printer};
};
```

### テストケースの記述

```cpp
TEST_F(DocumentTest, TestName) {
    // テストの内容
}
```

---

## EXPECT_CALLの使い方

### 基本構文

```cpp
EXPECT_CALL(モックオブジェクト, メソッド名(引数マッチャー))
    .Times(回数)
    .WillOnce(アクション);
```

### 最小限の例

```cpp
// Printメソッドが呼ばれることを期待
EXPECT_CALL(mock_printer, Print(_));
```

### 完全な例

```cpp
EXPECT_CALL(mock_printer, Print("Hello"))
    .Times(1)
    .WillOnce(::testing::Return());
```

---

## 引数のマッチャー

モックメソッドに渡される引数の条件を指定します。

### よく使うマッチャー

| マッチャー | 意味 | 使用例 |
|-----------|------|--------|
| `_` | 任意の値 | `EXPECT_CALL(mock, Method(_))` |
| `123` | 特定の値 | `EXPECT_CALL(mock, Method(123))` |
| `::testing::NotNull()` | NULLでないポインタ | `EXPECT_CALL(mock, Method(::testing::NotNull()))` |
| `::testing::Eq(値)` | 指定した値と等しい | `EXPECT_CALL(mock, Method(::testing::Eq(10)))` |
| `::testing::Ne(値)` | 指定した値と等しくない | `EXPECT_CALL(mock, Method(::testing::Ne(0)))` |
| `::testing::Gt(値)` | 指定した値より大きい | `EXPECT_CALL(mock, Method(::testing::Gt(5)))` |
| `::testing::Lt(値)` | 指定した値より小さい | `EXPECT_CALL(mock, Method(::testing::Lt(100)))` |
| `::testing::Ge(値)` | 指定した値以上 | `EXPECT_CALL(mock, Method(::testing::Ge(0)))` |
| `::testing::Le(値)` | 指定した値以下 | `EXPECT_CALL(mock, Method(::testing::Le(255)))` |

### 使用例

```cpp
// 引数は任意、テキストに"Hello"を含む
EXPECT_CALL(mock_printer, Print(::testing::HasSubstr("Hello")));

// すべての引数が任意
EXPECT_CALL(mock_printer, Print(_));

// テキストが"Error"で始まる
EXPECT_CALL(mock_printer, Print(::testing::StartsWith("Error")));
```

---

## 実行回数の指定

### `.Times(回数)` の使い方

```cpp
// 正確に1回呼ばれる
.Times(1)

// 正確に3回呼ばれる
.Times(3)

// 呼ばれない
.Times(0)

// 1回以上呼ばれる
.Times(::testing::AtLeast(1))

// 最大5回まで呼ばれる
.Times(::testing::AtMost(5))

// 2回から5回の間で呼ばれる
.Times(::testing::Between(2, 5))

// 何回呼ばれてもOK（デフォルト）
.Times(::testing::AnyNumber())
```

### 使用例

```cpp
// 3回印刷されるドキュメント
EXPECT_CALL(mock_printer, Print(_))
    .Times(3);

// 印刷されないことを期待（オフラインモードのテスト）
EXPECT_CALL(mock_printer, Print(_))
    .Times(0);

// 少なくとも1回は印刷される
EXPECT_CALL(mock_printer, Print(_))
    .Times(::testing::AtLeast(1));
```

---

## カスタム動作の定義

### `.WillOnce()` と `.WillRepeatedly()`

```cpp
// 1回目の呼び出し時のアクション
.WillOnce(アクション)

// 毎回のアクション
.WillRepeatedly(アクション)

// 複数回指定可能
.WillOnce(アクション1)
.WillOnce(アクション2)
.WillRepeatedly(アクション3)
```

### よく使うアクション

#### 1. Return() - 値を返す

```cpp
EXPECT_CALL(mock_printer, GetPageCount())
    .WillOnce(::testing::Return(42));

EXPECT_CALL(mock_printer, IsConnected())
    .WillOnce(::testing::Return(true));
```

#### 2. Invoke() - カスタム関数を実行

```cpp
EXPECT_CALL(mock_printer, Print(_))
    .WillOnce(::testing::Invoke([](const std::string& text) {
        // カスタム処理
        std::cout << "Printing: " << text << std::endl;
    }));
```

#### 3. 引数の内容を検証

```cpp
EXPECT_CALL(mock_printer, Print(_))
    .WillOnce(::testing::Invoke([](const std::string& text) {
        // 渡されたテキストを検証
        EXPECT_EQ(text, "Hello, World!");
        EXPECT_GT(text.length(), 0);
    }));
```

---

## 実践例

### 例1: 基本的なテスト

```cpp
// テスト対象: Documentクラス（ドキュメントを印刷する機能）
class Document {
  public:
    Document(IPrinter& printer) : printer_(printer) {}
    
    void PrintHeader(const std::string& title) {
        printer_.Print("=== " + title + " ===");
    }
    
  private:
    IPrinter& printer_;
};

// テストケース
TEST_F(DocumentTest, PrintHeader_CallsPrintOnce) {
    // 1. モックの期待動作を設定
    // "=== Report ==="というテキストで1回呼ばれることを期待
    EXPECT_CALL(mock_printer, Print("=== Report ==="))
        .Times(1);

    // 2. テスト対象のメソッドを実行
    document.PrintHeader("Report");
}
```

### 例2: 呼ばれないことを確認

```cpp
// テスト対象: オフラインモードでは印刷しない
class Document {
  public:
    void PrintIfOnline(const std::string& text) {
        if (printer_.IsConnected()) {
            printer_.Print(text);
        }
    }
  private:
    IPrinter& printer_;
};

// テストケース
TEST_F(DocumentTest, PrintIfOnline_DoesNotPrintWhenOffline) {
    // プリンターがオフライン（falseを返す）
    EXPECT_CALL(mock_printer, IsConnected())
        .WillOnce(::testing::Return(false));
    
    // Printは呼ばれないことを期待
    EXPECT_CALL(mock_printer, Print(_))
        .Times(0);

    document.PrintIfOnline("Test");
}
```

### 例3: データの内容を検証

```cpp
// テストケース: 印刷されるテキストの内容を検証
TEST_F(DocumentTest, Print_ContainsCorrectContent) {
    // Printが呼ばれたときに、引数の内容を検証
    EXPECT_CALL(mock_printer, Print(_))
        .WillOnce(::testing::Invoke([](const std::string& text) {
            // テキストに"Page"という文字が含まれているか
            EXPECT_TRUE(text.find("Page") != std::string::npos);
            // テキストが空でないか
            EXPECT_FALSE(text.empty());
        }));

    document.PrintPageInfo(1, 10);  // "Page 1 of 10"が印刷される
}
```

### 例4: 複数回の呼び出し

```cpp
// テスト対象: 複数行のテキストを印刷
class Document {
  public:
    void PrintMultiLine(const std::vector<std::string>& lines) {
        for (const auto& line : lines) {
            printer_.Print(line);
        }
    }
  private:
    IPrinter& printer_;
};

// テストケース
TEST_F(DocumentTest, PrintMultiLine_PrintsEachLine) {
    std::vector<std::string> lines = {"Line 1", "Line 2", "Line 3"};
    
    // 各行が順番に印刷されることを期待
    EXPECT_CALL(mock_printer, Print("Line 1")).Times(1);
    EXPECT_CALL(mock_printer, Print("Line 2")).Times(1);
    EXPECT_CALL(mock_printer, Print("Line 3")).Times(1);

    document.PrintMultiLine(lines);
}
```

---

## テスト実行の流れ

```
1. テストケース開始
   ↓
2. SetUp() 実行（フィクスチャの初期化）
   ↓
3. EXPECT_CALL でモックの期待動作を設定
   ↓
4. テスト対象のメソッドを実行
   ↓
5. モックメソッドが呼ばれる
   ├→ 引数が期待通りか自動チェック
   ├→ 呼び出し回数をカウント
   └→ WillOnce/WillRepeatedly で指定した処理を実行
   ↓
6. テスト対象のメソッド終了
   ↓
7. 期待通りの回数呼ばれたか自動チェック
   ↓
8. TearDown() 実行（クリーンアップ）
   ↓
9. テストケース終了
```

---

## よくあるパターン

### パターン1: 任意の引数で任意回数

```cpp
// デフォルト動作を設定（何回呼ばれてもOK）
ON_CALL(mock_printer, GetPageCount())
    .WillByDefault(::testing::Return(10));

// テストの中で実際に期待する呼び出しを設定
EXPECT_CALL(mock_printer, GetPageCount())
    .Times(::testing::AtLeast(1));
```

### パターン2: 順序を指定

```cpp
::testing::InSequence seq;

// 1番目の呼び出し: ヘッダーを印刷
EXPECT_CALL(mock_printer, Print("=== Header ==="))
    .Times(1);

// 2番目の呼び出し: 本文を印刷
EXPECT_CALL(mock_printer, Print("Content"))
    .Times(1);

// 3番目の呼び出し: フッターを印刷
EXPECT_CALL(mock_printer, Print("=== Footer ==="))
    .Times(1);
```

### パターン3: 引数を保存

```cpp
std::string saved_text;

EXPECT_CALL(mock_printer, Print(_))
    .WillOnce(::testing::Invoke([&saved_text](const std::string& text) {
        saved_text = text;  // 引数を保存
    }));

document.PrintSomething();

// 後で検証
EXPECT_EQ(saved_text, "Expected text");
EXPECT_GT(saved_text.length(), 10);
```

---

## トラブルシューティング

### よくあるエラー

#### 1. "Uninteresting mock function call"

```
原因: EXPECT_CALLで設定していないメソッドが呼ばれた
解決: EXPECT_CALLを追加するか、ON_CALLでデフォルト動作を設定
```

#### 2. "Expected to be called once, but has 0 interactions"

```
原因: EXPECT_CALLで設定したメソッドが呼ばれなかった
解決: テスト対象のコードが正しくメソッドを呼んでいるか確認
```

#### 3. "Expected to be called 1 time, but has 2 interactions"

```
原因: 期待した回数と実際の呼び出し回数が異なる
解決: .Times()の値を確認、またはコードのロジックを確認
```

---

## まとめ

### GMockテストの基本フォーマット

```cpp
TEST_F(TestFixture, TestName) {
    // 1. データ準備
    TestData data = CreateTestData();
    
    // 2. モックの期待設定
    EXPECT_CALL(mock_object, Method(引数マッチャー))
        .Times(回数)
        .WillOnce(アクション);
    
    // 3. テスト実行
    target_object.ExecuteMethod(data);
    
    // 4. 追加の検証（必要に応じて）
    EXPECT_EQ(expected, actual);
}
```

### チェックリスト

- ✅ モックオブジェクトを作成したか
- ✅ EXPECT_CALLで期待動作を設定したか
- ✅ 引数のマッチャーは適切か
- ✅ 呼び出し回数は正しいか
- ✅ 必要に応じてWillOnceでデータ検証をしているか
- ✅ テストが独立しているか（他のテストに依存していないか）

---

## 参考リンク
- [Google Test公式ドキュメント](https://google.github.io/googletest/)
