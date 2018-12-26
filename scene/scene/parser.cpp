// A Bison parser, made by GNU Bison 3.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.





#include "parser.hpp"


// Unqualified %code blocks.
#line 28 "parser.y" // lalr1.cc:437

#include "driver.hpp"

#line 49 "parser.cpp" // lalr1.cc:437


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (/*CONSTCOND*/ false)
# endif


// Suppress unused-variable warnings by "using" E.
#define YYUSE(E) ((void) (E))

// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yystack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)


namespace yy {
#line 144 "parser.cpp" // lalr1.cc:512

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr = "";
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              // Fall through.
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }


  /// Build a parser object.
  parser::parser (Driver& drv_yyarg)
    :
#if YYDEBUG
      yydebug_ (false),
      yycdebug_ (&std::cerr),
#endif
      drv (drv_yyarg)
  {}

  parser::~parser ()
  {}


  /*---------------.
  | Symbol types.  |
  `---------------*/



  // by_state.
  parser::by_state::by_state ()
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& other)
    : state (other.state)
  {}

  void
  parser::by_state::clear ()
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s)
    : state (s)
  {}

  parser::symbol_number_type
  parser::by_state::type_get () const
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[state];
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 7: // FLOAT
        value.YY_MOVE_OR_COPY< float > (YY_MOVE (that.value));
        break;

      case 6: // INTEGER
      case 27: // id
        value.YY_MOVE_OR_COPY< int > (YY_MOVE (that.value));
        break;

      case 8: // NAME
      case 9: // STRING
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if defined __cplusplus && 201103L <= __cplusplus
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 7: // FLOAT
        value.move< float > (YY_MOVE (that.value));
        break;

      case 6: // INTEGER
      case 27: // id
        value.move< int > (YY_MOVE (that.value));
        break;

      case 8: // NAME
      case 9: // STRING
        value.move< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.type = empty_symbol;
  }

#if defined __cplusplus && __cplusplus < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.type_get ())
    {
      case 7: // FLOAT
        value.move< float > (that.value);
        break;

      case 6: // INTEGER
      case 27: // id
        value.move< int > (that.value);
        break;

      case 8: // NAME
      case 9: // STRING
        value.move< std::string > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo,
                                     const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    symbol_number_type yytype = yysym.type_get ();
    // Avoid a (spurious) G++ 4.8 warning about "array subscript is
    // below array bounds".
    if (yysym.empty ())
      std::abort ();
    yyo << (yytype < yyntokens_ ? "token" : "nterm")
        << ' ' << yytname_[yytype] << " ("
        << yysym.location << ": ";
    switch (yytype)
    {
            case 6: // INTEGER

#line 44 "parser.y" // lalr1.cc:671
        { yyo << yysym.value.template as< int > (); }
#line 360 "parser.cpp" // lalr1.cc:671
        break;

      case 7: // FLOAT

#line 44 "parser.y" // lalr1.cc:671
        { yyo << yysym.value.template as< float > (); }
#line 367 "parser.cpp" // lalr1.cc:671
        break;

      case 8: // NAME

#line 44 "parser.y" // lalr1.cc:671
        { yyo << yysym.value.template as< std::string > (); }
#line 374 "parser.cpp" // lalr1.cc:671
        break;

      case 9: // STRING

#line 44 "parser.y" // lalr1.cc:671
        { yyo << yysym.value.template as< std::string > (); }
#line 381 "parser.cpp" // lalr1.cc:671
        break;

      case 27: // id

#line 44 "parser.y" // lalr1.cc:671
        { yyo << yysym.value.template as< int > (); }
#line 388 "parser.cpp" // lalr1.cc:671
        break;


      default:
        break;
    }
    yyo << ')';
  }
