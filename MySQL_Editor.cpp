/****************************************************************************
**
** Copyright (C) 2016 Ken Crossen, bugs corrected, code cleaned up
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Redistributions in source code or binary form may not be sold.
**
****************************************************************************/

#include "MySQL_Editor.h"

#include <QApplication>
#include <QtGui>

MySQL_Editor_Highlighter::MySQL_Editor_Highlighter ( QTextDocument *parent ) :
                            QSyntaxHighlighter ( parent ),
                            m_markCaseSensitivity(Qt::CaseInsensitive) {
    // Default color scheme, similar to Qt Creator's default
    m_colors[MySQL_Editor::Normal]     = QColor(0, 0, 0);
    m_colors[MySQL_Editor::Comment]    = QColor(128, 128, 128);

    m_colors[MySQL_Editor::Operator]   = QColor(0, 0, 0);

    m_colors[MySQL_Editor::Number]     = QColor(64, 0, 0);
    m_colors[MySQL_Editor::String]     = QColor(0, 160, 0);

    m_colors[MySQL_Editor::Keyword]    = QColor(160, 0, 96);
    m_colors[MySQL_Editor::Function]   = QColor(96, 0, 160);
    m_colors[MySQL_Editor::Interval]   = QColor(160, 0, 160);

    m_colors[MySQL_Editor::Type]       = QColor(0, 96, 96);
    m_colors[MySQL_Editor::Identifier] = QColor(0, 32, 192);

    m_colors[MySQL_Editor::Marker]     = QColor(255, 255, 0);
}

void
MySQL_Editor_Highlighter::setHighlightMySQLEditor ( MySQL_Editor *highlight_mysql_editor ) {
    Highlight_MySQL_Editor = highlight_mysql_editor;
}

void
MySQL_Editor_Highlighter::setColor ( MySQL_Editor::ColorComponent component,
                                     const QColor &color ) {
    m_colors[component] = color;
    rehighlight();
}

void
MySQL_Editor_Highlighter::highlightBlock ( const QString &text ) {
    // parsing state
    enum {
        Start = 0,
        Number = 1,
        Identifier = 2,
        Backticked_Identifier = 3,
        String = 4,
        Comment = 5,
        To_EOL_Comment = 6
    };

    QList<int> bracketPositions;

    int blockState = previousBlockState();
    int bracketLevel = blockState >> 4;
    int state = blockState & 15;
    if (blockState < 0) {
        bracketLevel = 0;
        state = Start;
    }

    int start = 0;
    int i = 0;
    while (i <= text.length()) {
        QChar ch = (i < text.length()) ? text.at(i) : QChar();
        QChar next_ch = ((i + 1) < text.length()) ? text.at(i + 1) : QChar();

        switch (state) {

        case Start:
            start = i;
            if (ch.isSpace()) {
                ++i;
            } else if (ch.isDigit()) {
                ++i;
                state = Number;
            } else if (((ch == '+') or (ch == '-')) and next_ch.isDigit()) {
                ++i;
                ++i;
                state = Number;
            } else if (ch.isLetter() or (ch == '_') or (ch == '$')) {
                ++i;
                state = Identifier;
            } else if (((ch == '.') or (ch == '@')) and
                       // Table's .column, database's .table, or local variable
                       ((next_ch.isLetter() or (next_ch == '_') or (next_ch == '$')))) {
                ++i;
                ++i;
                state = Identifier;
            } else if (ch == '`') {
                ++i;
                state = Backticked_Identifier;
            } else if ((ch == '\'') or (ch == '\"')) {
                ++i;
                state = String;
            } else if ((ch == '/') and (next_ch == '*')) {
                ++i;
                ++i;
                state = Comment;
            } else if (ch == '#') {
                ++i;
                state = To_EOL_Comment;
            } else if ((ch == '-') and (next_ch == '-')) {
                ++i;
                ++i;
                state = To_EOL_Comment;
            } else if ((ch == '/') and (next_ch == '/')) {
                i = text.length();
                setFormat(start, text.length(), m_colors[MySQL_Editor::Comment]);
            } else {
                if (not QString("(){}[]").contains(ch))
                    setFormat(start, 1, m_colors[MySQL_Editor::Operator]);
                if ((ch == Open_Fold_Bracket) or (ch == Close_Fold_Bracket)) {
                    bracketPositions += i;
                    if (ch == Open_Fold_Bracket)
                        bracketLevel++;
                    else
                        bracketLevel--;
                }
                ++i;
                state = Start;
            }
            break;

        case Number:
            if (ch.isSpace() or
                (not (ch.isDigit() or (ch == '.') or
                      (ch == '+') or (ch == '-') or
                      (ch == 'E') or (ch == 'e')))) {
                setFormat(start, i - start, m_colors[MySQL_Editor::Number]);
                state = Start;
            } else {
                ++i;
            }
            break;

        // For example:
        // SELECT DISTINCT `count` AS select_count
        // FROM `select`
        // WHERE (select.count > 1);
        // In this example, both `select` and `count` would be keywords except ...
        // ... when backticked or (mutually) 'qualified' as in select.count.
        case Identifier:
            if (ch.isSpace() or
                (not (ch.isLetter() or ch.isDigit() or (ch == '_') or (ch == '$')))) {
                // If (ch == '.'), it's an identifier
                QString token = text.mid(start, i - start).trimmed();
                if (token.startsWith('.') or (ch == '.'))
                    setFormat(start, i - start, m_colors[MySQL_Editor::Identifier]);
                else if (Highlight_MySQL_Editor->isKeyword(token.toUpper()))
                    setFormat(start, i - start, m_colors[MySQL_Editor::Keyword]);
                else if (Highlight_MySQL_Editor->isFunction(token.toUpper()))
                    setFormat(start, i - start, m_colors[MySQL_Editor::Function]);
                else if (Highlight_MySQL_Editor->isType(token.toUpper()))
                    setFormat(start, i - start, m_colors[MySQL_Editor::Type]);
                else if (Highlight_MySQL_Editor->isInterval(token.toUpper()))
                    setFormat(start, i - start, m_colors[MySQL_Editor::Interval]);
                else
                    setFormat(start, i - start, m_colors[MySQL_Editor::Identifier]);
                state = Start;
            } else {
                ++i;
            }
            break;

        case Backticked_Identifier:
            if (ch == '`') {
                ++i; // Closing backtick is part of identifier, incorporate it
                setFormat(start, i - start, m_colors[MySQL_Editor::Identifier]);
                state = Start;
            } else {
                ++i;
            }
            break;

        case String:
            if ((ch == '\\') and
                ((next_ch == '\\') or (next_ch == '\'') or (next_ch == '\"') or
                 (next_ch == 'b') or (next_ch == 'r') or (next_ch == 'f') or
                 (next_ch == 't') or (next_ch == 'v'))) {
                // Accept all valid escapes as part of string
                ++i;
                ++i;
            }
            else if (ch == text.at(start)) {
                QChar prev_prev = (i > 1) ? text.at(i - 2) : QChar();
                QChar prev = (i > 0) ? text.at(i - 1) : QChar();
                if ((not (prev == '\\')) or ((prev_prev == '\\') and (prev == '\\'))) {
                    ++i;
                    setFormat(start, i - start, m_colors[MySQL_Editor::String]);
                    state = Start;
                }
                else {
                    // If (ch == '\\') and we are here, there's an error.
                    // For example, an invalid escape sequence.
                    ++i;
                }
            } else {
                // If (ch == '\\') and we are here, there's an error
                // For example, an invalid escape sequence.
                ++i;
            }
            break;

        case Comment:
            if ((ch == '*') and (next_ch == '/')) {
                ++i; // "*/" part of comment ...
                ++i; // ... incorporate
                setFormat(start, i - start, m_colors[MySQL_Editor::Comment]);
                state = Start;
            } else {
                ++i;
            }
            break;

        case To_EOL_Comment:
            // (ch == '\0') == true, apparently
            if ((ch == '\n') or (ch == '\0')) {
                setFormat(start, i - start, m_colors[MySQL_Editor::Comment]);
                state = Start;
            } else {
                ++i;
            }
            break;

        default:
            state = Start;
            break;
        }
    }

    if (state == Comment)
        setFormat(start, text.length(), m_colors[MySQL_Editor::Comment]);
    else
        state = Start;

    if (!m_markString.isEmpty()) {
        int pos = 0;
        int len = m_markString.length();
        QTextCharFormat markerFormat;
        markerFormat.setBackground(m_colors[MySQL_Editor::Marker]);
        markerFormat.setForeground(m_colors[MySQL_Editor::Normal]);
        for (;;) {
            pos = text.indexOf(m_markString, pos, m_markCaseSensitivity);
            if (pos < 0)
                break;
            setFormat(pos, len, markerFormat);
            ++pos;
        }
    }

    if (!bracketPositions.isEmpty()) {
        MySQLBlockData *blockData = reinterpret_cast<MySQLBlockData*>(currentBlock().userData());
        if (!blockData) {
            blockData = new MySQLBlockData;
            currentBlock().setUserData(blockData);
        }
        blockData->bracketPositions = bracketPositions;
    }

    blockState = (state & 15) | (bracketLevel << 4);
    setCurrentBlockState(blockState);
}

void
MySQL_Editor_Highlighter::mark ( const QString &str,
                          Qt::CaseSensitivity caseSensitivity ) {
    m_markString = str;
    m_markCaseSensitivity = caseSensitivity;
    rehighlight();
}

MySQL_Editor_Sidebar::MySQL_Editor_Sidebar ( MySQL_Editor *editor ) : QWidget ( editor ),
                                                                      foldIndicatorWidth ( 0 ) {
    backgroundColor = QColor(200, 200, 200);
    lineNumberColor = Qt::black;
    indicatorColor = Qt::white;
    foldIndicatorColor = Qt::lightGray;
}

void
MySQL_Editor_Sidebar::mousePressEvent ( QMouseEvent *event ) {
    if (foldIndicatorWidth > 0) {
        int xofs = width() - foldIndicatorWidth;
        int lineNo = -1;
        int fh = fontMetrics().lineSpacing();
        int ys = event->pos().y();
        if (event->pos().x() > xofs) {
            foreach (BlockInfo ln, lineNumbers)
                if (ln.position < ys && (ln.position + fh) > ys) {
                    if (ln.foldable)
                        lineNo = ln.number;
                    break;
                }
        }
        if (lineNo >= 0) {
            MySQL_Editor *editor = qobject_cast<MySQL_Editor*>(parent());
            if (editor)
                editor->toggleFold(lineNo);
        }
    }
}

