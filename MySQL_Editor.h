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

#ifndef MYSQL_EDITOR_H
#define MYSQL_EDITOR_H

#include <QtCore>
#include <QtGui>

#include <QPlainTextEdit>
#include <QScopedPointer>
#include <QColor>
#include <QCompleter>
#include <QAbstractItemView>
#include <QAbstractListModel>
#include <QStringListModel>
#include <QScrollBar>

class MySQL_Editor;
class MySQL_Editor_Sidebar;
class MySQL_Editor_Highlighter;
class MySQL_Editor_DocLayout;

#define Open_Fold_Bracket '('
#define Close_Fold_Bracket ')'

class MySQL_Editor: public QPlainTextEdit {
    Q_OBJECT

    Q_PROPERTY(bool CodeFoldingEnabled READ isCodeFoldingEnabled WRITE setCodeFoldingEnabled)
    Q_PROPERTY(bool ShowLineNumbersEnabled READ isShowLineNumbersEnabled WRITE setShowLineNumbersEnabled)
    Q_PROPERTY(bool TextWrapEnabled READ isTextWrapEnabled WRITE setTextWrapEnabled)

    Q_PROPERTY(bool BracketsMatchingEnabled READ isBracketsMatchingEnabled WRITE setBracketsMatchingEnabled)

    Q_PROPERTY(bool AutoIndentEnabled READ isAutoIndentEnabled WRITE setAutoIndentEnabled)
    Q_PROPERTY(bool AutoCompleteKeywordsEnabled READ isAutoCompleteKeywordsEnabled WRITE setAutoCompleteKeywordsEnabled)
    Q_PROPERTY(bool AutoCompleteIdentifiersEnabled READ isAutoCompleteIdentifiersEnabled WRITE setAutoCompleteIdentifiersEnabled)
    Q_PROPERTY(bool AutoUppercaseKeywordsEnabled READ isAutoUppercaseKeywordsEnabled WRITE setAutoUppercaseKeywordsEnabled)

public:
    MySQL_Editor ( QWidget *parent = 0 );
    ~MySQL_Editor ( );

    typedef enum {
        Background,
        Normal,
        Comment,
        Number,
        String,
        Operator,
        Identifier,
        Keyword,
        Function,
        Type,
        Interval,
        Sidebar,
        LineNumber,
        Cursor,
        Marker,
        BracketMatch,
        BracketError,
        FoldIndicator
    } ColorComponent;

private:
    MySQL_Editor_Sidebar *Editor_Sidebar;
    MySQL_Editor_Highlighter *Editor_Highlighter;
    MySQL_Editor_DocLayout *Editor_Layout;

    QList<int> matchPositions;
    QList<int> errorPositions;
    QColor cursorColor;
    QColor bracketMatchColor;
    QColor bracketErrorColor;

    bool CodeFoldingEnabled;
    bool ShowLineNumbersEnabled;

    bool TextWrapEnabled;

    QRegularExpression MySQL_Bracket_RegEx;
    QString Bracket_Source_Text;
    QString Bracket_Text;

    bool BracketsMatchingEnabled;

    bool Quote_Bracket_Character;
    bool Post_Select_Bracket_Enclosed_Text;

    bool AutoIndentEnabled;
    int Tab_Modulus;

    bool AutoCompleteKeywordsEnabled;
    bool AutoCompleteIdentifiersEnabled;

    bool AutoUppercaseKeywordsEnabled;

    QRegularExpression SQL_Token_Regular_Expression;

    QStringList MySQL_Keywords;
    QStringList MySQL_Functions;
    QStringList MySQL_Types;
    QStringList MySQL_Intervals;
    QStringList All_MySQL_Keywords;

    QStringList Auto_Complete_Identifier_List;
    QHash <QString, QStringList> Auto_Complete_Context_Identifier_List;
    // A 'completion context' is entered when a context indetifier is followed by '.', ...
    // for example, 'table_name.' in which case all of the column names in table_name ...
    // ... are displayed in the completer popup.
    bool In_Completion_Context;

    QStringList Newline_Word_List;
    QStringList JOIN_Modifiers;

    bool Uppercasing_In_Process;
    int Previous_Cursor_Line;

public:
    void
    Set_PlainText ( QString Set_Plain_Text );

    QString
    Replace_Paragraph_Separator ( QString Initial_Text );

    QString
    Selected_Text ( );

    void
    Uppercase_SQL_Keywords ( int start_uppercase_position = -1,
                             int uppercase_length = -1 );

    QString
    Initial_SQL_Keyword ( QString SQL_Statement );

    void
    Simple_Format_SQL ( );

    void
    setColor ( ColorComponent component,
               const QColor &color );

    QStringList
    mysqlKeywords ( ) const;

    QStringList
    mysqlFunctions ( ) const;

    QStringList
    mysqlTypes ( ) const;

    QStringList
    mysqlIntervals ( ) const;

    bool
    isKeyword ( QString potential_keyword );

    bool
    isFunction ( QString potential_function );

    bool
    isType ( QString potential_type );