#endif

  void
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if defined __cplusplus && 201103L <= __cplusplus
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
  {
    // State.
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

    // A new symbol was pushed on the stack.
  yynewstate:
    YYCDEBUG << "Entering state " << yystack_[0].state << '\n';

    // Accept?
    if (yystack_[0].state == yyfinal_)
      goto yyacceptlab;

    goto yybackup;

    // Backup.
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token: ";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (drv));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.type_get ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.type_get ())
      goto yydefault;

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", yyn, YY_MOVE (yyla));
    goto yynewstate;

  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;

  /*-----------------------------.
  | yyreduce -- Do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case 7: // FLOAT
        yylhs.value.emplace< float > ();
        break;

      case 6: // INTEGER
      case 27: // id
        yylhs.value.emplace< int > ();
        break;

      case 8: // NAME
      case 9: // STRING
        yylhs.value.emplace< std::string > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        slice<stack_symbol_type, stack_type> slice (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, slice, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 7:
#line 56 "parser.y" // lalr1.cc:906
    { drv.setSection(Driver::Section::eScene); }
#line 635 "parser.cpp" // lalr1.cc:906
    break;

  case 10:
#line 62 "parser.y" // lalr1.cc:906
    { drv.setSection(Driver::Section::eAssets); }
#line 641 "parser.cpp" // lalr1.cc:906
    break;

  case 14:
#line 68 "parser.y" // lalr1.cc:906
    { drv.addAsset(yystack_[1].value.as< int > (), yystack_[0].value.as< std::string > ()); }
#line 647 "parser.cpp" // lalr1.cc:906
    break;

  case 15:
#line 71 "parser.y" // lalr1.cc:906
    { drv.setSection(Driver::Section::eEntities); }
#line 653 "parser.cpp" // lalr1.cc:906
    break;

  case 19:
#line 77 "parser.y" // lalr1.cc:906
    { drv.addEntity(yystack_[0].value.as< int > ()); }
#line 659 "parser.cpp" // lalr1.cc:906
    break;

  case 25:
#line 87 "parser.y" // lalr1.cc:906
    { drv.addComponent(yystack_[0].value.as< std::string > ()); }
#line 665 "parser.cpp" // lalr1.cc:906
    break;

  case 28:
#line 95 "parser.y" // lalr1.cc:906
    { yylhs.value.as< int > () = yystack_[0].value.as< int > (); }
#line 671 "parser.cpp" // lalr1.cc:906
    break;

  case 29:
#line 96 "parser.y" // lalr1.cc:906
    { yylhs.value.as< int > () = -1; }
#line 677 "parser.cpp" // lalr1.cc:906
    break;

  case 31:
#line 101 "parser.y" // lalr1.cc:906
    {
    throw yy::parser::syntax_error(drv.m_location,
                                   "bracketed values must start at "
                                   "the same line as the property name");
  }
#line 687 "parser.cpp" // lalr1.cc:906
    break;

  case 32:
#line 106 "parser.y" // lalr1.cc:906
    {
    throw yy::parser::syntax_error(drv.m_location,
                                   "property name cannot be used as a value");
  }
#line 696 "parser.cpp" // lalr1.cc:906
    break;

  case 39:
#line 118 "parser.y" // lalr1.cc:906
    { drv.addProperty(yystack_[0].value.as< std::string > ()); }
#line 702 "parser.cpp" // lalr1.cc:906
    break;

  case 48:
#line 137 "parser.y" // lalr1.cc:906
    { drv.addValue(yystack_[0].value.as< int > ()); }
#line 708 "parser.cpp" // lalr1.cc:906
    break;

  case 49:
#line 138 "parser.y" // lalr1.cc:906
    { drv.addValue(yystack_[0].value.as< float > ()); }
#line 714 "parser.cpp" // lalr1.cc:906
    break;

  case 50:
#line 139 "parser.y" // lalr1.cc:906
    { drv.addValue(yystack_[0].value.as< std::string > ()); }
#line 720 "parser.cpp" // lalr1.cc:906
    break;


#line 724 "parser.cpp" // lalr1.cc:906
            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;
      YY_STACK_PRINT ();

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;

  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        error (yyla.location, yysyntax_error_ (yystack_[0].state, yyla));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.type_get () == yyeof_)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:

    /* Pacify compilers like GCC when the user code never invokes
       YYERROR and the label yyerrorlab therefore never appears in user
       code.  */
    if (false)
      goto yyerrorlab;
    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    goto yyerrlab1;

  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    {
      stack_symbol_type error_token;
      for (;;)
        {
          yyn = yypact_[yystack_[0].state];
          if (!yy_pact_value_is_default_ (yyn))
            {
              yyn += yyterror_;
              if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yyterror_)
                {
                  yyn = yytable_[yyn];
                  if (0 < yyn)
                    break;
                }
            }

          // Pop the current state because it cannot handle the error token.
          if (yystack_.size () == 1)
            YYABORT;

          yyerror_range[1].location = yystack_[0].location;
          yy_destroy_ ("Error: popping", yystack_[0]);
          yypop_ ();
          YY_STACK_PRINT ();
        }

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = yyn;
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;

    // Accept.
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;

    // Abort.
  yyabortlab:
    yyresult = 1;
    goto yyreturn;

  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (state_type yystate, const symbol_type& yyla) const
  {
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    size_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state
         merging (from LALR or IELR) and default reductions corrupt the
         expected token list.  However, the list is correct for
         canonical LR with one exception: it will still contain any
         token that will not be accepted due to an error action in a
         later state.
    */
    if (!yyla.empty ())
      {
        int yytoken = yyla.type_get ();
        yyarg[yycount++] = yytname_[yytoken];
        int yyn = yypact_[yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            // Stay within bounds of both yycheck and yytname.
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yyterror_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    size_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const signed char parser::yypact_ninf_ = -15;

  const signed char parser::yytable_ninf_ = -1;

  const signed char
  parser::yypact_[] =
  {
      -8,   -15,    57,   -15,   -15,   -15,   -15,    16,    39,    40,
      21,    44,    20,   -15,   -15,   -15,    41,    -1,   -15,    37,
     -15,   -15,   -15,   -15,    49,   -15,   -15,   -15,   -15,    -3,
     -15,    64,   -15,   -15,   -15,   -15,   -15,   -15,    28,   -15,
       2,     7,   -15,   -15,   -15,    30,    53,   -15,   -15,    17,
     -15,    64,   -15,   -15,    54,   -15,    21,   -15,    12,   -15,
     -15,    55,   -15,   -15,   -15
  };

  const unsigned char
  parser::yydefact_[] =
  {
       2,     3,     0,     1,     7,    10,    15,     4,     5,     6,
       8,    11,    16,    39,    31,     9,    30,    29,    12,     0,
      13,    25,    17,    18,     0,    19,    48,    49,    50,     0,
      32,    33,    43,    45,    46,    47,    28,    14,     0,    34,
       0,     0,    40,    44,    20,     0,     0,    26,    35,     0,
      36,     0,    42,    21,     0,    22,     0,    37,     0,    41,
      23,     0,    27,    38,    24
  };

  const signed char
  parser::yypgoto_[] =
  {
     -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,    27,
      62,   -10,    59,    36,   -15,   -14,   -15,   -15,   -15
  };

  const signed char
  parser::yydefgoto_[] =
  {
      -1,     2,     7,     8,    18,     9,    22,    23,    24,    46,
      19,    47,    16,    41,    31,    42,    33,    34,    35
  };

  const unsigned char
  parser::yytable_[] =
  {
      15,    20,    32,    26,    27,    36,    28,     1,    26,    27,
      39,    28,    40,    26,    27,    48,    28,    43,    26,    27,
      50,    28,    51,    26,    27,    63,    28,    52,    21,    13,
      57,    10,    58,    14,    17,    52,    13,    59,    13,    44,
      14,    53,    14,    45,    59,    37,    62,    26,    27,    13,
      28,    62,    13,    29,    11,    12,    14,     3,    17,    38,
       4,     5,     6,    13,    55,    60,    64,    14,    56,    61,
      26,    27,    54,    28,    25,    30,    49
  };

  const unsigned char
  parser::yycheck_[] =
  {
      10,    11,    16,     6,     7,     6,     9,    15,     6,     7,
      13,     9,    15,     6,     7,    13,     9,    31,     6,     7,
      13,     9,    15,     6,     7,    13,     9,    41,     8,     8,
      13,    15,    15,    12,    14,    49,     8,    51,     8,    11,
      12,    11,    12,    15,    58,     8,    56,     6,     7,     8,
       9,    61,     8,    12,    15,    15,    12,     0,    14,    10,
       3,     4,     5,     8,    11,    11,    11,    12,    15,    15,
       6,     7,    45,     9,    12,    16,    40
  };

  const unsigned char
  parser::yystos_[] =
  {
       0,    15,    18,     0,     3,     4,     5,    19,    20,    22,
      15,    15,    15,     8,    12,    28,    29,    14,    21,    27,
      28,     8,    23,    24,    25,    27,     6,     7,     9,    12,
      29,    31,    32,    33,    34,    35,     6,     8,    10,    13,
      15,    30,    32,    32,    11,    15,    26,    28,    13,    30,
      13,    15,    32,    11,    26,    11,    15,    13,    15,    32,
      11,    15,    28,    13,    11
  };

  const unsigned char
  parser::yyr1_[] =
  {
       0,    17,    18,    18,    18,    18,    18,    19,    19,    19,
      20,    20,    20,    20,    21,    22,    22,    22,    22,    23,
      24,    24,    24,    24,    24,    25,    26,    26,    27,    27,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    29,
      30,    30,    30,    31,    31,    32,    32,    32,    33,    34,
      35
  };

  const unsigned char
  parser::yyr2_[] =
  {
       0,     2,     0,     1,     2,     2,     2,     1,     2,     3,
       1,     2,     3,     3,     2,     1,     2,     3,     3,     1,
       3,     4,     4,     5,     6,     1,     1,     3,     2,     1,
       1,     1,     2,     2,     3,     4,     4,     5,     6,     1,
       1,     3,     2,     1,     2,     1,     1,     1,     1,     1,
       1
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"end of file\"", "error", "$undefined", "SCENE_HEADER",
  "ASSETS_HEADER", "ENTITIES_HEADER", "INTEGER", "FLOAT", "NAME", "STRING",
  "OPEN_CURLY", "CLOSE_CURLY", "OPEN_SQUARE", "CLOSE_SQUARE", "POUND",
  "EOL", "SPACE", "$accept", "scene", "scene_section", "assets_section",
  "asset_name", "entities_section", "entity_id", "component",
  "component_name", "component_properties", "id", "property",
  "property_name", "bracketed_values", "values", "value", "int_value",
  "float_value", "string_value", YY_NULLPTR
  };

#if YYDEBUG
  const unsigned char
  parser::yyrline_[] =
  {
       0,    48,    48,    49,    50,    51,    52,    56,    57,    58,
      62,    63,    64,    65,    68,    71,    72,    73,    74,    77,
      80,    81,    82,    83,    84,    87,    90,    91,    95,    96,
     100,   101,   106,   110,   111,   112,   113,   114,   115,   118,
     121,   122,   123,   127,   128,   132,   133,   134,   137,   138,
     139
  };

  // Print the state stack on the debug stream.
  void
  parser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << i->state;
    *yycdebug_ << '\n';
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  parser::yy_reduce_print_ (int yyrule)
  {
    unsigned yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG



} // yy
#line 1145 "parser.cpp" // lalr1.cc:1217
#line 141 "parser.y" // lalr1.cc:1218


void yy::parser::error (const location_type& l, const std::string& m) {
  std::cerr << l << ": " << m << '\n';
}