void
MySQL_Editor_Sidebar::paintEvent ( QPaintEvent *event ) {
    QPainter p(this);
    p.fillRect(event->rect(), backgroundColor);
    p.setPen(lineNumberColor);
    p.setFont(font);
    int fh = QFontMetrics(font).height();
    foreach (BlockInfo ln, lineNumbers)
        p.drawText(0, ln.position, width() - 4 - foldIndicatorWidth, fh, Qt::AlignRight, QString::number(ln.number));

    if (foldIndicatorWidth > 0) {
        int xofs = width() - foldIndicatorWidth;
        p.fillRect(xofs, 0, foldIndicatorWidth, height(), indicatorColor);

        // initialize (or recreate) the arrow icons whenever necessary
        if (foldIndicatorWidth != rightArrowIcon.width()) {
            QPainter iconPainter;
            QPolygonF polygon;

            int dim = foldIndicatorWidth;
            rightArrowIcon = QPixmap(dim, dim);
            rightArrowIcon.fill(Qt::transparent);
            downArrowIcon = rightArrowIcon;

            polygon << QPointF(dim * 0.4, dim * 0.25);
            polygon << QPointF(dim * 0.4, dim * 0.75);
            polygon << QPointF(dim * 0.8, dim * 0.5);
            iconPainter.begin(&rightArrowIcon);
            iconPainter.setRenderHint(QPainter::Antialiasing);
            iconPainter.setPen(Qt::NoPen);
            iconPainter.setBrush(foldIndicatorColor);
            iconPainter.drawPolygon(polygon);
            iconPainter.end();

            polygon.clear();
            polygon << QPointF(dim * 0.25, dim * 0.4);
            polygon << QPointF(dim * 0.75, dim * 0.4);
            polygon << QPointF(dim * 0.5, dim * 0.8);
            iconPainter.begin(&downArrowIcon);
            iconPainter.setRenderHint(QPainter::Antialiasing);
            iconPainter.setPen(Qt::NoPen);
            iconPainter.setBrush(foldIndicatorColor);
            iconPainter.drawPolygon(polygon);
            iconPainter.end();
        }

        foreach (BlockInfo ln, lineNumbers)
            if (ln.foldable) {
                if (ln.folded)
                    p.drawPixmap(xofs, ln.position, rightArrowIcon);
                else
                    p.drawPixmap(xofs, ln.position, downArrowIcon);
            }
    }
}

MySQL_Editor_DocLayout::MySQL_Editor_DocLayout ( QTextDocument *doc ) : QPlainTextDocumentLayout ( doc ) {
}

void
MySQL_Editor_DocLayout::forceUpdate ( ) {
    emit documentSizeChanged(documentSize());
}

