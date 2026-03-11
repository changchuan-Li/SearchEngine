-- SearchEngine MySQL 数据库初始化脚本
-- 使用方式: mysql -u root -p < init.sql

CREATE DATABASE IF NOT EXISTS search_engine
    DEFAULT CHARACTER SET utf8mb4
    DEFAULT COLLATE utf8mb4_unicode_ci;

USE search_engine;

-- ============================================================
-- Page 模块: 网页库（替代 ripepage.dat + offset.dat）
-- ============================================================
CREATE TABLE IF NOT EXISTS web_pages (
    docid       INT          NOT NULL,
    title       TEXT         NOT NULL,
    url         VARCHAR(2048) NOT NULL,
    description MEDIUMTEXT   NOT NULL,
    PRIMARY KEY (docid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- Page 模块: 倒排索引（替代 invertIndex.dat）
-- ============================================================
CREATE TABLE IF NOT EXISTS inverted_index (
    word    VARCHAR(256) NOT NULL,
    docid   INT          NOT NULL,
    weight  DOUBLE       NOT NULL,
    PRIMARY KEY (word, docid),
    INDEX idx_word (word),
    INDEX idx_docid (docid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- Word 模块: 词典库（替代 dict.dat + idMap.dat）
-- ============================================================
CREATE TABLE IF NOT EXISTS dictionary (
    id          INT          NOT NULL,
    word        VARCHAR(256) NOT NULL,
    frequency   INT          NOT NULL DEFAULT 0,
    PRIMARY KEY (id),
    UNIQUE INDEX idx_word (word)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- Word 模块: 字符索引（替代 index.dat）
-- ============================================================
CREATE TABLE IF NOT EXISTS char_index (
    char_key    VARCHAR(12)  NOT NULL,
    word_id     INT          NOT NULL,
    PRIMARY KEY (char_key, word_id),
    INDEX idx_char_key (char_key),
    INDEX idx_word_id (word_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
