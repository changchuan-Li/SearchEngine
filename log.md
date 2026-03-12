MySQL 默认的 `utf8mb4_general_ci` 或 `utf8mb4_unicode_ci` 排序规则，不仅仅不区分大小写，它还会把以下情况视为**完全相同的主键**：

1. **全角和半角字符**：比如半角的 `v` 和全角的 `ｖ`（在中文输入法下很容易打出全角字母）。
2. **末尾带空格的字符串**：比如 `"v"` 和 `"v "`（MySQL 在比较 VARCHAR 类型时会自动忽略末尾空格）。
3. **带某些音标的变体字符**。

由于 Jieba 分词出来的结果可能同时包含了半角的 `v` 和全角的 `ｖ`（或者带有看不见的尾随空格），C++ 的 `unordered_map` 认为它们是两个完全不同的词，于是向 MySQL 发送了两条插入指令。MySQL 一看，认为这两个词是同一个主键，于是又报了 `Duplicate entry` 错误。



解决方法：

在批量执行前，加上主键冲突时权重累加的逻辑                

`sql += " ON DUPLICATE KEY UPDATE weight = weight + VALUES(weight)";`