MySQL_Editor::MySQL_Editor ( QWidget *parent ) : QPlainTextEdit( parent ) {
    Editor_Layout = new MySQL_Editor_DocLayout(document());
    Editor_Highlighter = new MySQL_Editor_Highlighter(document());
    // Highlighter can tokenize SQL, but can't distinguish ...
    // ... keywords, etc. from identifiers. Only one copy of these lists ...
    // ... will be maintained (in the editor itself). The editor tokenizes ...
    // ... SQL using a regular expression.
    Editor_Highlighter->setHighlightMySQLEditor(this);
    Editor_Sidebar = new MySQL_Editor_Sidebar(this);

    // Default colors for these
    cursorColor = QColor(255, 255, 192);
    bracketMatchColor = QColor(96, 255, 96);
    bracketErrorColor= QColor(255, 96, 96);

    MySQL_Keywords.clear();
    MySQL_Keywords << "ACCESS" << "ADD" << "ALL" << "ALTER" << "ANALYZE";
    MySQL_Keywords << "AND" << "AS" << "ASC" << "AUTO_INCREMENT" << "BDB";
    MySQL_Keywords << "BERKELEYDB" << "BETWEEN" << "BOTH" << "BY" << "CASCADE";
    MySQL_Keywords << "CASE" << "CHANGE" << "CHARSET" << "COLUMN" << "COLUMNS";
    MySQL_Keywords << "CONSTRAINT" << "CREATE" << "CROSS" << "CURRENT_DATE" << "CURRENT_TIME";
    MySQL_Keywords << "CURRENT_TIMESTAMP" << "DATABASE" << "DATABASES" << "DAY_HOUR" << "DAY_MINUTE";
    MySQL_Keywords << "DAY_SECOND" << "DEC" << "DEFAULT" << "DELAYED" << "DELETE";
    MySQL_Keywords << "DESC" << "DESCRIBE" << "DISTINCT" << "DISTINCTROW" << "DROP";
    MySQL_Keywords << "ELSE" << "ENCLOSED" << "ESCAPED" << "EXISTS" << "EXPLAIN";
    MySQL_Keywords << "FIELDS" << "FOR" << "FOREIGN" << "FROM" << "FULL" << "FULLTEXT";
    MySQL_Keywords << "FUNCTION" << "GRANT" << "GROUP" << "HAVING" << "HIGH_PRIORITY";
    MySQL_Keywords << "IF" << "IGNORE" << "IN" << "INDEX" << "INFILE";
    MySQL_Keywords << "INNER" << "INNODB" << "INSERT" << "INTERVAL" << "INTO" << "IS";
    MySQL_Keywords << "JOIN" << "KEY" << "KEYS" << "KILL" << "LEADING";
    MySQL_Keywords << "LEFT" << "LIKE" << "LIMIT" << "LINES" << "LOAD";
    MySQL_Keywords << "LOCK" << "LOW_PRIORITY" << "MASTER_SERVER_ID" << "MATCH" << "MRG_MYISAM";
    MySQL_Keywords << "NATIONAL" << "NATURAL" << "NOT" << "NULL" << "NUMERIC";
    MySQL_Keywords << "ON" << "OPTIMIZE" << "OPTION" << "OPTIONALLY" << "OR";
    MySQL_Keywords << "ORDER" << "OUTER" << "OUTFILE" << "PARTIAL" << "PRECISION";
    MySQL_Keywords << "PRIMARY" << "PRIVILEGES" << "PROCEDURE" << "PURGE" << "READ";
    MySQL_Keywords << "REFERENCES" << "REGEXP" << "RENAME" << "REPLACE" << "REQUIRE";
    MySQL_Keywords << "RESTRICT" << "RETURNS" << "REVOKE" << "RIGHT" << "RLIKE";
    MySQL_Keywords << "SELECT" << "SET" << "SHOW" << "SONAME" << "SQL_BIG_RESULT";
    MySQL_Keywords << "SQL_CALC_FOUND_ROWS" << "SQL_SMALL_RESULT" << "SSL";
    MySQL_Keywords << "STARTING" << "STATUS" << "STRAIGHT_JOIN";
    MySQL_Keywords << "STRIPED" << "TABLE" << "TABLES" << "TERMINATED" << "THEN";
    MySQL_Keywords << "TO" << "TRAILING" << "TRUNCATE" << "TYPE" << "UNION";
    MySQL_Keywords << "UNIQUE" << "UNLOCK" << "UNSIGNED" << "UPDATE" << "USAGE";
    MySQL_Keywords << "USE" << "USER_RESOURCES" << "USING" << "VALUES" << "VARYING";
    MySQL_Keywords << "WHEN" << "WHERE" << "WHILE" << "WITH" << "WRITE";
    MySQL_Keywords << "XOR" << "YEAR_MONTH" << "ZEROFILL";

    QStringList functions;
    functions << "ASCII" << "BIN" << "BIT_LENGTH" << "CHAR" << "CHARACTER_LENGTH";
    functions << "CHAR_LENGTH" << "CONCAT" << "CONCAT_WS" << "CONV" << "ELT";
    functions << "EXPORT_SET" << "FIELD" << "FIND_IN_SET" << "HEX" << "INSERT";
    functions << "INSTR" << "LCASE" << "LEFT" << "LENGTH" << "LOAD_FILE";
    functions << "LOCATE" << "LOWER" << "LPAD" << "LTRIM" << "MAKE_SET";
    functions << "MID" << "OCT" << "OCTET_LENGTH" << "ORD" << "POSITION";
    functions << "QUOTE" << "REPEAT" << "REPLACE" << "REVERSE" << "RIGHT";
    functions << "RPAD" << "RTRIM" << "SOUNDEX" << "SPACE" << "SUBSTRING";
    functions << "SUBSTRING_INDEX" << "TRIM" << "UCASE" << "UPPER";

    QStringList math_functions;
    math_functions << "ABS" << "ACOS" << "ASIN" << "ATAN" << "ATAN2";
    math_functions << "CEILING" << "COS" << "COT" << "DEGREES" << "EXP";
    math_functions << "FLOOR" << "GREATEST" << "LEAST" << "LN" << "LOG";
    math_functions << "LOG10" << "LOG2" << "MOD" << "PI" << "POW";
    math_functions << "POWER" << "RADIANS" << "RAND" << "ROUND" << "SIGN";
    math_functions << "SIN" << "SQRT" << "TAN";

    QStringList date_time_functions;
    date_time_functions << "ADDDATE" << "CURDATE" << "CURRENT_DATE" << "CURRENT_TIME" << "CURRENT_TIMESTAMP";
    date_time_functions << "CURTIME" << "DATE_ADD" << "DATE_FORMAT" << "DATE_SUB" << "DAYNAME";
    date_time_functions << "DAYOFMONTH" << "DAYOFWEEK" << "DAYOFYEAR" << "EXTRACT" << "FROM_DAYS";
    date_time_functions << "FROM_UNIXTIME";
    date_time_functions << "NOW" << "PERIOD_ADD" << "PERIOD_DIFF" << "QUARTER" << "SECOND";
    date_time_functions << "SEC_TO_TIME" << "SUBDATE" << "SYSDATE" << "TIME_FORMAT" << "TIME_TO_SEC";
    date_time_functions << "TO_DAYS" << "UNIX_TIMESTAMP" << "WEEK" << "WEEKDAY" << "YEAR";
    date_time_functions << "YEARWEEK";

    QStringList cast_functions;
    cast_functions << "CAST" << "CONVERT";

    QStringList misc_functions;
    misc_functions << "AES_DECRYPT" << "AES_ENCRYPT" << "BENCHMARK" << "BIT_COUNT" << "CONNECTION_ID";
    misc_functions << "DATABASE" << "DECODE" << "DES_DECRYPT" << "DES_ENCRYPT" << "ENCODE";
    misc_functions << "ENCRYPT" << "FORMAT" << "FOUND_ROWS" << "GET_LOCK" << "IFNULL";
    misc_functions << "INET_ATON" << "INET_NTOA" << "ISNULL";
    misc_functions << "IS_FREE_LOCK" << "LAST_INSERT_ID" << "MASTER_POS_WAIT" << "MD5";
    misc_functions << "PASSWORD" << "RELEASE_LOCK" << "SESSION_USER" << "SHA" << "SHA1";
    misc_functions << "SYSTEM_USER" << "USER" << "VERSION";

    QStringList aggregate_functions;
    aggregate_functions << "AVG" << "BIT_AND" << "BIT_OR" << "BIT_XOR" << "COUNT" << "GROUP_CONCAT";
    aggregate_functions << "MAX" << "MIN" << "SEPARATOR" << "STD" << "STDDEV" << "STDDEV_POP";
    aggregate_functions << "STDDEV_SAMP" << "SUM" << "VAR_POP" << "VAR_SAMP" << "VARIANCE";

    MySQL_Functions.clear();
    MySQL_Functions << functions << math_functions << date_time_functions;
    MySQL_Functions << cast_functions << misc_functions << aggregate_functions;

    QStringList strings_types;
    strings_types << "BINARY" << "BLOB" << "CHAR" << "CHARACTER" << "ENUM";
    strings_types << "LONGBLOB" << "LONGTEXT" << "MEDIUMBLOB" << "MEDIUMTEXT" << "TEXT";
    strings_types << "TINYBLOB" << "TINYTEXT" << "VARBINARY" << "VARCHAR" << "SET";

    QStringList numeric_types;
    numeric_types << "BIGINT" << "BIT" << "BOOL" << "BOOLEAN" << "DEC";
    numeric_types << "DECIMAL" << "DOUBLE" << "FIXED" << "FLOAT" << "INT";
    numeric_types << "INTEGER" << "LONG" << "MEDIUMINT" << "MIDDLEINT" << "NUMERIC";
    numeric_types << "TINYINT" << "REAL" << "SERIAL" << "SMALLINT";

    QStringList date_time_types;
    date_time_types << "DATE" << "DATETIME" << "TIME" << "TIMESTAMP" << "YEAR";

    MySQL_Types.clear();
    MySQL_Types << strings_types << numeric_types << date_time_types;

    QStringList date_time_intervals;
    date_time_intervals << "MICROSECOND" << "MINUTE" << "HOUR" << "DAY" << "MONTH";
    date_time_intervals << "SECOND_MICROSECOND" << "MINUTE_MICROSECOND" << "MINUTE_SECOND";
    date_time_intervals << "HOUR_MICROSECOND" << "HOUR_SECOND" << "HOUR_MINUTE";
    date_time_intervals << "DAY_MICROSECOND" << "DAY_SECOND" << "DAY_MINUTE" << "DAY_HOUR";
    date_time_intervals << "YEAR_MONTH";

    MySQL_Intervals.clear();
    MySQL_Intervals << date_time_intervals;

    All_MySQL_Keywords.clear();
    All_MySQL_Keywords << MySQL_Keywords << MySQL_Functions << MySQL_Types << MySQL_Intervals;

    CodeFoldingEnabled = true;
    ShowLineNumbersEnabled = true;

    TextWrapEnabled = true;

    AutoIndentEnabled = true;
    Tab_Modulus = Default_Tab_Modulus;

    AutoCompleteKeywordsEnabled = false;
    AutoCompleteIdentifiersEnabled = false;
    In_Completion_Context = false;

    AutoUppercaseKeywordsEnabled = true;

    Quote_Bracket_Character = true;
    Post_Select_Bracket_Enclosed_Text = true;

    BracketsMatchingEnabled = true;

    // Example of a different color scheme
    // this->setColor(MySQL_Editor::Background,    QColor(255, 255, 255));
    // this->setColor(MySQL_Editor::Normal,        QColor(0, 0, 0));
    // this->setColor(MySQL_Editor::Comment,       QColor(128, 128, 128));
    // this->setColor(MySQL_Editor::Number,        QColor(192, 0, 0));
    // this->setColor(MySQL_Editor::String,        QColor(0, 128, 0));
    // this->setColor(MySQL_Editor::Operator,      QColor(0, 0, 0));
    // this->setColor(MySQL_Editor::Identifier,    QColor(128, 0, 128));
    // this->setColor(MySQL_Editor::Keyword,       QColor(0, 160, 160));
    // this->setColor(MySQL_Editor::Function,      QColor(0, 128, 192));
    // this->setColor(MySQL_Editor::Type,          QColor(0, 128, 192));
    // this->setColor(MySQL_Editor::Interval,      QColor(0, 128, 192));
    // this->setColor(MySQL_Editor::Cursor,        QColor(255, 255, 192));
    // this->setColor(MySQL_Editor::Marker,        QColor(255, 255, 0));
    // this->setColor(MySQL_Editor::BracketMatch,  QColor(128, 255, 128));
    // this->setColor(MySQL_Editor::BracketError,  QColor(255, 128, 128));
    // this->setColor(MySQL_Editor::FoldIndicator, Qt::lightGray);

    document()->setDocumentLayout(Editor_Layout);

    connect(this, SIGNAL(textChanged()), this, SLOT(onTextChanged()));

    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateCursor()));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorPositionChanged()));

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateSidebar()));
    connect(this, SIGNAL(updateRequest(QRect, int)), this, SLOT(updateSidebar(QRect, int)));

    QString doublequoted_string = "\"(?:[^\\\\\"]|\\\\.)*\"";
    QString singlequoted_string = "'(?:[^\\\\']|\\\\.)*'";
    QString c_style_comment = "/\\*(?:[^*]*|\\*[^/])*\\*/";
    QString hash_comment = "#[^\\n]*\\n";
    QString doubledash_comment = "--\\s+[^\\n]*\\n";
    QString bracket_characters = QString("[") + QString(MySQL_Bracket_List) + QString("]");

    MySQL_Bracket_RegEx =
      QRegularExpression(doublequoted_string + "|" + singlequoted_string + "|" +
                         c_style_comment + "|" + hash_comment + "|" + doubledash_comment + "|" +
                         bracket_characters);

    // QString identifier = "(?:`[^`]+`)|[A-Za-z0-9_$]+";
    QString backticked_identifier = "`[^`]+`";

    QString significant_punctuation = QString("[") + ",;" + QString("]");

    // QString identifier_characters = "A-Za-z0-9_$";
    // Note inclusion of '$', will not be present in keyword list
    QString keyword_characters = "A-Za-z0-9_";
    QString optional_keyword_delimiters = "[^" + keyword_characters + "]?";
    // QString potential_keyword = ".?([A-Za-z_0-9]+).?";
    QString potential_keyword =
              optional_keyword_delimiters + "([" + keyword_characters + "]+)" + optional_keyword_delimiters;

    // Some delimiters can potentially turn a keyword into a normal db object name, ...
    // as for example in "SELECT * FROM database.table AS `table`".
    QString sql_token_pattern =
              doublequoted_string + "|" + singlequoted_string + "|" +
              c_style_comment + "|" + hash_comment + "|" + doubledash_comment + "|" +
              bracket_characters + "|" + significant_punctuation + "|" +
              backticked_identifier + "|" + potential_keyword;

    SQL_Token_Regular_Expression.setPattern(sql_token_pattern);

    Newline_Word_List << "SELECT" << "UPDATE" << "SET" << "DELETE" << "INSERT" << "VALUES";
    Newline_Word_List << "FROM" << "LEFT" << "RIGHT" << "INNER" << "OUTER" << "JOIN" << "ON";
    Newline_Word_List << "UNION" << "WHERE" << "ORDER" << "GROUP" << "HAVING" << "LIMIT";
    JOIN_Modifiers << "LEFT" << "RIGHT" << "INNER" << "OUTER";

    Previous_Cursor_Line = -1;
    Uppercasing_In_Process = false;

    Completer = new QCompleter(this);
    Completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    Completer->setCaseSensitivity(Qt::CaseInsensitive);
    Completer->setWrapAround(false);
    this->setCompleter(Completer);

#if defined(Q_OS_MAC)
    QFont textFont = font();
    textFont.setPointSize(15);
    textFont.setFamily("Courier");
    setFont(textFont);
#elif defined(Q_OS_UNIX)
    QFont textFont = font();
    textFont.setFamily("Monospace");
    setFont(textFont);
#endif
}

MySQL_Editor::~MySQL_Editor ( ) {
    delete Editor_Layout;
}

QString
MySQL_Editor::Replace_Paragraph_Separator ( QString Initial_Text ) {
    // 0x2029 paragraph separator
    return Initial_Text.replace(QChar(0x2029), QChar('\n'));
}

void
MySQL_Editor::Set_PlainText ( QString Set_Plain_Text ) {
    // Don't defeat Undo/Redo
    QPlainTextEdit::selectAll();
    QPlainTextEdit::insertPlainText(Set_Plain_Text);
    QTextCursor txt_cursor = QPlainTextEdit::textCursor();
    txt_cursor.movePosition(QTextCursor::Start);
    QPlainTextEdit::setTextCursor(txt_cursor);
}

QString
MySQL_Editor::Selected_Text ( ) {
    QString sel_text = QPlainTextEdit::textCursor().selectedText();
    return Replace_Paragraph_Separator(sel_text);
}

QStringList
MySQL_Editor::mysqlKeywords ( ) const {
    return MySQL_Keywords;
}

QStringList
MySQL_Editor::mysqlFunctions ( ) const {
    return MySQL_Functions;
}

QStringList
MySQL_Editor::mysqlTypes ( ) const {
    return MySQL_Types;
}

QStringList
MySQL_Editor::mysqlIntervals ( ) const {
    return MySQL_Intervals;
}

bool
MySQL_Editor::isKeyword ( QString potential_keyword ) {
    return MySQL_Keywords.contains(potential_keyword);
}

