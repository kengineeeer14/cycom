# 要件・設計書

要件から詳細設計までを管理します。資料は実装の変更に合わせて更新してください。

## 資料一覧

- [要件定義](./requirements.md)
- 基本設計
  - [機能一覧](./basic_design/functions.md)
  - [スレッド一覧](./basic_design/threads.md)
- 詳細設計
  - [SensorManager](./detail_design/sensor_manager.md)
  - [L76k](./detail_design/l76k.md)
  - [DisplayManager](./detail_design/display_manager.md)
  - [TextRenderer](./detail_design/text_renderer.md)
  - [Logger](./detail_design/logger.md)
  - [TouchManager](./detail_design/touch_manager.md)

## 運用ルール

- IDは一意にします。
- 要件・機能・スレッド・関数はIDで管理し、機能に対応するクラスは機能一覧に記載します。
- 未確定の項目は「未定」と記載します。
- クラスや関数を変更した場合は、対応する設計書も更新します。
