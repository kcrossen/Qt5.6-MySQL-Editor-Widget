# Qt5.6-MySQL-Editor-Widget

Qt5.6 C++ MySQL query editor widget w/ syntax highlighting, intelligent tabbing, code folding, paren matching, auto uppercasing of MySQL keywords, auto complete keywords and identifiers, auto-formatting, plus other typical IDE code editor features. 

<h4>Intelligent Tabbing:</h4> Not fixed modulus tabs, e.g. 4 spaces, but tabbing to match the context of what has already been typed.

<h4>Auto Complete:</h4> There are two independently enablable auto completion modes, keyword ('SELECT', 'FROM', 'WHERE', etc.) and identifier (database entities including database, table, and column names). For identifiers, auto complete works two different ways, with or without context. In this connection, context refers to having typed, for example, a table name followed by a '.', in which case a list of the columns of that table is displayed as a popup. Without a context, all database, table, and column names are potential auto complete matches.