bool
MySQL_Editor::isFunction ( QString potential_function ) {
    return MySQL_Functions.contains(potential_function);
}


bool
MySQL_Editor::isType ( QString potential_type ) {
    return MySQL_Types.contains(potential_type);
}


bool
MySQL_Editor::isInterval ( QString potential_interval ) {
    return MySQL_Intervals.contains(potential_interval);
}


QString
MySQL_Editor::Compute_Bracket_Text ( QString Source_Text ) {
    QString bracket_list = MySQL_Bracket_List;
    QString bracket_text = QString(" ").repeated(Source_Text.length());
    if (MySQL_Bracket_RegEx.isValid()) {
        QRegularExpressionMatchIterator regex_iterator =
                                          MySQL_Bracket_RegEx.globalMatch(Source_Text);
        if (regex_iterator.hasNext()) {
            // At least one found
            while (regex_iterator.hasNext()) {
                QRegularExpressionMatch match = regex_iterator.next();
                if ((match.capturedLength() == 1) and
                    bracket_list.contains(match.captured())) {
                    bracket_text[match.capturedStart()] = match.captured().at(0);
                }
            }
        }
    }
    return bracket_text;
}

int
MySQL_Editor::Bracket_Match_Position ( int Current_Position ) {
    QString target_text = this->toPlainText();

    if (not (target_text.length() == Bracket_Text.length())) onTextChanged();
    if (not (target_text == Bracket_Source_Text)) onTextChanged();

    QString bracket_list = MySQL_Bracket_List;

    if ((Current_Position < 0) or (Current_Position >= target_text.length()) or
        (Current_Position >= Bracket_Text.length())) return -1;
    if (not bracket_list.contains(target_text.at(Current_Position))) return -1;

    // For example, may have found bracket_list character in comment or string.
    if (not (Bracket_Text.at(Current_Position) == target_text.at(Current_Position))) return -1;

    int match_position = Current_Position;
    int paren_level = 0;
    if (Bracket_Text.at(match_position) == '(') {
        while (match_position < Bracket_Text.length()) {
            if (Bracket_Text.at(match_position) == '(') paren_level += 1;
            else if (Bracket_Text.at(match_position) == ')') paren_level -= 1;
            if (paren_level == 0) break;
            match_position += 1;
        }
    }
    else if (Bracket_Text.at(match_position) == ')') {
        while (match_position >= 0) {
            if (Bracket_Text.at(match_position) == ')') paren_level += 1;
            else if (Bracket_Text.at(match_position) == '(') paren_level -= 1;
            if (paren_level == 0) break;
            match_position -= 1;
        }
    }
    else if (Bracket_Text.at(match_position) == '[') {
        while (match_position < Bracket_Text.length()) {
            if (Bracket_Text.at(match_position) == '[') paren_level += 1;
            else if (Bracket_Text.at(match_position) == ']') paren_level -= 1;
            if (paren_level == 0) break;
            match_position += 1;
        }
    }
    else if (Bracket_Text.at(match_position) == ']') {
        while (match_position >= 0) {
            if (Bracket_Text.at(match_position) == ']') paren_level += 1;
            else if (Bracket_Text.at(match_position) == '[') paren_level -= 1;
            if (paren_level == 0) break;
            match_position -= 1;
        }
    }

    if ((match_position >= 0) and (match_position < Bracket_Text.length())) {
        return match_position;
    }

    return -1; // No match found
}

int
MySQL_Editor::Compute_Current_Paren_Indent ( QString Current_Text,
                                             int Current_Position ) {
    // Returns paren_indent in units of spaces
    QString bracket_text = Compute_Bracket_Text(Current_Text);
    // For example:
    // WHERE ((abc LIKE "%def%") OR <Return Here>
    // ... and then:
    // WHERE ((abc LIKE "%def%") OR
    //        <cursor>
    int paren_indent = 0;
    // Search back for closest unmatched "(", how is it "indented"?
    int paren_level = 0;
    int paren_position = Current_Position;
    while (paren_position >= 0) {
        if (bracket_text.at(paren_position) == QChar(')')) paren_level -= 1;
        else if (bracket_text.at(paren_position) == QChar('(')) {
            paren_level += 1;
            if (paren_level == 1) {
                // Found closest unmatched "(", now find immediately preceding newline
                int newline_position = paren_position;
                while ((newline_position >= 0) and
                       (not (Current_Text.at(newline_position) == QChar('\n')))) newline_position -= 1;
                // Found immediately preceding newline
                if (((newline_position >= 0) and
                     (Current_Text.at(newline_position) == QChar('\n'))) or
                    // Start of text is also 'newline'
                    (newline_position == -1)) {
                    paren_indent = paren_position - newline_position;
                    break;
                }
            }
        }
        paren_position -= 1;
    }

    return paren_indent;
}