    bool
    isInterval ( QString potential_interval );

public slots:
    void
    setBracketsMatchingEnabled ( bool enable );

public:
    bool
    isBracketsMatchingEnabled ( ) const;

public slots:
    void
    setShowLineNumbersEnabled ( bool enable );

public:
    bool
    isShowLineNumbersEnabled ( ) const;

public slots:
    void
    setTextWrapEnabled ( bool enable );

public:
    bool
    isTextWrapEnabled ( ) const;

public slots:
    void
    setAutoIndentEnabled ( bool enable );

public:
    bool
    isAutoIndentEnabled ( ) const;

public:
    void
    Set_Tab_Modulus ( int New_Tab_modulus );

    void
    Indent_Selected_Text_Lines ( int Indent_Tab_Modulus );

public slots:
    void
    setCodeFoldingEnabled ( bool enable );

public:
    bool
    isCodeFoldingEnabled ( ) const;

    bool
    isFoldable ( int line );

    bool
    isFolded ( int line ) const;

public slots:
    void
    fold ( int line );

    void
    unfold ( int line );

    void
    toggleFold ( int line );

public slots:
    void
    mark ( const QString &str,
           Qt::CaseSensitivity sens = Qt::CaseInsensitive );

protected:
    void
    keyPressEvent ( QKeyEvent* event ) Q_DECL_OVERRIDE;

    void
    resizeEvent ( QResizeEvent* event ) Q_DECL_OVERRIDE;

    void
    wheelEvent ( QWheelEvent* event ) Q_DECL_OVERRIDE;

    void
    insertFromMimeData ( const QMimeData* source ) Q_DECL_OVERRIDE;

private:
#define MySQL_Bracket_List "()"
#define Default_Tab_Modulus 4

    QString
    Compute_Bracket_Text ( QString Source_Text );

    int
    Bracket_Match_Position ( int Current_Position );

    int
    Compute_Current_Paren_Indent ( QString Current_Text,
                                   int Current_Position );

    QTextCursor
    Select_Selected_Text_Lines ( );

private slots:
    void
    onTextChanged ( );

    void
    onCursorPositionChanged ( );

private:
    int
    findClosingConstruct ( const QTextBlock &block );

private slots:
    void
    updateCursor ( );

public slots:
    void
    updateSidebar ( );

private slots:
    void
    updateSidebar ( const QRect &rect,
                    int d );

// Auto upper case keywords ...
public:
    void
    setAutoUppercaseKeywordsEnabled ( bool enable ) ;

    bool
    isAutoUppercaseKeywordsEnabled ( ) const;

// Auto complete ...
private:
    void
    initializeAutoComplete ( );

public:
    void
    finalizeAutoComplete ( );

    void
    setAutoCompleteKeywordsEnabled ( bool enable ) ;

    bool
    isAutoCompleteKeywordsEnabled ( ) const;

    void
    setAutoCompleteIdentifierList ( QStringList identifier_list );

    void
    setAutoCompleteContextIdentifierList ( QHash <QString, QStringList> context_identifier_list );

    void
    setAutoCompleteIdentifiersEnabled ( bool enable ) ;

    bool
    isAutoCompleteIdentifiersEnabled ( ) const;

private:
    void
    setCompleter ( QCompleter *completer );

protected:
    void
    focusInEvent ( QFocusEvent *event ) Q_DECL_OVERRIDE;

private slots:
    void
    insertCompletion ( const QString &completion,
                       bool replace_entire = true );

private:
    QString
    textUnderCursor ( ) const;

private:
    QCompleter *Completer;
// ... Auto complete

};

class MySQLBlockData: public QTextBlockUserData {
public:
    QList<int> bracketPositions;
};

class MySQL_Editor_Highlighter : public QSyntaxHighlighter {
public:
    MySQL_Editor_Highlighter ( QTextDocument *parent = 0 );

    void
    setColor( MySQL_Editor::ColorComponent component,
              const QColor &color );

    void
    mark( const QString &str,
          Qt::CaseSensitivity caseSensitivity );

public:
    void
    setHighlightMySQLEditor ( MySQL_Editor *highlight_mysql_editor );

protected:
    void
    highlightBlock ( const QString &text );

private:
    MySQL_Editor *Highlight_MySQL_Editor;

    QHash<MySQL_Editor::ColorComponent, QColor> m_colors;
    QString m_markString;
    Qt::CaseSensitivity m_markCaseSensitivity;
};

struct BlockInfo {
    int position;
    int number;
    bool foldable: 1;
    bool folded : 1;
};

Q_DECLARE_TYPEINFO(BlockInfo, Q_PRIMITIVE_TYPE);

class MySQL_Editor_Sidebar : public QWidget {
public:
    MySQL_Editor_Sidebar ( MySQL_Editor *editor );

    QVector<BlockInfo> lineNumbers;

    QColor backgroundColor;
    QColor lineNumberColor;
    QColor indicatorColor;
    QColor foldIndicatorColor;
    int foldIndicatorWidth;

    QFont font;

    QPixmap rightArrowIcon;
    QPixmap downArrowIcon;

protected:
    void mousePressEvent ( QMouseEvent *event );
    void paintEvent ( QPaintEvent *event );
};

class MySQL_Editor_DocLayout: public QPlainTextDocumentLayout {
public:
    MySQL_Editor_DocLayout ( QTextDocument *doc );

    void
    forceUpdate ( );
};

#endif // MYSQL_EDITOR_H
