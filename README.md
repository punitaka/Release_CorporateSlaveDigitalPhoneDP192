## 【社畜デジタルホン】
20世紀の"デジタルホン"の携帯電話を使った【社畜デジタルホン】のプログラムたちです。<br>
前世紀にサービスが終了したデジタルホンの携帯電話をSeeed Studio XIAO ESP32を使って令和に復活させます。<br>
1996年にグッドデザイン賞を受賞した日本電装製の携帯電話DP192の不動品を利用します。<br>
社畜パトランプシリーズの最新作です。<br>
<br>
デジタル"フォン"じゃなくて"ホン"ってところがシャレオツでお気に入りです。<br>
Let's 東京デジタルホーン。<br>
<br>
1.3インチOLEDディスプレイとパッシブブザーは別途調達しました。

### 概要
* マイコンはESP32-C3を利用しています。
* OLEDディスプレイとブザーは外部のパーツを利用しました。
* 電源は純正のクレードル経由で本体にUSB Type-Cで供給します。
* 機種は1996年製の日本電装（現デンソー）のDP192です。
* メールを検知するとディスプレイを約3分表示して、ブザーを約1分鳴らします。
* 5分に1回メールをチェックします。「メールチェック中」の文字をOLCDに表示します。
* 初回起動した時は動作確認のため、画面とブザーを短時間起動します。

### Youtubeの動画
* [1996年製のデジタルホンの携帯電話をESP32で令和に復活させる](https://youtu.be/iH_d21-d03c)
* [再生リスト 【IoT/IT】社畜パトランプ](https://www.youtube.com/playlist?list=PLWImbCHDGxLq3FAbEvxdpbAjMivbufqSf)

### 使用した携帯電話やパーツたちはこちらです。
* [デジタルホン　DP192 1996 GOOD DESIGN AWARD](https://www.g-mark.org/en/gallery/winners/9ceb71d1-803d-11ed-862b-0242ac130002?companies=43280d13-c8b8-49d7-b503-0043e8dfc0b9&years=1996)
* [1.3 インチ OLED モジュール Witte Kleur 128X64 OLED Lcd LED](https://ja.aliexpress.com/item/1005007451015054.html)
* [アクティブバザーセンサーアラームモジュール](https://ja.aliexpress.com/item/1005007592230799.html)
* [Seeed Studio XIAO ESP32C3 入門ガイド](https://wiki.seeedstudio.com/ja/XIAO_ESP32C3_Getting_Started/)
* [USB Type-Cのオス](https://ja.aliexpress.com/item/1005010764792650.html)
* [USB Type-Cのメス](https://ja.aliexpress.com/item/1005006044405766.html)

### 社畜パトランプは試行錯誤を経て色々バージョンが増えているので整理しました。
マイコンは初代を除き、Seeed Studio XIAOのESP32シリーズを利用しています。
* [Seeed Studio XIAO 概要](https://wiki.seeedstudio.com/ja/SeeedStudio_XIAO_Series_Introduction/)

| バージョン＆フォルダ名                | メールサービス     | GitHubのレポジトリ名                     | 補足                                                                                                                                       | 
| -----------------------------         | ------------------ | ---------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------ | 
| 1.Raspberry Pi zero2wバージョン       | Gmail              | Release_CorporateSlavePatrolLamp         | 初代です。Raspberry Pi zero2wで動作します。<br>処理能力が過剰なのと、ケーブルを抜くだけで電源を気楽に切れないのでESP32版の方がお勧めです。 | 
| 2.ESP32-S1バージョン                  | Microsoft Exchange | Release_CorporateSlavePatrolLamp         | 2代目です。SeeedStudioXIAOESP32-S3で動くバージョンです。                                                                                   | 
| 3.ESP32-C3バージョン                  | Microsoft Exchange | Release_CorporateSlavePatrolLamp         | 3代目です。SeeedStudioXIAOESP32-S3ではこれまた処理能力が過剰なので、お安いC3に置き換えたバージョンです。                                   | 
| 4.ESP32-C3_ブザー付きバージョン       | Microsoft Exchange | Release_CorporateSlavePatrolLamp         | 4代目です。パトランプだけでは深夜に起きられませんでした。。。<br>ブザーを追加しています。                                                  | 
| 5.ESP32-C6_シールド・ブザーバージョン | Microsoft Exchange | Release_CorporateSlavePatrolLamp         | 5代目です。SeeedStudio XIAOのシールドを使用した版です。マイコンやC3より安くて高性能なC6を使っています。                                    | 
| 6.ESP32-C3_デジタルホン携帯バージョン | Microsoft Exchange | Release_CorporateSlaveDigitalPhoneDP192  | 1996年製のデジタルホンの携帯電話の筐体を使ったバージョンです。OLEDディスプレイとブザーを利用しています。                                   | 
| 7.ESP32-C6_IDO携帯バージョン          | Microsoft Exchange | Release_CorporateSlave_IDOPhone_D316     | 1996年製のIDOの携帯電話の筐体を使ったバージョンです。OLEDディスプレイとブザーを利用しています。                                            | 
| 8.ESP32-S3_R2-D2バージョン            | Microsoft Exchange | Release_CorporateSlave_r2d2_xiao_esp32s3 | 現在開発中です。                                                                                                                           | 

ESP32バージョンは、メールサーバーはExchangeServerと接続する仕組みにしてありますが、ExchangeServerへのアプリの登録などいろいろめんどくさかったです。
メールサービスはGmailの方が楽に実装できます。

### Youtubeで動画を公開しています。ぜひご覧ください。
* [★社畜パトランプシリーズの再生リストはこちら★](https://www.youtube.com/playlist?list=PLWImbCHDGxLp54xjUhu779lQDscNa8osS)
* [深夜のメールに対応するため社畜パトランプを作る Raspberry Pi Zero 2w](https://youtu.be/jD-DJ_TBCCw)
* [社畜パトランプを小型化・省電力化する Seeed Studio XIAO ESP32-S3](https://youtu.be/1cY0oliM73M)
* [社畜呼び込み君 深夜にメールを受信したらあなたを楽しく叩き起こします Seeed Studio XIAO ESP32-S3](https://youtu.be/54o2braTIRY)
* [社畜パトランプをUSB 5Vのみ駆動型に改良しました Seeed Studio XIAO ESP32-C3](https://youtu.be/b0TYgWEAZyU)
* [【初心者向け】ESP32の基本とシンプルな使い方 Seeed Studio XIAO ESP32 + Groveモジュール](https://youtu.be/KH83TCG_Z40)
* [【ESP32】1996年製のデジタルホンの携帯電話を現代に復活させる](https://youtu.be/STJVEMpMfwk)
* [【ESP32】1996年製のIDOの携帯電話をESP32で復活させてみた](https://youtu.be/R0ccokd_-4c)

### 生成AIのmanusが本当に大活躍です。有料版の価値ありです。
* [生成AI manus](https://manus.im/app)