void
MySQL_Editor::keyPressEvent ( QKeyEvent* event ) {
    static QString part_of_word("abcdefghijklmnopqrstuvwxyz_0123456789");
    static QString end_of_word("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-=");

    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

    if ((AutoCompleteKeywordsEnabled or AutoCompleteIdentifiersEnabled) and
        Completer and Completer->popup()->isVisible()) {
        // The following keys are forwarded by QCompleter to the widget
        switch (event->key()) {
           case Qt::Key_Escape:
               // Return to normal completion (exit context)
               if (In_Completion_Context) initializeAutoComplete();
               In_Completion_Context = false;
               event->ignore();
               return; // Let QCompleter do default behavior

           case Qt::Key_Enter:
           case Qt::Key_Return:
           case Qt::Key_Tab:
           case Qt::Key_Backtab:
                event->ignore();
                return; // Let QCompleter do default behavior

           default:
               break;
        }
    }

    if ((AutoCompleteKeywordsEnabled or AutoCompleteIdentifiersEnabled) and Completer) {
        if (Completer->popup()->isVisible() and
            (event->text().isEmpty() or
             end_of_word.contains(event->text().right(1).toLower()) or
             (not part_of_word.contains(event->text().right(1).toLower())))) {
            Completer->popup()->hide();
            // A 'completion context' is entered when a context indetifier is followed by '.', ...
            // for example, 'table_name.' in which case all of the column names in table_name ...
            // ... are displayed in the completer popup.
            // Now return to normal completion (exit context)
            if (In_Completion_Context) initializeAutoComplete();
            In_Completion_Context = false;
        }
        else if (((modifiers & Qt::ControlModifier) == Qt::NoModifier) and
                 ((modifiers & Qt::AltModifier) == Qt::NoModifier) and
                 (not event->text().isEmpty()) and
                 part_of_word.contains(event->text().right(1).toLower())) {
            QString completionPrefix = textUnderCursor() + event->text();

            if (((not In_Completion_Context) and (completionPrefix.length() < 3)) or
                (In_Completion_Context and (completionPrefix.length() < 1))) {
                if (Completer->popup()->isVisible()) Completer->popup()->hide();
            }
            else {
                if (not (completionPrefix == Completer->completionPrefix())) {
                    Completer->setCompletionPrefix(completionPrefix);
                    Completer->popup()->setCurrentIndex(Completer->completionModel()->index(0, 0));
                }
                QRect cur_rect = cursorRect();
                cur_rect.setWidth(Completer->popup()->sizeHintForColumn(0) +
                                  Completer->popup()->verticalScrollBar()->sizeHint().width());
                Completer->complete(cur_rect); // popup it up!
            }
        }
    }

    if (((event->key() == Qt::Key_ParenLeft) or
         (event->key() == Qt::Key_BracketLeft) or
         ((event->key() == Qt::Key_QuoteDbl) and Quote_Bracket_Character) or
         ((event->key() == Qt::Key_Apostrophe) and Quote_Bracket_Character)) and
        ((modifiers & Qt::ControlModifier) == Qt::NoModifier) and
        (this->textCursor().selectedText().count() > 0)) {
        event->accept();
        // For these "bracketing" characters, if the "opening" character is typed ...
        // ... when text is selected, the selected text will be enclosed by ...
        // ... "open" and "close" characters.
        QString left_encloser;
        QString right_encloser;
        if (event->key() == Qt::Key_ParenLeft) {
            left_encloser = "(";
            right_encloser = ")";
        }
        else if (event->key() == Qt::Key_BracketLeft) {
            left_encloser = "[";
            right_encloser = "]";
        }
        else if (event->key() == Qt::Key_BraceLeft) {
            left_encloser = "{";
            right_encloser = "}";
        }
        else if (event->key() == Qt::Key_QuoteDbl) {
            left_encloser = "\"";
            right_encloser = "\"";
        }
        else if (event->key() == Qt::Key_Apostrophe) {
            left_encloser = "'";
            right_encloser = "'";
        }

        QTextCursor txt_cursor = this->textCursor();
        int sel_begin_pos = txt_cursor.selectionStart();
        int sel_end_pos = txt_cursor.selectionEnd();
        txt_cursor.insertText(left_encloser + txt_cursor.selectedText() + right_encloser);

        if (Post_Select_Bracket_Enclosed_Text) {
            txt_cursor.setPosition((sel_begin_pos + 1));
            txt_cursor.setPosition((sel_end_pos + 1), QTextCursor::KeepAnchor);
            this->setTextCursor(txt_cursor);
        }

        return;
    }
    else if ((event->key() == Qt::Key_Return) and AutoIndentEnabled) {
        QTextCursor txt_cursor = QPlainTextEdit::textCursor();
        QString before_cursor_text = QPlainTextEdit::toPlainText();
        before_cursor_text.truncate(txt_cursor.position());

        // For example:
        // SELECT abc
        // FROM
        // (SELECT def AS abc <Return Here>
        // ... and then:
        // SELECT abc
        // FROM
        // (SELECT def AS abc
        //  FROM
        int paren_indent = Compute_Current_Paren_Indent(before_cursor_text,
                                                        (before_cursor_text.length() - 1));

        // For example:
        // SELECT abc
        //        def <Return Here>
        // ... and then:
        // SELECT abc
        //        def
        //        ghi
        QTextCursor current_line_txt_cursor = QPlainTextEdit::textCursor();
        current_line_txt_cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        QString current_line_text_before_cursor = current_line_txt_cursor.selectedText();

        int word_boundary_idx = current_line_text_before_cursor.indexOf(QRegExp("[^\\s]"));
        word_boundary_idx = qMax(word_boundary_idx, paren_indent);

        QString whitespace = QString(" ").repeated(word_boundary_idx);
        QPlainTextEdit::insertPlainText("\n" + whitespace);
    }
    else if ((event->key() == Qt::Key_Tab) or (event->key() == Qt::Key_Escape)) {
        QTextCursor txt_cursor = QPlainTextEdit::textCursor();
        if (txt_cursor.selectedText().length() == 0) {
            int cursor_position = txt_cursor.position();
            txt_cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            QString line_text_before_cursor = txt_cursor.selectedText();
            if (line_text_before_cursor.indexOf(QRegExp("[^\\s]")) < 0) {
                // Only whitespace before cursor
                int cursor_position_on_line = line_text_before_cursor.length();
                QTextCursor previous_line_txt_cursor = QPlainTextEdit::textCursor();
                previous_line_txt_cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
                QString previous_line_text_before_cursor = previous_line_txt_cursor.selectedText();
                // Find word boundary on previous line in direction requested
                int word_boundary_idx = cursor_position_on_line;
                if ((event->key() == Qt::Key_Tab) and
                    ((modifiers & Qt::AltModifier) == Qt::NoModifier)) {
                    // For example:
                    // SELECT abc
                    // <Tab Here>
                    // ... and then:
                    // SELECT abc
                    //        def
                    word_boundary_idx =
                      previous_line_text_before_cursor.indexOf(QRegularExpression("(?<=\\s)[^\\s]"),
                                                               (cursor_position_on_line + 1));
                }
                else if ((event->key() == Qt::Key_Escape) or
                         ((event->key() == Qt::Key_Tab) and
                          ((modifiers & Qt::AltModifier) == Qt::AltModifier))) {
                    // For example:
                    // (SELECT abc
                    //         def
                    //         <BackTab/Esc Here>
                    //  ... and then:
                    // (SELECT abc
                    //         def
                    //  FROM
                    word_boundary_idx =
                      previous_line_text_before_cursor.lastIndexOf(QRegularExpression("(?<=\\s)[^\\s]"),
                                                                   (cursor_position_on_line - 1));
                    if (word_boundary_idx < 0) {
                        QString before_cursor_text = QPlainTextEdit::toPlainText();
                        before_cursor_text.truncate(cursor_position);
                        word_boundary_idx = Compute_Current_Paren_Indent(before_cursor_text,
                                                                         (before_cursor_text.length() - 1));
                    }
                }
                if (not (word_boundary_idx == cursor_position_on_line)) {
                    txt_cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                    QPlainTextEdit::setTextCursor(txt_cursor);
                    // Add enough whitespace to bring cursor to first non-whitespace character on previous line
                    QPlainTextEdit::insertPlainText(QString(" ").repeated(word_boundary_idx));
                }
            }
            else {
                // Non-whitespace before cursor, just add tab modulus spaces
                QPlainTextEdit::insertPlainText(QString(" ").repeated(Tab_Modulus));
            }
        }
        else if ((modifiers & Qt::AltModifier) == Qt::NoModifier) {
            Indent_Selected_Text_Lines(Tab_Modulus);
        }
    }
    else if ((event->key() == Qt::Key_BracketLeft) and
             ((modifiers & Qt::ControlModifier) == Qt::ControlModifier) and
             ((modifiers & Qt::ShiftModifier) == Qt::NoModifier)) {
        Indent_Selected_Text_Lines(-2);
    }
    else if ((event->key() == Qt::Key_BracketRight) and
             ((modifiers & Qt::ControlModifier) == Qt::ControlModifier) and
             ((modifiers & Qt::ShiftModifier) == Qt::NoModifier)) {
        Indent_Selected_Text_Lines(2);
    }
    else if ((event->key() == Qt::Key_BracketLeft) and
             ((modifiers & Qt::ControlModifier) == Qt::ControlModifier) and
             ((modifiers & Qt::ShiftModifier) == Qt::ShiftModifier)) {
        Indent_Selected_Text_Lines(-1);
    }
    else if ((event->key() == Qt::Key_BracketRight) and
             ((modifiers & Qt::ControlModifier) == Qt::ControlModifier) and
             ((modifiers & Qt::ShiftModifier) == Qt::ShiftModifier)) {
        Indent_Selected_Text_Lines(1);
    }
    else if ((event->key() == Qt::Key_Equal) and
             ((modifiers & Qt::ControlModifier) == Qt::ControlModifier) and
             ((modifiers & Qt::ShiftModifier) == Qt::NoModifier)) {
        Simple_Format_SQL();
    }
    else {
        if ((event->key() == Qt::Key_Period) and
            AutoCompleteIdentifiersEnabled and
            Completer) {
            QTextCursor txt_cursor = QPlainTextEdit::textCursor();
            txt_cursor.movePosition(QTextCursor::PreviousWord);
            txt_cursor.select(QTextCursor::WordUnderCursor);
            QString context_identifier = txt_cursor.selectedText();
            if (Auto_Complete_Context_Identifier_List.contains(context_identifier)) {
                QStringList word_list;
                word_list << Auto_Complete_Context_Identifier_List[context_identifier];
                // In a context, leave word-List in database order
                // word_list.sort(Qt::CaseInsensitive);
                Completer->setModel(new QStringListModel(word_list));
                // A 'completion context' is entered when a context indetifier is followed by '.', ...
                // for example, 'table_name.' in which case all of the column names in table_name ...
                // ... are displayed in the completer popup.
                In_Completion_Context = true;

                Completer->setCompletionPrefix("");
                Completer->popup()->setCurrentIndex(Completer->completionModel()->index(0, 0));

                QRect cur_rect = cursorRect();
                cur_rect.setWidth(Completer->popup()->sizeHintForColumn(0) +
                                  Completer->popup()->verticalScrollBar()->sizeHint().width());
                Completer->complete(cur_rect); // popup it up!

            }
        }
        // Allow parent class (normal) handling of key
        QPlainTextEdit::keyPressEvent(event);
    }
}

QTextCursor
MySQL_Editor::Select_Selected_Text_Lines ( ) {
    QTextCursor begin_txt_cursor = QPlainTextEdit::textCursor();
    begin_txt_cursor.setPosition(begin_txt_cursor.selectionStart());
    begin_txt_cursor.select(QTextCursor::LineUnderCursor);

    QTextCursor end_txt_cursor = QPlainTextEdit::textCursor();
    end_txt_cursor.setPosition(end_txt_cursor.selectionEnd());
    end_txt_cursor.select(QTextCursor::LineUnderCursor);

    QTextCursor new_txt_cursor = QPlainTextEdit::textCursor();
    new_txt_cursor.setPosition(qMin(qMin(begin_txt_cursor.selectionStart(),
                                         begin_txt_cursor.selectionEnd()),
                                    qMin(end_txt_cursor.selectionStart(),
                                         end_txt_cursor.selectionEnd())));
    new_txt_cursor.setPosition(qMax(qMax(begin_txt_cursor.selectionStart(),
                                         begin_txt_cursor.selectionEnd()),
                                    qMax(end_txt_cursor.selectionStart(),
                                         end_txt_cursor.selectionEnd())),
                               QTextCursor::KeepAnchor);
    QPlainTextEdit::setTextCursor(new_txt_cursor);

    return new_txt_cursor;
}

void
MySQL_Editor::Indent_Selected_Text_Lines ( int Indent_Tab_Modulus ) {
    if (Indent_Tab_Modulus == 0) return;

    QTextCursor new_txt_cursor = Select_Selected_Text_Lines();
    int start_position = new_txt_cursor.selectionStart();
    QString selected_text = new_txt_cursor.selectedText();
    selected_text = Replace_Paragraph_Separator(selected_text);
    QStringList selected_text_lines = selected_text.split(QChar('\n'));
    QString indented_text = "";
    if (Indent_Tab_Modulus > 0) {
        for (int line_idx = 0; line_idx < selected_text_lines.count(); line_idx += 1) {
            indented_text += QString(" ").repeated(Indent_Tab_Modulus) + selected_text_lines.at(line_idx);
            if (line_idx < (selected_text_lines.count() - 1)) indented_text += "\n";
        }
    }
    else if (Indent_Tab_Modulus < 0) {
        for (int line_idx = 0; line_idx < selected_text_lines.count(); line_idx += 1) {
            QString line_text = selected_text_lines.at(line_idx);
            if (line_text.startsWith(QString(" ").repeated(-Indent_Tab_Modulus))) {
                line_text = line_text.mid(-Indent_Tab_Modulus);
            }
            indented_text += line_text;
            if (line_idx < (selected_text_lines.count() - 1)) indented_text += "\n";
        }
    }
    QPlainTextEdit::insertPlainText(indented_text);
    new_txt_cursor.setPosition(start_position, QTextCursor::KeepAnchor);
    QPlainTextEdit::setTextCursor(new_txt_cursor);
}

void
MySQL_Editor::insertFromMimeData ( const QMimeData* source ) {
    if (source->hasText()) {
        QString paste_text = source->text();
        // Convert tab characters into tab modulus spaces
        paste_text = paste_text.replace("\t", QString(" ").repeated(Tab_Modulus));
        QStringList paste_lines = paste_text.split("\n");
        if (paste_lines.count() == 1) {
            QPlainTextEdit::insertPlainText(paste_text);
        }
        else {
            // Indent pasted text intelligently ...
            // int initial_position = QPlainTextEdit::textCursor().position();

            // QPlainTextEdit::insertPlainText(paste_text);

            // QTextCursor txt_cursor = QPlainTextEdit::textCursor();
            // txt_cursor.setPosition(initial_position, QTextCursor::KeepAnchor);
            // QPlainTextEdit::setTextCursor(txt_cursor);
            // // ... using general formatting code
            // // Format_Selected_Text_Lines();

            // Since we don't format, just paste and be done
            QPlainTextEdit::insertPlainText(paste_text);
        }
    }
}

void
MySQL_Editor::onTextChanged ( ) {
    Bracket_Source_Text = this->toPlainText();
    Bracket_Text = Compute_Bracket_Text(Bracket_Source_Text);
}

