%skeleton "lalr1.cc"

%code requires{
# include <iostream>
# include <sstream>
# include <Parsing/Processor.hpp>

  class YYParserAdapter;
}

%define "parser_class_name" "PlainParser"
%locations
%parse-param { YYParserAdapter& parse_adapter }

%error-verbose

%union {
  Identifier* identifier;
  IdentifierList* identifier_list;
  TypeSpecifier* type_specifier;
  TypeSpecifierList* type_specifier_list;
}

%token PTOKEN_COMMENT
%token PTOKEN_CONTROL_COMMENT
%token <identifier> PTOKEN_IDENTIFIER
%token PTOKEN_TYPEDEF
%token PTOKEN_STRUCT PTOKEN_READER PTOKEN_WRITER PTOKEN_NAMESPACE
%token PTOKEN_AUTOREADER PTOKEN_AUTOWRITER
%token PTOKEN_END 0
%token-table

%type <identifier_list> identifier_list
%type <type_specifier> type_specifier
%type <type_specifier_list> type_specifier_list

%destructor { delete $$; } PTOKEN_IDENTIFIER
%destructor { delete $$; } identifier_list
%destructor { delete $$; } type_specifier
%destructor { delete $$; } type_specifier_list

%{
# include <Parsing/YYParserAdapter.hpp>

# undef yylex
# define yylex parse_adapter.scanner->lex
%}

%start translation_unit

// grammar
%%
translation_unit
  :
  | specifier_list
  ;

specifier_list
  : specifier
  | specifier_list specifier
  ;

specifier
  : namespace_specifier
  | struct_specifier
  | typedef_specifier
  | autoreader_specifier
  | autowriter_specifier
  | reader_specifier
  | writer_specifier
  | ';'
  ;

typedef_specifier
  : PTOKEN_TYPEDEF type_specifier PTOKEN_IDENTIFIER ';'
    {
      parse_adapter.processor->clone_type($3->c_str(), *$2);
    }
  ;

namespace_specifier
  : PTOKEN_NAMESPACE PTOKEN_IDENTIFIER
    {
      parse_adapter.processor->open_namespace($2->c_str());
    } '{' specifier_list '}'
    {
      parse_adapter.processor->close_namespace();
    }
  | PTOKEN_NAMESPACE error
    {
      error(yyla.location, "Expected namespace name");
    }
  ;

struct_specifier
  : PTOKEN_STRUCT PTOKEN_IDENTIFIER '{'
    {
      parse_adapter.processor->open_descriptor($2->c_str());
    }
    struct_declaration_list '}'
    {
      parse_adapter.processor->close_descriptor();
    }
  | PTOKEN_STRUCT error
    {
      error(yyla.location, "Expected struct name");
    }
  ;

struct_declaration_list
  : struct_declaration
  | struct_declaration_list struct_declaration
  ;