void
MySQL_Editor::onCursorPositionChanged ( ) {
    QTextCursor text_cursor = this->textCursor();
    int cursor_line = text_cursor.blockNumber();

    if (AutoUppercaseKeywordsEnabled) {
        if (not (cursor_line == Previous_Cursor_Line)) {
            // int previous_line_ch_idx = SQL_Editor->positionFromLineIndex(Previous_Cursor_Line, 0);
            // int previous_line_length = SQL_Editor->lineLength(Previous_Cursor_Line);

            // Uppercase_SQL_Keywords(previous_line_ch_idx, previous_line_length);
            Uppercase_SQL_Keywords(0, 0);
        }
    }

    Previous_Cursor_Line = cursor_line;
}

void
MySQL_Editor::Uppercase_SQL_Keywords ( int start_uppercase_position,
                                       int uppercase_length ) {
    // start_uppercase_position and uppercase_length < 0, only selection
    // start_uppercase_position and uppercase_length == 0, entire sql text
    if (Uppercasing_In_Process) return;

    Uppercasing_In_Process = true;

    // Save cursor position so that it can be restored, ...
    // ... this operation MUST NOT move the cursor.
    QTextCursor text_cursor = this->textCursor();

    QString sql_text;
    int sql_text_position;
    int sql_text_length;
    bool inside_c_style_comment = false;

    int first_modified_position = INT_MAX;
    int last_modified_position = -1;

    if ((start_uppercase_position < 0) or (uppercase_length < 0)) {
        // Operate on selection only
        sql_text = this->Selected_Text();
        sql_text_position = 0;
        sql_text_length = sql_text.length();
    }
    else {
        sql_text = this->toPlainText();
        if ((start_uppercase_position == 0) and (uppercase_length == 0)) {
            // Operate on entire content
            sql_text_position = 0;
            sql_text_length = sql_text.length();
        }
        else {
            // Operate on bounded portion only
            sql_text_position = start_uppercase_position;
            sql_text_length = uppercase_length;
        }

        QRegExp c_style_comment_delimiter("/\\*|\\*/");
        int delimiter_idx = sql_text.lastIndexOf(c_style_comment_delimiter, sql_text_position);
        if (delimiter_idx >= 0) {
            QString delimiter = c_style_comment_delimiter.cap(0);
            if (delimiter == "/*") {
                inside_c_style_comment = true;
            }
        }
    }

    if (not inside_c_style_comment) {
        int sql_text_idx = sql_text_position;
        bool modified = false;

        while  ((sql_text_idx >= 0) and
                (sql_text_idx <= (sql_text_position + sql_text_length))) {
            QRegularExpressionMatch uppercase_keyword_match =
                                      SQL_Token_Regular_Expression.match(sql_text, sql_text_idx);
            int word_idx = uppercase_keyword_match.capturedStart();
            if (word_idx >= 0) {
                int new_sql_text_idx = uppercase_keyword_match.capturedEnd();
                QString word = uppercase_keyword_match.captured(1);
                if (word.length() > 0) {
                    QString entire_word_capture = uppercase_keyword_match.captured(0);
                    QString delimiters = "`.";
                    // These delimiters can turn a keyword into a normal db object name, ...
                    // as for example in "SELECT * FROM database.table AS `table`".
                    bool leading_delimiter = delimiters.contains(entire_word_capture.left(1));
                    bool trailing_delimiter = delimiters.contains(entire_word_capture.right(1));
                    if (All_MySQL_Keywords.contains(word.toUpper()) and
                        (not (word == word.toUpper())) and
                        (not leading_delimiter) and
                        (not trailing_delimiter)) {
                        int word_index = uppercase_keyword_match.capturedStart(1);
                        sql_text.replace(word_index, word.length(), word.toUpper());
                        if (word_index < first_modified_position) first_modified_position = word_index;
                        if ((word_index + word.length()) > last_modified_position)
                            last_modified_position = word_index + word.length();
                        modified = true;
                    }

                    if ((entire_word_capture.left(1) == "`") and
                        (entire_word_capture.right(1) == "`"))
                        // "FROM `database.table`" is not valid, treat "`name`" as atomic, ...
                        // ... i.e. any "`" pertains to exactly one identifier and must enclose it.
                        sql_text_idx = new_sql_text_idx;
                    else if (entire_word_capture.right(1) == ".")
                        // But if trailing delimiter is ".", ...
                        // ... it better be next word's leading delimiter, ...
                        // ... as in "FROM database.table", for example.
                        sql_text_idx = new_sql_text_idx - 1;
                    else
                        sql_text_idx = new_sql_text_idx;
                }
                else sql_text_idx = new_sql_text_idx;
            }
            else sql_text_idx = -1;
        }

        if (modified) {
            if ((start_uppercase_position < 0) or (uppercase_length < 0))
                this->insertPlainText(sql_text);
            else {
                // Replace modified portion only, why bother with the rest?
                QTextCursor modified_cursor = this->textCursor();
                modified_cursor.setPosition(first_modified_position);
                modified_cursor.setPosition(last_modified_position, QTextCursor::KeepAnchor);
                this->setTextCursor(modified_cursor);
                this->insertPlainText(sql_text.mid(first_modified_position,
                                                   (last_modified_position - first_modified_position)));
            }
            // Cursor position should not have changed
            this->setTextCursor(text_cursor);
        }
    }

    Uppercasing_In_Process = false;
}

QString
MySQL_Editor::Initial_SQL_Keyword ( QString SQL_Statement ) {
    // The initial keyword defines the statement type ...
    // ... (for example, data fetch only vs data modify) and ...
    // ... may only be preceded by a comment.
    int sql_statement_idx = 0;
    int sql_statement_length = SQL_Statement.count();

    while  ((sql_statement_idx >= 0) and
            (sql_statement_idx <= sql_statement_length)) {
        QRegularExpressionMatch sql_token_match =
                                  SQL_Token_Regular_Expression.match(SQL_Statement, sql_statement_idx);
        int word_idx = sql_token_match.capturedStart();
        if (word_idx >= 0) {
            int new_sql_statement_idx = sql_token_match.capturedEnd();
            QString word = sql_token_match.captured(1);
            if (word.length() > 0) {
                QString entire_word_capture = sql_token_match.captured(0);
                QString delimiters = "`.";
                // These delimiters can turn a keyword into a normal db object name, ...
                // as for example in "SELECT * FROM database.table AS `table`".
                bool leading_delimiter = delimiters.contains(entire_word_capture.left(1));
                bool trailing_delimiter = delimiters.contains(entire_word_capture.right(1));
                if (All_MySQL_Keywords.contains(word.toUpper()) and
                    (not leading_delimiter) and
                    (not trailing_delimiter)) return word.toUpper(); // Return first keyword

                if ((entire_word_capture.left(1) == "`") and
                    (entire_word_capture.right(1) == "`"))
                    // "FROM `database.table`" is not valid, treat "`name`" as atomic, ...
                    // ... i.e. any "`" pertains to exactly one identifier and must enclose it.
                    sql_statement_idx = new_sql_statement_idx;
                else if (entire_word_capture.right(1) == ".")
                    // But if trailing delimiter is ".", ...
                    // ... it better be next word's leading delimiter, ...
                    // ... as in "FROM database.table", for example.
                    sql_statement_idx = new_sql_statement_idx - 1;
                else
                    sql_statement_idx = new_sql_statement_idx;
            }
            else sql_statement_idx = new_sql_statement_idx;
        }
        else sql_statement_idx = -1;
    }

    return ""; // No keyword found
}