struct_declaration
  : type_specifier identifier_list ';'
    {
      try
      {
        for(IdentifierList::const_iterator id_it = $2->begin();
            id_it != $2->end(); ++id_it)
        {
          parse_adapter.processor->add_descriptor_field(
            *$1, id_it->c_str());
        }
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
  ;

autoreader_specifier
  : PTOKEN_AUTOREADER PTOKEN_IDENTIFIER '<' PTOKEN_IDENTIFIER '>'
    {
      try
      {
        parse_adapter.processor->create_auto_reader($2->c_str(), $4->c_str());
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
  | PTOKEN_AUTOREADER error 
    {
      error(yyla.location, "Expected auto reader name");
    }
  ;

autowriter_specifier
  : PTOKEN_AUTOWRITER PTOKEN_IDENTIFIER '<' PTOKEN_IDENTIFIER '>'
    {
      try
      {
        parse_adapter.processor->create_auto_writer($2->c_str(), $4->c_str());
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
  | PTOKEN_AUTOWRITER error
    {
      error(yyla.location, "Expected auto writer name");
    }
  ;

// reader declaration
reader_specifier
  : PTOKEN_READER PTOKEN_IDENTIFIER '<' PTOKEN_IDENTIFIER '>' '{'
    {
      try
      {
        parse_adapter.processor->open_reader($2->c_str(), $4->c_str());
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
    reader_declaration_list '}'
    {
      parse_adapter.processor->close_reader();
    }
  | PTOKEN_READER error 
    {
      error(yyla.location, "Expected reader name");
    }
  ;

reader_declaration_list
  : reader_declaration
  | reader_declaration_list reader_declaration
  ;

reader_declaration
  : type_specifier identifier_list ';'
    {
      try
      {
        for(IdentifierList::const_iterator id_it = $2->begin();
            id_it != $2->end(); ++id_it)
        {
          parse_adapter.processor->add_reader_field(
            *$1, id_it->c_str());
        }
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
  | identifier_list ';'
    {
      try
      {
        for(IdentifierList::const_iterator id_it = $1->begin();
            id_it != $1->end(); ++id_it)
        {
          parse_adapter.processor->add_reader_field("", id_it->c_str());
        }
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
  ;

// writer declaration
writer_specifier
  : PTOKEN_WRITER PTOKEN_IDENTIFIER '<' PTOKEN_IDENTIFIER '>' '{'
    {
      parse_adapter.processor->open_writer($2->c_str(), $4->c_str());
    }
    writer_declaration_list '}'
    {
      parse_adapter.processor->close_writer();
    }
  | PTOKEN_WRITER PTOKEN_IDENTIFIER error
    {
      std::ostringstream ostr;
      ostr << "Expected base struct name for writer '" << *$2 << "'";
      error(yyla.location, ostr.str());
    }
  | PTOKEN_WRITER error
    {
      error(yyla.location, "Expected writer name");
    }
  ;

writer_declaration_list
  : writer_declaration
  | writer_declaration_list writer_declaration
  ;

writer_declaration
  : type_specifier '(' identifier_list ')' identifier_list ';'
    {
      try
      {
        for(IdentifierList::const_iterator id_it = $5->begin();
            id_it != $5->end(); ++id_it)
        {
          parse_adapter.processor->add_writer_field(
            *$1,
            id_it->c_str(),
            *$3);
        }
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
  | type_specifier identifier_list ';'
    {
      try
      {
        for(IdentifierList::const_iterator id_it = $2->begin();
            id_it != $2->end(); ++id_it)
        {
          parse_adapter.processor->add_writer_field(
            *$1,
            id_it->c_str());
        }
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
  | '(' identifier_list ')' identifier_list ';'
    {
      try
      {
        for(IdentifierList::const_iterator id_it = $4->begin();
            id_it != $4->end(); ++id_it)
        {
          parse_adapter.processor->add_writer_field(
            "",
            id_it->c_str(),
            *$2);
        }
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
  | identifier_list ';'
    {
      try
      {
        for(IdentifierList::const_iterator id_it = $1->begin();
            id_it != $1->end(); ++id_it)
        {
          parse_adapter.processor->add_writer_field(
            "", id_it->c_str());
        }
      }
      catch(const Parsing::Processor::Exception& ex)
      {
        error(yyla.location, ex.what());
        YYABORT;
      }
    }
  ;

type_specifier_list
  : type_specifier
    {
      $$ = new TypeSpecifierList;
      $$->push_back(*$1);
    }
  | type_specifier_list ',' type_specifier
    {
      $$ = new TypeSpecifierList;
      $$->swap(*$1);
      $$->push_back(*$3);
    }
  ;

type_specifier
  : PTOKEN_IDENTIFIER
    {
      $$ = new TypeSpecifier($1->c_str());
    }
  | PTOKEN_IDENTIFIER '<' type_specifier_list '>'
    {
      $$ = new TypeSpecifier($1->c_str(), *$3);
    }
  ;

identifier_list
  : PTOKEN_IDENTIFIER
    {
      $$ = new IdentifierList;
      $$->push_back(*$1);
    }
  | identifier_list ',' PTOKEN_IDENTIFIER
    {
      $$ = new IdentifierList;      
      $$->swap(*$1);
      $$->push_back(*$3);
    }
  ;

%%

void
yy::PlainParser::error(
  const yy::PlainParser::location_type& location,
  const std::string& message)
{
  parse_adapter.processor->error(
    location.begin.line,
    location.begin.column,
    message.c_str());
}