void
MySQL_Editor::Simple_Format_SQL ( ) {
    // Not intended to significantly beautify the SQL, ...
    // ... but merely to make single-line SQLs more readable.
    // Significant beautification requires a full parse of the SQL into tokens.
    // For example, detecting a sub-query would be difficult w/o a full parse.
    // Similarly, it would not be safe to insert sets of parens ...
    // ... because, unless the SQL were fully parsed, it would be ...
    // ... difficult to determine what condition grouping would be implied ...
    // ... by operator precedence.
    QString sql_text = this->toPlainText();
    int sql_text_length = sql_text.length();

    // QString doublequoted_string = "\"(?:[^\\\\\"]|\\\\.)*\"";
    // QString singlequoted_string = "'(?:[^\\\\']|\\\\.)*'";
    // QString c_style_comment = "/\\*(?:[^*]*|\\*[^/])*\\*/";
    // QString hash_comment = "#[^\\n]*\\n";
    // QString doubledash_comment = "--\\s+[^\\n]*\\n";
    // QString keyword_characters = "A-Za-z_0-9";
    // QString significant_punctuation = "(,);";
    // QString optional_keyword_delimiters = "[^" + keyword_characters + significant_punctuation + "]?";
    // QString potential_keyword =
    //           optional_keyword_delimiters + "([" + keyword_characters + "]+)" + optional_keyword_delimiters;
    // QString significant_token_pattern =
    //             doublequoted_string + "|" + singlequoted_string + "|" +
    //             c_style_comment + "|" + hash_comment + "|" + doubledash_comment + "|" +
    //             "[" + significant_punctuation + "]" + "|" +
    //             potential_keyword;

    // QRegularExpression significant_token_rx(significant_token_pattern);
    // Some delimiters can potentially turn a keyword into a normal db object name, ...
    // ... as for example in "SELECT * FROM database.table AS `table`", ...
    // ... where "`" and "." are delimiters.
    QString significant_punctuation = QString(MySQL_Bracket_List) + ",;";

    // Search SQL text begin-to-end saving tokens requiring either ...
    // ... preceding or succeeding newlines. The key will be the position and ...
    // ... the value will be the token's text.
    QMap <int, QString> insert_newline_tokens;
    int sql_text_idx = 0;
    bool modified = false;
    int paren_level = 0;
    QStringList newline_after_tokens; newline_after_tokens << "," << "VALUES";

    QStringList comma_newline_words; comma_newline_words << "SELECT" << "VALUES";
    QString current_newline_word = "";

    while  ((sql_text_idx >= 0) and
            (sql_text_idx <= sql_text_length)) {
        QRegularExpressionMatch significant_token_match =
                                  SQL_Token_Regular_Expression.match(sql_text, sql_text_idx);
        int word_idx = significant_token_match.capturedStart();
        if (word_idx >= 0) {
            int new_sql_text_idx = significant_token_match.capturedEnd();
            QString token_capture = significant_token_match.captured(0);
            QString word = significant_token_match.captured(1);
            if (significant_punctuation.contains(token_capture)) {
                if (token_capture == "(") paren_level = paren_level + 1;
                else if (token_capture == ")") paren_level = paren_level - 1;
                else if (token_capture == ",") {
                    if ((paren_level == 0) and
                        comma_newline_words.contains(current_newline_word))
                        insert_newline_tokens[word_idx] = token_capture;
                }
                sql_text_idx = new_sql_text_idx;
            }
            else if (word.length() > 0) {
                QString delimiters = "`.";
                // These delimiters can turn a keyword into a normal db object name, ...
                // as for example in "SELECT * FROM database.table AS `table`".
                bool leading_delimiter = delimiters.contains(token_capture.left(1));
                bool trailing_delimiter = delimiters.contains(token_capture.right(1));
                if (All_MySQL_Keywords.contains(word.toUpper()) and
                    (not leading_delimiter) and
                    (not trailing_delimiter)) {
                    int word_index = significant_token_match.capturedStart(1);
                    if (not (word == word.toUpper())) {
                        sql_text.replace(word_index, word.length(), word.toUpper());
                        modified = true;
                    }
                    if (Newline_Word_List.contains(word.toUpper())) {
                        current_newline_word = word.toUpper();
                        insert_newline_tokens[word_index] = word.toUpper();
                    }
                }

                if ((token_capture.left(1) == "`") and (token_capture.right(1) == "`"))
                    // "FROM `database.table`" is not valid, treat "`name`" as atomic, ...
                    // ... i.e. any "`" pertains to exactly one identifier and must enclose it.
                    sql_text_idx = new_sql_text_idx;
                else if (token_capture.right(1) == ".")
                    // But if trailing delimiter is ".", ...
                    // ... it better be next word's leading delimiter, ...
                    // ... as in "FROM database.table", for example.
                    sql_text_idx = new_sql_text_idx - 1;
                else
                    sql_text_idx = new_sql_text_idx;
            }
            else sql_text_idx = new_sql_text_idx;
        }
        else sql_text_idx = -1;
    }

    if (insert_newline_tokens.count() > 0) {
        QList <int> newline_index = insert_newline_tokens.keys();
        int from_index = -1;
        if (insert_newline_tokens[newline_index[0]] == "SELECT") {
            for (int token_idx = 0; token_idx < newline_index.count(); token_idx += 1) {
                if (insert_newline_tokens[newline_index[token_idx]] == "FROM") {
                    from_index = token_idx;
                    break;
                }
            }
        }
        // Step backward through insert required newline tokens so that ...
        // ... preceding positions (the key) remain valid despite newline insertions
        for (int token_idx = (newline_index.count() - 1);
             token_idx >= 0; token_idx = token_idx - 1) {
            QString current_token = insert_newline_tokens[newline_index[token_idx]];
            // Comma, SELECT, VALUES need newline after ...
            // (Afters must be done first, because if the before is done first ...
            //  ... the index to after will likely be invalid.)
            if (newline_after_tokens.contains(current_token)) {
                // Is an immediately succeeding newline alreay there?
                bool found_newline = false;
                for (int ch_idx = newline_index[token_idx] + current_token.length();
                     ch_idx < sql_text_length; ch_idx += 1) {
                    if (sql_text[ch_idx] == '\n') {
                        found_newline = true;
                        break;
                    }
                    else if (not sql_text[ch_idx].isSpace()) break;
                }
                // No immediately succeeding newline found, insert one ...
                if (not found_newline) {
                    int ch_idx = newline_index[token_idx] + current_token.length();
                    // ... but first, get past extra space characters
                    while ((ch_idx < sql_text_length) and
                           sql_text[ch_idx].isSpace()) ch_idx += 1;
                    QString insert_newline = "\n";
                    if ((current_token == ",") and
                        (from_index >= 0) and (token_idx < from_index)) {
                        insert_newline += "       ";
                    }
                    sql_text.insert(ch_idx, insert_newline);
                    modified = true;
                }
            }
            if (newline_index[token_idx] > 0) {
                // ... SELECT, VALUES and others need newline before
                if (not (current_token == ",")) {
                    QString previous_word = "";
                    if (token_idx > 0) previous_word = insert_newline_tokens[newline_index[token_idx - 1]];
                    // If this is JOIN, don't separate from preceding JOIN modifier.
                    if (not ((current_token == "JOIN") and JOIN_Modifiers.contains(previous_word))) {
                        // Is an immediately preceding newline alreay there?
                        bool found_newline = false;
                        for (int ch_idx = newline_index[token_idx] - 1;
                             ch_idx >= 0; ch_idx = ch_idx - 1) {
                            if (sql_text[ch_idx] == '\n') {
                                found_newline = true;
                                break;
                            }
                            else if (not sql_text[ch_idx].isSpace()) break;
                        }
                        // No immediately preceding newline found, insert one
                        if (not found_newline) {
                            sql_text.insert(newline_index[token_idx], "\n");
                            modified = true;
                        }
                    }
                }
            }
        }
    }

    if (modified) {
        // "Note that the undo/redo history is cleared by this function."
        // this->setText(sql_text);

        this->Set_PlainText(sql_text);

        // Cursor position probably changed, no point computing, set to origin
        QTextCursor text_cursor = this->textCursor();
        text_cursor.setPosition(0);
        this->setTextCursor(text_cursor);
    }
}

void
MySQL_Editor::setColor ( ColorComponent component,
                         const QColor &color ) {
    if (component == Background) {
        QPalette pal = palette();
        pal.setColor(QPalette::Base, color);
        setPalette(pal);
        Editor_Sidebar->indicatorColor = color;
        updateSidebar();
    } else if (component == Normal) {
        QPalette pal = palette();
        pal.setColor(QPalette::Text, color);
        setPalette(pal);
    } else if (component == Sidebar) {
        Editor_Sidebar->backgroundColor = color;
        updateSidebar();
    } else if (component == LineNumber) {
        Editor_Sidebar->lineNumberColor = color;
        updateSidebar();
    } else if (component == Cursor) {
        cursorColor = color;
        updateCursor();
    } else if (component == BracketMatch) {
        bracketMatchColor = color;
        updateCursor();
    } else if (component == BracketError) {
        bracketErrorColor= color;
        updateCursor();
    } else if (component == FoldIndicator) {
        Editor_Sidebar->foldIndicatorColor = color;
        updateSidebar();
    } else {
        Editor_Highlighter->setColor(component, color);
        updateCursor();
    }
}

void
MySQL_Editor::setShowLineNumbersEnabled ( bool enable ) {
    ShowLineNumbersEnabled = enable;
    updateSidebar();
}

bool
MySQL_Editor::isShowLineNumbersEnabled ( ) const {
    return ShowLineNumbersEnabled;
}

void
MySQL_Editor::setTextWrapEnabled ( bool enable ) {
    TextWrapEnabled = enable;
    setLineWrapMode(enable ? WidgetWidth : NoWrap);
}

bool
MySQL_Editor::isTextWrapEnabled ( ) const {
    return TextWrapEnabled;
}

void
MySQL_Editor::setBracketsMatchingEnabled ( bool enable ) {
    BracketsMatchingEnabled = enable;
    updateCursor();
}

bool
MySQL_Editor::isBracketsMatchingEnabled ( ) const {
    return BracketsMatchingEnabled;
}

void
MySQL_Editor::setAutoUppercaseKeywordsEnabled ( bool enable ) {
   AutoUppercaseKeywordsEnabled = enable;
}

bool
MySQL_Editor::isAutoUppercaseKeywordsEnabled ( ) const {
    return AutoUppercaseKeywordsEnabled;
}

void
MySQL_Editor::setAutoIndentEnabled ( bool enable ) {
    AutoIndentEnabled = enable;
}

bool
MySQL_Editor::isAutoIndentEnabled ( ) const {
    return AutoIndentEnabled;
}

void
MySQL_Editor::Set_Tab_Modulus ( int New_Tab_Modulus ) {
    Tab_Modulus = New_Tab_Modulus;
}

void
MySQL_Editor::setCodeFoldingEnabled ( bool enable ) {
    CodeFoldingEnabled = enable;
    updateSidebar();
}

bool
MySQL_Editor::isCodeFoldingEnabled ( ) const {
    return CodeFoldingEnabled;
}

bool
MySQL_Editor::isFoldable ( int line ) {
    int matchPos = findClosingConstruct(document()->findBlockByNumber(line - 1));
    if (matchPos >= 0) {
        QTextBlock matchBlock = document()->findBlock(matchPos);
        if (matchBlock.isValid() && matchBlock.blockNumber() > line)
            return true;
    }
    return false;
}

bool
MySQL_Editor::isFolded ( int line ) const {
    QTextBlock block = document()->findBlockByNumber(line - 1);
    if (!block.isValid())
        return false;
    block = block.next();
    if (!block.isValid())
        return false;
    return !block.isVisible();
}

void
MySQL_Editor::fold ( int line ) {
    QTextBlock startBlock = document()->findBlockByNumber(line - 1);
    int endPos = findClosingConstruct(startBlock);
    if (endPos < 0)
        return;
    QTextBlock endBlock = document()->findBlock(endPos);

    QTextBlock block = startBlock.next();
    while (block.isValid() && block != endBlock) {
        block.setVisible(false);
        block.setLineCount(0);
        block = block.next();
    }

    document()->markContentsDirty(startBlock.position(), endPos - startBlock.position() + 1);
    updateSidebar();
    update();

    MySQL_Editor_DocLayout *layout = reinterpret_cast<MySQL_Editor_DocLayout*>(document()->documentLayout());
    layout->forceUpdate();
}

void
MySQL_Editor::unfold ( int line ) {
    QTextBlock startBlock = document()->findBlockByNumber(line - 1);
    int endPos = findClosingConstruct(startBlock);

    QTextBlock block = startBlock.next();
    while (block.isValid() && !block.isVisible()) {
        block.setVisible(true);
        block.setLineCount(block.layout()->lineCount());
        endPos = block.position() + block.length();
        block = block.next();
    }

    document()->markContentsDirty(startBlock.position(), endPos - startBlock.position() + 1);
    updateSidebar();
    update();

    MySQL_Editor_DocLayout *layout = reinterpret_cast<MySQL_Editor_DocLayout*>(document()->documentLayout());
    layout->forceUpdate();
}

void
MySQL_Editor::toggleFold ( int line ) {
    if (isFolded(line))
        unfold(line);
    else
        fold(line);
}

int
MySQL_Editor::findClosingConstruct ( const QTextBlock &block ) {
    if (!block.isValid())
        return -1;
    MySQLBlockData *blockData = reinterpret_cast<MySQLBlockData*>(block.userData());
    if (!blockData)
        return -1;
    if (blockData->bracketPositions.isEmpty())
        return -1;
    const QTextDocument *doc = block.document();
    int offset = block.position();
    foreach (int pos, blockData->bracketPositions) {
        int absPos = offset + pos;
        if (doc->characterAt(absPos) == Open_Fold_Bracket) {
            int matchPos = Bracket_Match_Position(absPos); // findClosingMatch(doc, absPos);
            if (matchPos >= 0)
                return matchPos;
        }
    }
    return -1;
}

void
MySQL_Editor::resizeEvent ( QResizeEvent *event ) {
    QPlainTextEdit::resizeEvent(event);
    updateSidebar();
}

void
MySQL_Editor::wheelEvent ( QWheelEvent *event ) {
    if (event->modifiers() == Qt::ControlModifier) {
        int steps = event->delta() / 20;
        steps = qBound(-3, steps, 3);
        QFont textFont = font();
        int pointSize = textFont.pointSize() + steps;
        pointSize = qBound(10, pointSize, 40);
        textFont.setPointSize(pointSize);
        setFont(textFont);
        updateSidebar();
        event->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void
MySQL_Editor::updateCursor ( ) {
    if (isReadOnly()) {
        setExtraSelections(QList<QTextEdit::ExtraSelection>());
    } else {

        matchPositions.clear();
        errorPositions.clear();

        if (BracketsMatchingEnabled) {
            QTextCursor cursor = textCursor();
            int cursorPosition = cursor.position();

            if (document()->characterAt(cursorPosition) == '(') {
                int matchPos = Bracket_Match_Position(cursorPosition); // findClosingMatch(document(), cursorPosition);
                if (matchPos < 0) {
                    errorPositions += cursorPosition;
                } else {
                    matchPositions += cursorPosition;
                    matchPositions += matchPos;
                }
            }

            if (document()->characterAt(cursorPosition - 1) == ')') {
                int matchPos = Bracket_Match_Position(cursorPosition - 1); // findOpeningMatch(document(), cursorPosition);
                if (matchPos < 0) {
                    errorPositions += cursorPosition - 1;
                } else {
                    matchPositions += cursorPosition - 1;
                    matchPositions += matchPos;
                }
            }
        }

        QTextEdit::ExtraSelection highlight;
        highlight.format.setBackground(cursorColor);
        highlight.format.setProperty(QTextFormat::FullWidthSelection, true);
        highlight.cursor = textCursor();
        highlight.cursor.clearSelection();

        QList<QTextEdit::ExtraSelection> extraSelections;
        extraSelections.append(highlight);

        for (int i = 0; i < matchPositions.count(); ++i) {
            int pos = matchPositions.at(i);
            QTextEdit::ExtraSelection matchHighlight;
            matchHighlight.format.setBackground(bracketMatchColor);
            matchHighlight.cursor = textCursor();
            matchHighlight.cursor.setPosition(pos);
            matchHighlight.cursor.setPosition(pos + 1, QTextCursor::KeepAnchor);
            extraSelections.append(matchHighlight);
        }

        for (int i = 0; i < errorPositions.count(); ++i) {
            int pos = errorPositions.at(i);
            QTextEdit::ExtraSelection errorHighlight;
            errorHighlight.format.setBackground(bracketErrorColor);
            errorHighlight.cursor = textCursor();
            errorHighlight.cursor.setPosition(pos);
            errorHighlight.cursor.setPosition(pos + 1, QTextCursor::KeepAnchor);
            extraSelections.append(errorHighlight);
        }

        setExtraSelections(extraSelections);
    }
}

void
MySQL_Editor::updateSidebar ( const QRect &rect,
                              int d ) {
    Q_UNUSED(rect)
    if (d != 0)
        updateSidebar();
}

void
MySQL_Editor::updateSidebar ( ) {
    if ((not ShowLineNumbersEnabled) and (not CodeFoldingEnabled)) {
        Editor_Sidebar->hide();
        setViewportMargins(0, 0, 0, 0);
        Editor_Sidebar->setGeometry(3, 0, 0, height());
        return;
    }

    Editor_Sidebar->foldIndicatorWidth = 0;
    Editor_Sidebar->font = this->font();
    Editor_Sidebar->show();

    int sw = 0;
    if (ShowLineNumbersEnabled) {
        int digits = 2;
        int maxLines = blockCount();
        for (int number = 10; number < maxLines; number *= 10)
            ++digits;
        sw += fontMetrics().width('w') * digits;
    }
    if (CodeFoldingEnabled) {
        int fh = fontMetrics().lineSpacing();
        int fw = fontMetrics().width('w');
        Editor_Sidebar->foldIndicatorWidth = qMax(fw, fh);
        sw += Editor_Sidebar->foldIndicatorWidth;
    }
    setViewportMargins(sw, 0, 0, 0);

    Editor_Sidebar->setGeometry(0, 0, sw, height());
    QRectF sidebarRect(0, 0, sw, height());

    QTextBlock block = firstVisibleBlock();
    int index = 0;
    while (block.isValid()) {
        if (block.isVisible()) {
            QRectF rect = blockBoundingGeometry(block).translated(contentOffset());
            if (sidebarRect.intersects(rect)) {
                if (Editor_Sidebar->lineNumbers.count() >= index)
                    Editor_Sidebar->lineNumbers.resize(index + 1);
                Editor_Sidebar->lineNumbers[index].position = rect.top();
                Editor_Sidebar->lineNumbers[index].number = block.blockNumber() + 1;
                Editor_Sidebar->lineNumbers[index].foldable =
                  CodeFoldingEnabled ? isFoldable(block.blockNumber() + 1) : false;
                Editor_Sidebar->lineNumbers[index].folded =
                  CodeFoldingEnabled ? isFolded(block.blockNumber() + 1) : false;
                ++index;
            }
            if (rect.top() > sidebarRect.bottom())
                break;
        }
        block = block.next();
    }
    Editor_Sidebar->lineNumbers.resize(index);
    Editor_Sidebar->update();
}

void
MySQL_Editor::mark ( const QString &str,
                     Qt::CaseSensitivity sens ) {
    Editor_Highlighter->mark(str, sens);
}

// Auto complete ...
void
MySQL_Editor::initializeAutoComplete ( ) {
    if (AutoCompleteKeywordsEnabled or AutoCompleteIdentifiersEnabled) {
        QStringList word_list;

        if (AutoCompleteKeywordsEnabled) word_list << All_MySQL_Keywords;
        if (AutoCompleteIdentifiersEnabled) word_list << Auto_Complete_Identifier_List;

        word_list.sort(Qt::CaseInsensitive);

        Completer->setModel(new QStringListModel(word_list));
    }

    In_Completion_Context = false;
}

void
MySQL_Editor::finalizeAutoComplete ( ) {
    if (Completer and Completer->popup()->isVisible()) Completer->popup()->hide();
}

void
MySQL_Editor::setAutoCompleteKeywordsEnabled ( bool enable ) {
    AutoCompleteKeywordsEnabled = enable;
    initializeAutoComplete();
}

bool
MySQL_Editor::isAutoCompleteKeywordsEnabled ( ) const {
    return AutoCompleteKeywordsEnabled;
}

void
MySQL_Editor::setAutoCompleteIdentifierList ( QStringList identifier_list ) {
    Auto_Complete_Identifier_List = identifier_list;
    initializeAutoComplete();
}

void
MySQL_Editor::setAutoCompleteContextIdentifierList ( QHash <QString, QStringList> context_identifier_list ) {
    Auto_Complete_Context_Identifier_List = context_identifier_list;
    initializeAutoComplete();
}

void
MySQL_Editor::setAutoCompleteIdentifiersEnabled ( bool enable ) {
    AutoCompleteIdentifiersEnabled = enable;
    initializeAutoComplete();
}

bool
MySQL_Editor::isAutoCompleteIdentifiersEnabled ( ) const {
    return AutoCompleteIdentifiersEnabled;
}

void
MySQL_Editor::setCompleter ( QCompleter *completer ) {
    if (Completer) QObject::disconnect(Completer, 0, this, 0);

    Completer = completer;

    if (not Completer) return;

    Completer->setWidget(this);
    Completer->setCompletionMode(QCompleter::PopupCompletion);
    Completer->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(Completer, SIGNAL(activated(QString)),
                     this, SLOT(insertCompletion(QString)));
}

void
MySQL_Editor::insertCompletion ( const QString& completion,
                                 bool replace_entire ) {
    if (not (Completer->widget() == this)) return;

    QTextCursor txt_cur = textCursor();

    if (replace_entire) {
        // Replace whole word, enforce word list capitalization
        txt_cur.movePosition(QTextCursor::Left);
        txt_cur.select(QTextCursor::WordUnderCursor);
        // A 'completion context' is entered when a context indetifier is followed by '.', ...
        // for example, 'table_name.' in which case all of the column names in table_name ...
        // ... are displayed in the completer popup.
        if (In_Completion_Context and (txt_cur.selectedText() == ".")) txt_cur.insertText("." + completion);
        else txt_cur.insertText(completion);
    }
    else {
        int extra = completion.length() - Completer->completionPrefix().length();
        txt_cur.movePosition(QTextCursor::Left);
        txt_cur.movePosition(QTextCursor::EndOfWord);
        txt_cur.insertText(completion.right(extra));
        setTextCursor(txt_cur);
    }

    if (In_Completion_Context) {
        // A 'completion context' is entered when a context indetifier is followed by '.', ...
        // for example, 'table_name.' in which case all of the column names in table_name ...
        // ... are displayed in the completer popup.
        // Now return to normal completion, but first hide completer popup
        if (Completer and Completer->popup()->isVisible()) Completer->popup()->hide();
        initializeAutoComplete();
    }
    // Belt and suspenders
    In_Completion_Context = false;
}

QString
MySQL_Editor::textUnderCursor ( ) const {
    QTextCursor txt_cur = textCursor();
    txt_cur.select(QTextCursor::WordUnderCursor);
    return txt_cur.selectedText();
}

void
MySQL_Editor::focusInEvent ( QFocusEvent *event ) {
    if (Completer) Completer->setWidget(this);
    QPlainTextEdit::focusInEvent(event);
}
// ... Auto complete
